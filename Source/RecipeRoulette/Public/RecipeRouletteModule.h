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
    void ApplyRecipeScaling();
};
