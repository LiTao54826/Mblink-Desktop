#include "html_audio_element.h"

#include "../document.h"
#include "../event.h"
#include "core/window/window.h"
#include "core/window/repaint_reason.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_iostream.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <limits>
#include <vector>

namespace mbink {

namespace {

constexpr float kAudioControlPadding = 8.0f;
constexpr float kAudioPlayButtonSize = 22.0f;
constexpr float kAudioVolumeWidth = 54.0f;
constexpr float kAudioTrackHeight = 4.0f;
constexpr double kAudioInitialQueueSeconds = 0.20;

std::vector<HTMLAudioElement*> g_active_audio_elements;

bool IsRemoteOrDataUrl(const std::string& src) {
    return src.rfind("http://", 0) == 0 ||
           src.rfind("https://", 0) == 0 ||
           src.rfind("data:", 0) == 0;
}

} // namespace

void HTMLAudioElement::SDLStreamDeleter::operator()(SDL_AudioStream* stream) const {
    if (stream) {
        SDL_DestroyAudioStream(stream);
    }
}

HTMLAudioElement::HTMLAudioElement()
    : Element("audio") {
}

HTMLAudioElement::~HTMLAudioElement() {
    StopProgressUpdates();
    StopStream();
}

void HTMLAudioElement::SetSrc(const std::string& src) {
    if (src_ == src) {
        return;
    }

    src_ = src;
    Element::SetAttribute("src", src);
    ResetDecodedAudio();

    if (!src_.empty()) {
        Load();
    }
}

void HTMLAudioElement::SetControls(bool controls) {
    if (controls_ == controls) {
        return;
    }

    controls_ = controls;
    if (controls) {
        Element::SetAttribute("controls", "");
    } else {
        Element::RemoveAttribute("controls");
    }
    MarkDirty();
}

void HTMLAudioElement::SetLoop(bool loop) {
    if (loop_ == loop) {
        return;
    }

    if (stream_ && SDL_LockAudioStream(stream_.get())) {
        loop_ = loop;
        SDL_UnlockAudioStream(stream_.get());
    } else {
        loop_ = loop;
    }
    if (loop) {
        Element::SetAttribute("loop", "");
    } else {
        Element::RemoveAttribute("loop");
    }
}

bool HTMLAudioElement::GetPaused() {
    RefreshPlaybackState();
    return paused_;
}

bool HTMLAudioElement::GetEnded() {
    RefreshPlaybackState();
    return ended_;
}

double HTMLAudioElement::CurrentTime() {
    RefreshPlaybackState();
    return current_time_seconds_;
}

void HTMLAudioElement::SetCurrentTime(double seconds) {
    const double clamped = ClampTime(seconds);
    const bool was_playing = !paused_;

    DispatchMediaEvent("seeking");
    if (was_playing) {
        StartStreamAt(clamped);
    } else {
        current_time_seconds_ = clamped;
        ended_ = HasDecodedAudio() && clamped >= duration_seconds_;
    }
    DispatchMediaEvent("timeupdate");
    DispatchMediaEvent("seeked");
    RequestAudioRepaint();
}

void HTMLAudioElement::SetVolume(double volume) {
    const double clamped = std::clamp(volume, 0.0, 1.0);
    if (std::abs(volume_ - clamped) < 0.0001) {
        return;
    }

    volume_ = clamped;
    ApplyStreamGain();
    DispatchMediaEvent("volumechange");
    RequestAudioRepaint();
}

void HTMLAudioElement::SetMuted(bool muted) {
    if (muted_ == muted) {
        return;
    }

    muted_ = muted;
    if (muted) {
        Element::SetAttribute("muted", "");
    } else {
        Element::RemoveAttribute("muted");
    }
    ApplyStreamGain();
    DispatchMediaEvent("volumechange");
    RequestAudioRepaint();
}

bool HTMLAudioElement::Load() {
    StopStream();
    ResetDecodedAudio();

    if (src_.empty()) {
        return false;
    }

    DispatchMediaEvent("loadstart");
    load_state_ = AudioLoadState::LOADING;

    if (!AcceptsSourceAsWav(src_)) {
        SetError("Unsupported audio source: only WAV is supported by this SDL3 first pass");
        return false;
    }

    auto doc = GetOwnerDocument();
    const std::string resolved_path = doc ? doc->ResolvePath(src_) : src_;

    if (auto provider = Document::GetAssetProvider()) {
        std::vector<uint8_t> data;
        if (provider(src_, data) ||
            (!resolved_path.empty() && resolved_path != src_ && provider(resolved_path, data))) {
            if (LoadFromMemory(data)) {
                return true;
            }
        }
    }

    if (IsRemoteOrDataUrl(resolved_path)) {
        SetError("Unsupported audio source: network and data URLs are not supported yet");
        return false;
    }

    return LoadFromResolvedPath(resolved_path.empty() ? src_ : resolved_path);
}

bool HTMLAudioElement::Play() {
    if (!HasDecodedAudio() && !Load()) {
        return false;
    }

    const double start_time = ended_ ? 0.0 : ClampTime(current_time_seconds_);
    if (!StartStreamAt(start_time)) {
        return false;
    }

    paused_ = false;
    ended_ = false;
    DispatchMediaEvent("play");
    StartProgressUpdates();
    RequestAudioRepaint();
    return true;
}

void HTMLAudioElement::Pause() {
    if (paused_) {
        return;
    }

    current_time_seconds_ = PlaybackTimeFromClock();
    StopStream();
    paused_ = true;
    DispatchMediaEvent("pause");
    StopProgressUpdates();
    RequestAudioRepaint();
}

void HTMLAudioElement::ScheduleControlsRepaint() {
    if (!paused_) {
        StartProgressUpdates();
    }
}

void HTMLAudioElement::TickActiveAudioElements() {
    const auto active_audio_elements = g_active_audio_elements;
    for (auto* audio : active_audio_elements) {
        if (!audio || !audio->progress_updates_registered_) {
            continue;
        }

        audio->RefreshPlaybackState();
        if (audio->paused_) {
            audio->StopProgressUpdates();
            continue;
        }

        audio->RequestAudioRepaint();
    }
}

void HTMLAudioElement::SetAttribute(const std::string& name, const std::string& value) {
    if (name == "src") {
        if (src_ == value) {
            Element::SetAttribute(name, value);
            return;
        }
        src_ = value;
        Element::SetAttribute(name, value);
        ResetDecodedAudio();
        if (!value.empty()) {
            Load();
        }
        return;
    }

    if (name == "controls") {
        controls_ = ParseBoolAttributeValue(value);
        Element::SetAttribute(name, value);
        MarkDirty();
        return;
    }

    if (name == "loop") {
        SetLoop(ParseBoolAttributeValue(value));
        Element::SetAttribute(name, value);
        return;
    }

    if (name == "muted") {
        muted_ = ParseBoolAttributeValue(value);
        Element::SetAttribute(name, value);
        ApplyStreamGain();
        DispatchMediaEvent("volumechange");
        RequestAudioRepaint();
        return;
    }

    if (name == "volume") {
        try {
            SetVolume(std::stod(value));
        } catch (...) {
            SetVolume(1.0);
        }
        Element::SetAttribute(name, value);
        return;
    }

    Element::SetAttribute(name, value);
}

void HTMLAudioElement::RemoveAttribute(const std::string& name) {
    if (name == "src") {
        src_.clear();
        StopStream();
        ResetDecodedAudio();
        Element::RemoveAttribute(name);
        return;
    }

    if (name == "controls") {
        controls_ = false;
        Element::RemoveAttribute(name);
        MarkDirty();
        return;
    }

    if (name == "loop") {
        SetLoop(false);
        return;
    }

    if (name == "muted") {
        muted_ = false;
        Element::RemoveAttribute(name);
        ApplyStreamGain();
        DispatchMediaEvent("volumechange");
        RequestAudioRepaint();
        return;
    }

    Element::RemoveAttribute(name);
}

HTMLAudioElement::ControlGeometry HTMLAudioElement::ComputeControlGeometry(float content_x,
                                                                           float content_y,
                                                                           float content_width,
                                                                           float content_height) const {
    ControlGeometry geometry;
    geometry.bounds = SkRect::MakeXYWH(content_x, content_y, content_width, content_height);

    if (content_width <= 0.0f || content_height <= 0.0f) {
        return geometry;
    }

    const float center_y = content_y + content_height * 0.5f;
    const float control_height = std::min(kAudioPlayButtonSize, std::max(0.0f, content_height - 8.0f));
    const float left = content_x + kAudioControlPadding;
    geometry.play_button = SkRect::MakeXYWH(left,
                                            center_y - control_height * 0.5f,
                                            control_height,
                                            control_height);

    const float volume_width = std::min(kAudioVolumeWidth, std::max(0.0f, content_width * 0.25f));
    const float volume_left = content_x + content_width - kAudioControlPadding - volume_width;
    geometry.volume_track = SkRect::MakeXYWH(volume_left,
                                             center_y - kAudioTrackHeight * 0.5f,
                                             std::max(0.0f, volume_width),
                                             kAudioTrackHeight);

    const float progress_left = geometry.play_button.right() + kAudioControlPadding;
    const float progress_right = volume_left - kAudioControlPadding;
    geometry.progress_track = SkRect::MakeXYWH(progress_left,
                                               center_y - kAudioTrackHeight * 0.5f,
                                               std::max(0.0f, progress_right - progress_left),
                                               kAudioTrackHeight);

    return geometry;
}

HTMLAudioElement::ControlPart HTMLAudioElement::HitTestControls(float local_x,
                                                                float local_y,
                                                                float content_x,
                                                                float content_y,
                                                                float content_width,
                                                                float content_height) const {
    if (!controls_) {
        return ControlPart::None;
    }

    const auto geometry = ComputeControlGeometry(content_x, content_y, content_width, content_height);
    const SkRect play_hit = geometry.play_button.makeOutset(4.0f, 4.0f);
    const SkRect progress_hit = geometry.progress_track.makeOutset(0.0f, 8.0f);
    const SkRect volume_hit = geometry.volume_track.makeOutset(0.0f, 8.0f);

    if (play_hit.contains(local_x, local_y)) {
        return ControlPart::PlayButton;
    }
    if (progress_hit.contains(local_x, local_y)) {
        return ControlPart::ProgressTrack;
    }
    if (volume_hit.contains(local_x, local_y)) {
        return ControlPart::VolumeTrack;
    }
    return ControlPart::None;
}

bool HTMLAudioElement::HandleControlMouseDown(float local_x,
                                              float local_y,
                                              float content_x,
                                              float content_y,
                                              float content_width,
                                              float content_height) {
    const auto part = HitTestControls(local_x, local_y, content_x, content_y, content_width, content_height);
    if (part == ControlPart::None) {
        return false;
    }

    dragging_part_ = part;
    const auto geometry = ComputeControlGeometry(content_x, content_y, content_width, content_height);

    if (part == ControlPart::PlayButton) {
        if (paused_) {
            Play();
        } else {
            Pause();
        }
        dragging_part_ = ControlPart::None;
        return true;
    }

    if (part == ControlPart::ProgressTrack && geometry.progress_track.width() > 0.0f) {
        SeekFromRatio((local_x - geometry.progress_track.left()) / geometry.progress_track.width());
        return true;
    }

    if (part == ControlPart::VolumeTrack && geometry.volume_track.width() > 0.0f) {
        SetVolumeFromRatio((local_x - geometry.volume_track.left()) / geometry.volume_track.width());
        return true;
    }

    return true;
}

bool HTMLAudioElement::HandleControlMouseMove(float local_x,
                                              float local_y,
                                              float content_x,
                                              float content_y,
                                              float content_width,
                                              float content_height) {
    (void)local_y;
    if (dragging_part_ == ControlPart::None) {
        return false;
    }

    const auto geometry = ComputeControlGeometry(content_x, content_y, content_width, content_height);
    if (dragging_part_ == ControlPart::ProgressTrack && geometry.progress_track.width() > 0.0f) {
        SeekFromRatio((local_x - geometry.progress_track.left()) / geometry.progress_track.width());
        return true;
    }
    if (dragging_part_ == ControlPart::VolumeTrack && geometry.volume_track.width() > 0.0f) {
        SetVolumeFromRatio((local_x - geometry.volume_track.left()) / geometry.volume_track.width());
        return true;
    }

    return false;
}

bool HTMLAudioElement::HandleControlMouseUp() {
    if (dragging_part_ == ControlPart::None) {
        return false;
    }

    dragging_part_ = ControlPart::None;
    return true;
}

void HTMLAudioElement::ResetDecodedAudio() {
    StopStream();
    StopProgressUpdates();
    audio_data_.clear();
    audio_spec_ = SDL_AudioSpec{};
    frame_size_bytes_ = 0;
    current_time_seconds_ = 0.0;
    duration_seconds_ = 0.0;
    paused_ = true;
    ended_ = false;
    load_state_ = src_.empty() ? AudioLoadState::EMPTY : AudioLoadState::LOADING;
    error_message_.clear();
}

void HTMLAudioElement::StopStream() {
    if (stream_) {
        SDL_PauseAudioStreamDevice(stream_.get());
        SDL_SetAudioStreamGetCallback(stream_.get(), nullptr, nullptr);
        SDL_ClearAudioStream(stream_.get());
        stream_.reset();
    }
    playback_started_ticks_ = 0;
    playback_start_time_seconds_ = current_time_seconds_;
    playback_byte_offset_ = ByteOffsetForTime(current_time_seconds_);
    stream_end_flushed_ = false;
}

bool HTMLAudioElement::EnsureAudioSubsystem() {
    if (SDL_WasInit(SDL_INIT_AUDIO)) {
        return true;
    }
    return SDL_InitSubSystem(SDL_INIT_AUDIO);
}

bool HTMLAudioElement::LoadFromResolvedPath(const std::string& resolved_path) {
    if (resolved_path.empty()) {
        SetError("Audio source path is empty");
        return false;
    }

    SDL_IOStream* io = SDL_IOFromFile(resolved_path.c_str(), "rb");
    if (!io) {
        SetError("Failed to open audio source: " + resolved_path);
        return false;
    }

    return DecodeWavFromIO(io, true);
}

bool HTMLAudioElement::LoadFromMemory(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        SetError("Audio source is empty");
        return false;
    }

