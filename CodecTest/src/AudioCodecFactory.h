#pragma once

#include "../include/IAudioCodec.h"
#include <functional>
#include <memory>
#include <string>

namespace CodecTest
{
    using CodecCreator = std::function<std::unique_ptr<IAudioCodec>()>;

    // 単純なレジストリ／ファクトリ
    class AudioCodecFactory
    {
    public:
        static AudioCodecFactory& Instance() noexcept;

        bool Register(const std::string& name, CodecCreator creator) noexcept;
        std::unique_ptr<IAudioCodec> Create(const std::string& name) const noexcept;

    private:
        AudioCodecFactory() = default;
        ~AudioCodecFactory() = default;
        AudioCodecFactory(const AudioCodecFactory&) = delete;
        AudioCodecFactory& operator=(const AudioCodecFactory&) = delete;

        struct Impl;
        Impl* m_impl{ nullptr };
    };
}