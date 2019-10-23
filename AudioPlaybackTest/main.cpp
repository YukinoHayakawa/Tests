#include <filesystem>
#include <future>

#include <Usagi/Utility/Utf8Main.hpp>
#include <Usagi/Asset/Package/Filesystem/FilesystemAssetPackage.hpp>
#include <Usagi/Game/Game.hpp>
#include <Usagi/Asset/AssetRoot.hpp>
#include <Usagi/Extensions/RtWin32/Win32Runtime.hpp>
#include <Usagi/Extensions/AssetAudioFormats/PassthroughAudioAssetConverter.hpp>
#include <Usagi/Runtime/Audio/AudioManager.hpp>
#include <Usagi/Runtime/Audio/AudioDevice.hpp>
#include <Usagi/Runtime/Audio/AudioOutputStream.hpp>
#include <Usagi/Utility/RotateCounter.hpp>

using namespace usagi;

int usagi_main(const std::vector<std::string> &args)
{
    if(args.size() < 1)
        USAGI_THROW(std::runtime_error("usage: <file path> <loop>"));

    const auto path = std::filesystem::u8path(args[0]);
    auto loop = true;
    if(args.size() > 1)
        loop = args[1] == "true";

    auto runtime = std::make_shared<Win32Runtime>();
    runtime->initAudio();
    Game game { runtime };

    game.assets()->addChild<FilesystemAssetPackage>(
        "test", path.parent_path());
    auto audio = game.assets()->res<PassthroughAudioAssetConverter>(
        path.filename().u8string());

    RotateCounter cursor { audio->frames };
    AudioStreamFormat format;
    format.format = DataFormat::FLOAT32;
    format.num_channels = static_cast<uint8_t>(audio->channels.size());
    format.interleaved = false;
    format.sample_rate = audio->sample_rate;

    std::promise<void> play_finished_promise;
    const auto play_finished_future = play_finished_promise.get_future();

    auto stream = runtime->audioManager()->defaultOutputDevice()
        .acquireDevice()->createOutputStream(format, [&](
            const ArrayView<std::byte*> &channels,
            const std::size_t frames) -> AudioStreamStatus {
                assert(channels.size() == audio->channels.size());
                // todo perf
                std::size_t written = 0;
                for(; written < frames; ++written)
                {
                    for(std::size_t c = 0; c < format.num_channels; ++c)
                    {
                        const auto ch = reinterpret_cast<float*>(channels[c]);
                        ch[written] = audio->channels[c][cursor];
                    }
                    ++cursor;
                    if(cursor.current() == 0 && !loop)
                        break;
                }
                if(written != frames)
                {
                    assert(written < frames);
                    for(std::size_t c = 0; c < format.num_channels; ++c)
                    {
                        const auto ch = reinterpret_cast<float*>(channels[c]);
                        memset(ch + written, 0, frames - written);
                    }
                    play_finished_promise.set_value();
                    return AudioStreamStatus::STOP;
                }
                return AudioStreamStatus::CONTINUE;
            });

    stream->start();

    play_finished_future.wait();

    return 0;
}
