#pragma once

#include "../element.h"
#include "include/core/SkRect.h"
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_stdinc.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace mbink {

enum class AudioLoadState {
    EMPTY,
    LOADING,
    READY,
    ERROR
};

class HTMLAudioElement : public Element {
public:
    enum class ControlPart {
        None,
        PlayButton,
        ProgressTrack,
        VolumeTrack
    };

    struct ControlGeometry {
        SkRect play_button = SkRect::MakeEmpty();
        SkRect progress_track = SkRect::MakeEmpty();
        SkRect volume_track = SkRect::MakeEmpty();
        SkRect bounds = SkRect::MakeEmpty();
    };

    HTMLAudioElement();
    ~HTMLAudioElement() override;

    std::string GetSrc() const { return src_; }
    void SetSrc(const std::string& src);

    bool GetControls() const { return controls_; }
    void SetControls(bool controls);

    bool GetLoop() const { return loop_; }
    void SetLoop(bool loop);

    bool GetPaused();
    bool GetEnded();
    double CurrentTime();
    void SetCurrentTime(double seconds);
    double GetDuration() const { return duration_seconds_; }

    double GetVolume() const { return volume_; }
    void SetVolume(double volume);
    bool GetMuted() const { return muted_; }
    void SetMuted(bool muted);

    AudioLoadState GetLoadState() const { return load_state_; }
    std::string GetError() const { return error_message_; }
    bool HasDecodedAudio() const { return !audio_data_.empty() && duration_seconds_ > 0.0; }

    bool Load();
    bool Play();
    void Pause();
    void ScheduleControlsRepaint();
    static void TickActiveAudioElements();

    void SetAttribute(const std::string& name, const std::string& value) override;
    void RemoveAttribute(const std::string& name) override;

    ControlGeometry ComputeControlGeometry(float content_x,
                                           float content_y,
                                           float content_width,
                                           float content_height) const;
    ControlPart HitTestControls(float local_x,
                                float local_y,
                                float content_x,
                                float content_y,
                                float content_width,
                                float content_height) const;
    bool HandleControlMouseDown(float local_x,
                                float local_y,
                                float content_x,
                                float content_y,
                                float content_width,
                                float content_height);
    bool HandleControlMouseMove(float local_x,
                                float local_y,
                                float content_x,
                                float content_y,
                                float content_width,
                                float content_height);
    bool HandleControlMouseUp();
    bool IsDraggingControls() const { return dragging_part_ != ControlPart::None; }

private:
    struct SDLStreamDeleter {
        void operator()(SDL_AudioStream* stream) const;
    };

    using SDLStreamPtr = std::unique_ptr<SDL_AudioStream, SDLStreamDeleter>;

    void ResetDecodedAudio();
    void StopStream();
    bool EnsureAudioSubsystem();
    bool LoadFromResolvedPath(const std::string& resolved_path);
    bool LoadFromMemory(const std::vector<uint8_t>& data);
    bool AcceptsSourceAsWav(const std::string& src) const;
    bool DecodeWavFromIO(SDL_IOStream* io, bool close_io);
    bool StartStreamAt(double seconds);
    void QueueAudioData(SDL_AudioStream* stream, int requested_bytes);
    size_t ByteOffsetForTime(double seconds) const;
    double TimeForByteOffset(size_t byte_offset) const;
    double PlaybackTimeFromClock() const;
    double ClampTime(double seconds) const;
    void SeekFromRatio(float ratio);
    void SetVolumeFromRatio(float ratio);
    void ApplyStreamGain();
    void RefreshPlaybackState();
    void StartProgressUpdates();
    void StopProgressUpdates();
    void SetError(const std::string& message);
    void DispatchMediaEvent(const std::string& type);
    void RequestAudioRepaint();

    static bool ParseBoolAttributeValue(const std::string& value);
    static std::string LowercasePath(const std::string& value);

private:
    std::string src_;
    bool controls_ = false;
    bool paused_ = true;
    bool ended_ = false;
    bool loop_ = false;
    double current_time_seconds_ = 0.0;
    double duration_seconds_ = 0.0;
    double volume_ = 1.0;
    bool muted_ = false;
    AudioLoadState load_state_ = AudioLoadState::EMPTY;
    std::string error_message_;

    SDL_AudioSpec audio_spec_{};
    std::vector<uint8_t> audio_data_;
    size_t frame_size_bytes_ = 0;
    SDLStreamPtr stream_;
    Uint64 playback_started_ticks_ = 0;
    double playback_start_time_seconds_ = 0.0;
    size_t playback_byte_offset_ = 0;
    bool stream_end_flushed_ = false;
    bool progress_updates_registered_ = false;

    ControlPart dragging_part_ = ControlPart::None;
};

} // namespace mbink
