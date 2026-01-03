#include "pch.h"
#include "CodecApi.h"
#include "include/IAudioCodec.h"
#include "src/AudioCodecFactory.h"
#include <cstdlib>
#include <cstring>

using namespace CodecTest;

extern "C"
{

void* Codec_Create(const char* name)
{
    if (!name) return nullptr;
    auto codec = AudioCodecFactory::Instance().Create(name);
    if (!codec) return nullptr;
    // unique_ptr の管理から外して呼び出し元（void*）に委譲する
    return codec.release();
}

void Codec_Destroy(void* codec)
{
    if (!codec) return;
    IAudioCodec* c = static_cast<IAudioCodec*>(codec);
    delete c;
}

bool Codec_Initialize(void* codec, int sampleRate, int channels, int bitsPerSample)
{
    if (!codec) return false;
    IAudioCodec* c = static_cast<IAudioCodec*>(codec);
    return c->Initialize(sampleRate, channels, bitsPerSample);
}

uint8_t* Codec_Encode(void* codec, const void* input, size_t inSize, size_t* outSize)
{
    if (!codec || !input || !outSize) return nullptr;
    IAudioCodec* c = static_cast<IAudioCodec*>(codec);
    auto outVec = c->Encode(input, inSize);
    if (outVec.empty())
    {
        *outSize = 0;
        return nullptr;
    }
    *outSize = outVec.size();
    uint8_t* buf = static_cast<uint8_t*>(std::malloc(*outSize));
    if (!buf)
    {
        *outSize = 0;
        return nullptr;
    }
    std::memcpy(buf, outVec.data(), *outSize);
    return buf;
}

uint8_t* Codec_Decode(void* codec, const void* input, size_t inSize, size_t* outSize)
{
    if (!codec || !input || !outSize) return nullptr;
    IAudioCodec* c = static_cast<IAudioCodec*>(codec);
    auto outVec = c->Decode(input, inSize);
    if (outVec.empty())
    {
        *outSize = 0;
        return nullptr;
    }
    *outSize = outVec.size();
    uint8_t* buf = static_cast<uint8_t*>(std::malloc(*outSize));
    if (!buf)
    {
        *outSize = 0;
        return nullptr;
    }
    std::memcpy(buf, outVec.data(), *outSize);
    return buf;
}

bool Codec_GetLastFormat(void* codec, int* sampleRate, int* channels, int* bitsPerSample)
{
    if (!codec) return false;
    IAudioCodec* c = static_cast<IAudioCodec*>(codec);
    int r = 0, ch = 0, b = 0;
    c->GetFormat(r, ch, b);
    if (sampleRate) *sampleRate = r;
    if (channels) *channels = ch;
    if (bitsPerSample) *bitsPerSample = b;
    return true;
}

void Codec_FreeBuffer(uint8_t* buffer)
{
    if (buffer) std::free(buffer);
}

} // extern "C"
