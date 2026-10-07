#include "KalmalaRecipeRequirements.h"
#include "KalmalaRecipeCatalogue.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaToolProgressionContract.h"

FString FKalmalaRecipeRequirements::Describe(const FKalmalaRecipe& Recipe,
    const UKalmalaInventoryComponent*, int32 CarriedHammerLevel, const FString& Availability)
{
    const auto Name = [](FName Id)
    {
        const auto* Item = UKalmalaItemCatalogue::Get()->FindItem(Id);
        return Item ? Item->DisplayName : Id.ToString();
    };
    const bool bDirectBuild = UKalmalaRecipeCatalogue::IsDirectMaterialBuildable(Recipe.Output);
    if (bDirectBuild)
    {
        FString Text = TEXT("Build requirements\n");
        Text += FString::Printf(TEXT("Construction Hammer level 1: %s.\n"),
            CarriedHammerLevel < 0 ? TEXT("Waiting for your tool state")
                : CarriedHammerLevel >= 1 ? TEXT("Present") : TEXT("Missing"));
        Text += TEXT("Placement: clear, dry ground with a gentle slope and room for the structure.\n");
        if (Recipe.Output == TEXT("CampfireKit"))
            Text += TEXT("Hearth fuel: one raw Wood, Lightwood, Densewood, or Coal; starts with 60 seconds.\n");
        const FString Blocker = !Recipe.bEnabled
            ? FString(TEXT("Recipe unavailable")) : Availability.TrimStartAndEnd();
        if (!Blocker.IsEmpty() && !Blocker.Equals(TEXT("Ready"), ESearchCase::IgnoreCase))
            Text += TEXT("Unavailable: ") + Blocker;
        return Text;
    }

    const FString ResultName = Name(Recipe.Output);
    FString Text = Recipe.OutputCount > 0
        ? FString::Printf(TEXT("Result: %d %s per batch.\n"), Recipe.OutputCount, *ResultName)
        : FString::Printf(TEXT("Result: %s.\n"), *ResultName);
    Text += TEXT("Quantity: one batch per press");
    if (Recipe.MaxBatch > 1)
        Text += FString::Printf(TEXT("; up to %d batches per request"), Recipe.MaxBatch);
    Text += TEXT(".\n");
    {
        TArray<FName> Stations = Recipe.RequiredStation;
        if (FKalmalaToolProgressionContract::IsStationAttachmentKit(Recipe.Output))
        {
            Stations.AddUnique(FKalmalaToolProgressionContract::GetAttachmentStationKit(Recipe.Output));
            Text += TEXT("Attachment placement: within 1.25 m of its matching station.\n");
        }
        if (!Stations.IsEmpty()) Text += TEXT("Station: any one visible same-world station within 2.5 m: ");
        bool bNeedsHeat = false;
        for (int32 Index = 0; Index < Stations.Num(); ++Index)
        {
            Text += (Index ? TEXT(" or ") : TEXT("")) + Name(Stations[Index]);
            bNeedsHeat |= Stations[Index] == TEXT("CookingRackKit")
                || Stations[Index] == TEXT("CauldronKit") || Stations[Index] == TEXT("FryingPanKit");
        }
        if (!Stations.IsEmpty()) Text += TEXT(".\n");
        if (bNeedsHeat)
            Text += TEXT("Cooking heat: usable lit hearth with positive heat within 2.5 m of both you and the cooking station.\n");
        if (!Recipe.RequiredTool.IsNone())
            Text += FString::Printf(TEXT("Reusable tool: %s (not consumed).\n"), *Name(Recipe.RequiredTool));
    }
    const FString Blocker = !Recipe.bEnabled
        ? FString(TEXT("Recipe unavailable")) : Availability.TrimStartAndEnd();
    if (!Blocker.IsEmpty() && !Blocker.Equals(TEXT("Ready"), ESearchCase::IgnoreCase))
        Text += TEXT("Unavailable: ") + Blocker;
    return Text;
}