    SDL_IOStream* io = SDL_IOFromConstMem(data.data(), data.size());
    if (!io) {
        SetError("Failed to open in-memory audio source");
        return false;
    }

    return DecodeWavFromIO(io, true);
}

bool HTMLAudioElement::AcceptsSourceAsWav(const std::string& src) const {
    if (src.empty()) {
        return false;
    }

    std::string lower = LowercasePath(src);
    const size_t query = lower.find_first_of("?#");
    if (query != std::string::npos) {
        lower = lower.substr(0, query);
    }
    return lower.size() >= 4 && lower.substr(lower.size() - 4) == ".wav";
}

bool HTMLAudioElement::DecodeWavFromIO(SDL_IOStream* io, bool close_io) {
    Uint8* decoded = nullptr;
    Uint32 decoded_len = 0;
    SDL_AudioSpec spec{};

    if (!SDL_LoadWAV_IO(io, close_io, &spec, &decoded, &decoded_len)) {
        SetError(std::string("Failed to decode WAV audio: ") + SDL_GetError());
        return false;
    }

    if (!decoded || decoded_len == 0 || spec.freq <= 0 || spec.channels <= 0) {
        if (decoded) {
            SDL_free(decoded);
        }
        SetError("Decoded WAV audio is empty or invalid");
        return false;
    }

    audio_spec_ = spec;
    audio_data_.assign(decoded, decoded + decoded_len);
    SDL_free(decoded);

    frame_size_bytes_ = static_cast<size_t>(SDL_AUDIO_FRAMESIZE(audio_spec_));
    if (frame_size_bytes_ == 0) {
        ResetDecodedAudio();
        SetError("Decoded WAV audio has an invalid frame size");
        return false;
    }

    const double frame_count = static_cast<double>(audio_data_.size() / frame_size_bytes_);
    duration_seconds_ = frame_count / static_cast<double>(audio_spec_.freq);
    current_time_seconds_ = 0.0;
    paused_ = true;
    ended_ = false;
    load_state_ = AudioLoadState::READY;
    error_message_.clear();

    DispatchMediaEvent("loadedmetadata");
    DispatchMediaEvent("canplay");
    RequestAudioRepaint();
    return true;
}

