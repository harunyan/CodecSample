#include "../pch.h"
#include "AudioCodecFactory.h"
#include <map>
#include <mutex>

namespace CodecTest
{
    struct AudioCodecFactory::Impl
    {
        std::map<std::string, CodecCreator> registry;
        mutable std::mutex mtx;
    };

    AudioCodecFactory& AudioCodecFactory::Instance() noexcept
    {
        static AudioCodecFactory instance;
        return instance;
    }

    bool AudioCodecFactory::Register(const std::string& name, CodecCreator creator) noexcept
    {
        if (!m_impl) m_impl = new Impl();
        std::lock_guard<std::mutex> lk(m_impl->mtx);
        if (m_impl->registry.count(name)) return false;
        m_impl->registry.emplace(name, std::move(creator));
        return true;
    }

    std::unique_ptr<IAudioCodec> AudioCodecFactory::Create(const std::string& name) const noexcept
    {
        if (!m_impl) return nullptr;
        std::lock_guard<std::mutex> lk(m_impl->mtx);
        auto it = m_impl->registry.find(name);
        if (it == m_impl->registry.end()) return nullptr;
        return it->second();
    }
}