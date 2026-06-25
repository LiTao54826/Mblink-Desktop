#include <gtest/gtest.h>

#include "dom/document.h"
#include "dom/elements/html_audio_element.h"
#include "core/api/mblink.h"
#include "core/lexbor/lexbor_stylesheet.h"
#include "core/network/fetch_bindings.h"
#include "core/render/image/image_loader.h"
#include "core/utils/encoding_utils.h"

#include <SDL3/SDL.h>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace mblink {
namespace test {

namespace {

void AppendU16(std::vector<uint8_t>& out, uint16_t value) {
    out.push_back(static_cast<uint8_t>(value & 0xff));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xff));
}

void AppendU32(std::vector<uint8_t>& out, uint32_t value) {
    out.push_back(static_cast<uint8_t>(value & 0xff));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xff));
    out.push_back(static_cast<uint8_t>((value >> 16) & 0xff));
    out.push_back(static_cast<uint8_t>((value >> 24) & 0xff));
}

std::vector<uint8_t> MakeSilentWav(uint32_t frames) {
    constexpr uint16_t kChannels = 1;
    constexpr uint16_t kBitsPerSample = 16;
    constexpr uint32_t kSampleRate = 8000;
    constexpr uint16_t kBlockAlign = kChannels * (kBitsPerSample / 8);
    constexpr uint32_t kByteRate = kSampleRate * kBlockAlign;

    const uint32_t data_size = frames * kBlockAlign;
    std::vector<uint8_t> wav;
    wav.reserve(44 + data_size);
    wav.insert(wav.end(), {'R', 'I', 'F', 'F'});
    AppendU32(wav, 36 + data_size);
    wav.insert(wav.end(), {'W', 'A', 'V', 'E'});
    wav.insert(wav.end(), {'f', 'm', 't', ' '});
    AppendU32(wav, 16);
    AppendU16(wav, 1);
    AppendU16(wav, kChannels);
    AppendU32(wav, kSampleRate);
    AppendU32(wav, kByteRate);
    AppendU16(wav, kBlockAlign);
    AppendU16(wav, kBitsPerSample);
    wav.insert(wav.end(), {'d', 'a', 't', 'a'});
    AppendU32(wav, data_size);
    wav.resize(44 + data_size, 0);
    return wav;
}

std::vector<uint8_t> MakeTinyWav() {
    return MakeSilentWav(1);
}

bool WriteBinaryFile(const std::filesystem::path& path, const std::vector<uint8_t>& data) {
    std::ofstream out(path, std::ios::binary);
    return out.is_open() &&
           (data.empty() || static_cast<bool>(out.write(reinterpret_cast<const char*>(data.data()),
                                                        static_cast<std::streamsize>(data.size()))));
}

bool WriteTextFile(const std::filesystem::path& path, const std::string& data) {
    std::ofstream out(path, std::ios::binary);
    return out.is_open() && static_cast<bool>(out.write(data.data(), static_cast<std::streamsize>(data.size())));
}

std::string PathToUtf8(const std::filesystem::path& path) {
#ifdef _WIN32
    return utils::WideToUTF8(path.wstring());
#else
    return path.string();
#endif
}

void ClearGlobalAssetProviders() {
    Document::SetAssetProvider(nullptr);
    FetchBindings::SetAssetProvider(nullptr);
    ImageLoader::SetAssetProvider(nullptr);
    LexborStyleSheet::SetAssetProvider(nullptr);
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
            data = MakeSilentWav(8000);
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
    ASSERT_GT(geometry.time_label.width(), 0.0f);
    ASSERT_GT(geometry.volume_track.width(), 0.0f);
    EXPECT_LT(geometry.progress_track.right(), geometry.time_label.left());
    EXPECT_LT(geometry.time_label.right(), geometry.volume_track.left());
    EXPECT_EQ(audio->HitTestControls(geometry.time_label.centerX(),
                                     geometry.time_label.centerY(),
                                     0.0f,
                                     0.0f,
                                     320.0f,
                                     36.0f),
              HTMLAudioElement::ControlPart::None);

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

TEST(HTMLAudioElementTest, PlayStartsSDLStreamWithDummyAudioDriver) {
    if (!SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy")) {
        GTEST_SKIP() << "SDL audio driver hint was already locked";
    }

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
    ASSERT_TRUE(audio->HasDecodedAudio()) << audio->GetError();

    EXPECT_TRUE(audio->Play()) << audio->GetError();
    EXPECT_FALSE(audio->GetPaused());
    audio->Pause();
    EXPECT_TRUE(audio->GetPaused());

    Document::SetAssetProvider(nullptr);
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

TEST(HTMLAudioElementTest, MountedResourcePackageResolvesAudioRelativeToLoadedJS) {
    const auto temp_root = std::filesystem::temp_directory_path() / "mblink-audio-resource-package-test";
    const auto input_dir = temp_root / "app";
    const auto package_file = temp_root / "app.mbrp";
    std::error_code ec;
    std::filesystem::remove_all(temp_root, ec);
    ASSERT_TRUE(std::filesystem::create_directories(input_dir, ec) || std::filesystem::exists(input_dir));

    ASSERT_TRUE(WriteTextFile(input_dir / "app.js", R"JS(
const audio = document.createElement('audio');
audio.src = 'tone.wav';
document.body.appendChild(audio);
globalThis.__audioPackageResult = {
  src: audio.src,
  loaded: audio.load(),
  duration: audio.duration,
  error: audio.error
};
)JS"));
    ASSERT_TRUE(WriteBinaryFile(input_dir / "tone.wav", MakeSilentWav(8000)));

    std::string input_text = PathToUtf8(input_dir);
    std::string package_text = PathToUtf8(package_file);
    ASSERT_EQ(mblink_init(), MBLINK_OK);
    ASSERT_EQ(mblink_compile_resources(input_text.c_str(), package_text.c_str(), ""), MBLINK_OK)
        << mblink_last_error();

    void* resource_data = nullptr;
    size_t resource_size = 0;
    uint32_t resource_flags = 0;
    ASSERT_EQ(mblink_load_resource_file(package_text.c_str(), "app/tone.wav", "",
                                       &resource_data, &resource_size, &resource_flags), MBLINK_OK)
        << mblink_last_error();
    EXPECT_EQ(resource_size, MakeSilentWav(8000).size());
    mblink_free(resource_data);

    MBlinkConfig config = mblink_default_config();
    config.headless = true;
    config.gpu = false;
    config.width = 320;
    config.height = 120;
    MBlinkHandle handle = mblink_create_ex(&config);
    ASSERT_NE(handle, nullptr) << mblink_last_error();

    ASSERT_EQ(mblink_mount_resource_package(handle, package_text.c_str(), "", "/"), MBLINK_OK)
        << mblink_last_error();
    ASSERT_EQ(mblink_load_js_file(handle, "/app/app.js"), MBLINK_OK)
        << mblink_last_error();

    ASSERT_EQ(mblink_eval_js(handle, R"JS(
(function(result) {
  if (!result || result.src !== 'tone.wav' || result.loaded !== true ||
      result.duration !== 1 || result.error) {
    throw new Error(JSON.stringify(result));
  }
  return true;
})(globalThis.__audioPackageResult)
)JS"), MBLINK_OK) << mblink_last_error();

    mblink_destroy(handle);
    mblink_cleanup();
    ClearGlobalAssetProviders();
    std::filesystem::remove_all(temp_root, ec);
}

} // namespace test
} // namespace mblink
