#include <filesystem>
#include <future>

#include <Usagi/Utility/Utf8Main.hpp>
#include <Usagi/Asset/Package/Filesystem/FilesystemAssetPackage.hpp>
#include <Usagi/Game/Game.hpp>
#include <Usagi/Asset/AssetRoot.hpp>
#include <Usagi/Extension/Win32/Win32Runtime.hpp>
#include <Usagi/Extensions/AssetAudioFormats/PassthroughAudioAssetConverter.hpp>
#include <Usagi/Runtime/Audio/AudioManager.hpp>
#include <Usagi/Runtime/Audio/AudioDevice.hpp>
#include <Usagi/Runtime/Audio/AudioOutputStream.hpp>

using namespace usagi;

int usagi_main(const std::vector<std::string> &args)
{
    if(args.size() < 2)
        throw std::runtime_error("usage: <file path> <loop>");

    const auto path = std::filesystem::u8path(args[0]);
    const auto loop = args[1] == "true";

    auto runtime = std::make_shared<Win32Runtime>();
    runtime->initAudio();
    Game game { runtime };

    game.assets()->addChild<FilesystemAssetPackage>(
        "test", path.parent_path());
    auto audio = game.assets()->res<PassthroughAudioAssetConverter>(
        path.filename().u8string());

    std::size_t cursor = 0;
    std::promise<void> play_finished_promise;
    const auto play_finished_future = play_finished_promise.get_future();

    auto stream = runtime->audioManager()->defaultOutputDevice()
        .acquireDevice()->createOutputStream(audio->format,
            [&](
            std::uint8_t *output_buffer,
            std::size_t frames) -> AudioStreamStatus {
                const auto elements_to_read =
                    frames * audio->format.num_channels;
                const auto elements_left =
                    audio->samples.size() - cursor;
                const auto elements_actual_read = std::min(
                    elements_left, elements_to_read);
                const auto bytes_actual =
                    elements_actual_read * sizeof(float);
                memcpy(
                    output_buffer,
                    &audio->samples[cursor],
                    bytes_actual
                );
                // handle end-of-stream
                if(elements_actual_read != elements_to_read)
                {
                    const auto buffer_left = elements_to_read * sizeof(float) -
                        bytes_actual;
                    memset(output_buffer + bytes_actual, 0, buffer_left);
                }
                cursor += elements_actual_read;
                if(cursor >= audio->samples.size())
                {
                    play_finished_promise.set_value();
                    return AudioStreamStatus::STOP;
                }
                return AudioStreamStatus::CONTINUE;
            });

    stream->start();

    play_finished_future.wait();

    return 0;
}