bool HTMLAudioElement::StartStreamAt(double seconds) {
    if (!HasDecodedAudio()) {
        return false;
    }

    if (!EnsureAudioSubsystem()) {
        SetError(std::string("Failed to initialize SDL audio: ") + SDL_GetError());
        return false;
    }

    double start_seconds = ClampTime(seconds);
    if (duration_seconds_ > 0.0 && start_seconds >= duration_seconds_) {
        if (loop_) {
            start_seconds = 0.0;
        } else {
            StopStream();
            current_time_seconds_ = duration_seconds_;
            playback_start_time_seconds_ = current_time_seconds_;
            playback_byte_offset_ = audio_data_.size();
            paused_ = true;
            ended_ = true;
            DispatchMediaEvent("ended");
            StopProgressUpdates();
            RequestAudioRepaint();
            return false;
        }
    }

    const size_t byte_offset = ByteOffsetForTime(start_seconds);
    if (byte_offset >= audio_data_.size()) {
        StopStream();
        current_time_seconds_ = duration_seconds_;
        playback_start_time_seconds_ = current_time_seconds_;
        playback_byte_offset_ = audio_data_.size();
        paused_ = true;
        ended_ = true;
        StopProgressUpdates();
        return false;
    }

    StopStream();
    current_time_seconds_ = start_seconds;
    playback_byte_offset_ = byte_offset;
    stream_end_flushed_ = false;
    ended_ = false;
    stream_.reset(SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
        &audio_spec_,
        [](void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount) {
            (void)total_amount;
            auto* audio = static_cast<HTMLAudioElement*>(userdata);
            if (audio) {
                audio->QueueAudioData(stream, additional_amount);
            }
        },
        this));
    if (!stream_) {
        SetError(std::string("Failed to open SDL audio stream: ") + SDL_GetError());
        return false;
    }

    ApplyStreamGain();
    const size_t bytes_per_second = frame_size_bytes_ * static_cast<size_t>(audio_spec_.freq);
    size_t initial_queue_bytes = static_cast<size_t>(
        static_cast<double>(bytes_per_second) * kAudioInitialQueueSeconds);
    initial_queue_bytes = std::max(initial_queue_bytes, frame_size_bytes_);
    initial_queue_bytes -= initial_queue_bytes % frame_size_bytes_;
    if (!loop_) {
        initial_queue_bytes = std::min(initial_queue_bytes, audio_data_.size() - playback_byte_offset_);
    }
    QueueAudioData(stream_.get(), static_cast<int>(std::min<size_t>(
        initial_queue_bytes,
        static_cast<size_t>(std::numeric_limits<int>::max()))));

    if (!SDL_ResumeAudioStreamDevice(stream_.get())) {
        stream_.reset();
        SetError(std::string("Failed to start SDL audio stream: ") + SDL_GetError());
        return false;
    }

    playback_started_ticks_ = SDL_GetTicks();
    playback_start_time_seconds_ = current_time_seconds_;
    return true;
}

