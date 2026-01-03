#include "../pch.h"
#include "LdacCodec.h"
#include "AudioCodecFactory.h"
#include "libldac/inc/ldacBT.h"
extern "C" {
#include "libldacdec/ldacdec.h"
}
#include <vector>
#include <cstring>
#include <algorithm>
#include <memory>

namespace CodecTest
{
    // Auto-registration
    namespace {
        const bool registered = []() {
            AudioCodecFactory::Instance().Register("ldac", []() -> std::unique_ptr<IAudioCodec> {
                return std::make_unique<LdacCodec>();
            });
            return true;
        }();
    }

    LdacCodec::LdacCodec()
    {
    }

    LdacCodec::~LdacCodec()
    {
        Reset();
    }

    bool LdacCodec::Initialize(int sampleRate, int channels, int bitsPerSample)
    {
        Reset();

        m_sampleRate = sampleRate;
        m_channels = channels;
        m_bitsPerSample = bitsPerSample;

        m_hLdac = ldacBT_get_handle();
        if (!m_hLdac) return false;

        // Configure LDAC
        int mtu = 990; // High Quality / Max MTU
        int eqmid = LDACBT_EQMID_HQ; // Force High Quality
        
        int cm = LDACBT_CHANNEL_MODE_STEREO;
        if (channels == 1) cm = LDACBT_CHANNEL_MODE_MONO;
        else if (channels == 2) cm = LDACBT_CHANNEL_MODE_STEREO;
        else {
             // Fallback or error? defaulting to stereo for now
             cm = LDACBT_CHANNEL_MODE_STEREO;
        }

        LDACBT_SMPL_FMT_T fmt = LDACBT_SMPL_FMT_S16;
        if (bitsPerSample == 16) fmt = LDACBT_SMPL_FMT_S16;
        else if (bitsPerSample == 24) fmt = LDACBT_SMPL_FMT_S24;
        else if (bitsPerSample == 32) fmt = LDACBT_SMPL_FMT_S32;
        else if (bitsPerSample == 32) fmt = LDACBT_SMPL_FMT_F32; // Assuming float if 32-bit (needs context, but usually PCM is int)
        // Note: For simplicity assuming integer PCM for 32-bit here unless float is explicit. 
        // Standard Windows audio might be float, but let's assume S32 for now.
        
        int ret = ldacBT_init_handle_encode((HANDLE_LDAC_BT)m_hLdac, mtu, eqmid, cm, fmt, sampleRate);
        return (ret == 0);
    }

    std::vector<uint8_t> LdacCodec::Encode(const void* pcmData, size_t pcmBytes)
    {
        if (!m_hLdac) return {};

        std::vector<uint8_t> outBuffer;
        
        // LDAC expects fixed 128 samples per channel
        int bytesPerSample = m_bitsPerSample / 8;
        int frameSize = 128 * m_channels * bytesPerSample; 
        
        const uint8_t* src = static_cast<const uint8_t*>(pcmData);
        size_t processed = 0;

        unsigned char streamBuf[LDACBT_MAX_NBYTES]; 

        while (processed < pcmBytes)
        {
            // Prepare 128-sample block
            std::vector<uint8_t> inputBlock(frameSize, 0); // Zero init for padding
            size_t remain = pcmBytes - processed;
            size_t copySize = (remain < (size_t)frameSize) ? remain : (size_t)frameSize;
            
            std::memcpy(inputBlock.data(), src + processed, copySize);

            int pcm_used = 0;
            int stream_sz = 0;
            int frame_num = 0;

            int ret = ldacBT_encode((HANDLE_LDAC_BT)m_hLdac, 
                                    inputBlock.data(), 
                                    &pcm_used, 
                                    streamBuf, 
                                    &stream_sz, 
                                    &frame_num);
            
            if (ret != 0) {
                // Encoding error (fatal or non-fatal)
                // We might want to handle it, but for now just stop or continue?
                // ldacBT_get_error_code(m_hLdac) could be used.
                // If we stop here, we return what we have.
                break;
            }

            if (stream_sz > 0) {
                outBuffer.insert(outBuffer.end(), streamBuf, streamBuf + stream_sz);
            }
            
            processed += copySize;
        }

        return outBuffer;
    }

    std::vector<uint8_t> LdacCodec::Decode(const void* codedData, size_t codedBytes)
    {
        if (!codedData || codedBytes == 0) return {};

        if (!m_hDec) {
             m_hDec = new ldacdec_t;
             ldacdecInit((ldacdec_t*)m_hDec);
        }
        
        ldacdec_t* dec = (ldacdec_t*)m_hDec;
        std::vector<uint8_t> pcmOut;
        
        // Reserve some space to avoid reallocs (approximate ratio)
        pcmOut.reserve(codedBytes * 10); 

        const uint8_t* src = static_cast<const uint8_t*>(codedData);
        size_t processed = 0;
        
        // Output buffer for one frame (2ch * 256 samples * 2 bytes = 1024 bytes)
        // libldacdec outputs int16_t[256 * 2] (interleaved?) -> verify implementation
        // ldacDecode takes int16_t *pcm. 
        // Inside ldacDecode: pcmFloatToShort writes to pcmOut.
        // It writes `frameSamples` * `channelCount` samples.
        // Max frame samples 256. Max channels 2. So 512 samples -> 1024 bytes.
        int16_t tempPcm[256 * 2]; 

        while (processed < codedBytes)
        {
            // Sync word check (0xAA) - ldacDecode does this, but we should probably 
            // search for it if we are not aligned. 
            // For now assume aligned or let ldacDecode fail.
            
            if (src[processed] != 0xAA) {
                 // Skip until sync word found
                 processed++;
                 continue;
            }

            // Check if enough data for minimal header? 
            if (processed + 2 > codedBytes) break;

            int bytesUsed = 0;
            int ret = ldacDecode(dec, (uint8_t*)(src + processed), tempPcm, &bytesUsed);
            
            if (ret < 0) {
                // Decode failed, maybe false sync word
                processed++; 
                continue;
            }
            
            if (bytesUsed <= 0) {
                 // Should not happen on success
                 processed++;
                 continue;
            }

            // Update format info from decoder state
            m_sampleRate = ldacdecGetSampleRate(dec);
            m_channels = ldacdecGetChannelCount(dec);
            m_bitsPerSample = 16; // libldacdec always outputs 16-bit PCM

            int samplesProduced = dec->frame.frameSamples; // samples per channel
            int channels = dec->frame.channelCount;
            int totalSamples = samplesProduced * channels;
            
            const uint8_t* pcmBytes = reinterpret_cast<const uint8_t*>(tempPcm);
            pcmOut.insert(pcmOut.end(), pcmBytes, pcmBytes + totalSamples * sizeof(int16_t));

            processed += bytesUsed;
        }

        return pcmOut;
    }

    void LdacCodec::Reset()
    {
        if (m_hLdac) {
            ldacBT_free_handle((HANDLE_LDAC_BT)m_hLdac);
            m_hLdac = nullptr;
        }
        if (m_hDec) {
            delete (ldacdec_t*)m_hDec;
            m_hDec = nullptr;
        }
        m_sampleRate = 0;
        m_channels = 0;
        m_bitsPerSample = 0;
    }
}