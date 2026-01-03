#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include "../CodecTest/CodecApi.h"

#define DR_FLAC_IMPLEMENTATION
#include "../CodecTest/include/dr_libs/dr_flac.h"

#define DR_MP3_IMPLEMENTATION
#include "../CodecTest/include/dr_libs/dr_mp3.h"

struct AudioData {
    uint32_t sampleRate = 0;
    uint32_t channels = 0;
    uint32_t bitsPerSample = 16;
    std::vector<int16_t> pcmS16;
};

// WAV Header
#pragma pack(push, 1)
struct WavHeader {
    char riff[4]; uint32_t overallSize; char wave[4]; char fmtChunkMarker[4];
    uint32_t lengthOfFmt; uint16_t formatType; uint16_t channels; uint32_t sampleRate;
    uint32_t byteRate; uint16_t blockAlign; uint16_t bitsPerSample;
    char dataChunkHeader[4]; uint32_t dataSize;
};
#pragma pack(pop)

std::string GetExtension(const std::string& path) {
    size_t dot = path.find_last_of(".");
    if (dot == std::string::npos) return "";
    std::string ext = path.substr(dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

bool LoadWav(const std::string& path, AudioData& outAudio) {
    std::ifstream inFile(path, std::ios::binary);
    if (!inFile) return false;

    WavHeader header;
    inFile.read(reinterpret_cast<char*>(&header), sizeof(WavHeader));

    if (std::strncmp(header.riff, "RIFF", 4) != 0 || std::strncmp(header.wave, "WAVE", 4) != 0) return false;
    if (header.formatType != 1) {
        std::cerr << "Unsupported WAV format (PCM only)." << std::endl;
        return false;
    }

    outAudio.sampleRate = header.sampleRate;
    outAudio.channels = header.channels;
    outAudio.bitsPerSample = header.bitsPerSample;

    std::vector<char> rawData(header.dataSize);
    inFile.read(rawData.data(), header.dataSize);
    size_t bytesRead = inFile.gcount();
    
    if (header.bitsPerSample == 16) {
        outAudio.pcmS16.resize(bytesRead / 2);
        std::memcpy(outAudio.pcmS16.data(), rawData.data(), bytesRead);
    } else {
        std::cerr << "Only 16-bit WAV supported." << std::endl;
        return false;
    }
    return true;
}

bool LoadFlac(const std::string& path, AudioData& outAudio) {
    unsigned int channels, sampleRate;
    drflac_uint64 totalFrames;
    drflac_int16* data = drflac_open_file_and_read_pcm_frames_s16(path.c_str(), &channels, &sampleRate, &totalFrames, NULL);
    if (!data) return false;

    outAudio.channels = channels;
    outAudio.sampleRate = sampleRate;
    outAudio.bitsPerSample = 16;
    outAudio.pcmS16.assign(data, data + totalFrames * channels);
    drflac_free(data, NULL);
    return true;
}

bool LoadMp3(const std::string& path, AudioData& outAudio) {
    drmp3_config config;
    drflac_uint64 totalFrames;
    drmp3_int16* data = drmp3_open_file_and_read_pcm_frames_s16(path.c_str(), &config, &totalFrames, NULL);
    if (!data) return false;

    outAudio.channels = config.channels;
    outAudio.sampleRate = config.sampleRate;
    outAudio.bitsPerSample = 16;
    outAudio.pcmS16.assign(data, data + totalFrames * config.channels);
    drmp3_free(data, NULL);
    return true;
}

bool WriteWav(const std::string& path, const AudioData& audio) {
    std::ofstream outFile(path, std::ios::binary);
    if (!outFile) return false;

    WavHeader h = {};
    std::memcpy(h.riff, "RIFF", 4);
    std::memcpy(h.wave, "WAVE", 4);
    std::memcpy(h.fmtChunkMarker, "fmt ", 4);
    h.lengthOfFmt = 16;
    h.formatType = 1;
    h.channels = (uint16_t)audio.channels;
    h.sampleRate = audio.sampleRate;
    h.bitsPerSample = 16;
    h.blockAlign = h.channels * h.bitsPerSample / 8;
    h.byteRate = h.sampleRate * h.blockAlign;
    std::memcpy(h.dataChunkHeader, "data", 4);
    
    size_t dataSize = audio.pcmS16.size() * sizeof(int16_t);
    h.dataSize = (uint32_t)dataSize;
    h.overallSize = h.dataSize + sizeof(WavHeader) - 8;

    outFile.write(reinterpret_cast<char*>(&h), sizeof(WavHeader));
    outFile.write(reinterpret_cast<const char*>(audio.pcmS16.data()), dataSize);
    return true;
}

int main(int argc, char* argv[])
{
    std::string inFile, outFile;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind("if=", 0) == 0) inFile = arg.substr(3);
        else if (arg.rfind("of=", 0) == 0) outFile = arg.substr(3);
    }

    if (inFile.empty()) {
        std::cout << "Usage: " << argv[0] << " if=<input_file> [of=<output_file>]" << std::endl;
        std::cout << "  Auto-detects format based on extension." << std::endl;
        std::cout << "  Supported Input:  .wav, .flac, .mp3, .ldac" << std::endl;
        std::cout << "  Supported Output: .ldac, .wav" << std::endl;
        return 0;
    }

    if (outFile.empty()) {
        // Auto-generate output filename if not provided
        std::string inExt = GetExtension(inFile);
        if (inExt == "ldac") outFile = inFile + ".wav";
        else outFile = inFile + ".ldac";
    }

    std::string inExt = GetExtension(inFile);
    std::string outExt = GetExtension(outFile);

    // MODE DETECTION
    bool isEncoding = (outExt == "ldac");
    bool isDecoding = (inExt == "ldac" && outExt == "wav");

    if (!isEncoding && !isDecoding) {
        std::cerr << "Error: Invalid conversion path. Must be Audio->LDAC or LDAC->WAV." << std::endl;
        return 1;
    }

    void* codec = Codec_Create("ldac");
    if (!codec) {
        std::cerr << "Failed to create codec." << std::endl;
        return 1;
    }

    if (isEncoding) {
        // ENCODE (WAV/FLAC/MP3 -> LDAC)
        AudioData audio;
        bool loaded = false;
        if (inExt == "wav") loaded = LoadWav(inFile, audio);
        else if (inExt == "flac") loaded = LoadFlac(inFile, audio);
        else if (inExt == "mp3") loaded = LoadMp3(inFile, audio);
        else {
             std::cerr << "Unsupported input format: " << inExt << std::endl;
             return 1;
        }

        if (!loaded) {
            std::cerr << "Failed to load input file." << std::endl;
            return 1;
        }
        
        std::cout << "Encoding " << inFile << " -> " << outFile << " ..." << std::endl;
        std::cout << "  Source: " << audio.sampleRate << "Hz, " << audio.channels << "ch" << std::endl;

        if (!Codec_Initialize(codec, audio.sampleRate, audio.channels, audio.bitsPerSample)) {
            std::cerr << "Codec initialization failed." << std::endl;
            return 1;
        }

        size_t pcmBytes = audio.pcmS16.size() * sizeof(int16_t);
        size_t outSize = 0;
        uint8_t* encodedData = Codec_Encode(codec, audio.pcmS16.data(), pcmBytes, &outSize);

        if (encodedData && outSize > 0) {
            std::ofstream ofs(outFile, std::ios::binary);
            ofs.write(reinterpret_cast<char*>(encodedData), outSize);
            Codec_FreeBuffer(encodedData);
            std::cout << "Done." << std::endl;
        } else {
            std::cerr << "Encoding failed (no output)." << std::endl;
        }

    } else {
        // DECODE (LDAC -> WAV)
        std::ifstream ifs(inFile, std::ios::binary);
        if (!ifs) {
            std::cerr << "Failed to open input file." << std::endl;
            return 1;
        }
        
        std::vector<char> ldacData((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        if (ldacData.empty()) {
            std::cerr << "Input file is empty." << std::endl;
            return 1;
        }

        std::cout << "Decoding " << inFile << " -> " << outFile << " ..." << std::endl;

        size_t decodedSize = 0;
        uint8_t* decodedPcm = Codec_Decode(codec, ldacData.data(), ldacData.size(), &decodedSize);
        
        if (decodedPcm && decodedSize > 0) {
            AudioData outAudio;
            int rate = 0, ch = 0, bits = 0;
            if (Codec_GetLastFormat(codec, &rate, &ch, &bits)) {
                outAudio.sampleRate = rate;
                outAudio.channels = ch;
                outAudio.bitsPerSample = bits;
                std::cout << "Decoded Format: " << rate << "Hz, " << ch << "ch, " << bits << "bit" << std::endl;
            } else {
                // Fallback
                outAudio.sampleRate = 48000;
                outAudio.channels = 2;
                outAudio.bitsPerSample = 16;
                std::cerr << "Warning: Could not retrieve decoded format. Defaulting to 48kHz/2ch." << std::endl;
            }
            
            outAudio.pcmS16.resize(decodedSize / 2);
            std::memcpy(outAudio.pcmS16.data(), decodedPcm, decodedSize);
            
            WriteWav(outFile, outAudio);
            Codec_FreeBuffer(decodedPcm);
            std::cout << "Done." << std::endl;
        } else {
            std::cerr << "Decoding failed." << std::endl;
        }
    }

    Codec_Destroy(codec);
    return 0;
}