void HTMLAudioElement::QueueAudioData(SDL_AudioStream* stream, int requested_bytes) {
    if (!stream || audio_data_.empty() || requested_bytes <= 0) {
        return;
    }

    int remaining_request = requested_bytes;
    while (remaining_request > 0 && !audio_data_.empty()) {
        if (playback_byte_offset_ >= audio_data_.size()) {
            if (!loop_) {
                if (!stream_end_flushed_) {
                    SDL_FlushAudioStream(stream);
                    stream_end_flushed_ = true;
                }
                return;
            }
            playback_byte_offset_ = 0;
        }

        const size_t available = audio_data_.size() - playback_byte_offset_;
        const int chunk = static_cast<int>(std::min<size_t>(
            available,
            static_cast<size_t>(std::min(remaining_request, std::numeric_limits<int>::max()))));
        if (chunk <= 0) {
            return;
        }

        if (!SDL_PutAudioStreamData(stream, audio_data_.data() + playback_byte_offset_, chunk)) {
            return;
        }

        playback_byte_offset_ += static_cast<size_t>(chunk);
        remaining_request -= chunk;

        if (playback_byte_offset_ >= audio_data_.size() && !loop_) {
            if (!stream_end_flushed_) {
                SDL_FlushAudioStream(stream);
                stream_end_flushed_ = true;
            }
            return;
        }
    }
}

