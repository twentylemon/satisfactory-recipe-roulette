#include "RecipeRouletteModule.h"

#include "FGRecipe.h"
#include "FGGameMode.h"

// Scale range — 0.25x (quarter cost/output) to 4x (quadruple cost/output)
static constexpr float kMinScale = 0.25f;
static constexpr float kMaxScale = 4.0f;

void URecipeRouletteModule::DispatchLifecycleEvent(ELifecyclePhase Phase)
{
    Super::DispatchLifecycleEvent(Phase);

    if (Phase == ELifecyclePhase::POST_INITIALIZATION)
    {
        ApplyRecipeScaling();
    }
}

void URecipeRouletteModule::ApplyRecipeScaling()
{
    UWorld* World = GetWorld();
    if (!World) return;

    // TODO: verify the exact API for reading the world seed once headers are available.
    // Satisfactory 1.2 stores the session seed in AFGGameMode's session settings.
    // Candidate paths:
    //   AFGGameMode* GM = Cast<AFGGameMode>(World->GetAuthGameMode());
    //   int32 RawSeed = GM->GetSessionSettings().Seed;  // method/field name TBD
    // For now fall back to 0 so scaling still runs deterministically (all recipes
    // get the same hash-distribution, just not world-specific).
    int32 RawSeed = 0;

    AFGGameMode* GameMode = Cast<AFGGameMode>(World->GetAuthGameMode());
    if (GameMode)
    {
        // Uncomment and adjust once the seed accessor is confirmed from headers:
        // RawSeed = GameMode->GetSessionSettings().MapSeed;
    }

    uint32 Seed = static_cast<uint32>(RawSeed);
    int32 PatchedCount = 0;

    for (TObjectIterator<UClass> It; It; ++It)
    {
        UClass* Class = *It;
        if (!Class->IsChildOf(UFGRecipe::StaticClass())) continue;
        if (Class->HasAnyClassFlags(CLASS_Abstract)) continue;

        UFGRecipe* CDO = GetMutableDefault<UFGRecipe>(Class);
        if (!CDO) continue;

        float Scale = ComputeScaleFactor(Seed, Class->GetName(), kMinScale, kMaxScale);

        // Scale ingredients
        TArray<FItemAmount>& Ingredients = CDO->mIngredients;
        for (FItemAmount& Item : Ingredients)
        {
            Item.Amount = FMath::Max(1, FMath::RoundToInt(Item.Amount * Scale));
        }

        // Scale products by the same factor — preserves the conversion ratio
        TArray<FItemAmount>& Products = CDO->mProducts;
        for (FItemAmount& Item : Products)
        {
            Item.Amount = FMath::Max(1, FMath::RoundToInt(Item.Amount * Scale));
        }

        ++PatchedCount;
    }

    UE_LOG(LogTemp, Log, TEXT("RecipeRoulette: patched %d recipes (seed=%d)"), PatchedCount, RawSeed);
}

float URecipeRouletteModule::ComputeScaleFactor(uint32 Seed, const FString& RecipeName, float MinScale, float MaxScale)
{
    uint32 Hash = HashCombine(Seed, RecipeName);

    // Normalise to [0, 1]
    float T = static_cast<float>(Hash & 0x00FFFFFFu) / static_cast<float>(0x00FFFFFFu);

    // Log-uniform mapping: equal probability of halving and doubling
    float LogMin = FMath::Loge(MinScale);
    float LogMax = FMath::Loge(MaxScale);
    return FMath::Exp(LogMin + T * (LogMax - LogMin));
}

uint32 URecipeRouletteModule::HashCombine(uint32 Seed, const FString& Str)
{
    uint32 Hash = Seed;
    for (TCHAR Ch : Str)
    {
        Hash ^= static_cast<uint32>(Ch);
        Hash *= 0x9E3779B9u; // golden-ratio constant — good avalanche for strings
    }
    // Final avalanche pass
    Hash ^= Hash >> 16;
    Hash *= 0x45D9F3Bu;
    Hash ^= Hash >> 16;
    return Hash;
}
