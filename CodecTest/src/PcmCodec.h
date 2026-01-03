#pragma once

#include "../include/IAudioCodec.h"

namespace CodecTest
{
    // PCM のパススルー実装（サンプル）
    class PcmCodec final : public IAudioCodec
    {
    public:
        PcmCodec() = default;
        ~PcmCodec() override = default;

        bool Initialize(int sampleRate, int channels, int bitsPerSample) override;
        std::vector<uint8_t> Encode(const void* pcmData, size_t pcmBytes) override;
        std::vector<uint8_t> Decode(const void* codedData, size_t codedBytes) override;
        void GetFormat(int& sampleRate, int& channels, int& bitsPerSample) const override {
            sampleRate = m_sampleRate;
            channels = m_channels;
            bitsPerSample = m_bitsPerSample;
        }
        void Reset() override;
        std::string Name() const override { return "pcm"; }

    private:
        int m_sampleRate{ 0 };
        int m_channels{ 0 };
        int m_bitsPerSample{ 0 };
    };
}