size_t HTMLAudioElement::ByteOffsetForTime(double seconds) const {
    if (frame_size_bytes_ == 0 || audio_spec_.freq <= 0) {
        return 0;
    }

    const double clamped = ClampTime(seconds);
    const auto frame_index = static_cast<size_t>(std::floor(clamped * static_cast<double>(audio_spec_.freq)));
    return std::min(frame_index * frame_size_bytes_, audio_data_.size());
}

double HTMLAudioElement::TimeForByteOffset(size_t byte_offset) const {
    if (frame_size_bytes_ == 0 || audio_spec_.freq <= 0) {
        return 0.0;
    }

    const size_t frame_index = byte_offset / frame_size_bytes_;
    return static_cast<double>(frame_index) / static_cast<double>(audio_spec_.freq);
}

double HTMLAudioElement::PlaybackTimeFromClock() const {
    if (paused_ || playback_started_ticks_ == 0) {
        return current_time_seconds_;
    }

    const double elapsed = static_cast<double>(SDL_GetTicks() - playback_started_ticks_) / 1000.0;
    double time = playback_start_time_seconds_ + elapsed;
    if (duration_seconds_ > 0.0 && time >= duration_seconds_) {
        if (loop_) {
            time = std::fmod(time, duration_seconds_);
        } else {
            time = duration_seconds_;
        }
    }
    return ClampTime(time);
}

