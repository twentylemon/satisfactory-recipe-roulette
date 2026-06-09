using UnrealBuildTool;

public class RecipeRoulette : ModuleRules
{
    public RecipeRoulette(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine",
            "SML",
            "FactoryGame",
        });
    }
}
