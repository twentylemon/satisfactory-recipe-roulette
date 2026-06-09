#pragma once

#include <cmath>
#include <cstdint>
#include <string>

// Pure C++ — no Unreal dependencies. The module adapts FString -> std::string before calling in.
namespace RecipeRoulette
{

inline uint32_t HashCombine(uint32_t Seed, const std::string& Str)
{
    uint32_t Hash = Seed;
    for (unsigned char Ch : Str)
    {
        Hash ^= static_cast<uint32_t>(Ch);
        Hash *= 0x9E3779B9u;
    }
    // Final avalanche
    Hash ^= Hash >> 16;
    Hash *= 0x45D9F3Bu;
    Hash ^= Hash >> 16;
    return Hash;
}

// Returns a scale factor in [MinScale, MaxScale] distributed log-uniformly.
// Equal probability of halving and doubling across the range.
inline float ComputeScaleFactor(uint32_t Seed, const std::string& RecipeName, float MinScale, float MaxScale)
{
    uint32_t Hash = HashCombine(Seed, RecipeName);
    float T = static_cast<float>(Hash & 0x00FFFFFFu) / static_cast<float>(0x00FFFFFFu);
    float LogMin = std::log(MinScale);
    float LogMax = std::log(MaxScale);
    return std::exp(LogMin + T * (LogMax - LogMin));
}

} // namespace RecipeRoulette
