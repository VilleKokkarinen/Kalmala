#include "KalmalaRecipeRequirements.h"
#include "KalmalaRecipeCatalogue.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaToolProgressionContract.h"

FString FKalmalaRecipeRequirements::Describe(const FKalmalaRecipe& Recipe,
    const UKalmalaInventoryComponent* Inventory, int32 CarriedHammerLevel,
    const FString& Availability)
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

    FString Text = TEXT("Requirements — selected recipe\n");
    {
        TArray<FName> Stations = Recipe.RequiredStation;
        if (FKalmalaToolProgressionContract::IsStationAttachmentKit(Recipe.Output))
        {
            Stations.AddUnique(FKalmalaToolProgressionContract::GetAttachmentStationKit(Recipe.Output));
            Text += TEXT("Attachment placement: within 1.25 m of its matching station.\n");
        }
        Text += Stations.IsEmpty() ? TEXT("Station: none (handcrafted).\n")
            : TEXT("Station: any one visible same-world station within 2.5 m: ");
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
        if (Recipe.RequiredTool.IsNone()) Text += TEXT("Tool: no reusable pack item required.\n");
        else
        {
            const bool bHasTool = Inventory && UKalmalaRecipeCatalogue::HasRequiredTool(Recipe.RequiredTool, Inventory->GetStacks());
            Text += FString::Printf(TEXT("Tool: %s in your pack (not consumed) — %s.\n"), *Name(Recipe.RequiredTool),
                !Inventory ? TEXT("Waiting for pack") : bHasTool ? TEXT("Present") : TEXT("Missing"));
        }
    }
    Text += TEXT("Skill level: no recipe requirement.\n");
    Text += Recipe.bEnabled ? TEXT("Unlock: no additional recipe lock.\n")
        : TEXT("Unlock: recipe disabled; unavailable.\n");
    Text += !Recipe.bEnabled ? TEXT("Unavailable: Recipe unavailable")
        : !Availability.IsEmpty() ? TEXT("Unavailable: ") + Availability
        : TEXT("Preview: no unmet requirement reported; the server rechecks every request.");
    Text += TEXT("\nRejected requests preserve ingredients and tool condition.\n");
    return Text;
}
