#include "html_audio_element.h"

#include "../document.h"
#include "../event.h"
#include "core/window/window.h"
#include "core/window/repaint_reason.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_iostream.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <limits>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif

namespace mbink {

namespace {

constexpr float kAudioControlPadding = 8.0f;
constexpr float kAudioPlayButtonSize = 22.0f;
constexpr float kAudioVolumeWidth = 54.0f;
constexpr float kAudioTimeLabelWidth = 78.0f;
constexpr float kAudioMinimumProgressWidth = 48.0f;
constexpr float kAudioTrackHeight = 4.0f;
constexpr double kAudioInitialQueueSeconds = 0.20;
constexpr double kAudioTargetQueueSeconds = 0.20;
constexpr double kAudioDebugQueueIntervalSeconds = 0.25;

std::vector<HTMLAudioElement*> g_active_audio_elements;

bool IsRemoteOrDataUrl(const std::string& src) {
    return src.rfind("http://", 0) == 0 ||
           src.rfind("https://", 0) == 0 ||
           src.rfind("data:", 0) == 0;
}

bool AudioDebugEnabled() {
    static const bool enabled = [] {
        const char* value = std::getenv("MBINK_AUDIO_DEBUG");
        return value && value[0] && std::string(value) != "0";
    }();
    return enabled;
}

std::string LowercaseAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string AudioSpecSummary(const SDL_AudioSpec& spec) {
    std::ostringstream out;
    out << "freq=" << spec.freq
        << " channels=" << static_cast<int>(spec.channels)
        << " format=0x" << std::hex << static_cast<unsigned int>(spec.format);
    return out.str();
}

std::string AudioStreamFormatSummary(SDL_AudioStream* stream) {
    if (!stream) {
        return "stream=<null>";
    }

    SDL_AudioSpec src_spec{};
    SDL_AudioSpec dst_spec{};
    std::ostringstream out;
    if (SDL_GetAudioStreamFormat(stream, &src_spec, &dst_spec)) {
        out << "stream_src{" << AudioSpecSummary(src_spec) << "} "
            << "stream_dst{" << AudioSpecSummary(dst_spec) << "}";
    } else {
        out << "stream_format_error=" << SDL_GetError();
    }
    return out.str();
}

void AudioDebugLog(const std::string& message) {
    if (!AudioDebugEnabled()) {
        return;
    }
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);
    std::ostringstream line;
    line << "[mbink audio pid="
#if defined(_WIN32)
         << _getpid()
#else
         << getpid()
#endif
         << " tid=" << std::this_thread::get_id()
         << " tick=" << SDL_GetTicks()
         << "] " << message;
    const std::string text = line.str();
    std::cerr << text << std::endl;

    const char* explicit_path = std::getenv("MBINK_AUDIO_DEBUG_FILE");
    std::filesystem::path log_path;
    if (explicit_path && explicit_path[0]) {
        log_path = explicit_path;
    } else if (const char* temp = std::getenv("TEMP")) {
        log_path = std::filesystem::path(temp) / "mbink-audio-debug.log";
    } else if (const char* tmp = std::getenv("TMP")) {
        log_path = std::filesystem::path(tmp) / "mbink-audio-debug.log";
    }
    if (!log_path.empty()) {
        std::ofstream file(log_path, std::ios::app);
        if (file) {
            file << text << '\n';
        }
    }
}

void AudioDebugLogDevices() {
    if (!AudioDebugEnabled()) {
        return;
    }

    const char* driver = SDL_GetCurrentAudioDriver();
    std::ostringstream out;
    out << "driver=" << (driver ? driver : "<none>");

    int count = 0;
    SDL_AudioDeviceID* devices = SDL_GetAudioPlaybackDevices(&count);
    out << " playback_devices=" << count;
    if (devices) {
        for (int i = 0; i < count; ++i) {
            SDL_AudioSpec device_spec{};
            int sample_frames = 0;
            const char* name = SDL_GetAudioDeviceName(devices[i]);
            out << " [" << devices[i] << ":" << (name ? name : "<unnamed>");
            if (SDL_GetAudioDeviceFormat(devices[i], &device_spec, &sample_frames)) {
                out << " " << AudioSpecSummary(device_spec)
                    << " frames=" << sample_frames;
            } else {
                out << " format_error=" << SDL_GetError();
            }
            out << "]";
        }
        SDL_free(devices);
    } else {
        out << " list_error=" << SDL_GetError();
    }

    AudioDebugLog(out.str());
}

void ConfigureAudioDriverHint() {
#if defined(_WIN32)
    const char* mbink_driver = std::getenv("MBINK_AUDIO_DRIVER");
    if (mbink_driver && mbink_driver[0]) {
        if (SDL_SetHint(SDL_HINT_AUDIO_DRIVER, mbink_driver)) {
            AudioDebugLog(std::string("audio driver set from MBINK_AUDIO_DRIVER=\"") + mbink_driver + "\"");
        } else {
            AudioDebugLog(std::string("audio driver override ignored by SDL MBINK_AUDIO_DRIVER=\"") + mbink_driver + "\"");
        }
        return;
    }

    const char* existing_driver = SDL_GetHint(SDL_HINT_AUDIO_DRIVER);
    if (existing_driver && existing_driver[0]) {
        AudioDebugLog(std::string("audio driver already set SDL_AUDIO_DRIVER=\"") + existing_driver + "\"");
        return;
    }

    if (SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "directsound,wasapi")) {
        AudioDebugLog("audio driver default set for Windows playback=\"directsound,wasapi\"");
    } else {
        AudioDebugLog("audio driver default ignored by SDL for Windows playback");
    }
#endif
}

