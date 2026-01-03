#pragma once

#include <cstddef>
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

// DLL 外部公開の簡易 C API (Codec_FreeBuffer で解放が必要)
__declspec(dllexport) void* Codec_Create(const char* name);
__declspec(dllexport) void  Codec_Destroy(void* codec);

// 初期化：サンプリングレート、チャンネル数、ビット深度を設定
__declspec(dllexport) bool  Codec_Initialize(void* codec, int sampleRate, int channels, int bitsPerSample);

// Encode: 入力バッファを受け取り、内部で malloc して出力ポインタを返す（outSize に出力サイズ）。
// エラー時は nullptr を返す。
__declspec(dllexport) uint8_t* Codec_Encode(void* codec, const void* input, size_t inSize, size_t* outSize);

// デコード後のフォーマット取得
__declspec(dllexport) bool Codec_GetLastFormat(void* codec, int* sampleRate, int* channels, int* bitsPerSample);

// Decode: エンコード済みデータを受け取り、PCMデータを返す。
__declspec(dllexport) uint8_t* Codec_Decode(void* codec, const void* input, size_t inSize, size_t* outSize);

// Codec 関数内で確保されたバッファを解放する
__declspec(dllexport) void Codec_FreeBuffer(uint8_t* buffer);

#ifdef __cplusplus
}
#endif
