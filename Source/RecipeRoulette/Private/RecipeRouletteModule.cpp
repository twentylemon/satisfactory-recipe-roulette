#include "RecipeRouletteModule.h"
#include "RecipeRouletteScaling.h"

#include "FGRecipe.h"
#include "FGGameMode.h"

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

        float Scale = RecipeRoulette::ComputeScaleFactor(Seed, TCHAR_TO_UTF8(*Class->GetName()), kMinScale, kMaxScale);

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