SDL_AudioDeviceID ResolvePlaybackDevice() {
    const char* requested = std::getenv("MBINK_AUDIO_PLAYBACK_DEVICE");
    if (!requested || !requested[0]) {
        AudioDebugLog("playback device override not set, using SDL default playback");
        return SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;
    }

    int count = 0;
    SDL_AudioDeviceID* devices = SDL_GetAudioPlaybackDevices(&count);
    if (!devices) {
        AudioDebugLog(std::string("playback device override list failed: ") + SDL_GetError());
        return SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;
    }

    const std::string needle = LowercaseAscii(requested);
    SDL_AudioDeviceID selected = 0;
    std::string selected_name;
    for (int i = 0; i < count; ++i) {
        const char* name = SDL_GetAudioDeviceName(devices[i]);
        const std::string candidate = name ? name : "";
        if (LowercaseAscii(candidate).find(needle) != std::string::npos) {
            selected = devices[i];
            selected_name = candidate;
            break;
        }
    }
    SDL_free(devices);

    if (selected == 0) {
        AudioDebugLog(std::string("playback device override \"") + requested + "\" not found, using SDL default playback");
        return SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;
    }

    AudioDebugLog("playback device override matched id=" + std::to_string(selected) +
                  " name=\"" + selected_name + "\"");
    return selected;
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
        AudioDebugLog("set src unchanged src=\"" + src + "\"");
        return;
    }

    AudioDebugLog("set src from=\"" + src_ + "\" to=\"" + src + "\"");
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

    AudioDebugLog("set controls=" + std::to_string(controls));
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
    AudioDebugLog("set currentTime requested=" + std::to_string(seconds) +
                  " clamped=" + std::to_string(clamped) +
                  " was_playing=" + std::to_string(was_playing));

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
    AudioDebugLog("set volume=" + std::to_string(volume_) +
                  " requested=" + std::to_string(volume));
    ApplyStreamGain();
    DispatchMediaEvent("volumechange");
    RequestAudioRepaint();
}

