#include <cassert>
#include <thread>
#include <chrono>

#include <Usagi/Utility/Utf8Main.hpp>
#include <Usagi/Extensions/RtPortAudio/PortAudioAudioManager.hpp>
#include <Usagi/Runtime/Audio/AudioDevice.hpp>
#include <Usagi/Runtime/Audio/AudioOutputStream.hpp>

using namespace usagi;

namespace
{
// class code from portaudio/bindings/cpp/example/sine.cxx:

// Some constants:
const int NUM_SECONDS = 5;
const double SAMPLE_RATE = 44100.0;
const int FRAMES_PER_BUFFER = 64;
const int TABLE_SIZE = 200;

// ---------------------------------------------------------------------------------------

// SineGenerator class:
class SineGenerator
{
public:
    SineGenerator(int tableSize)
        : tableSize_(tableSize)
        , leftPhase_(0)
        , rightPhase_(0)
    {
        const double PI = 3.14159265;
        table_ = new float[tableSize];
        for(int i = 0; i < tableSize; ++i)
        {
            table_[i] = 0.125f * (float)sin(
                ((double)i / (double)tableSize) * PI * 2.);
        }
    }

    ~SineGenerator()
    {
        delete[] table_;
    }

    int generate(
        const void *inputBuffer,
        void *outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo *timeInfo,
        PaStreamCallbackFlags statusFlags)
    {
        assert(outputBuffer != NULL);

        float **out = static_cast<float **>(outputBuffer);

        for(unsigned int i = 0; i < framesPerBuffer; ++i)
        {
            out[0][i] = table_[leftPhase_];
            out[1][i] = table_[rightPhase_];

            leftPhase_ += 1;
            if(leftPhase_ >= tableSize_)
                leftPhase_ -= tableSize_;

            rightPhase_ += 3;
            if(rightPhase_ >= tableSize_)
                rightPhase_ -= tableSize_;
        }

        return paContinue;
    }

private:
    float *table_;
    int tableSize_;
    int leftPhase_;
    int rightPhase_;
};
}

int usagi_main(const std::vector<std::string> &args)
{
    SineGenerator sine_generator(TABLE_SIZE);

    PortAudioAudioManager manager;
    manager.enumerateDevices();

    AudioStreamFormat format;
    format.format = DataFormat::FLOAT32;
    format.num_channels = 2;
    format.interleaved = false;
    format.sample_rate = SAMPLE_RATE;

    auto stream = manager.defaultOutputDevice().acquireDevice()->
        createOutputStream(format,
            [&](void *output_buffer, const std::size_t frames) {
                return static_cast<AudioStreamStatus>(
                    sine_generator.generate(
                        nullptr,
                        output_buffer,
                        static_cast<unsigned long>(frames),
                        nullptr,
                        { }
                    ));
            });

    using namespace std::chrono_literals;

    stream->start();
    std::this_thread::sleep_for(5s);
    stream->stop();

    return 0;
}
