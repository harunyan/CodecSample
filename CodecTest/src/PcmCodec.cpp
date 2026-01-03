#include "../pch.h"
#include "PcmCodec.h"
#include "AudioCodecFactory.h"
#include <cstring>
#include <cstdlib>

// 単純にそのままコピーする PCM コーデック（エンコード=パススルー、デコード=パススルー）
namespace CodecTest
{
    bool PcmCodec::Initialize(int sampleRate, int channels, int bitsPerSample)
    {
        m_sampleRate = sampleRate;
        m_channels = channels;
        m_bitsPerSample = bitsPerSample;
        return true;
    }

    std::vector<uint8_t> PcmCodec::Encode(const void* pcmData, size_t pcmBytes)
    {
        // 実際は圧縮するが、ひな形としてそのままコピーする
        std::vector<uint8_t> out;
        if (pcmData == nullptr || pcmBytes == 0) return out;
        out.resize(pcmBytes);
        std::memcpy(out.data(), pcmData, pcmBytes);
        return out;
    }

    std::vector<uint8_t> PcmCodec::Decode(const void* codedData, size_t codedBytes)
    {
        // そのままコピー
        std::vector<uint8_t> out;
        if (codedData == nullptr || codedBytes == 0) return out;
        out.resize(codedBytes);
        std::memcpy(out.data(), codedData, codedBytes);
        return out;
    }

    void PcmCodec::Reset()
    {
        m_sampleRate = 0;
        m_channels = 0;
        m_bitsPerSample = 0;
    }

    // ライブラリ起動時に登録するための静的初期化子
    namespace {
        const bool registered = []() {
            AudioCodecFactory::Instance().Register("pcm", []() -> std::unique_ptr<IAudioCodec> {
                return std::make_unique<PcmCodec>();
            });
            return true;
        }();
    }
}