double HTMLAudioElement::ClampTime(double seconds) const {
    if (std::isnan(seconds) || seconds < 0.0) {
        return 0.0;
    }
    if (duration_seconds_ > 0.0) {
        return std::min(seconds, duration_seconds_);
    }
    return seconds;
}

void HTMLAudioElement::SeekFromRatio(float ratio) {
    if (!HasDecodedAudio()) {
        return;
    }

    SetCurrentTime(duration_seconds_ * std::clamp(static_cast<double>(ratio), 0.0, 1.0));
}

void HTMLAudioElement::SetVolumeFromRatio(float ratio) {
    SetVolume(std::clamp(static_cast<double>(ratio), 0.0, 1.0));
}

void HTMLAudioElement::ApplyStreamGain() {
    if (stream_) {
        SDL_SetAudioStreamGain(stream_.get(), muted_ ? 0.0f : static_cast<float>(volume_));
    }
}

void HTMLAudioElement::RefreshPlaybackState() {
    if (paused_) {
        return;
    }

    const double time = PlaybackTimeFromClock();
    current_time_seconds_ = time;
    if (HasDecodedAudio() && duration_seconds_ > 0.0 && time >= duration_seconds_) {
        StopStream();
        current_time_seconds_ = duration_seconds_;
        paused_ = true;
        ended_ = true;
        StopProgressUpdates();
        DispatchMediaEvent("timeupdate");
        DispatchMediaEvent("ended");
        RequestAudioRepaint();
    }
}

void HTMLAudioElement::StartProgressUpdates() {
    if (progress_updates_registered_) {
        return;
    }

    g_active_audio_elements.push_back(this);
    progress_updates_registered_ = true;
}

void HTMLAudioElement::StopProgressUpdates() {
    if (!progress_updates_registered_) {
        return;
    }

    g_active_audio_elements.erase(
        std::remove(g_active_audio_elements.begin(), g_active_audio_elements.end(), this),
        g_active_audio_elements.end());
    progress_updates_registered_ = false;
}

void HTMLAudioElement::SetError(const std::string& message) {
    StopStream();
    StopProgressUpdates();
    audio_data_.clear();
    audio_spec_ = SDL_AudioSpec{};
    frame_size_bytes_ = 0;
    duration_seconds_ = 0.0;
    current_time_seconds_ = 0.0;
    paused_ = true;
    ended_ = false;
    load_state_ = AudioLoadState::ERROR;
    error_message_ = message.empty() ? "Failed to load audio" : message;
    DispatchMediaEvent("error");
    RequestAudioRepaint();
}

void HTMLAudioElement::DispatchMediaEvent(const std::string& type) {
    auto event = std::make_shared<Event>(type);
    DispatchEvent(event);
}

void HTMLAudioElement::RequestAudioRepaint() {
    RequestRepaint(RepaintReason::MouseButton);
}

bool HTMLAudioElement::ParseBoolAttributeValue(const std::string& value) {
    (void)value;
    return true;
}

std::string HTMLAudioElement::LowercasePath(const std::string& value) {
    std::string lower = value;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return lower;
}

} // namespace mbink
