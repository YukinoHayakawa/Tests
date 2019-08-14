#include <fstream>
#include <filesystem>

#include <Usagi/Utility/Utf8Main.hpp>
#include <Usagi/Core/Logging.hpp>
#include <Usagi/Extensions/AssetAudioFormats/OggVorbisAudioAssetDecoder.hpp>

using namespace usagi;

int usagi_main(const std::vector<std::string> &args)
{
    if(args.size() < 1)
        USAGI_THROW(std::runtime_error("usage: <file path>"));

    const OggVorbisAudioAssetDecoder decoder;
    const auto path = std::filesystem::u8path(args[0]);
    std::ifstream in(path, std::ios::binary);
    auto audio = decoder(in);

    LOG(info, "File:            {}", args[0]);
    LOG(info, "Num Channels:    {}", audio.channels.size());
    LOG(info, "Sample Rate:     {}", audio.sample_rate);
    LOG(info, "Num Frames:      {}", audio.frames);
    LOG(info, "Time:            {}s", audio.time);

    return 0;
}
