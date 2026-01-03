#pragma once
#include "../include/IAudioCodec.h"

namespace CodecTest
{
    class LdacCodec final : public IAudioCodec
    {
    public:
        LdacCodec();
        ~LdacCodec() override;

        bool Initialize(int sampleRate, int channels, int bitsPerSample) override;
        std::vector<uint8_t> Encode(const void* pcmData, size_t pcmBytes) override;
        std::vector<uint8_t> Decode(const void* codedData, size_t codedBytes) override;
        void GetFormat(int& sampleRate, int& channels, int& bitsPerSample) const override {
            sampleRate = m_sampleRate;
            channels = m_channels;
            bitsPerSample = m_bitsPerSample;
        }
        void Reset() override;
        std::string Name() const override { return "ldac"; }

    private:
        void* m_hLdac{ nullptr }; // HANDLE_LDAC_BT
        void* m_hDec{ nullptr };  // ldacdec_t*
        int m_sampleRate{ 0 };
        int m_channels{ 0 };
        int m_bitsPerSample{ 0 };
    };
}