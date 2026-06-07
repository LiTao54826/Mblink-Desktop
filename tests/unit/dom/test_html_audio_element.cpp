#include <gtest/gtest.h>

#include "dom/document.h"
#include "dom/elements/html_audio_element.h"

#include <cstdint>
#include <string>
#include <vector>

namespace mbink {
namespace test {

namespace {

std::vector<uint8_t> MakeTinyWav() {
    return {
        'R', 'I', 'F', 'F',
        38, 0, 0, 0,
        'W', 'A', 'V', 'E',
        'f', 'm', 't', ' ',
        16, 0, 0, 0,
        1, 0,
        1, 0,
        0x40, 0x1F, 0, 0,
        0x80, 0x3E, 0, 0,
        2, 0,
        16, 0,
        'd', 'a', 't', 'a',
        2, 0, 0, 0,
        0, 0
    };
}

} // namespace

TEST(HTMLAudioElementTest, DocumentCreateElementReturnsAudioElement) {
    auto document = std::make_shared<Document>();
    document->Initialize();

    auto element = document->CreateElement("audio");

    ASSERT_NE(element, nullptr);
    EXPECT_EQ(element->GetTagName(), "audio");
    EXPECT_NE(std::dynamic_pointer_cast<HTMLAudioElement>(element), nullptr);
}

TEST(HTMLAudioElementTest, LoadsWavFromAssetProviderWithoutAudioDevice) {
    Document::SetAssetProvider([](const std::string& path, std::vector<uint8_t>& data) {
        if (path == "tone.wav") {
            data = MakeTinyWav();
            return true;
        }
        return false;
    });

    auto document = std::make_shared<Document>();
    document->Initialize();
    auto audio = std::dynamic_pointer_cast<HTMLAudioElement>(document->CreateElement("audio"));
    ASSERT_NE(audio, nullptr);

    audio->SetSrc("tone.wav");

    EXPECT_EQ(audio->GetLoadState(), AudioLoadState::READY);
    EXPECT_TRUE(audio->HasDecodedAudio());
    EXPECT_GT(audio->GetDuration(), 0.0);
    EXPECT_TRUE(audio->GetError().empty());

    Document::SetAssetProvider(nullptr);
}

TEST(HTMLAudioElementTest, RejectsUnsupportedSources) {
    auto document = std::make_shared<Document>();
    document->Initialize();
    auto audio = std::dynamic_pointer_cast<HTMLAudioElement>(document->CreateElement("audio"));
    ASSERT_NE(audio, nullptr);

    audio->SetSrc("tone.mp3");
    EXPECT_EQ(audio->GetLoadState(), AudioLoadState::ERROR);
    EXPECT_FALSE(audio->GetError().empty());

    audio->SetSrc("https://example.test/tone.wav");
    EXPECT_EQ(audio->GetLoadState(), AudioLoadState::ERROR);
    EXPECT_FALSE(audio->GetError().empty());
}

TEST(HTMLAudioElementTest, ReflectsCommonAudioAttributes) {
    auto audio = std::make_shared<HTMLAudioElement>();

    audio->SetAttribute("controls", "");
    audio->SetAttribute("loop", "true");
    audio->SetAttribute("muted", "");
    audio->SetAttribute("volume", "2");

    EXPECT_TRUE(audio->GetControls());
    EXPECT_TRUE(audio->GetLoop());
    EXPECT_TRUE(audio->GetMuted());
    EXPECT_DOUBLE_EQ(audio->GetVolume(), 1.0);

    audio->SetVolume(-1.0);
    EXPECT_DOUBLE_EQ(audio->GetVolume(), 0.0);

    audio->RemoveAttribute("controls");
    audio->RemoveAttribute("loop");
    audio->RemoveAttribute("muted");
    EXPECT_FALSE(audio->GetControls());
    EXPECT_FALSE(audio->GetLoop());
    EXPECT_FALSE(audio->GetMuted());

    audio->SetAttribute("controls", "false");
    EXPECT_TRUE(audio->GetControls());
}

TEST(HTMLAudioElementTest, ControlDraggingUpdatesSeekAndVolume) {
    Document::SetAssetProvider([](const std::string& path, std::vector<uint8_t>& data) {
        if (path == "tone.wav") {
            data = MakeTinyWav();
            return true;
        }
        return false;
    });

    auto document = std::make_shared<Document>();
    document->Initialize();
    auto audio = std::dynamic_pointer_cast<HTMLAudioElement>(document->CreateElement("audio"));
    ASSERT_NE(audio, nullptr);

    audio->SetControls(true);
    audio->SetSrc("tone.wav");
    ASSERT_TRUE(audio->HasDecodedAudio());
    ASSERT_GT(audio->GetDuration(), 0.0);

    const auto geometry = audio->ComputeControlGeometry(0.0f, 0.0f, 320.0f, 36.0f);
    ASSERT_GT(geometry.progress_track.width(), 0.0f);
    ASSERT_GT(geometry.volume_track.width(), 0.0f);

    EXPECT_TRUE(audio->HandleControlMouseDown(geometry.progress_track.left() + geometry.progress_track.width() * 0.5f,
                                              geometry.progress_track.centerY(),
                                              0.0f,
                                              0.0f,
                                              320.0f,
                                              36.0f));
    EXPECT_NEAR(audio->CurrentTime(), audio->GetDuration() * 0.5, audio->GetDuration() * 0.2);
    EXPECT_TRUE(audio->HandleControlMouseUp());

    EXPECT_TRUE(audio->HandleControlMouseDown(geometry.volume_track.left() + geometry.volume_track.width() * 0.25f,
                                              geometry.volume_track.centerY(),
                                              0.0f,
                                              0.0f,
                                              320.0f,
                                              36.0f));
    EXPECT_NEAR(audio->GetVolume(), 0.25, 0.01);
    EXPECT_TRUE(audio->HandleControlMouseMove(geometry.volume_track.right(),
                                              geometry.volume_track.centerY(),
                                              0.0f,
                                              0.0f,
                                              320.0f,
                                              36.0f));
    EXPECT_NEAR(audio->GetVolume(), 1.0, 0.01);
    EXPECT_TRUE(audio->HandleControlMouseUp());

    Document::SetAssetProvider(nullptr);
}

} // namespace test
} // namespace mbink
