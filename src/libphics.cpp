#include <immintrin.h>

#define ulong unsigned long
#define uchar unsigned char

struct ClickSoundItem {
    const void *pcm;
    ulong len;
};

static inline void mix(
    void *__restrict dst,
    const void *__restrict src,
    ulong len
) {
    const __m256i *s = (const __m256i *)src;
    __m256i *d = (__m256i *)dst;

    ulong n = len >> 4;
    for (ulong i = 0; i < n; ++i, ++s, ++d) {
        __m256i a = _mm256_loadu_si256(d);
        __m256i b = _mm256_loadu_si256(s);
        _mm256_storeu_si256(d, _mm256_adds_epi16(a, b));
    }

    ulong tail = len & 15;
    if (tail) {
        const __m128i *s128 = (const __m128i *)s;
        __m128i *d128 = (__m128i *)d;
        __m128i a = _mm_loadu_si128(d128);
        __m128i b = _mm_loadu_si128(s128);
        __m128i mask = _mm_loadu_si128(
            (const __m128i*)(const short[]){
                -1,-1,-1,-1,-1,-1,-1,-1,
                0,0,0,0,0,0,0,0
            } + (8 - (tail & 7))
        );

        _mm_maskstore_epi32((int*)d128, _mm_castps_si128(
            _mm_castsi128_ps(mask)),
            _mm_adds_epi16(a, b)
        );
    }
}

extern "C" __declspec(dllexport) void process(
    short PCM_CLICK[],  ulong PCM_CLICK_LEN,
    short PCM_FLICK[],  ulong PCM_FLICK_LEN,
    short PCM_DRAG[],   ulong PCM_DRAG_LEN,
    char  chart_data[], ulong note_num,
    short output[],     ulong output_len,
    ulong channels,    ulong frame_rate
) {
    const ClickSoundItem map[5] = {
        { nullptr,    0               },
        { PCM_CLICK,  PCM_CLICK_LEN   },
        { PCM_DRAG,   PCM_DRAG_LEN    },
        { PCM_CLICK,  PCM_CLICK_LEN   },
        { PCM_FLICK,  PCM_FLICK_LEN   }
    };

    ulong time_mult = frame_rate * channels;

    for (ulong i = 0; i < note_num; ++i) {
        uchar  type = *(uchar*)(chart_data + i * 9ULL);
        double time = *(double*)(chart_data + i * 9ULL + 1ULL);

        ulong off = (ulong)(time * time_mult);
        if (off >= output_len) continue;

        const ClickSoundItem &cs = map[type];
        ulong len = cs.len;
        if (off + len > output_len) len = output_len - off;

        mix(output + off, cs.pcm, len);
    }
}
