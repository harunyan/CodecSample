#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace CodecTest
{
    // シンプルな音声コーデックインターフェイス
    class IAudioCodec
    {
    public:
        virtual ~IAudioCodec() = default;

        // 初期化：sampleRate, channels 等は実装側で保持する
        virtual bool Initialize(int sampleRate, int channels, int bitsPerSample) = 0;

        // エンコード（PCMデータ -> 圧縮データ）
        // 戻り値はバイト列。エラー時は空を返す。
        virtual std::vector<uint8_t> Encode(const void* pcmData, size_t pcmBytes) = 0;

        // デコード（圧縮データ -> PCMデータ）
        virtual std::vector<uint8_t> Decode(const void* codedData, size_t codedBytes) = 0;

        // 最後に処理した（あるいは設定された）フォーマットを取得
        virtual void GetFormat(int& sampleRate, int& channels, int& bitsPerSample) const = 0;

        // リセット / クローズ
        virtual void Reset() = 0;

        // 識別用の名前
        virtual std::string Name() const = 0;
    };
}
