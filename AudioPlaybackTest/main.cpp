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
#include <Usagi/Utility/CyclicContainerAdapter.hpp>

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

    utility::CyclicContainerAdapter circular_buffer {
        audio->samples,
        audio->samples.begin(),
        static_cast<std::size_t>(loop ? -1 : 1)
    };
    auto it = circular_buffer.begin();
    auto end = circular_buffer.end();

    std::promise<void> play_finished_promise;
    const auto play_finished_future = play_finished_promise.get_future();

    auto stream = runtime->audioManager()->defaultOutputDevice()
        .acquireDevice()->createOutputStream(audio->format, [&](
            std::uint8_t *output_buffer,
            const std::size_t frames) -> AudioStreamStatus {
                auto flt_out = reinterpret_cast<float*>(output_buffer);
                std::size_t frames_written = 0;
                while(it != end && frames_written < frames)
                {
                    for(std::size_t c = 0; c < audio->format.num_channels; ++c)
                    {
                        *(flt_out++) = *(it++);
                    }
                    ++frames_written;
                }
                // handle end-of-stream
                if(frames_written < frames)
                {
                    const auto bytes_per_frame =
                        audio->format.num_channels * sizeof(float);
                    memset(
                        output_buffer + frames_written * bytes_per_frame,
                        0,
                        (frames - frames_written) * bytes_per_frame
                    );
                    play_finished_promise.set_value();
                    return AudioStreamStatus::STOP;
                }
                return AudioStreamStatus::CONTINUE;
            });

    stream->start();

    play_finished_future.wait();

    return 0;
}
