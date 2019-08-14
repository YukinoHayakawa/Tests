#include <filesystem>
#include <thread>

#include <Usagi/Utility/Utf8Main.hpp>
#include <Usagi/Extension/Win32/Win32Runtime.hpp>
#include <Usagi/Extensions/AssetAudioFormats/PassthroughAudioAssetConverter.hpp>
#include <Usagi/Game/Game.hpp>
#include <Usagi/Asset/Package/Filesystem/FilesystemAssetPackage.hpp>
#include <Usagi/Asset/AssetRoot.hpp>
#include <Usagi/Game/GameStateManager.hpp>
#include <Usagi/Game/GameState.hpp>
#include <Usagi/Extensions/SysBasicAudioMixing/BasicAudioMixingSystem.hpp>

using namespace usagi;
namespace fs = std::filesystem;

int usagi_main(const std::vector<std::string> &args)
{
    const auto path = fs::u8path(args.size() < 1 ? "." : args[0]);

    auto runtime = std::make_shared<Win32Runtime>();
    runtime->initAudio();

    Game game { runtime };

    game.assets()->addChild<FilesystemAssetPackage>(
        "test", path);

    auto state = game.states()->pushState<GameState>("test");
    state->addSystem<BasicAudioMixingSystem>("audio_mix",
        runtime->audioManager());

    for(auto &&p : fs::directory_iterator(path))
    {
        auto filename = p.path().filename();
        if(filename.extension() != ".ogg")
            continue;
        const auto name = filename.u8string();
        auto buf = game.assets()->res<PassthroughAudioAssetConverter>(name);
        auto c = state->addChild(name);
        c->addComponent<BasicAudioStreamComponent>(
            std::move(buf),
            true,
            true,
            1.f
        );
    }

    while(game.continueGame())
    {
        using namespace std::chrono_literals;
        game.frame();
        std::this_thread::sleep_for(16ms);
    }

    return 0;
}
