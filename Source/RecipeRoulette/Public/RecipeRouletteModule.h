#pragma once

#include "CoreMinimal.h"
#include "Module/GameWorldModule.h"
#include "RecipeRouletteModule.generated.h"

UCLASS()
class RECIPEROULETTE_API URecipeRouletteModule : public UGameWorldModule
{
    GENERATED_BODY()

public:
    virtual void DispatchLifecycleEvent(ELifecyclePhase Phase) override;

private:
    // Scales all loaded recipe CDOs using the world seed.
    void ApplyRecipeScaling();

    // Deterministic per-recipe scale factor in [MinScale, MaxScale] (log-uniform).
    static float ComputeScaleFactor(uint32 Seed, const FString& RecipeName, float MinScale, float MaxScale);

    // Hashes a string into the seed using xorshift-multiply.
    static uint32 HashCombine(uint32 Seed, const FString& Str);
};
