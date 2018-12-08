#pragma once

#include <cstdint>
#include <algorithm>
#include <cassert>

template <uint32_t sampleCount>
class SampledSpectrumBase
{
public:
    enum : uint32_t
    {
        SampleCount = sampleCount,
    };

public:
    explicit SampledSpectrumBase(float val = 0.0f)
    {
        std::fill(std::begin(samples), std::end(samples), val);
    }

    float& operator [](size_t i) { assert(i < SampleCount); return samples[i]; }
    const float& operator [](size_t i) const { assert(i < SampleCount); return samples[i]; }

protected:
    float samples[sampleCount] = {};
};

enum : uint32_t
{
    WavelengthBegin = 400, // 360
    WavelengthEnd = 700, // 830
    SpectralSampleCount = 60
};

using Float3 = struct { float a, b, c; };

class SampledSpectrum : public SampledSpectrumBase<SpectralSampleCount>
{
public:
    static SampledSpectrum fromSamples(const float wavelengths[], const float values[], uint32_t count);
    static float averageSamples(const float wavelengths[], const float values[], uint32_t count, float l0, float l1);

    static const SampledSpectrum& X();
    static const SampledSpectrum& Y();
    static const SampledSpectrum& Z();

    static const float yIntegral();
    
public:
    Float3 toXYZ() const;
    Float3 toRGB() const;
};

namespace CIE {
constexpr uint32_t sampleCount = 471;
extern const float wavelengths[sampleCount];
extern const float x[sampleCount];
extern const float y[sampleCount];
extern const float z[sampleCount];
}

inline SampledSpectrum SampledSpectrum::fromSamples(const float wavelengths[], const float values[], uint32_t count)
{
    for (uint32_t i = 0; i + 1 < count; ++i)
    {
        assert(wavelengths[i] < wavelengths[i + 1]);
    }

    SampledSpectrum result;
    for (uint32_t i = 0; i < SampleCount; ++i)
    {
        float t0 = float(i) / float(SampleCount);
        float l0 = float(WavelengthBegin) * (1.0f - t0) + float(WavelengthEnd) * t0;

        float t1 = float(i + 1) / float(SampleCount);
        float l1 = float(WavelengthBegin) * (1.0f - t1) + float(WavelengthEnd) * t1;
        
        result[i] = averageSamples(wavelengths, values, count, l0, l1);
    }
    return result;
}

inline float SampledSpectrum::averageSamples(const float wavelengths[], const float values[], uint32_t count, float lBegin, float lEnd)
{
    assert(count > 0);

    if (count == 1)
        return values[0];

    if (lEnd <= wavelengths[0])
        return values[0];

    if (lBegin >= wavelengths[count - 1])
        return values[count - 1];

    float result = 0.0f;
    
    if (lBegin < wavelengths[0])
        result += 0.0f; //  values[0] * (wavelengths[0] - lBegin);

    if (lEnd > wavelengths[count - 1])
        result += 0.0f; //  values[count - 1] * (lEnd - wavelengths[count - 1]);

    uint32_t i = 0;
    while (lBegin > wavelengths[i + 1])
    {
        ++i;
    }

    auto interpolate = [wavelengths, values](float l, uint32_t i) {
        float t = (l - wavelengths[i]) / (wavelengths[i + 1] - wavelengths[i]);
        return values[i] * (1.0f - t) + values[i + 1] * t;
    };

    for (; (i + 1 < count) && (lEnd >= wavelengths[i]); ++i)
    {
        float l0 = std::max(lBegin, wavelengths[i]);
        float l1 = std::min(lEnd, wavelengths[i + 1]);
        result += 0.5f * (interpolate(l0, i) + interpolate(l1, i)) * (l1 - l0);
    }

    result *= 1.0f / (lEnd - lBegin);

    return result;
}

inline const SampledSpectrum& SampledSpectrum::X()
{
    static SampledSpectrum x = SampledSpectrum::fromSamples(CIE::wavelengths, CIE::x, CIE::sampleCount);
    return x;
}

inline const SampledSpectrum& SampledSpectrum::Y()
{
    static SampledSpectrum y = SampledSpectrum::fromSamples(CIE::wavelengths, CIE::y, CIE::sampleCount);
    return y;
}

inline const SampledSpectrum& SampledSpectrum::Z()
{
    static SampledSpectrum z = SampledSpectrum::fromSamples(CIE::wavelengths, CIE::z, CIE::sampleCount);
    return z;
}

inline const float SampledSpectrum::yIntegral()
{
    auto acquireIntegral = []() -> float {
        float result = 0.0f;
        for (uint32_t i = 0; i < CIE::sampleCount; ++i)
            result += CIE::y[i];
        return result;
    };
    static const float value = acquireIntegral();
    return value;
}

inline Float3 SampledSpectrum::toXYZ() const
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    const SampledSpectrum sX = X();
    const SampledSpectrum sY = Y();
    const SampledSpectrum sZ = Z();
    for (uint32_t i = 0; i < SampleCount; ++i)
    {
        x += sX.samples[i] * samples[i];
        y += sY.samples[i] * samples[i];
        z += sZ.samples[i] * samples[i];
    }
    float scale = (float(WavelengthEnd) - float(WavelengthBegin)) / (float(SampleCount) * yIntegral());
    return { x * scale, y * scale, z * scale };
}

inline Float3 SampledSpectrum::toRGB() const
{
    Float3 xyz = toXYZ();
    float r =  3.240479f * xyz.a - 1.537150f * xyz.b - 0.498535f * xyz.c;
    float g = -0.969256f * xyz.a + 1.875991f * xyz.b + 0.041556f * xyz.c;
    float b =  0.055648f * xyz.a - 0.204043f * xyz.b + 1.057311f * xyz.c;
    return { r, g, b };
}