void HTMLAudioElement::SetMuted(bool muted) {
    if (muted_ == muted) {
        return;
    }

    muted_ = muted;
    AudioDebugLog("set muted=" + std::to_string(muted_));
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
        AudioDebugLog("load skipped: empty src");
        return false;
    }

    AudioDebugLog("load start src=\"" + src_ + "\"");
    DispatchMediaEvent("loadstart");
    load_state_ = AudioLoadState::LOADING;

    if (!AcceptsSourceAsWav(src_)) {
        AudioDebugLog("load rejected unsupported source src=\"" + src_ + "\"");
        SetError("Unsupported audio source: only WAV is supported by this SDL3 first pass");
        return false;
    }

    auto doc = GetOwnerDocument();
    const std::string resolved_path = doc ? doc->ResolvePath(src_) : src_;
    AudioDebugLog("resolved src=\"" + src_ + "\" path=\"" + resolved_path + "\"");

    if (auto provider = Document::GetAssetProvider()) {
        std::vector<uint8_t> data;
        if (provider(src_, data) ||
            (!resolved_path.empty() && resolved_path != src_ && provider(resolved_path, data))) {
            AudioDebugLog("load from asset provider bytes=" + std::to_string(data.size()));
            if (LoadFromMemory(data)) {
                return true;
            }
        }
    }

    if (IsRemoteOrDataUrl(resolved_path)) {
        AudioDebugLog("load rejected remote/data source path=\"" + resolved_path + "\"");
        SetError("Unsupported audio source: network and data URLs are not supported yet");
        return false;
    }

    return LoadFromResolvedPath(resolved_path.empty() ? src_ : resolved_path);
}

bool HTMLAudioElement::Play() {
    AudioDebugLog("play requested src=\"" + src_ + "\" state=" + std::to_string(static_cast<int>(load_state_)) +
                  " decoded_bytes=" + std::to_string(audio_data_.size()) +
                  " duration=" + std::to_string(duration_seconds_) +
                  " paused=" + std::to_string(paused_) +
                  " ended=" + std::to_string(ended_) +
                  " volume=" + std::to_string(volume_) +
                  " muted=" + std::to_string(muted_));
    if (!HasDecodedAudio() && !Load()) {
        AudioDebugLog("play failed: load did not produce decoded audio error=\"" + error_message_ + "\"");
        return false;
    }

    const double start_time = ended_ ? 0.0 : ClampTime(current_time_seconds_);
    if (!StartStreamAt(start_time)) {
        AudioDebugLog("play failed: StartStreamAt failed error=\"" + error_message_ + "\"");
        return false;
    }

    paused_ = false;
    ended_ = false;
    DispatchMediaEvent("play");
    StartProgressUpdates();
    RequestAudioRepaint();
    AudioDebugLog("play started start_time=" + std::to_string(start_time));
    return true;
}

void HTMLAudioElement::Pause() {
    if (paused_) {
        return;
    }

    AudioDebugLog("pause at current_time=" + std::to_string(current_time_seconds_));
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

        audio->PumpAudioStream();
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
    const float controls_right = volume_left - kAudioControlPadding;
    const float available_progress_and_time = std::max(0.0f, controls_right - progress_left);
    const float desired_time_width = std::min(kAudioTimeLabelWidth, available_progress_and_time);
    const float time_width = available_progress_and_time > kAudioMinimumProgressWidth
        ? std::min(desired_time_width, available_progress_and_time - kAudioMinimumProgressWidth)
        : 0.0f;
    const float progress_right = controls_right - (time_width > 0.0f ? time_width + kAudioControlPadding : 0.0f);
    geometry.progress_track = SkRect::MakeXYWH(progress_left,
                                               center_y - kAudioTrackHeight * 0.5f,
                                               std::max(0.0f, progress_right - progress_left),
                                               kAudioTrackHeight);
    if (time_width > 0.0f) {
        geometry.time_label = SkRect::MakeXYWH(progress_right + kAudioControlPadding,
                                               content_y,
                                               time_width,
                                               content_height);
    }

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

    AudioDebugLog("control mouse down part=" + std::to_string(static_cast<int>(part)) +
                  " local=(" + std::to_string(local_x) + "," + std::to_string(local_y) + ")" +
                  " content=(" + std::to_string(content_x) + "," + std::to_string(content_y) + "," +
                  std::to_string(content_width) + "," + std::to_string(content_height) + ")" +
                  " paused=" + std::to_string(paused_));
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
        AudioDebugLog("control seek via progress track");
        SeekFromRatio((local_x - geometry.progress_track.left()) / geometry.progress_track.width());
        return true;
    }

    if (part == ControlPart::VolumeTrack && geometry.volume_track.width() > 0.0f) {
        AudioDebugLog("control volume via volume track");
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
        AudioDebugLog("stop stream src=\"" + src_ + "\" current_time=" + std::to_string(current_time_seconds_) +
                      " byte_offset=" + std::to_string(playback_byte_offset_));
        SDL_PauseAudioStreamDevice(stream_.get());
        SDL_SetAudioStreamGetCallback(stream_.get(), nullptr, nullptr);
        SDL_ClearAudioStream(stream_.get());
        stream_.reset();
    }
    playback_started_ticks_ = 0;
    last_queue_log_ticks_ = 0;
    playback_start_time_seconds_ = current_time_seconds_;
    playback_byte_offset_ = ByteOffsetForTime(current_time_seconds_);
    stream_end_flushed_ = false;
}

