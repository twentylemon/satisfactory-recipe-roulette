#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cmath>
#include <numeric>
#include <set>
#include <vector>

#include "RecipeRouletteScaling.h"

using namespace RecipeRoulette;
using Catch::Approx;

// A representative sample of real Satisfactory recipe class names.
static const std::vector<std::string> kRecipes = {
    "Recipe_IronPlate",
    "Recipe_IronRod",
    "Recipe_IronIngot",
    "Recipe_CopperIngot",
    "Recipe_CopperWire",
    "Recipe_Cable",
    "Recipe_SteelIngot",
    "Recipe_SteelBeam",
    "Recipe_SteelPipe",
    "Recipe_Concrete",
    "Recipe_QuartzCrystal",
    "Recipe_Silica",
    "Recipe_CircuitBoard",
    "Recipe_AILimiter",
    "Recipe_Computer",
    "Recipe_Supercomputer",
    "Recipe_NuclearFuelRod",
    "Recipe_AluminumIngot",
    "Recipe_AluminumCasing",
    "Recipe_Motor",
};

static const std::vector<uint32_t> kSeeds = { 0, 1, 42, 12345, 0xDEADBEEFu };

// ---- Determinism ---------------------------------------------------------

TEST_CASE("HashCombine is deterministic")
{
    CHECK(HashCombine(0,     "Recipe_IronPlate") == HashCombine(0,     "Recipe_IronPlate"));
    CHECK(HashCombine(42,    "Recipe_Computer")  == HashCombine(42,    "Recipe_Computer"));
    CHECK(HashCombine(0,     "")                 == HashCombine(0,     ""));
}

TEST_CASE("ComputeScaleFactor is deterministic")
{
    for (uint32_t seed : kSeeds)
    {
        for (const auto& recipe : kRecipes)
        {
            float a = ComputeScaleFactor(seed, recipe, 0.25f, 4.0f);
            float b = ComputeScaleFactor(seed, recipe, 0.25f, 4.0f);
            CHECK(a == b);
        }
    }
}

// ---- Range ---------------------------------------------------------------

TEST_CASE("ComputeScaleFactor stays within [MinScale, MaxScale]")
{
    constexpr float Min = 0.25f, Max = 4.0f;
    for (uint32_t seed : kSeeds)
    {
        for (const auto& recipe : kRecipes)
        {
            float s = ComputeScaleFactor(seed, recipe, Min, Max);
            CHECK(s >= Min);
            CHECK(s <= Max);
        }
    }
}

TEST_CASE("Range bounds are respected for custom min/max values")
{
    for (auto [mn, mx] : std::vector<std::pair<float,float>>{{0.5f, 2.0f}, {1.0f, 1.0f}, {0.1f, 10.0f}})
    {
        float s = ComputeScaleFactor(99, "Recipe_IronPlate", mn, mx);
        CHECK(s >= mn);
        CHECK(s <= mx);
    }
}

TEST_CASE("Scale is exactly MinScale when MinScale == MaxScale")
{
    float s = ComputeScaleFactor(0, "Recipe_IronPlate", 2.0f, 2.0f);
    CHECK(s == Approx(2.0f).epsilon(1e-5f));
}

// ---- Sensitivity ---------------------------------------------------------

TEST_CASE("Different recipe names produce different scale factors")
{
    // All recipes should get a distinct factor for a given seed (no trivial collisions).
    std::set<float> seen;
    for (const auto& recipe : kRecipes)
    {
        float s = ComputeScaleFactor(42, recipe, 0.25f, 4.0f);
        CHECK(seen.find(s) == seen.end());
        seen.insert(s);
    }
}

TEST_CASE("Different seeds produce different scales for the same recipe")
{
    std::set<float> seen;
    for (uint32_t seed : kSeeds)
    {
        float s = ComputeScaleFactor(seed, "Recipe_IronPlate", 0.25f, 4.0f);
        CHECK(seen.find(s) == seen.end());
        seen.insert(s);
    }
}

TEST_CASE("Recipe names that differ by one character produce different hashes")
{
    uint32_t h1 = HashCombine(0, "Recipe_IronPlate");
    uint32_t h2 = HashCombine(0, "Recipe_IronPlates"); // one extra char
    uint32_t h3 = HashCombine(0, "recipe_IronPlate");  // case change
    CHECK(h1 != h2);
    CHECK(h1 != h3);
}

// ---- Distribution --------------------------------------------------------

TEST_CASE("Scale factors are log-uniformly distributed")
{
    // Sample ~1000 (seed, recipe) pairs and check that log(scale) has a mean
    // and variance close to what a uniform distribution on [log(Min), log(Max)] predicts.
    constexpr float Min = 0.25f, Max = 4.0f;
    const float LogMin = std::log(Min);
    const float LogMax = std::log(Max);
    const float ExpectedMean = (LogMin + LogMax) / 2.0f;                      // 0.0
    const float ExpectedVar  = (LogMax - LogMin) * (LogMax - LogMin) / 12.0f; // ~0.64

    std::vector<float> logScales;
    logScales.reserve(kSeeds.size() * kRecipes.size() * 3);

    for (uint32_t seed : kSeeds)
    {
        for (const auto& recipe : kRecipes)
        {
            // Generate extra samples by appending index suffixes
            for (int i = 0; i < 10; ++i)
            {
                float s = ComputeScaleFactor(seed, recipe + std::to_string(i), Min, Max);
                logScales.push_back(std::log(s));
            }
        }
    }

    float n = static_cast<float>(logScales.size());
    float mean = std::accumulate(logScales.begin(), logScales.end(), 0.0f) / n;
    float var  = std::accumulate(logScales.begin(), logScales.end(), 0.0f,
                     [mean](float acc, float x) { return acc + (x - mean) * (x - mean); }) / n;

    // Loose tolerance — we're checking the hash isn't badly biased, not proving uniformity.
    CHECK(mean == Approx(ExpectedMean).margin(0.15f));
    CHECK(var  == Approx(ExpectedVar ).margin(0.15f));
}

TEST_CASE("Scale factors span the full range across recipes")
{
    // At least one recipe per seed should land in the lower quarter and one in the upper quarter.
    constexpr float Min = 0.25f, Max = 4.0f;
    const float LowThreshold  = std::exp(std::log(Min) + 0.25f * (std::log(Max) - std::log(Min)));
    const float HighThreshold = std::exp(std::log(Min) + 0.75f * (std::log(Max) - std::log(Min)));

    for (uint32_t seed : kSeeds)
    {
        bool hasLow = false, hasHigh = false;
        for (const auto& recipe : kRecipes)
        {
            float s = ComputeScaleFactor(seed, recipe, Min, Max);
            if (s < LowThreshold)  hasLow  = true;
            if (s > HighThreshold) hasHigh = true;
        }
        CHECK(hasLow);
        CHECK(hasHigh);
    }
}
