#include <fstream>
#include <filesystem>

#include <Usagi/Utility/Utf8Main.hpp>
#include <Usagi/Extensions/AssetAudioFormats/LibNyquistAudioAssetDecoder.hpp>
#include <Usagi/Core/Logging.hpp>

using namespace usagi;

int usagi_main(const std::vector<std::string> &args)
{
    if(args.size() < 1)
        throw std::runtime_error("usage: <file path>");

    const LibNyquistAudioAssetDecoder decoder;
    auto path = std::filesystem::u8path(args[0]);
    std::ifstream in(path, std::ios::binary);
    auto audio = decoder(in);

    LOG(info, "File:        {}", path);
    LOG(info, "Channels:    {}", audio.format.num_channels);
    LOG(info, "Sample Rate: {}", audio.format.sample_rate);
    LOG(info, "Interleaved: {}", audio.format.interleaved);
    LOG(info, "Format:      {}", to_string(audio.format.format));
    LOG(info, "Frame Count: {}", audio.num_frames);

    return 0;
}