bool HTMLAudioElement::EnsureAudioSubsystem() {
    if (SDL_WasInit(SDL_INIT_AUDIO)) {
        AudioDebugLog("audio subsystem already initialized");
        AudioDebugLogDevices();
        return true;
    }
    ConfigureAudioDriverHint();
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        AudioDebugLog(std::string("audio subsystem init failed: ") + SDL_GetError());
        return false;
    }
    AudioDebugLog("audio subsystem initialized");
    AudioDebugLogDevices();
    return true;
}

bool HTMLAudioElement::LoadFromResolvedPath(const std::string& resolved_path) {
    if (resolved_path.empty()) {
        AudioDebugLog("load file failed: empty resolved path");
        SetError("Audio source path is empty");
        return false;
    }

    AudioDebugLog("open file path=\"" + resolved_path + "\"");
    SDL_IOStream* io = SDL_IOFromFile(resolved_path.c_str(), "rb");
    if (!io) {
        AudioDebugLog(std::string("open file failed path=\"") + resolved_path + "\" error=\"" + SDL_GetError() + "\"");
        SetError("Failed to open audio source: " + resolved_path);
        return false;
    }

    return DecodeWavFromIO(io, true);
}

bool HTMLAudioElement::LoadFromMemory(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        AudioDebugLog("load memory failed: empty data");
        SetError("Audio source is empty");
        return false;
    }

    AudioDebugLog("open memory bytes=" + std::to_string(data.size()));
    SDL_IOStream* io = SDL_IOFromConstMem(data.data(), data.size());
    if (!io) {
        AudioDebugLog(std::string("open memory failed: ") + SDL_GetError());
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
        AudioDebugLog(std::string("decode wav failed: ") + SDL_GetError());
        SetError(std::string("Failed to decode WAV audio: ") + SDL_GetError());
        return false;
    }

    if (!decoded || decoded_len == 0 || spec.freq <= 0 || spec.channels <= 0) {
        if (decoded) {
            SDL_free(decoded);
        }
        AudioDebugLog("decode wav invalid decoded_len=" + std::to_string(decoded_len) +
                      " " + AudioSpecSummary(spec));
        SetError("Decoded WAV audio is empty or invalid");
        return false;
    }

    audio_spec_ = spec;
    audio_data_.assign(decoded, decoded + decoded_len);
    SDL_free(decoded);

    frame_size_bytes_ = static_cast<size_t>(SDL_AUDIO_FRAMESIZE(audio_spec_));
    if (frame_size_bytes_ == 0) {
        AudioDebugLog("decode wav invalid frame size " + AudioSpecSummary(audio_spec_));
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

    AudioDebugLog("decode wav ok " + AudioSpecSummary(audio_spec_) +
                  " bytes=" + std::to_string(audio_data_.size()) +
                  " frame_size=" + std::to_string(frame_size_bytes_) +
                  " duration=" + std::to_string(duration_seconds_));
    DispatchMediaEvent("loadedmetadata");
    DispatchMediaEvent("canplay");
    RequestAudioRepaint();
    return true;
}

bool HTMLAudioElement::StartStreamAt(double seconds) {
    if (!HasDecodedAudio()) {
        AudioDebugLog("start stream failed: no decoded audio");
        return false;
    }

    AudioDebugLog("start stream requested start_seconds=" + std::to_string(seconds) +
                  " " + AudioSpecSummary(audio_spec_) +
                  " bytes=" + std::to_string(audio_data_.size()) +
                  " frame_size=" + std::to_string(frame_size_bytes_));
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
            AudioDebugLog("start stream refused: requested at end");
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
        AudioDebugLog("start stream refused: byte offset at end offset=" + std::to_string(byte_offset));
        return false;
    }

    StopStream();
    current_time_seconds_ = start_seconds;
    playback_byte_offset_ = byte_offset;
    stream_end_flushed_ = false;
    ended_ = false;
    const SDL_AudioDeviceID playback_device = ResolvePlaybackDevice();
    stream_.reset(SDL_OpenAudioDeviceStream(
        playback_device,
        &audio_spec_,
        nullptr,
        nullptr));
    if (!stream_) {
        AudioDebugLog(std::string("open stream failed: ") + SDL_GetError());
        SetError(std::string("Failed to open SDL audio stream: ") + SDL_GetError());
        return false;
    }
    const SDL_AudioDeviceID stream_device = SDL_GetAudioStreamDevice(stream_.get());
    const char* stream_device_name = stream_device ? SDL_GetAudioDeviceName(stream_device) : nullptr;
    AudioDebugLog("open stream ok requested_device=" + std::to_string(playback_device) +
                  " stream_device=" + std::to_string(stream_device) +
                  " name=\"" + (stream_device_name ? stream_device_name : "<unknown>") + "\"" +
                  " offset=" + std::to_string(playback_byte_offset_) +
                  " " + AudioStreamFormatSummary(stream_.get()) +
                  " paused=" + std::to_string(SDL_AudioStreamDevicePaused(stream_.get())));

    ApplyStreamGain();
    const size_t bytes_per_second = frame_size_bytes_ * static_cast<size_t>(audio_spec_.freq);
    size_t initial_queue_bytes = static_cast<size_t>(
        static_cast<double>(bytes_per_second) * kAudioInitialQueueSeconds);
    initial_queue_bytes = std::max(initial_queue_bytes, frame_size_bytes_);
    initial_queue_bytes -= initial_queue_bytes % frame_size_bytes_;
    if (!loop_) {
        initial_queue_bytes = std::min(initial_queue_bytes, audio_data_.size() - playback_byte_offset_);
    }
    AudioDebugLog("initial queue bytes=" + std::to_string(initial_queue_bytes) +
                  " bytes_per_second=" + std::to_string(bytes_per_second));
    QueueAudioData(stream_.get(), static_cast<int>(std::min<size_t>(
        initial_queue_bytes,
        static_cast<size_t>(std::numeric_limits<int>::max()))));
    AudioDebugLog("after initial queue queued=" + std::to_string(SDL_GetAudioStreamQueued(stream_.get())) +
                  " available=" + std::to_string(SDL_GetAudioStreamAvailable(stream_.get())) +
                  " paused=" + std::to_string(SDL_AudioStreamDevicePaused(stream_.get())));

    if (!SDL_ResumeAudioStreamDevice(stream_.get())) {
        AudioDebugLog(std::string("resume stream failed: ") + SDL_GetError());
        stream_.reset();
        SetError(std::string("Failed to start SDL audio stream: ") + SDL_GetError());
        return false;
    }

    playback_started_ticks_ = SDL_GetTicks();
    last_queue_log_ticks_ = playback_started_ticks_;
    playback_start_time_seconds_ = current_time_seconds_;
    AudioDebugLog("resume stream ok ticks=" + std::to_string(playback_started_ticks_) +
                  " queued=" + std::to_string(SDL_GetAudioStreamQueued(stream_.get())) +
                  " available=" + std::to_string(SDL_GetAudioStreamAvailable(stream_.get())) +
                  " paused=" + std::to_string(SDL_AudioStreamDevicePaused(stream_.get())));
    return true;
}

void HTMLAudioElement::PumpAudioStream() {
    if (!stream_ || paused_ || audio_data_.empty() || stream_end_flushed_ ||
        frame_size_bytes_ == 0 || audio_spec_.freq <= 0) {
        return;
    }

    const size_t bytes_per_second = frame_size_bytes_ * static_cast<size_t>(audio_spec_.freq);
    size_t target_queue_bytes = static_cast<size_t>(
        static_cast<double>(bytes_per_second) * kAudioTargetQueueSeconds);
    target_queue_bytes = std::max(target_queue_bytes, frame_size_bytes_);
    target_queue_bytes -= target_queue_bytes % frame_size_bytes_;

    const int queued = SDL_GetAudioStreamQueued(stream_.get());
    if (queued < 0) {
        return;
    }

    const int available = SDL_GetAudioStreamAvailable(stream_.get());
    const Uint64 now = SDL_GetTicks();
    if (AudioDebugEnabled() &&
        (last_queue_log_ticks_ == 0 ||
         now - last_queue_log_ticks_ >= static_cast<Uint64>(kAudioDebugQueueIntervalSeconds * 1000.0))) {
        last_queue_log_ticks_ = now;
        AudioDebugLog("pump stream queued=" + std::to_string(queued) +
                      " available=" + std::to_string(available) +
                      " target=" + std::to_string(target_queue_bytes) +
                      " paused=" + std::to_string(SDL_AudioStreamDevicePaused(stream_.get())));
    }

    if (static_cast<size_t>(queued) >= target_queue_bytes) {
        return;
    }

    size_t request = target_queue_bytes - static_cast<size_t>(queued);
    if (!loop_) {
        request = std::min(request, audio_data_.size() - std::min(playback_byte_offset_, audio_data_.size()));
    }
    request -= request % frame_size_bytes_;
    if (request == 0) {
        return;
    }

    QueueAudioData(stream_.get(), static_cast<int>(std::min<size_t>(
        request,
        static_cast<size_t>(std::numeric_limits<int>::max()))));
}

void HTMLAudioElement::QueueAudioData(SDL_AudioStream* stream, int requested_bytes) {
    if (!stream || audio_data_.empty() || requested_bytes <= 0) {
        if (AudioDebugEnabled()) {
            AudioDebugLog("queue skipped stream=" + std::to_string(stream ? 1 : 0) +
                          " decoded_bytes=" + std::to_string(audio_data_.size()) +
                          " requested=" + std::to_string(requested_bytes));
        }
        return;
    }

    const Uint64 now = SDL_GetTicks();
    const bool should_log = AudioDebugEnabled() &&
        (last_queue_log_ticks_ == 0 ||
         now - last_queue_log_ticks_ >= static_cast<Uint64>(kAudioDebugQueueIntervalSeconds * 1000.0));
    const size_t before_offset = playback_byte_offset_;
    int remaining_request = requested_bytes;
    size_t queued_bytes = 0;
    while (remaining_request > 0 && !audio_data_.empty()) {
        if (playback_byte_offset_ >= audio_data_.size()) {
            if (!loop_) {
                if (!stream_end_flushed_) {
                    SDL_FlushAudioStream(stream);
                    stream_end_flushed_ = true;
                    AudioDebugLog("queue reached end and flushed stream");
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
            AudioDebugLog(std::string("queue put failed: ") + SDL_GetError());
            return;
        }

        playback_byte_offset_ += static_cast<size_t>(chunk);
        queued_bytes += static_cast<size_t>(chunk);
        remaining_request -= chunk;

        if (playback_byte_offset_ >= audio_data_.size() && !loop_) {
            if (!stream_end_flushed_) {
                SDL_FlushAudioStream(stream);
                stream_end_flushed_ = true;
                AudioDebugLog("queue reached end and flushed stream");
            }
            return;
        }
    }

    if (should_log) {
        last_queue_log_ticks_ = now;
        AudioDebugLog("queue data requested=" + std::to_string(requested_bytes) +
                      " queued=" + std::to_string(queued_bytes) +
                      " stream_queued=" + std::to_string(SDL_GetAudioStreamQueued(stream)) +
                      " available=" + std::to_string(SDL_GetAudioStreamAvailable(stream)) +
                      " offset=" + std::to_string(before_offset) +
                      "->" + std::to_string(playback_byte_offset_) +
                      " remaining=" + std::to_string(audio_data_.size() - std::min(playback_byte_offset_, audio_data_.size())));
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
        const float gain = muted_ ? 0.0f : static_cast<float>(volume_);
        SDL_SetAudioStreamGain(stream_.get(), gain);
        AudioDebugLog("set stream gain=" + std::to_string(gain));
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
        AudioDebugLog("playback ended duration=" + std::to_string(duration_seconds_));
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
    AudioDebugLog("error: " + (message.empty() ? std::string("Failed to load audio") : message));
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
