#include "KalmalaCraftingSubsystem.h"
#include "Misc/Paths.h"
#include "KalmalaInventoryInspectWidget.h"
#include "KalmalaItemDetailWidget.h"
#include "KalmalaIngredientWidget.h"
#include "KalmalaRecipeRequirements.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaUITheme.h"
#include "KalmalaThemedButton.h"
#include "KalmalaIconWidget.h"
#include "Components/SizeBox.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaPlacementPreview.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaCampfire.h"
#include "KalmalaCampfireWeatherResponse.h"
#include "KalmalaDiscoveryActor.h"
#include "KalmalaHarvestNode.h"
#include "KalmalaInteractable.h"
#include "KalmalaOceanSkiff.h"
#include "KalmalaRecipeCatalogue.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaCharacter.h"
#include "KalmalaStationContextWidget.h"
#include "KalmalaGeneratedTerrainPatch.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaToolProgressionContract.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/InputComponent.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Styling/CoreStyle.h"
#include "InputCoreTypes.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"
#include "Engine/World.h"

namespace
{
constexpr int32 RecipeGridColumns = 4;
const FString NoPanelImage;

bool IsCookingRackMenuRecipe(const FKalmalaRecipe& Recipe)
{
    return Recipe.RecipeId == TEXT("CookedBoarMeatRecipe")
        || Recipe.RecipeId == TEXT("CookedDeerMeatRecipe");
}

bool IsCauldronMenuRecipe(const FKalmalaRecipe& Recipe)
{
    return Recipe.RecipeId == TEXT("MeatStewRecipe")
        || Recipe.RecipeId == TEXT("RootVegetableSoupRecipe");
}

bool IsFryingPanMenuRecipe(const FKalmalaRecipe& Recipe)
{
    return Recipe.RecipeId == TEXT("RoastedRootVegetablesRecipe")
        || Recipe.RecipeId == TEXT("DeerRootRoastRecipe");
}

bool IsCookingMenuRecipe(const FName StationKit, const FKalmalaRecipe& Recipe)
{
    if (StationKit == TEXT("CookingRackKit")) return IsCookingRackMenuRecipe(Recipe);
    if (StationKit == TEXT("CauldronKit")) return IsCauldronMenuRecipe(Recipe);
    if (StationKit == TEXT("FryingPanKit")) return IsFryingPanMenuRecipe(Recipe);
    return false;
}

FString GetReadableToolName(const FName ToolId)
{
    if (ToolId == TEXT("ReedKnife")) return TEXT("Reed Knife");
    if (ToolId == TEXT("FieldHatchet")) return TEXT("Field Hatchet");
    if (ToolId == TEXT("StonePick")) return TEXT("Stone Pick");
    if (ToolId == TEXT("BronzeAxe")) return TEXT("Bronze Axe");
    if (ToolId == TEXT("IronAxe")) return TEXT("Iron Axe");
    if (ToolId == TEXT("ConstructionHammer")) return TEXT("Construction Hammer");
    return ToolId.ToString();
}

bool IsInWorldCookingStation(const FName KitId)
{
    return KitId == TEXT("CookingRackKit") || KitId == TEXT("CauldronKit") || KitId == TEXT("FryingPanKit");
}

bool IsStationContextShellKit(const FName KitId)
{
    return KitId == TEXT("CookingRackKit") || KitId == TEXT("CauldronKit")
        || KitId == TEXT("FryingPanKit") || KitId == TEXT("WorkbenchKit") || KitId == TEXT("ForgeKit")
        || KitId == TEXT("StorageKit");
}

bool IsStationContextSectionSupported(const FName KitId, const FString& Section)
{
    return (KitId == TEXT("CookingRackKit") && Section.Equals(TEXT("Cook"), ESearchCase::IgnoreCase))
        || (KitId == TEXT("CauldronKit") && Section.Equals(TEXT("Cook"), ESearchCase::IgnoreCase))
        || (KitId == TEXT("FryingPanKit") && Section.Equals(TEXT("Cook"), ESearchCase::IgnoreCase))
        || (KitId == TEXT("WorkbenchKit") && (Section.Equals(TEXT("Craft"), ESearchCase::IgnoreCase)
            || Section.Equals(TEXT("Repair"), ESearchCase::IgnoreCase)))
        || (KitId == TEXT("ForgeKit") && (Section.Equals(TEXT("Craft"), ESearchCase::IgnoreCase)
            || Section.Equals(TEXT("Upgrade"), ESearchCase::IgnoreCase)
            || Section.Equals(TEXT("Repair"), ESearchCase::IgnoreCase)))
        || (KitId == TEXT("StorageKit") && Section.Equals(TEXT("Store"), ESearchCase::IgnoreCase));
}

FString BuildForgeUpgradePresentationText(const UKalmalaCraftingComponent* Crafting)
{
    if (!Crafting) return TEXT("Upgrade requirements unavailable.");
    FString Text = Crafting->GetToolProgressionText(TEXT("ForgeKit"));
    Text.ReplaceInline(TEXT("\nTOOL OPTIONS — OWNER ONLY\n"), TEXT(""));
    Text.ReplaceInline(TEXT("Requires:"), TEXT("Requirements:"));
    Text.ReplaceInline(TEXT("Cost:"), TEXT("Materials:"));
    Text.ReplaceInline(TEXT("Status: Ready to upgrade."), TEXT("Ready to upgrade."));
    Text.ReplaceInline(TEXT("Status: Blocked — "), TEXT("Unavailable: "));
    Text.ReplaceInline(TEXT("Status: Already carried."), TEXT("Iron Axe is already carried."));
    return Text.TrimStartAndEnd();
}

FString GetInitialStationContextSection(const FName KitId)
{
    if (KitId == TEXT("WorkbenchKit") || KitId == TEXT("ForgeKit")) return TEXT("Craft");
    if (KitId == TEXT("CookingRackKit") || KitId == TEXT("CauldronKit") || KitId == TEXT("FryingPanKit")) return TEXT("Cook");
    if (KitId == TEXT("StorageKit")) return TEXT("Store");
    return FString();
}

struct FInteractionPromptDescription
{
    FString TargetName;
    FString ActionName;
    FString UnavailableReason;
};

FString GetPromptItemName(const FName ItemId, const TCHAR* Fallback)
{
    const UKalmalaItemCatalogue* Catalogue = UKalmalaItemCatalogue::Get();
    const FKalmalaItemDefinition* Definition = Catalogue ? Catalogue->FindItem(ItemId) : nullptr;
    return Definition && !Definition->DisplayName.IsEmpty() ? Definition->DisplayName : FString(Fallback);
}

const TCHAR* GetHarvestActionName(const EKalmalaToolAction Action)
{
    switch (Action)
    {
    case EKalmalaToolAction::Gathering: return TEXT("Gather");
    case EKalmalaToolAction::Woodcutting: return TEXT("Chop");
    case EKalmalaToolAction::Mining: return TEXT("Mine");
    default: return TEXT("Harvest");
    }
}

bool ResolveInteractionPrompt(const AKalmalaCharacter* Character, AActor* Target,
    const FHitResult& Hit, FInteractionPromptDescription& OutDescription)
{
    OutDescription = {};
    if (!IsValid(Character) || !IsValid(Target)) return false;

    if (Cast<AKalmalaGeneratedTerrainPatch>(Target) != nullptr)
    {
        const AKalmalaWorldGenerationGameState* State = Character->GetWorld()
            ? Character->GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>() : nullptr;
        if (!State || !State->GetWorldGenerationConfig().IsValid() || Hit.ImpactPoint.ContainsNaN()) return false;

        const FKalmalaWorldGenerationConfig& Config = State->GetWorldGenerationConfig();
        const FKalmalaOceanSample Ocean = FKalmalaOceanSampler::Sample(Config, FVector2D(Hit.ImpactPoint));
        if (!Ocean.IsWater()) return false;
        OutDescription.TargetName = TEXT("Open water");
        OutDescription.ActionName = TEXT("Launch skiff");
        if (Ocean.WaterDepth < 100.0f) OutDescription.UnavailableReason = TEXT("Need at least 100 cm of water");
        else if (!FKalmalaWorldBounds::Contains(Config, FVector2D(Hit.ImpactPoint), 160.0))
            OutDescription.UnavailableReason = TEXT("Outside the safe world boundary");
        return true;
    }

    if (const AKalmalaHarvestNode* Node = Cast<AKalmalaHarvestNode>(Target))
    {
        const FString ItemName = GetPromptItemName(Node->GetHarvestItemId(), TEXT("Resource"));
        OutDescription.TargetName = FString::Printf(TEXT("%s source"), *ItemName);
        OutDescription.ActionName = TEXT("Harvest");
        if (Node->IsHarvested())
        {
            OutDescription.UnavailableReason = TEXT("Depleted");
            return true;
        }
        if (FVector::DistSquared(Character->GetActorLocation(), Node->GetActorLocation())
            > FMath::Square(Character->GetInteractionRange()))
            OutDescription.UnavailableReason = TEXT("Out of reach");

        if (!Node->GetGatheringSourceId().IsNone())
        {
            FName ToolId = NAME_None;
            uint8 ActionValue = 0;
            bool bHasUsableTool = false;
            if (!Character->GetLocalHarvestInteractionIntent(Node, ToolId, ActionValue, bHasUsableTool))
            {
                OutDescription.UnavailableReason = TEXT("Unsupported source");
                return true;
            }
            OutDescription.ActionName = GetHarvestActionName(static_cast<EKalmalaToolAction>(ActionValue));
            if (!bHasUsableTool) OutDescription.UnavailableReason = TEXT("No suitable tool available");
        }
        return true;
    }

    if (const AKalmalaConstructionActor* Construction = Cast<AKalmalaConstructionActor>(Target))
    {
        OutDescription.TargetName = GetPromptItemName(Construction->GetConstructionKit(), TEXT("Workbench"));
        OutDescription.ActionName = TEXT("Use");
        if (!Construction->CanUse(Character)) OutDescription.UnavailableReason = TEXT("Unavailable here");
        return true;
    }

    if (const AKalmalaCampfire* Campfire = Cast<AKalmalaCampfire>(Target))
    {
        OutDescription.TargetName = TEXT("Campfire");
        OutDescription.ActionName = TEXT("Light");
        if (!Campfire->CanUse(Character)) OutDescription.UnavailableReason = TEXT("Unavailable here");
        else if (Campfire->IsLit()) OutDescription.UnavailableReason = TEXT("Already lit");
        else if (Campfire->GetFuelSeconds() <= 0.0f) OutDescription.UnavailableReason = TEXT("No fuel");
        else if (!FMath::IsFinite(Campfire->GetFuelWetness()) || Campfire->GetFuelWetness() < 0.0f
            || Campfire->GetFuelWetness() >= FKalmalaCampfireWeatherResponse::ExtinguishWetness)
            OutDescription.UnavailableReason = TEXT("Too wet to light");
        return true;
    }

    if (Cast<AKalmalaDiscoveryActor>(Target) != nullptr)
    {
        OutDescription.TargetName = TEXT("Landmark");
        OutDescription.ActionName = TEXT("Discover");
        if (FVector::DistSquared(Character->GetActorLocation(), Target->GetActorLocation())
            > FMath::Square(Character->GetInteractionRange()))
            OutDescription.UnavailableReason = TEXT("Out of reach");
        return true;
    }

    if (const AKalmalaOceanSkiff* Skiff = Cast<AKalmalaOceanSkiff>(Target))
    {
        OutDescription.TargetName = TEXT("Skiff");
        if (Skiff->GetHelmOccupant() == Character || Skiff->GetPassengerOccupant() == Character)
        {
            OutDescription.ActionName = TEXT("Disembark");
            if (Skiff->GetMode() == EKalmalaOceanSkiffMode::Underway)
                OutDescription.UnavailableReason = TEXT("Stop before disembarking");
        }
        else
        {
            OutDescription.ActionName = TEXT("Board");
            if (Skiff->GetHelmOccupant() != nullptr && Skiff->GetPassengerOccupant() != nullptr)
                OutDescription.UnavailableReason = TEXT("No open seat");
        }
        if (OutDescription.UnavailableReason.IsEmpty()
            && FVector::DistSquared(Character->GetActorLocation(), Skiff->GetActorLocation())
                > FMath::Square(Character->GetInteractionRange()))
            OutDescription.UnavailableReason = TEXT("Out of reach");
        return true;
    }

    if (Target->Implements<UKalmalaInteractable>())
    {
        OutDescription.TargetName = Target->GetClass()->GetDisplayNameText().ToString();
        if (OutDescription.TargetName.IsEmpty()) OutDescription.TargetName = TEXT("Object");
        OutDescription.ActionName = TEXT("Use");
        return true;
    }
    return false;
}
}

FString UKalmalaInteractionPromptWidget::BuildPromptText(const FString& TargetName, const FString& ActionName,
    const FString& UnavailableReason, const bool bModalOpen)
{
    if (bModalOpen || TargetName.TrimStartAndEnd().IsEmpty() || ActionName.TrimStartAndEnd().IsEmpty()) return FString();

    FString Text = FString::Printf(TEXT("%s\n%s"), *TargetName, *ActionName);
    if (!UnavailableReason.TrimStartAndEnd().IsEmpty())
    {
        Text += FString::Printf(TEXT(" — Unavailable: %s"), *UnavailableReason.TrimStartAndEnd());
    }
    return Text;
}

void UKalmalaInteractionPromptWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!WidgetTree) return;
    UBorder* Border = WidgetTree->ConstructWidget<UBorder>();
    Border->SetPadding(FMargin(12.0f, 8.0f));
    Border->SetHorizontalAlignment(HAlign_Center);
    Border->SetVerticalAlignment(VAlign_Center);
    PromptText = WidgetTree->ConstructWidget<UTextBlock>();
    PromptText->SetJustification(ETextJustify::Center);
    PromptText->SetAutoWrapText(true);
    Border->SetContent(PromptText);
    USizeBox* PromptSize = WidgetTree->ConstructWidget<USizeBox>();
    PromptSize->SetWidthOverride(520.0f);
    PromptSize->SetHeightOverride(176.0f);
    PromptSize->SetContent(Border);
    WidgetTree->RootWidget = PromptSize;
    ApplyPromptStyle();
    SetVisibility(ESlateVisibility::Collapsed);
}

void UKalmalaInteractionPromptWidget::ApplyPromptStyle()
{
    const int32 TextScalePercent = UKalmalaSettingsWidget::GetTextScalePercent();
    const int32 ContrastMode = UKalmalaSettingsWidget::GetContrastMode();
    if (LastTextScalePercent == TextScalePercent && LastContrastMode == ContrastMode) return;
    LastTextScalePercent = TextScalePercent;
    LastContrastMode = ContrastMode;
    UBorder* PromptBorder = Cast<UBorder>(GetRootWidget());
    if (const USizeBox* PromptSize = Cast<USizeBox>(GetRootWidget()))
        PromptBorder = Cast<UBorder>(PromptSize->GetContent());
    if (PromptBorder)
        FKalmalaUITheme::Get().ApplyPanel(*PromptBorder, ContrastMode, &NoPanelImage);
    if (PromptText)
        FKalmalaUITheme::Get().ApplyText(*PromptText, FKalmalaUITheme::Get().BodySize + 4,
            false, TextScalePercent, ContrastMode);
}

void UKalmalaInteractionPromptWidget::SetPrompt(const FString& Text)
{
    ApplyPromptStyle();
    if (!PromptText) return;
    if (Text.IsEmpty())
    {
        SetVisibility(ESlateVisibility::Collapsed);
        return;
    }
    PromptText->SetText(FText::FromString(Text));
    SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UKalmalaCraftingWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized(); SetIsFocusable(true);
    auto* Border = WidgetTree->ConstructWidget<UBorder>(); Border->SetPadding(FMargin(20));
    MenuBackground = Border;
    Border->SetBrushColor(FLinearColor(.025f,.035f,.04f,.98f));
    CraftingScrollBox = WidgetTree->ConstructWidget<UScrollBox>();
    auto* Column = WidgetTree->ConstructWidget<UVerticalBox>(); CraftingScrollBox->AddChild(Column);
    auto AddText = [&](const FString& Text, int32 Size) {
        auto* Label = WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetText(FText::FromString(Text)); Label->SetAutoWrapText(true);
        Label->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), Size));
        Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        WrappedTextBlocks.Add(Label);
        Column->AddChildToVerticalBox(Label)->SetPadding(
            FMargin(0.0f, 0.0f, 0.0f, FKalmalaUITheme::Get().SlotPadding));
        return Label;
    };
    HeaderText = AddText(TEXT("Construction hammer — Build and craft"), 28);
    GeneralInstructions = TEXT("Floor, wall, and roof are built directly from Wood and Fibre; no kit is created. Selection is marked with >. Requirements and unavailable reasons are written in text; colour is never the only cue.");
    InstructionsText = AddText(GeneralInstructions, 16);
    StationContextStatusText = AddText(TEXT(""), 18);
    StationContextStatusText->SetVisibility(ESlateVisibility::Collapsed);
    RecipeGrid = WidgetTree->ConstructWidget<UUniformGridPanel>();
    Column->AddChild(RecipeGrid);
    WorkbenchRepairExcludedWidgets.Add(RecipeGrid);
    RecipesText = AddText(TEXT(""), 18);
    Column->RemoveChild(RecipesText);
    auto* RecipeRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    auto* IconBox = WidgetTree->ConstructWidget<USizeBox>();
    IconBox->SetWidthOverride(32); IconBox->SetHeightOverride(32);
    SelectedIcon = WidgetTree->ConstructWidget<UKalmalaIconWidget>();
    IconBox->SetContent(SelectedIcon); RecipeRow->AddChild(IconBox);
    RecipeRow->AddChildToHorizontalBox(RecipesText)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Column->AddChild(RecipeRow);
    WorkbenchRepairExcludedWidgets.Add(RecipeRow);
    DetailText = AddText(TEXT(""), 18);
    WorkbenchRepairExcludedWidgets.Add(DetailText);
    Ingredients = WidgetTree->ConstructWidget<UKalmalaIngredientWidget>();
    Column->AddChild(Ingredients);
    WorkbenchRepairExcludedWidgets.Add(Ingredients);
    RequirementText = AddText(TEXT(""), 18);
    WorkbenchRepairExcludedWidgets.Add(RequirementText);
    auto AddButton = [&](const TCHAR* Label, UHorizontalBox* Row = nullptr, const TCHAR* Help = nullptr) {
        auto* Button = WidgetTree->ConstructWidget<UKalmalaThemedButton>(); auto* Text = WidgetTree->ConstructWidget<UTextBlock>();
        Text->SetText(FText::FromString(Label)); Text->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(),18));
        Text->SetColorAndOpacity(FSlateColor(FLinearColor::Black)); Button->SetContent(Text);
        Button->SetToolTipText(FText::FromString(Help ? Help : Label));
        if(Row) { auto* Slot=Row->AddChildToHorizontalBox(Button); Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); Slot->SetPadding(FMargin(2,4)); }
        else Column->AddChild(Button); return Button;
    };
    auto* WorkbenchSections = WidgetTree->ConstructWidget<UHorizontalBox>();
    StationSectionSwitcher = WorkbenchSections;
    WorkbenchCraftSectionButton = AddButton(TEXT("Craft"), WorkbenchSections,
        TEXT("Show this Workbench's supported craft options."));
    WorkbenchCraftSectionButton->OnClicked.AddDynamic(this, &ThisClass::SelectWorkbenchCraftSection);
    WorkbenchRepairSectionButton = AddButton(TEXT("Repair"), WorkbenchSections,
        TEXT("Inspect and repair one selected carried tool at no cost."));
    WorkbenchRepairSectionButton->OnClicked.AddDynamic(this, &ThisClass::SelectWorkbenchRepairSection);
    ForgeCraftSectionButton = AddButton(TEXT("Craft"), WorkbenchSections,
        TEXT("Show recipes supported by this Forge."));
    ForgeCraftSectionButton->OnClicked.AddDynamic(this, &ThisClass::SelectForgeCraftSection);
    ForgeUpgradeSectionButton = AddButton(TEXT("Upgrade"), WorkbenchSections,
        TEXT("Compare the carried Bronze Axe with its Iron Axe upgrade."));
    ForgeUpgradeSectionButton->OnClicked.AddDynamic(this, &ThisClass::SelectForgeUpgradeSection);
    ForgeRepairSectionButton = AddButton(TEXT("Repair"), WorkbenchSections,
        TEXT("Inspect and repair one selected carried tool at no cost."));
    ForgeRepairSectionButton->OnClicked.AddDynamic(this, &ThisClass::SelectForgeRepairSection);
    Column->InsertChildAt(2, StationSectionSwitcher);
    RecipeSearchBox = WidgetTree->ConstructWidget<UEditableTextBox>();
    RecipeSearchBox->SetHintText(FText::FromString(TEXT("Search recipe names")));
    RecipeSearchStyle = FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox");
    RecipeSearchStyle.SetForegroundColor(FSlateColor(FLinearColor::White));
    RecipeSearchStyle.SetBackgroundColor(FSlateColor(FLinearColor(.02f,.025f,.03f,1.f)));
    RecipeSearchBox->SetWidgetStyle(RecipeSearchStyle);
    RecipeSearchBox->OnTextChanged.AddDynamic(this, &ThisClass::RecipeSearchChanged);
    Column->InsertChildAt(2, RecipeSearchBox);
    WorkbenchRepairExcludedWidgets.Add(RecipeSearchBox);
    CookingRackExcludedWidgets.Add(RecipeSearchBox);
    auto* ClearSearchButton = AddButton(TEXT("Clear recipe search"), nullptr);
    ClearSearchButton->OnClicked.AddDynamic(this, &ThisClass::ClearRecipeSearch);
    WorkbenchRepairExcludedWidgets.Add(ClearSearchButton);
    CookingRackExcludedWidgets.Add(ClearSearchButton);
    auto* CategoryButton = AddButton(TEXT("Recipes: All"), nullptr);
    RecipeCategoryLabel = CastChecked<UTextBlock>(CategoryButton->GetContent());
    CategoryButton->OnClicked.AddDynamic(this, &ThisClass::CycleRecipeCategory);
    Column->RemoveChild(CategoryButton); Column->InsertChildAt(3, CategoryButton);
    WorkbenchRepairExcludedWidgets.Add(CategoryButton);
    CookingRackExcludedWidgets.Add(CategoryButton);
    auto* SortButton = AddButton(TEXT("Recipe order: Catalogue"), nullptr);
    RecipeSortLabel = CastChecked<UTextBlock>(SortButton->GetContent());
    SortButton->OnClicked.AddDynamic(this, &ThisClass::CycleRecipeSort);
    Column->RemoveChild(SortButton); Column->InsertChildAt(4, SortButton);
    WorkbenchRepairExcludedWidgets.Add(SortButton);
    CookingRackExcludedWidgets.Add(SortButton);
    WorkbenchRepairExcludedWidgets.Add(AddText(TEXT("All recipes stay within this menu's station scope."), 14));
    auto* InspectButton = AddButton(TEXT("Inspect inventory"), nullptr,
        TEXT("Show carried items, tools, and supported actions."));
    InspectButton->OnClicked.AddDynamic(this, &ThisClass::FocusInventoryDetails);
    StationCraftExcludedWidgets.Add(InspectButton);
    auto* RecipeActions=WidgetTree->ConstructWidget<UHorizontalBox>(); Column->AddChild(RecipeActions);
    WorkbenchRepairExcludedWidgets.Add(RecipeActions);
    AddButton(TEXT("Previous"),RecipeActions,TEXT("Select the previous recipe. Its ingredients, station, unlock, batch limit, and availability are shown above."))->OnClicked.AddDynamic(this, &ThisClass::Previous);
    AddButton(TEXT("Next"),RecipeActions,TEXT("Select the next recipe. Its ingredients, station, unlock, batch limit, and availability are shown above."))->OnClicked.AddDynamic(this, &ThisClass::Next);
    CraftButton = AddButton(TEXT("Craft one"),RecipeActions,TEXT("Craft batch 1 of the selected recipe. The server checks every requirement and rejected requests preserve ingredients."));
    CraftButton->OnClicked.AddDynamic(this, &ThisClass::Craft);
    PlacementPreviewButton = AddButton(TEXT("Preview placement"),RecipeActions,TEXT("Show a local placement preview for the selected buildable. This does not place it or spend ingredients."));
    PlacementPreviewButton->OnClicked.AddDynamic(this, &ThisClass::Preview);
    StationCraftExcludedWidgets.Add(PlacementPreviewButton);
    StationCraftExcludedWidgets.Add(AddText(TEXT("\nBuild/place selected uses the derived ground ahead. The server checks hammer, raw materials, terrain, and placement before committing. Camp structures are built directly from their listed raw materials.\n"),16));
    auto* FireActions=WidgetTree->ConstructWidget<UHorizontalBox>(); Column->AddChild(FireActions);
    StationCraftExcludedWidgets.Add(FireActions);
    AddButton(TEXT("Build / place selected"),FireActions,TEXT("Ask the server to build the selected structure directly from its listed raw materials with the Construction Hammer. The server validates placement and costs."))->OnClicked.AddDynamic(this, &ThisClass::Place);
    AddButton(TEXT("Add raw fuel"),FireActions,TEXT("Add one Wood, Lightwood, Densewood, or Coal to a nearby usable hearth if the server confirms access and capacity."))->OnClicked.AddDynamic(this, &ThisClass::Refuel);
    AddButton(TEXT("Light hearth"),FireActions,TEXT("Light a nearby usable hearth. The server checks access, dry fuel, and fire state."))->OnClicked.AddDynamic(this, &ThisClass::Light);
    StateText = AddText(TEXT(""), 18);
    FoodText = AddText(TEXT(""), 18);
    StationCraftExcludedWidgets.Add(StateText);
    StationCraftExcludedWidgets.Add(FoodText);
    UButton* EatMeatButton = AddButton(TEXT("Eat one roasted field meat"),nullptr,TEXT("Consume one roasted field meat for the steady meal effect. Another meal cannot replace an active effect."));
    EatMeatButton->OnClicked.AddDynamic(this, &ThisClass::EatFood);
    StationCraftExcludedWidgets.Add(EatMeatButton);
    UButton* EatBrothButton = AddButton(TEXT("Eat one hearth broth"),nullptr,TEXT("Consume one hearth broth for the steady meal effect. Another meal cannot replace an active effect."));
    EatBrothButton->OnClicked.AddDynamic(this, &ThisClass::EatBroth);
    StationCraftExcludedWidgets.Add(EatBrothButton);
    UButton* EatSmokedMeatButton = AddButton(TEXT("Eat one smoked field meat"),nullptr,TEXT("Consume one smoked field meat for the steady meal effect. Another meal cannot replace an active effect."));
    EatSmokedMeatButton->OnClicked.AddDynamic(this, &ThisClass::EatSmokedMeat);
    StationCraftExcludedWidgets.Add(EatSmokedMeatButton);
    RepairText = AddText(TEXT("\nRepair: restore a damaged carried tool to full condition at a visible same-world Workbench or Forge within 2.5 m (free).\nGrinding Stone: repair all damaged carried tools within 2.5 m.\n"), 16);
    auto* RepairActions = WidgetTree->ConstructWidget<UHorizontalBox>(); Column->AddChild(RepairActions);
    StationCraftExcludedWidgets.Add(RepairText);
    StationCraftExcludedWidgets.Add(RepairActions);
    AddButton(TEXT("Repair Reed Knife"), RepairActions)->OnClicked.AddDynamic(this, &ThisClass::RepairReedKnife);
    AddButton(TEXT("Repair Field Hatchet"), RepairActions)->OnClicked.AddDynamic(this, &ThisClass::RepairFieldHatchet);
    AddButton(TEXT("Repair Stone Pick"), RepairActions)->OnClicked.AddDynamic(this, &ThisClass::RepairStonePick);
    auto* AxeRepairActions = WidgetTree->ConstructWidget<UHorizontalBox>(); Column->AddChild(AxeRepairActions);
    StationCraftExcludedWidgets.Add(AxeRepairActions);
    AddButton(TEXT("Repair Bronze Axe"), AxeRepairActions)->OnClicked.AddDynamic(this, &ThisClass::RepairBronzeAxe);
    AddButton(TEXT("Repair Iron Axe"), AxeRepairActions)->OnClicked.AddDynamic(this, &ThisClass::RepairIronAxe);
    WorkbenchRepairContextText = AddText(
        TEXT("Choose one of your carried tools. Repair is free; the server checks a visible same-world Workbench or Forge within 2.5 m."), 16);
    StationCraftExcludedWidgets.Add(WorkbenchRepairContextText);
    WorkbenchRepairInspector = WidgetTree->ConstructWidget<UKalmalaInventoryInspectWidget>();
    Column->AddChild(WorkbenchRepairInspector);
    StationCraftExcludedWidgets.Add(WorkbenchRepairInspector);
    WorkbenchRepairButton = AddButton(TEXT("Repair selected tool"), nullptr,
        TEXT("Ask the server to restore the selected damaged carried tool to full condition for free."));
    WorkbenchRepairButton->OnClicked.AddDynamic(this, &ThisClass::RepairWorkbenchSelectedTool);
    StationCraftExcludedWidgets.Add(WorkbenchRepairButton);
    WorkbenchRepairStatusText = AddText(TEXT(""), 18);
    StationCraftExcludedWidgets.Add(WorkbenchRepairStatusText);
    ToolProgressionText = AddText(TEXT(""), 18);
    ToolProgressionActions = WidgetTree->ConstructWidget<UHorizontalBox>(); Column->AddChild(ToolProgressionActions);
    CraftBronzeAxeButton = AddButton(TEXT("Craft Bronze Axe"), ToolProgressionActions,
        TEXT("Ask the server to craft the level-one Bronze Axe at a visible same-world level-one Workbench. The server checks materials and private tool inventory."));
    CraftBronzeAxeButton->OnClicked.AddDynamic(this, &ThisClass::CraftBronzeAxe);
    UpgradeIronAxeButton = AddButton(TEXT("Upgrade to Iron Axe"), ToolProgressionActions,
        TEXT("Exchange a level-one Bronze Axe for a level-two Iron Axe when the Forge and shown requirements are ready."));
    UpgradeIronAxeButton->OnClicked.AddDynamic(this, &ThisClass::UpgradeIronAxe);
    StationCraftExcludedWidgets.Add(AddText(TEXT("\nWoven chest — shared nearby storage\nInspect a visible chest, choose an item, then store or take one. Contents clear when closed or out of reach."), 16));
    StationCraftExcludedWidgets.Add(AddText(TEXT("Chest contents use the shared 16-stack interface. Accepted construction and storage records are saved for this world; rejected transfers leave both inventories unchanged."), 16));
    StorageText = AddText(TEXT(""), 18);
    StationCraftExcludedWidgets.Add(StorageText);
    UButton* InspectChestButton = AddButton(TEXT("Inspect nearby chest"),nullptr,TEXT("Open the owner-only view of a visible nearby chest. The view closes when the chest is closed or out of reach."));
    InspectChestButton->OnClicked.AddDynamic(this, &ThisClass::InspectStorage);
    StationCraftExcludedWidgets.Add(InspectChestButton);
    auto* StorageActions = WidgetTree->ConstructWidget<UHorizontalBox>(); Column->AddChild(StorageActions);
    StationCraftExcludedWidgets.Add(StorageActions);
    AddButton(TEXT("Previous item"), StorageActions)->OnClicked.AddDynamic(this, &ThisClass::PreviousStorageItem);
    AddButton(TEXT("Next item"), StorageActions)->OnClicked.AddDynamic(this, &ThisClass::NextStorageItem);
    AddButton(TEXT("Store one"), StorageActions,TEXT("Ask the server to move one selected item from your pack into the nearby chest."))->OnClicked.AddDynamic(this, &ThisClass::DepositStorage);
    AddButton(TEXT("Take one"), StorageActions,TEXT("Ask the server to move one selected item from the nearby chest into your pack."))->OnClicked.AddDynamic(this, &ThisClass::WithdrawStorage);
    StorageContextPanel = WidgetTree->ConstructWidget<UVerticalBox>();
    Column->AddChild(StorageContextPanel);
    StationCraftExcludedWidgets.Add(StorageContextPanel);
    StorageContextStatusText = WidgetTree->ConstructWidget<UTextBlock>();
    StorageContextStatusText->SetAutoWrapText(true);
    StorageContextPanel->AddChildToVerticalBox(StorageContextStatusText);
    WrappedTextBlocks.Add(StorageContextStatusText);
    auto* StorageSelectors = WidgetTree->ConstructWidget<UHorizontalBox>();
    StorageContextPanel->AddChildToVerticalBox(StorageSelectors);
    auto* PackColumn = WidgetTree->ConstructWidget<UVerticalBox>();
    StorageSelectors->AddChildToHorizontalBox(PackColumn)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* PackHeading = WidgetTree->ConstructWidget<UTextBlock>();
    PackHeading->SetText(FText::FromString(TEXT("Your pack")));
    PackColumn->AddChildToVerticalBox(PackHeading);
    StoragePackInspector = WidgetTree->ConstructWidget<UKalmalaInventoryInspectWidget>();
    PackColumn->AddChildToVerticalBox(StoragePackInspector);
    auto* ChestColumn = WidgetTree->ConstructWidget<UVerticalBox>();
    StorageSelectors->AddChildToHorizontalBox(ChestColumn)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* ChestHeading = WidgetTree->ConstructWidget<UTextBlock>();
    ChestHeading->SetText(FText::FromString(TEXT("This chest")));
    ChestColumn->AddChildToVerticalBox(ChestHeading);
    StorageContentsInspector = WidgetTree->ConstructWidget<UKalmalaInventoryInspectWidget>();
    ChestColumn->AddChildToVerticalBox(StorageContentsInspector);
    auto* StorageContextActions = WidgetTree->ConstructWidget<UHorizontalBox>();
    StorageContextPanel->AddChildToVerticalBox(StorageContextActions);
    StorageStoreButton = AddButton(TEXT("Store one selected"), StorageContextActions,
        TEXT("Move one selected item from your pack into this chest."));
    StorageStoreButton->OnClicked.AddDynamic(this, &ThisClass::DepositStorage);
    StorageWithdrawButton = AddButton(TEXT("Take one selected"), StorageContextActions,
        TEXT("Move one selected item from this chest into your pack."));
    StorageWithdrawButton->OnClicked.AddDynamic(this, &ThisClass::WithdrawStorage);
    InventoryInspector = WidgetTree->ConstructWidget<UKalmalaInventoryInspectWidget>();
    Column->AddChild(InventoryInspector);
    StationCraftExcludedWidgets.Add(InventoryInspector);
    CloseButton=AddButton(TEXT("Close")); CloseButton->OnClicked.AddDynamic(this, &ThisClass::CloseClicked);
    CloseButton->RemoveFromParent();
    auto* Outer=WidgetTree->ConstructWidget<UVerticalBox>();
    Outer->AddChildToVerticalBox(CraftingScrollBox)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Outer->AddChildToVerticalBox(CloseButton)->SetPadding(FMargin(0,8,0,0));
    Border->SetContent(Outer); WidgetTree->RootWidget = Border;
    const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
    Theme.ApplyMenu(*WidgetTree, HeaderText,
        UKalmalaSettingsWidget::GetTextScalePercent(), UKalmalaSettingsWidget::GetContrastMode());
    Theme.ApplyPanel(*MenuBackground, UKalmalaSettingsWidget::GetContrastMode(), &Theme.BuildPanelImage);
    SetVisibility(ESlateVisibility::Collapsed);
}

UKalmalaCraftingComponent* UKalmalaCraftingWidget::Model() const
{
    auto* Pawn = GetOwningPlayerPawn(); return Pawn ? Pawn->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
}

int32 UKalmalaCraftingWidget::GetBuildBrowseGroup(const FName Output)
{
    if (!FKalmalaPlacementPreview::IsSupportedKit(Output)) return 0;
    if (Output == TEXT("FloorKit") || AKalmalaConstructionActor::IsShelterKit(Output)) return 4;
    if (AKalmalaConstructionActor::IsCraftingStationKit(Output)) return 5;
    return 6;
}

FString UKalmalaCraftingWidget::GetBrowseCategoryLabel(const int32 Category)
{
    switch (Category)
    {
    case 1: return TEXT("Other crafting");
    case 2: return TEXT("Cooking");
    case 3: return TEXT("All builds");
    case 4: return TEXT("Structural pieces");
    case 5: return TEXT("Stations");
    case 6: return TEXT("Camp utilities");
    default: return TEXT("All");
    }
}

TArray<int32> UKalmalaCraftingWidget::GetVisibleRecipeIndices() const
{
    TArray<int32> Indices;
    const auto& Recipes = UKalmalaRecipeCatalogue::Get()->Recipes;
    for (int32 Index = 0; Index < Recipes.Num(); ++Index)
    {
        const auto& Recipe = Recipes[Index];
        const bool bCooking = Recipe.ExperienceSkill == EKalmalaSkill::Cooking;
        const int32 BuildGroup = GetBuildBrowseGroup(Recipe.Output);
        const bool bCategoryMatches = RecipeCategory == 0
            || (RecipeCategory == 1 && !bCooking) || (RecipeCategory == 2 && bCooking)
            || (RecipeCategory == 3 && BuildGroup != 0)
            || (RecipeCategory >= 4 && BuildGroup == RecipeCategory);
        const bool bMatchingAttachment = FKalmalaToolProgressionContract::IsStationAttachmentKit(Recipe.Output)
            && FKalmalaToolProgressionContract::GetAttachmentStationKit(Recipe.Output) == StationFilterKit;
        const bool bMatchesStation = Recipe.RequiredStation.Contains(StationFilterKit)
            || (StationFilterKit == TEXT("CookingRackKit") && Recipe.RequiredStation.Contains(TEXT("CookingRack")))
            || (StationFilterKit == TEXT("CauldronKit") && Recipe.RequiredStation.Contains(TEXT("Cauldron")))
            || (StationFilterKit == TEXT("FryingPanKit") && Recipe.RequiredStation.Contains(TEXT("FryingPan")))
            || bMatchingAttachment;
        if ((StationFilterKit == TEXT("CookingRackKit") || StationFilterKit == TEXT("CauldronKit")
                || StationFilterKit == TEXT("FryingPanKit"))
            && !IsCookingMenuRecipe(StationFilterKit, Recipe)) continue;
        if ((StationFilterKit.IsNone() || bMatchesStation)
            && bCategoryMatches
            && (RecipeQuery.IsEmpty() || Recipe.DisplayName.Contains(RecipeQuery, ESearchCase::IgnoreCase)))
            Indices.Add(Index);
    }
    if (bRecipeNameSort || RecipeCategory == 3) Indices.StableSort([this, &Recipes](int32 A, int32 B)
    {
        if (RecipeCategory == 3)
        {
            const int32 GroupA = GetBuildBrowseGroup(Recipes[A].Output);
            const int32 GroupB = GetBuildBrowseGroup(Recipes[B].Output);
            if (GroupA != GroupB) return GroupA < GroupB;
        }
        if (!bRecipeNameSort) return A < B;
        const int32 Compare = Recipes[A].DisplayName.Compare(Recipes[B].DisplayName, ESearchCase::IgnoreCase);
        return Compare == 0 ? Recipes[A].RecipeId.LexicalLess(Recipes[B].RecipeId) : Compare < 0;
    });
    return Indices;
}

void UKalmalaCraftingWidget::SetRecipeBrowse(const FString& Query, int32 Category, bool bNameSort)
{
    const auto Before = GetVisibleRecipeIndices();
    const int32 OldRecipe = Before.IsValidIndex(Selected) ? Before[Selected] : INDEX_NONE;
    RecipeQuery = Query.Left(64).TrimStartAndEnd();
    RecipeCategory = FMath::Clamp(Category, 0, 6); bRecipeNameSort = bNameSort;
    const auto After = GetVisibleRecipeIndices();
    Selected = After.IndexOfByKey(OldRecipe);
    if (Selected == INDEX_NONE) Selected = 0;
    bPlacementPreviewEnabled = false;
    if (RecipeCategoryLabel) RecipeCategoryLabel->SetText(FText::FromString(TEXT("Recipes: ") + GetBrowseCategoryLabel(RecipeCategory)));
    if (RecipeSortLabel) RecipeSortLabel->SetText(FText::FromString(bRecipeNameSort
        ? TEXT("Recipe order: Name") : TEXT("Recipe order: Catalogue")));
    if (bOpen) Refresh();
}
void UKalmalaCraftingWidget::RecipeSearchChanged(const FText& Text) { SetRecipeBrowse(Text.ToString(), RecipeCategory, bRecipeNameSort); }
void UKalmalaCraftingWidget::CycleRecipeCategory() { SetRecipeBrowse(RecipeQuery, (RecipeCategory + 1) % 7, bRecipeNameSort); }
void UKalmalaCraftingWidget::CycleRecipeSort() { SetRecipeBrowse(RecipeQuery, RecipeCategory, !bRecipeNameSort); }
void UKalmalaCraftingWidget::ClearRecipeSearch() { RecipeSearchBox->SetText(FText::GetEmpty()); SetRecipeBrowse(TEXT(""), RecipeCategory, bRecipeNameSort); }

void UKalmalaCraftingWidget::Open() { OpenInternal(NAME_None); }

bool UKalmalaCraftingWidget::OpenForStation(const FName StationKit)
{
    return OpenInternal(StationKit);
}

bool UKalmalaCraftingWidget::OpenInStationContext(AKalmalaConstructionActor* Station, const FString& Section)
{
    UKalmalaCraftingComponent* Crafting = Model();
    const FName Kit = Crafting ? Crafting->GetLastStationContextKit() : NAME_None;
    const FString ConstructionId = Crafting ? Crafting->GetLastStationContextConstructionId() : FString();
    if (!Crafting || !IsValid(Station) || !IsStationContextShellKit(Kit)
        || !IsStationContextSectionSupported(Kit, Section)
        || !Crafting->IsStationContextTargetCurrent(Station, Kit, ConstructionId)) return false;
    return OpenInternal(Kit, Station, ConstructionId, true, Section);
}

bool UKalmalaCraftingWidget::IsStationContextValid() const
{
    const AKalmalaConstructionActor* Station = ContextStationActor.Get();
    const AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(GetOwningPlayerPawn());
    const UKalmalaCraftingComponent* Crafting = Model();
    return bOpen && bEmbeddedContext && IsValid(Station) && Character
        && Character == ContextOwnerPawn.Get() && Crafting
        && (!bStorageContext || (StationFilterKit == TEXT("StorageKit")
            && Station->GetConstructionKit() == TEXT("StorageKit") && Crafting->HasStorageView()))
        && Crafting->IsStationContextTargetCurrent(ContextStationActor.Get(), StationFilterKit, ContextConstructionId);
}

bool UKalmalaCraftingWidget::OpenInternal(const FName StationKit,
    AKalmalaConstructionActor* StationActor, FString StationContextConstructionId,
    const bool bInEmbeddedContext, FString StationContextSection)
{
    if (!StationKit.IsNone() && !IsInWorldCookingStation(StationKit)
        && !(bInEmbeddedContext && IsStationContextShellKit(StationKit)
            && IsStationContextSectionSupported(StationKit, StationContextSection))) return false;
    if (bInEmbeddedContext && !StationActor) return false;
    if (StationActor)
    {
        const UKalmalaCraftingComponent* Crafting = Model();
        if (!Crafting || !Crafting->IsStationContextTargetCurrent(StationActor, StationKit, StationContextConstructionId))
            return false;
    }
    if (bOpen)
    {
        if (bEmbeddedContext != bInEmbeddedContext) return false;
        const bool bSameStationContext = bInEmbeddedContext && StationActor == ContextStationActor.Get()
            && StationKit == StationFilterKit && StationContextConstructionId == ContextConstructionId;
        StationFilterKit = StationKit;
        bEmbeddedContext = bInEmbeddedContext;
        ContextStationActor = StationActor;
        ContextOwnerPawn = StationActor ? GetOwningPlayerPawn() : nullptr;
        ContextConstructionId = StationActor ? MoveTemp(StationContextConstructionId) : FString();
        this->StationContextSection = MoveTemp(StationContextSection);
        if (!bSameStationContext)
        {
            Selected = 0;
            bPlacementPreviewEnabled = false;
        }
        ConfigureStationContextPresentation(this->StationContextSection);
        if ((bCookingRackContext || bCauldronContext || bFryingPanContext || bWorkbenchCraftContext || bForgeCraftContext) && !bSameStationContext)
        {
            if (RecipeSearchBox) RecipeSearchBox->SetText(FText::GetEmpty());
            SetRecipeBrowse(TEXT(""), 0, false);
        }
        Refresh();
        return true;
    }
    auto* PC = GetOwningPlayer(); if (!PC || PC->IsMoveInputIgnored() || !Model()) return false;
    const auto* Character = Cast<AKalmalaCharacter>(PC->GetPawn());
    if (!Character || (StationKit.IsNone() && Character->GetCarriedToolLevel(TEXT("ConstructionHammer")) < 1)) return false;
    StationFilterKit = StationKit;
    bEmbeddedContext = bInEmbeddedContext;
    ContextStationActor = StationActor;
    ContextOwnerPawn = StationActor ? PC->GetPawn() : nullptr;
    ContextConstructionId = StationActor ? MoveTemp(StationContextConstructionId) : FString();
    this->StationContextSection = MoveTemp(StationContextSection);
    Selected = 0;
    if (StationKit.IsNone() && UKalmalaRecipeCatalogue::Get()->Recipes.IsValidIndex(Selected))
    {
        const int32 FirstBuild = UKalmalaRecipeCatalogue::Get()->Recipes.IndexOfByPredicate(
            [](const FKalmalaRecipe& Recipe)
            {
                return UKalmalaRecipeCatalogue::IsDirectMaterialBuildable(Recipe.Output);
            });
        if (FirstBuild != INDEX_NONE) Selected = FirstBuild;
    }
    ConfigureStationContextPresentation(this->StationContextSection);
    if (bCookingRackContext || bCauldronContext || bFryingPanContext || bWorkbenchCraftContext || bForgeCraftContext)
    {
        if (RecipeSearchBox) RecipeSearchBox->SetText(FText::GetEmpty());
        SetRecipeBrowse(TEXT(""), 0, false);
    }
    bOpen = true;
    if (!bEmbeddedContext) bPreviousCursor = PC->bShowMouseCursor;
    int32 X, Y; PC->GetViewportSize(X,Y);
    const float Scale = FMath::Max(.1f, UWidgetLayoutLibrary::GetViewportScale(this));
    const float PanelWidth = FMath::Min(840.0f, X / Scale - 32.0f);
    if (!bEmbeddedContext)
        SetDesiredSizeInViewport(FVector2D(PanelWidth, FMath::Min(980.0f, Y / Scale - 32.0f)));
    const float TextWrapWidth = FMath::Max(240.0f, PanelWidth - 64.0f);
    for (UTextBlock* Label : WrappedTextBlocks)
    {
        if (Label) Label->SetWrapTextAt(TextWrapWidth);
    }
    WidgetTree->ForEachWidget([TextWrapWidth](UWidget* Child)
    {
        auto* Label = Cast<UTextBlock>(Child);
        auto* Button = Label ? Cast<UButton>(Label->GetParent()) : nullptr;
        auto* Row = Button ? Cast<UHorizontalBox>(Button->GetParent()) : nullptr;
        if (Row && Row->GetChildrenCount() > 0)
        {
            Label->SetAutoWrapText(false);
            Label->SetWrapTextAt(FMath::Max(40.0f, TextWrapWidth / Row->GetChildrenCount() - 16.0f));
            Label->SetJustification(ETextJustify::Center);
        }
    });
    if (!bEmbeddedContext)
    {
        SetAlignmentInViewport(FVector2D(.5,.5)); SetPositionInViewport(FVector2D(X*.5f,Y*.5f), true);
    }
    if (CloseButton) CloseButton->SetVisibility(bEmbeddedContext ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    if (bEmbeddedContext && MenuBackground)
    {
        MenuBackground->SetPadding(FMargin(0.0f));
        MenuBackground->SetBrushColor(FLinearColor::Transparent);
    }
    SetVisibility(ESlateVisibility::Visible); Refresh();
    if (!bEmbeddedContext)
    {
        bPreviousMoveInputIgnored = PC->IsMoveInputIgnored();
        bPreviousLookInputIgnored = PC->IsLookInputIgnored();
        bPreviousCursor = PC->bShowMouseCursor;
        PC->SetIgnoreMoveInput(true); PC->SetIgnoreLookInput(true); PC->bShowMouseCursor = true;
        FInputModeGameAndUI Mode; Mode.SetWidgetToFocus(TakeWidget()); Mode.SetHideCursorDuringCapture(false); PC->SetInputMode(Mode);
        SetKeyboardFocus();
    }
    return true;
}

void UKalmalaCraftingWidget::ConfigureStationContextPresentation(const FString& Section)
{
    bCookingRackContext = bEmbeddedContext && StationFilterKit == TEXT("CookingRackKit")
        && Section.Equals(TEXT("Cook"), ESearchCase::IgnoreCase);
    bCauldronContext = bEmbeddedContext && StationFilterKit == TEXT("CauldronKit")
        && Section.Equals(TEXT("Cook"), ESearchCase::IgnoreCase);
    bFryingPanContext = bEmbeddedContext && StationFilterKit == TEXT("FryingPanKit")
        && Section.Equals(TEXT("Cook"), ESearchCase::IgnoreCase);
    bStorageContext = bEmbeddedContext && StationFilterKit == TEXT("StorageKit")
        && Section.Equals(TEXT("Store"), ESearchCase::IgnoreCase);
    bWorkbenchCraftContext = bEmbeddedContext && StationFilterKit == TEXT("WorkbenchKit")
        && Section.Equals(TEXT("Craft"), ESearchCase::IgnoreCase);
    bForgeCraftContext = bEmbeddedContext && StationFilterKit == TEXT("ForgeKit")
        && Section.Equals(TEXT("Craft"), ESearchCase::IgnoreCase);
    bForgeUpgradeContext = bEmbeddedContext && StationFilterKit == TEXT("ForgeKit")
        && Section.Equals(TEXT("Upgrade"), ESearchCase::IgnoreCase);
    bForgeRepairContext = bEmbeddedContext && StationFilterKit == TEXT("ForgeKit")
        && Section.Equals(TEXT("Repair"), ESearchCase::IgnoreCase);
    bWorkbenchRepairContext = bEmbeddedContext && StationFilterKit == TEXT("WorkbenchKit")
        && Section.Equals(TEXT("Repair"), ESearchCase::IgnoreCase);
    if ((bWorkbenchRepairContext || bForgeRepairContext) && WorkbenchRepairInspector)
    {
        WorkbenchRepairInspector->SetCategory(2);
    }
    ApplyStationCraftLayout();
    RefreshStationContextState();

    if (StationFilterKit.IsNone())
    {
        if (HeaderText) HeaderText->SetText(FText::FromString(TEXT("Construction hammer — Build and craft")));
        if (InstructionsText) InstructionsText->SetText(FText::FromString(GeneralInstructions));
        return;
    }

    const FKalmalaItemDefinition* StationItem = UKalmalaItemCatalogue::Get()->FindItem(StationFilterKit);
    const FString StationName = StationItem ? StationItem->DisplayName : StationFilterKit.ToString();
    if (bStorageContext)
    {
        if (HeaderText) HeaderText->SetText(FText::FromString(TEXT("Chest — Storage")));
        if (InstructionsText) InstructionsText->SetText(FText::FromString(
            TEXT("Choose one item from your pack to store or one item in this chest to take. The server moves one item per action and checks capacity.")));
    }
    else if (bCookingRackContext)
    {
        if (HeaderText) HeaderText->SetText(FText::FromString(TEXT("Cooking Rack — Cook")));
        if (InstructionsText) InstructionsText->SetText(FText::FromString(
            TEXT("Cooked boar meat and Cooked deer meat only. Ingredients, batch limits, result details, and live hearth heat availability are shown below.")));
    }
    else if (bCauldronContext)
    {
        if (HeaderText) HeaderText->SetText(FText::FromString(TEXT("Cauldron — Cook")));
        if (InstructionsText) InstructionsText->SetText(FText::FromString(
            TEXT("Meat stew and Root vegetable soup only. Ingredients, batch limits, result details, and live hearth heat availability are shown below.")));
    }
    else if (bFryingPanContext)
    {
        if (HeaderText) HeaderText->SetText(FText::FromString(TEXT("Frying Pan — Cook")));
        if (InstructionsText) InstructionsText->SetText(FText::FromString(
            TEXT("Roasted root vegetables and Deer and rutabaga roast only. Ingredients, batch limits, result details, and live hearth heat availability are shown below.")));
    }
    else if (bWorkbenchCraftContext)
    {
        if (HeaderText) HeaderText->SetText(FText::FromString(TEXT("Workbench — Craft")));
        if (InstructionsText) InstructionsText->SetText(FText::FromString(
            TEXT("Only recipes for this Workbench and the Bronze Axe tool operation are shown. The server checks station, materials, and tool state when you craft.")));
    }
    else if (bForgeCraftContext)
    {
        if (HeaderText) HeaderText->SetText(FText::FromString(TEXT("Forge — Craft")));
        if (InstructionsText) InstructionsText->SetText(FText::FromString(
            TEXT("Only Forge-compatible recipes and attachments are shown. Ingredient rows and selected requirements show their material and station needs; the server checks them again when you craft.")));
    }
    else if (bForgeUpgradeContext)
    {
        if (HeaderText) HeaderText->SetText(FText::FromString(TEXT("Forge — Upgrade")));
        if (InstructionsText) InstructionsText->SetText(FText::FromString(
            TEXT("Compare your carried Bronze Axe with the Iron Axe upgrade. The Forge, skill, tool, and material requirements are shown below.")));
    }
    else if (bWorkbenchRepairContext)
    {
        if (HeaderText) HeaderText->SetText(FText::FromString(TEXT("Workbench — Repair")));
        if (InstructionsText) InstructionsText->SetText(FText::FromString(
            TEXT("Select one of your carried tools to inspect its level and condition, then request a free repair. The server checks a nearby visible Workbench or Forge.")));
    }
    else if (bForgeRepairContext)
    {
        if (HeaderText) HeaderText->SetText(FText::FromString(TEXT("Forge — Repair")));
        if (InstructionsText) InstructionsText->SetText(FText::FromString(
            TEXT("Select one of your carried tools to inspect its level and condition, then request a free repair. The server checks a nearby visible Workbench or Forge.")));
    }
    else
    {
        if (HeaderText) HeaderText->SetText(FText::FromString(bEmbeddedContext
            ? TEXT("Recipes") : StationName + TEXT(" — Cook")));
        if (InstructionsText) InstructionsText->SetText(FText::FromString(
            TEXT("The server requires this placed station and a usable, lit hearth with heat at both the station and you. Ingredients and availability are shown in text.")));
    }
}

void UKalmalaCraftingWidget::ApplyStationCraftLayout()
{
    const bool bWorkbenchContext = bWorkbenchCraftContext || bWorkbenchRepairContext;
    const bool bForgeContext = bForgeCraftContext || bForgeUpgradeContext || bForgeRepairContext;
    const bool bCookingContext = bCookingRackContext || bCauldronContext || bFryingPanContext;
    const bool bStationCraftContext = bCookingContext || bWorkbenchCraftContext || bForgeCraftContext;
    const bool bRepairContext = bWorkbenchRepairContext || bForgeRepairContext;
    const bool bStationContext = bStationCraftContext || bForgeUpgradeContext || bRepairContext || bStorageContext;
    const bool bNonCraftSection = bRepairContext || bForgeUpgradeContext || bStorageContext;
    const ESlateVisibility ExcludedVisibility = bStationContext
        ? ESlateVisibility::Collapsed : ESlateVisibility::Visible;
    for (UWidget* Excluded : StationCraftExcludedWidgets)
        if (Excluded) Excluded->SetVisibility(ExcludedVisibility);
    const ESlateVisibility CookingBrowseVisibility = bCookingContext
        ? ESlateVisibility::Collapsed : ESlateVisibility::Visible;
    for (UWidget* Excluded : CookingRackExcludedWidgets)
        if (Excluded) Excluded->SetVisibility(CookingBrowseVisibility);
    const ESlateVisibility RepairExcludedVisibility = bNonCraftSection
        ? ESlateVisibility::Collapsed : ESlateVisibility::Visible;
    for (UWidget* Excluded : WorkbenchRepairExcludedWidgets)
        if (Excluded) Excluded->SetVisibility(RepairExcludedVisibility);
    if (StationSectionSwitcher)
        StationSectionSwitcher->SetVisibility(bWorkbenchContext || bForgeContext
            ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (WorkbenchRepairContextText)
        WorkbenchRepairContextText->SetVisibility(bRepairContext
            ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (WorkbenchRepairInspector)
        WorkbenchRepairInspector->SetVisibility(bRepairContext
            ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (WorkbenchRepairButton)
        WorkbenchRepairButton->SetVisibility(bRepairContext
            ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (WorkbenchRepairStatusText)
        WorkbenchRepairStatusText->SetVisibility(bRepairContext
            ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (StorageContextPanel)
        StorageContextPanel->SetVisibility(bStorageContext ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (WorkbenchCraftSectionButton)
    {
        WorkbenchCraftSectionButton->SetVisibility(bWorkbenchContext
            ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        if (UTextBlock* Label = Cast<UTextBlock>(WorkbenchCraftSectionButton->GetContent()))
            Label->SetText(FText::FromString(bWorkbenchCraftContext ? TEXT("> Craft") : TEXT("Craft")));
    }
    if (WorkbenchRepairSectionButton)
    {
        WorkbenchRepairSectionButton->SetVisibility(bWorkbenchContext
            ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        if (UTextBlock* Label = Cast<UTextBlock>(WorkbenchRepairSectionButton->GetContent()))
            Label->SetText(FText::FromString(bWorkbenchRepairContext ? TEXT("> Repair") : TEXT("Repair")));
    }
    if (ForgeCraftSectionButton)
    {
        ForgeCraftSectionButton->SetVisibility(bForgeContext
            ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        if (UTextBlock* Label = Cast<UTextBlock>(ForgeCraftSectionButton->GetContent()))
            Label->SetText(FText::FromString(bForgeCraftContext ? TEXT("> Craft") : TEXT("Craft")));
    }
    if (ForgeUpgradeSectionButton)
    {
        ForgeUpgradeSectionButton->SetVisibility(bForgeContext
            ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        if (UTextBlock* Label = Cast<UTextBlock>(ForgeUpgradeSectionButton->GetContent()))
            Label->SetText(FText::FromString(bForgeUpgradeContext ? TEXT("> Upgrade") : TEXT("Upgrade")));
    }
    if (ForgeRepairSectionButton)
    {
        ForgeRepairSectionButton->SetVisibility(bForgeContext
            ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        if (UTextBlock* Label = Cast<UTextBlock>(ForgeRepairSectionButton->GetContent()))
            Label->SetText(FText::FromString(bForgeRepairContext ? TEXT("> Repair") : TEXT("Repair")));
    }
    if (StationContextStatusText)
        StationContextStatusText->SetVisibility(bStationContext && !bStorageContext
            ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (UpgradeIronAxeButton)
        UpgradeIronAxeButton->SetVisibility(bCookingContext || bWorkbenchCraftContext || bForgeCraftContext || bWorkbenchRepairContext || bStorageContext
            ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    if (CraftBronzeAxeButton)
        CraftBronzeAxeButton->SetVisibility(bCookingContext || bForgeCraftContext || bForgeUpgradeContext || bWorkbenchRepairContext || bStorageContext
            ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    if (ToolProgressionText)
        ToolProgressionText->SetVisibility(bCookingContext || bForgeCraftContext || bRepairContext || bStorageContext
            ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    if (ToolProgressionActions)
        ToolProgressionActions->SetVisibility(bCookingContext || bForgeCraftContext || bRepairContext || bStorageContext
            ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}

void UKalmalaCraftingWidget::RefreshStationContextState()
{
    if (!StationContextStatusText) return;
    const bool bCookingContext = bCookingRackContext || bCauldronContext || bFryingPanContext;
    const bool bStationContext = bCookingContext || bWorkbenchCraftContext || bForgeCraftContext || bForgeUpgradeContext
        || bWorkbenchRepairContext || bForgeRepairContext || bStorageContext;
    if (!bStationContext)
    {
        StationContextStatusText->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }
    if (bStorageContext)
    {
        StationContextStatusText->SetVisibility(ESlateVisibility::Collapsed);
        const bool bContextValid = StationFilterKit == TEXT("StorageKit") && IsStationContextValid();
        if (StorageContextStatusText && !bContextValid)
            StorageContextStatusText->SetText(FText::FromString(TEXT("Chest context expired. Interact with a nearby Chest again.")));
        return;
    }
    const AKalmalaConstructionActor* Station = ContextStationActor.Get();
    if (bCookingContext)
    {
        const FName ExpectedKit = bFryingPanContext ? FName(TEXT("FryingPanKit"))
            : (bCauldronContext ? FName(TEXT("CauldronKit")) : FName(TEXT("CookingRackKit")));
        const TCHAR* StationName = bFryingPanContext ? TEXT("Frying Pan")
            : (bCauldronContext ? TEXT("Cauldron") : TEXT("Cooking Rack"));
        const bool bContextValid = StationFilterKit == ExpectedKit
            && IsStationContextValid() && IsValid(Station)
            && Station->GetConstructionKit() == ExpectedKit;
        if (!bContextValid)
        {
            StationContextStatusText->SetText(FText::FromString(FString::Printf(
                TEXT("%s context unavailable · hearth heat unchecked"), StationName)));
            return;
        }

        const TArray<int32> VisibleIndices = GetVisibleRecipeIndices();
        const auto& Recipes = UKalmalaRecipeCatalogue::Get()->Recipes;
        if (!VisibleIndices.IsValidIndex(Selected) || !Recipes.IsValidIndex(VisibleIndices[Selected]))
        {
            StationContextStatusText->SetText(FText::FromString(FString::Printf(
                TEXT("Hearth heat unavailable · select a %s recipe"),
                bFryingPanContext ? TEXT("Frying Pan") : (bCauldronContext ? TEXT("cauldron") : TEXT("rack"))));
            return;
        }

        const UKalmalaCraftingComponent* Crafting = Model();
        const FString Availability = Crafting
            ? Crafting->GetRecipeAvailability(Recipes[VisibleIndices[Selected]].RecipeId)
            : TEXT("Recipe availability unavailable");
        const bool bHeatUnavailable = Availability.Contains(
            TEXT("Need a usable lit hearth with positive heat"), ESearchCase::IgnoreCase)
            || Availability.Contains(TEXT("Need a visible same-world Cooking rack"), ESearchCase::IgnoreCase)
            || Availability.Contains(TEXT("Need a visible same-world Cauldron"), ESearchCase::IgnoreCase)
            || Availability.Contains(TEXT("Need a visible same-world Frying pan"), ESearchCase::IgnoreCase)
            || Availability.Equals(TEXT("Recipe unavailable"), ESearchCase::IgnoreCase);
        StationContextStatusText->SetText(FText::FromString(FString::Printf(
            TEXT("%s · Hearth heat: %s"), StationName, bHeatUnavailable ? TEXT("unavailable") : TEXT("available"))));
        return;
    }

    const bool bForgeContext = bForgeCraftContext || bForgeUpgradeContext || bForgeRepairContext;
    const FName ExpectedKit = bForgeContext ? FName(TEXT("ForgeKit")) : FName(TEXT("WorkbenchKit"));
    const TCHAR* StationName = bForgeContext ? TEXT("Forge") : TEXT("Workbench");
    const TCHAR* AttachmentName = bForgeContext ? TEXT("Anvil") : TEXT("Tool Rack");
    if (!IsValid(Station) || Station->GetConstructionKit() != ExpectedKit)
    {
        StationContextStatusText->SetText(FText::FromString(FString::Printf(
            TEXT("%s level unavailable · %s state unavailable"), StationName, AttachmentName)));
        return;
    }
    const int32 BaseLevel = FKalmalaToolProgressionContract::GetBaseStationLevel(ExpectedKit);
    const int32 EffectiveLevel = FKalmalaToolProgressionContract::GetEffectiveStationLevel(Station);
    const bool bAttachmentPresent = EffectiveLevel > BaseLevel;
    StationContextStatusText->SetText(FText::FromString(FString::Printf(
        TEXT("%s level %d · %s: %s"), StationName, EffectiveLevel, AttachmentName,
        bAttachmentPresent ? TEXT("attached") : TEXT("not attached"))));
}

void UKalmalaCraftingWidget::RefreshForgeUpgradeState(UKalmalaCraftingComponent* Crafting)
{
    if (!UpgradeIronAxeButton) return;
    if (!bForgeUpgradeContext)
    {
        UpgradeIronAxeButton->SetIsEnabled(true);
        return;
    }

    const APlayerController* PC = GetOwningPlayer();
    const AKalmalaConstructionActor* Station = ContextStationActor.Get();
    const bool bContextValid = IsStationContextValid() && StationFilterKit == TEXT("ForgeKit")
        && IsValid(Station) && Station->GetConstructionKit() == TEXT("ForgeKit");
    FString Details = bContextValid ? BuildForgeUpgradePresentationText(Crafting)
        : TEXT("Forge context unavailable. Reopen a nearby Forge.");
    if (ToolProgressionText && ToolProgressionText->GetText().ToString() != Details)
        ToolProgressionText->SetText(FText::FromString(Details));
    UpgradeIronAxeButton->SetIsEnabled(bContextValid && PC && PC->IsLocalController()
        && Details.Contains(TEXT("Ready to upgrade.")));
}

void UKalmalaCraftingWidget::SelectWorkbenchCraftSection()
{
    if (!IsStationContextValid() || StationFilterKit != TEXT("WorkbenchKit")) return;
    OpenInternal(StationFilterKit, ContextStationActor.Get(), ContextConstructionId, true, TEXT("Craft"));
}

void UKalmalaCraftingWidget::SelectWorkbenchRepairSection()
{
    if (!IsStationContextValid() || StationFilterKit != TEXT("WorkbenchKit")) return;
    OpenInternal(StationFilterKit, ContextStationActor.Get(), ContextConstructionId, true, TEXT("Repair"));
}

void UKalmalaCraftingWidget::SelectForgeCraftSection()
{
    if (!IsStationContextValid() || StationFilterKit != TEXT("ForgeKit")) return;
    OpenInternal(StationFilterKit, ContextStationActor.Get(), ContextConstructionId, true, TEXT("Craft"));
}

void UKalmalaCraftingWidget::SelectForgeUpgradeSection()
{
    if (!IsStationContextValid() || StationFilterKit != TEXT("ForgeKit")) return;
    OpenInternal(StationFilterKit, ContextStationActor.Get(), ContextConstructionId, true, TEXT("Upgrade"));
}

void UKalmalaCraftingWidget::SelectForgeRepairSection()
{
    if (!IsStationContextValid() || StationFilterKit != TEXT("ForgeKit")) return;
    OpenInternal(StationFilterKit, ContextStationActor.Get(), ContextConstructionId, true, TEXT("Repair"));
}

void UKalmalaCraftingWidget::RefreshWorkbenchRepairState(UKalmalaCraftingComponent* Crafting)
{
    if (!Crafting || !WorkbenchRepairButton || !WorkbenchRepairStatusText) return;
    if (bWorkbenchRepairPending && Crafting->GetResultSerial() != WorkbenchRepairResultSerial)
    {
        bWorkbenchRepairPending = false;
        WorkbenchRepairResultToolId = WorkbenchRepairPendingToolId;
        WorkbenchRepairPendingToolId = NAME_None;
        WorkbenchRepairResultText = Crafting->GetLastResult();
    }

    const APlayerController* PC = GetOwningPlayer();
    const AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(GetOwningPlayerPawn());
    const FName SelectedToolId = WorkbenchRepairInspector
        ? WorkbenchRepairInspector->GetSelectedItem() : NAME_None;
    const FKalmalaToolState* Tool = Character
        ? Character->GetCarriedToolInventory().FindByPredicate([SelectedToolId](const FKalmalaToolState& Candidate)
            { return Candidate.ToolId == SelectedToolId; })
        : nullptr;
    const FKalmalaToolDefinition* Definition = SelectedToolId == TEXT("ConstructionHammer")
        ? nullptr : FKalmalaToolLifecycleContract::FindDefinition(SelectedToolId);
    const AKalmalaConstructionActor* ContextStation = ContextStationActor.Get();
    const bool bRepairContext = bWorkbenchRepairContext || bForgeRepairContext;
    const bool bContextKitMatches = (bWorkbenchRepairContext && StationFilterKit == TEXT("WorkbenchKit"))
        || (bForgeRepairContext && StationFilterKit == TEXT("ForgeKit"));
    const bool bContextValid = bRepairContext && bContextKitMatches
        && IsStationContextValid() && IsValid(ContextStation)
        && ContextStation->GetConstructionKit() == StationFilterKit;
    const bool bToolStateValid = Tool && Definition && Tool->ToolLevel >= 1
        && Tool->Durability >= 0 && Tool->Durability <= Definition->MaxDurability;
    const bool bNeedsRepair = bToolStateValid && Tool->Durability < Definition->MaxDurability;
    WorkbenchRepairButton->SetIsEnabled(bContextValid && PC && PC->IsLocalController()
        && bNeedsRepair && !bWorkbenchRepairPending);

    FString Status;
    if (bWorkbenchRepairPending)
        Status = TEXT("Repair requested. Waiting for the server result.");
    else if (!bContextValid)
        Status = StationFilterKit == TEXT("ForgeKit")
            ? TEXT("Forge context unavailable. Reopen a nearby Forge.")
            : TEXT("Workbench context unavailable. Reopen a nearby Workbench.");
    else if (SelectedToolId.IsNone())
        Status = TEXT("No repairable carried tools are available.");
    else if (WorkbenchRepairResultToolId == SelectedToolId && !WorkbenchRepairResultText.IsEmpty())
        Status = WorkbenchRepairResultText;
    else if (!bToolStateValid)
        Status = TEXT("Selected tool condition is unavailable.");
    else if (!bNeedsRepair)
        Status = TEXT("Selected tool is at full condition.");
    else
        Status = TEXT("Repair is free. The server validates a visible same-world Workbench or Forge within 2.5 m.");
    if (WorkbenchRepairStatusText->GetText().ToString() != Status)
        WorkbenchRepairStatusText->SetText(FText::FromString(Status));
}

void UKalmalaCraftingWidget::Close()
{
    if (!bOpen) return; bOpen = false; bPlacementPreviewEnabled = false; SetVisibility(ESlateVisibility::Collapsed);
    if (auto* M = Model()) M->ServerCloseStorage();
    if (CloseButton) CloseButton->SetVisibility(ESlateVisibility::Visible);
    if (auto* PC=GetOwningPlayer(); PC && !bEmbeddedContext)
    {
        if (!bPreviousMoveInputIgnored) PC->SetIgnoreMoveInput(false);
        if (!bPreviousLookInputIgnored) PC->SetIgnoreLookInput(false);
        PC->bShowMouseCursor=bPreviousCursor;
        PC->SetInputMode(FInputModeGameOnly());
    }
    if (bEmbeddedContext && MenuBackground)
    {
        MenuBackground->SetPadding(FMargin(20.0f));
        MenuBackground->SetBrushColor(FLinearColor::White);
        const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
        Theme.ApplyPanel(*MenuBackground, UKalmalaSettingsWidget::GetContrastMode(), &Theme.BuildPanelImage);
    }
    bEmbeddedContext = false;
    bCookingRackContext = false;
    bCauldronContext = false;
    bFryingPanContext = false;
    bWorkbenchCraftContext = false;
    bForgeCraftContext = false;
    bForgeUpgradeContext = false;
    bForgeRepairContext = false;
    bWorkbenchRepairContext = false;
    bStorageContext = false;
    bWorkbenchRepairPending = false;
    WorkbenchRepairPendingToolId = NAME_None;
    WorkbenchRepairResultToolId = NAME_None;
    WorkbenchRepairResultText.Reset();
    StationContextSection.Reset();
    ApplyStationCraftLayout();
    ContextStationActor.Reset();
    ContextOwnerPawn.Reset();
    ContextConstructionId.Reset();
}

void UKalmalaCraftingWidget::RefreshRecipeGrid(const TArray<int32>& VisibleIndices,
    UKalmalaCraftingComponent* Crafting, const int32 TextScalePercent, const int32 ContrastMode)
{
    if (!RecipeGrid || !WidgetTree) return;

    const auto& Recipes = UKalmalaRecipeCatalogue::Get()->Recipes;
    const bool bRebuild = LastRecipeGridIndices != VisibleIndices;
    if (bRebuild)
    {
        RecipeGrid->ClearChildren();
        RecipeSlotCards.Reset();
        RecipeSlotNames.Reset();
        RecipeSlotStates.Reset();
        RecipeSlotVisualStates.Reset();

        for (int32 SlotIndex = 0; SlotIndex < VisibleIndices.Num(); ++SlotIndex)
        {
            const int32 RecipeIndex = VisibleIndices[SlotIndex];
            if (!Recipes.IsValidIndex(RecipeIndex)) continue;
            const FKalmalaRecipe& Recipe = Recipes[RecipeIndex];

            UBorder* Card = WidgetTree->ConstructWidget<UBorder>();
            Card->SetPadding(FMargin(4.0f));
            USizeBox* CardSize = WidgetTree->ConstructWidget<USizeBox>();
            CardSize->SetWidthOverride(184.0f);
            CardSize->SetMinDesiredHeight(104.0f);

            UVerticalBox* CardContent = WidgetTree->ConstructWidget<UVerticalBox>();
            UHorizontalBox* HeadingRow = WidgetTree->ConstructWidget<UHorizontalBox>();
            USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>();
            IconBox->SetWidthOverride(40.0f);
            IconBox->SetHeightOverride(40.0f);
            UKalmalaIconWidget* Icon = WidgetTree->ConstructWidget<UKalmalaIconWidget>();
            EKalmalaIcon Kind;
            int32 Variant;
            UKalmalaIconWidget::FindCatalogueIcon(Recipe.Output, Kind, Variant);
            Icon->SetIcon(Kind, Variant);
            IconBox->SetContent(Icon);
            HeadingRow->AddChildToHorizontalBox(IconBox)->SetPadding(FMargin(0.0f, 0.0f, 5.0f, 0.0f));

            UTextBlock* Name = WidgetTree->ConstructWidget<UTextBlock>();
            const int32 BuildGroup = GetBuildBrowseGroup(Recipe.Output);
            Name->SetText(FText::FromString((BuildGroup ? GetBrowseCategoryLabel(BuildGroup) + TEXT("\n") : TEXT("")) + Recipe.DisplayName));
            Name->SetAutoWrapText(true);
            Name->SetWrapTextAt(128.0f);
            HeadingRow->AddChildToHorizontalBox(Name)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            CardContent->AddChild(HeadingRow);

            UTextBlock* State = WidgetTree->ConstructWidget<UTextBlock>();
            State->SetAutoWrapText(true);
            State->SetWrapTextAt(168.0f);
            CardContent->AddChildToVerticalBox(State)->SetPadding(
                FMargin(0.0f, FKalmalaUITheme::Get().SlotPadding, 0.0f, 0.0f));
            CardSize->SetContent(CardContent);
            Card->SetContent(CardSize);
            UBorder* CellMargin = WidgetTree->ConstructWidget<UBorder>();
            CellMargin->SetBrushColor(FLinearColor::Transparent);
            CellMargin->SetPadding(FMargin(3.0f));
            CellMargin->SetContent(Card);
            UUniformGridSlot* GridPanelSlot = RecipeGrid->AddChildToUniformGrid(
                CellMargin, SlotIndex / RecipeGridColumns, SlotIndex % RecipeGridColumns);
            GridPanelSlot->SetHorizontalAlignment(HAlign_Fill);
            GridPanelSlot->SetVerticalAlignment(VAlign_Fill);

            RecipeSlotCards.Add(Card);
            RecipeSlotNames.Add(Name);
            RecipeSlotStates.Add(State);
            RecipeSlotVisualStates.Add(0xff);
        }
        LastRecipeGridIndices = VisibleIndices;
    }

    const bool bFocused = HasKeyboardFocus();
    const bool bRestyle = bRebuild || LastRecipeGridTextScalePercent != TextScalePercent
        || LastRecipeGridContrastMode != ContrastMode;
    RecipeGridSelectedIndex = VisibleIndices.IsEmpty() ? INDEX_NONE : FMath::Clamp(Selected, 0, VisibleIndices.Num() - 1);
    bRecipeGridFocused = bFocused;
    RecipeGridUnavailableCount = 0;
    const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();

    for (int32 SlotIndex = 0; SlotIndex < VisibleIndices.Num() && RecipeSlotCards.IsValidIndex(SlotIndex); ++SlotIndex)
    {
        const int32 RecipeIndex = VisibleIndices[SlotIndex];
        if (!Recipes.IsValidIndex(RecipeIndex)) continue;
        const FKalmalaRecipe& Recipe = Recipes[RecipeIndex];
        const FString Reason = Crafting ? Crafting->GetRecipeAvailability(Recipe.RecipeId) : TEXT("Waiting for pack");
        const bool bUnavailable = !Reason.IsEmpty();
        const bool bSelected = SlotIndex == RecipeGridSelectedIndex;
        if (bUnavailable) ++RecipeGridUnavailableCount;

        FString RecipeStateLabelText = bUnavailable ? TEXT("UNAVAILABLE") : TEXT("AVAILABLE");
        if (bSelected)
        {
            RecipeStateLabelText = (bFocused ? TEXT("FOCUSED · SELECTED\n") : TEXT("SELECTED\n"))
                + RecipeStateLabelText;
        }
        const int32 BuildGroup = GetBuildBrowseGroup(Recipe.Output);
        const FString CardName = (BuildGroup ? GetBrowseCategoryLabel(BuildGroup) + TEXT("\n") : TEXT("")) + Recipe.DisplayName;
        if (RecipeSlotNames[SlotIndex]->GetText().ToString() != CardName)
        {
            RecipeSlotNames[SlotIndex]->SetText(FText::FromString(CardName));
        }
        if (RecipeSlotStates[SlotIndex]->GetText().ToString() != RecipeStateLabelText)
        {
            RecipeSlotStates[SlotIndex]->SetText(FText::FromString(RecipeStateLabelText));
        }
        const FText ToolTip = FText::FromString(bUnavailable ? Reason : Recipe.DisplayName + TEXT(" — Available"));
        if (RecipeSlotCards[SlotIndex]->GetToolTipText().ToString() != ToolTip.ToString())
        {
            RecipeSlotCards[SlotIndex]->SetToolTipText(ToolTip);
        }

        const uint8 VisualState = static_cast<uint8>((bSelected ? 1 : 0) | (bFocused ? 2 : 0)
            | (bUnavailable ? 4 : 0) | (ContrastMode != 0 ? 8 : 0));
        if (bRestyle || RecipeSlotVisualStates[SlotIndex] != VisualState)
        {
            RecipeSlotVisualStates[SlotIndex] = VisualState;
            Theme.ApplySelectablePanel(*RecipeSlotCards[SlotIndex], bSelected, bFocused, bUnavailable, ContrastMode);
            Theme.ApplyText(*RecipeSlotNames[SlotIndex], 11, bSelected, TextScalePercent, ContrastMode);
            Theme.ApplyText(*RecipeSlotStates[SlotIndex], 9, false, TextScalePercent, ContrastMode);
        }
    }

    LastRecipeGridTextScalePercent = TextScalePercent;
    LastRecipeGridContrastMode = ContrastMode;
}

FString UKalmalaCraftingWidget::GetRecipeGridSummary() const
{
    const bool bScrollable = CraftingScrollBox && CraftingScrollBox->GetScrollOffsetOfEnd() > 1.0f;
    return FString::Printf(TEXT("Slots=%d Unavailable=%d Selected=%d Focused=%d ReadOnly=1 Scrollable=%d"),
        RecipeSlotCards.Num(), RecipeGridUnavailableCount, RecipeGridSelectedIndex, bRecipeGridFocused, bScrollable);
}

#if !UE_BUILD_SHIPPING
bool UKalmalaCraftingWidget::VerifyInventoryInspectionForTest()
{
    if (!InventoryInspector || InventoryInspector->GetSelectedItem().IsNone() || !FSlateApplication::IsInitialized()) return false;
    const FName First = InventoryInspector->GetSelectedItem();
    FocusInventoryDetails();
    const bool bFocused = InventoryInspector->HasKeyboardFocus();
    auto& Slate = FSlateApplication::Get();
    const FModifierKeysState Modifiers;
    Slate.ProcessKeyDownEvent(FKeyEvent(EKeys::Right, Modifiers, 0, false, 0, 0));
    const bool bChanged = InventoryInspector->GetSelectedItem() != First;
    Slate.ProcessKeyDownEvent(FKeyEvent(EKeys::Gamepad_DPad_Left, Modifiers, 0, false, 0, 0));
    const bool bReturned = InventoryInspector->GetSelectedItem() == First;
    Slate.ProcessKeyDownEvent(FKeyEvent(EKeys::Gamepad_LeftShoulder, Modifiers, 0, false, 0, 0));
    const bool bCategory = InventoryInspector->GetCategory() == 1 && InventoryInspector->HasKeyboardFocus();
    Slate.ProcessKeyDownEvent(FKeyEvent(EKeys::Gamepad_RightShoulder, Modifiers, 0, false, 0, 0));
    const bool bSort = InventoryInspector->GetSort() == 1 && InventoryInspector->HasKeyboardFocus();
    InventoryInspector->SetSearch(TEXT("no matching inventory entry"));
    const bool bNoResults = InventoryInspector->GetVisibleCount() == 0 && InventoryInspector->GetSelectedItem().IsNone();
    InventoryInspector->SetSearch(TEXT("")); InventoryInspector->SetCategory(0); InventoryInspector->SetSort(0);
    const bool bRestored = InventoryInspector->GetVisibleCount() > 0;
    UE_LOG(LogTemp, Display, TEXT("Inventory browsing: CategoryKey=%d SortKey=%d NoResults=%d Restored=%d"), bCategory, bSort, bNoResults, bRestored);
    SetKeyboardFocus();
    return bFocused && bChanged && bReturned && bCategory && bSort && bNoResults && bRestored && HasKeyboardFocus();
}

bool UKalmalaCraftingWidget::VerifyRecipeGridNavigationForTest()
{
    if (!bOpen || RecipeSlotCards.IsEmpty() || RecipeGridSelectedIndex < 0 || !bRecipeGridFocused
        || RecipeGridUnavailableCount < 1) return false;
    const int32 InitialSelection = Selected;
    const FModifierKeysState NoModifiers;
    const FKeyEvent KeyboardDown(EKeys::Down, NoModifiers, 0, false, 0, 0);
    const FReply KeyboardReply = NativeOnPreviewKeyDown(FGeometry(), KeyboardDown);
    const bool bKeyboardAdvanced = KeyboardReply.IsEventHandled() && Selected != InitialSelection
        && RecipeGridSelectedIndex == Selected;
    const FKeyEvent KeyboardUp(EKeys::Up, NoModifiers, 0, false, 0, 0);
    const FReply KeyboardUpReply = NativeOnPreviewKeyDown(FGeometry(), KeyboardUp);
    const bool bKeyboardRestored = KeyboardUpReply.IsEventHandled() && Selected == InitialSelection
        && RecipeGridSelectedIndex == InitialSelection;
    const FKeyEvent ControllerDown(EKeys::Gamepad_DPad_Down, NoModifiers, 0, false, 0, 0);
    const FReply ControllerDownReply = NativeOnPreviewKeyDown(FGeometry(), ControllerDown);
    const bool bControllerAdvanced = ControllerDownReply.IsEventHandled() && Selected != InitialSelection
        && RecipeGridSelectedIndex == Selected;
    const FKeyEvent ControllerUp(EKeys::Gamepad_DPad_Up, NoModifiers, 0, false, 0, 0);
    const FReply ControllerUpReply = NativeOnPreviewKeyDown(FGeometry(), ControllerUp);
    const bool bControllerRestored = ControllerUpReply.IsEventHandled() && Selected == InitialSelection
        && RecipeGridSelectedIndex == InitialSelection;
    const auto OriginalIndices = GetVisibleRecipeIndices();
    const int32 OriginalRecipe = OriginalIndices.IsValidIndex(InitialSelection) ? OriginalIndices[InitialSelection] : INDEX_NONE;
    SetRecipeBrowse(TEXT(""), 0, true);
    const auto SortedIndices = GetVisibleRecipeIndices();
    const bool bSelectionKept = SortedIndices.IsValidIndex(Selected) && SortedIndices[Selected] == OriginalRecipe;
    CycleRecipeCategory();
    const bool bCategoryWorked = RecipeCategory == 1;
    RecipeSearchBox->SetKeyboardFocus();
    const FKeyEvent TypingP(EKeys::P, NoModifiers, 0, false, 0, 0);
    const bool bSearchFocusSafe = (RecipeSearchBox->HasKeyboardFocus() || RecipeSearchBox->HasFocusedDescendants())
        && !NativeOnPreviewKeyDown(FGeometry(), TypingP).IsEventHandled();
    SetKeyboardFocus();
    RecipeSearchBox->OnTextChanged.Broadcast(FText::FromString(TEXT("zz-no-matching-recipe")));
    const bool bNoResults = GetVisibleRecipeIndices().IsEmpty() && RecipeSlotCards.IsEmpty()
        && !CraftButton->GetIsEnabled() && RequirementText->GetText().IsEmpty()
        && Ingredients->GetPresentationText().IsEmpty();
    SetRecipeBrowse(TEXT(""), 3, false);
    const auto Builds = GetVisibleRecipeIndices();
    bool bBuildGroups = !Builds.IsEmpty();
    for (int32 Group = 4; Group <= 6; ++Group)
    {
        SetRecipeBrowse(TEXT(""), Group, true);
        const auto Members = GetVisibleRecipeIndices();
        bBuildGroups &= !Members.IsEmpty() && RecipeCategoryLabel->GetText().ToString().Contains(GetBrowseCategoryLabel(Group));
        bBuildGroups &= !RecipeSlotNames.IsEmpty() && RecipeSlotNames[0]->GetText().ToString().StartsWith(GetBrowseCategoryLabel(Group) + TEXT("\n"));
        for (int32 Index : Members) bBuildGroups &= Builds.Contains(Index)
            && GetBuildBrowseGroup(UKalmalaRecipeCatalogue::Get()->Recipes[Index].Output) == Group;
    }
    SetRecipeBrowse(TEXT(""), 4, false);
    const auto Structural = GetVisibleRecipeIndices();
    const int32 BuildSelection = Structural.IsValidIndex(Selected) ? Structural[Selected] : INDEX_NONE;
    CycleRecipeSort();
    const auto BuildSorted = GetVisibleRecipeIndices();
    const bool bBuildSelection = BuildSorted.IsValidIndex(Selected) && BuildSorted[Selected] == BuildSelection;
    const FKeyEvent CategoryKey(EKeys::PageUp, NoModifiers, 0, false, 0, 0);
    const bool bBuildKeys = NativeOnPreviewKeyDown(FGeometry(), CategoryKey).IsEventHandled() && RecipeCategory == 5;
    RecipeSearchBox->OnTextChanged.Broadcast(FText::FromString(TEXT("zz-no-matching-build")));
    const bool bBuildEmpty = GetVisibleRecipeIndices().IsEmpty() && RecipeSlotCards.IsEmpty() && !CraftButton->GetIsEnabled();
    UE_LOG(LogTemp, Display, TEXT("Build browsing: Groups=%d SelectionKept=%d CategoryKey=%d NoResults=%d"),
        bBuildGroups, bBuildSelection, bBuildKeys, bBuildEmpty);
    ClearRecipeSearch(); SetRecipeBrowse(TEXT(""), 0, false);
    Selected = InitialSelection; Refresh();
    const bool bRestored = GetVisibleRecipeIndices() == OriginalIndices && RecipeGridSelectedIndex == InitialSelection;
    UE_LOG(LogTemp, Display, TEXT("Recipe browsing: SelectionKept=%d Category=%d NoResults=%d Restored=%d SearchFocus=%d"),
        bSelectionKept, bCategoryWorked, bNoResults, bRestored, bSearchFocusSafe);
    const float ScrollOffsetOfEnd = CraftingScrollBox ? CraftingScrollBox->GetScrollOffsetOfEnd() : 0.0f;
    const bool bScrollable = ScrollOffsetOfEnd > 1.0f;
    UE_LOG(LogTemp, Display, TEXT("Build grid input: KeyboardDown=%d KeyboardUp=%d DPadDown=%d DPadUp=%d Scrollable=%d ScrollEnd=%.1f Focused=%d"),
        bKeyboardAdvanced, bKeyboardRestored, bControllerAdvanced, bControllerRestored,
        bScrollable, ScrollOffsetOfEnd, HasKeyboardFocus());
    const FName PreviousStationFilterKit = StationFilterKit;
    const FString PreviousRecipeQuery = RecipeQuery;
    const int32 PreviousRecipeCategory = RecipeCategory;
    const bool bPreviousNameSort = bRecipeNameSort;
    StationFilterKit = TEXT("WorkbenchKit");
    RecipeQuery.Reset();
    RecipeCategory = 0;
    bRecipeNameSort = false;
    const TArray<int32> WorkbenchIndices = GetVisibleRecipeIndices();
    bool bHasGrindingStone = false;
    bool bHasToolRack = false;
    bool bNoUnrelatedRecipes = true;
    const auto& Recipes = UKalmalaRecipeCatalogue::Get()->Recipes;
    for (const int32 RecipeIndex : WorkbenchIndices)
    {
        if (!Recipes.IsValidIndex(RecipeIndex)) { bNoUnrelatedRecipes = false; continue; }
        const FKalmalaRecipe& Recipe = Recipes[RecipeIndex];
        bHasGrindingStone |= Recipe.Output == TEXT("GrindingStoneKit");
        bHasToolRack |= Recipe.Output == TEXT("WorkbenchToolRackKit");
        const bool bMatchingAttachment = FKalmalaToolProgressionContract::IsStationAttachmentKit(Recipe.Output)
            && FKalmalaToolProgressionContract::GetAttachmentStationKit(Recipe.Output) == TEXT("WorkbenchKit");
        bNoUnrelatedRecipes &= Recipe.RequiredStation.Contains(TEXT("WorkbenchKit")) || bMatchingAttachment;
    }
    const FString WorkbenchToolOptions = Model()
        ? Model()->GetToolProgressionText(TEXT("WorkbenchKit")) : FString();
    const FKalmalaToolProgressionEntry* BronzeAxe = FKalmalaToolProgressionContract::FindEntry(TEXT("BronzeAxe"));
    const bool bBronzeAxeScoped = BronzeAxe != nullptr
        && BronzeAxe->RequiredStation == EKalmalaToolStationKind::Workbench
        && WorkbenchToolOptions.Contains(TEXT("Bronze Axe"))
        && !WorkbenchToolOptions.Contains(TEXT("Iron Axe"));
    const AKalmalaCharacter* ToolOwner = Cast<AKalmalaCharacter>(GetOwningPlayerPawn());
    const bool bBronzeAlreadyCarried = ToolOwner && ToolOwner->GetCarriedToolLevel(TEXT("BronzeAxe")) > 0;
    const bool bToolPrerequisites = bBronzeAlreadyCarried
        ? WorkbenchToolOptions.Contains(TEXT("Already carried"))
        : WorkbenchToolOptions.Contains(TEXT("Cost:"))
            && WorkbenchToolOptions.Contains(TEXT("Requires: Workbench level 1"));
    const bool bPreviousWorkbenchContext = bWorkbenchCraftContext;
    const bool bPreviousWorkbenchForgeRepairContext = bForgeRepairContext;
    bWorkbenchCraftContext = true;
    bForgeRepairContext = false;
    ApplyStationCraftLayout();
    bool bWorkbenchUiScope = StationContextStatusText
        && StationContextStatusText->GetVisibility() == ESlateVisibility::Visible
        && CraftButton && CraftButton->GetVisibility() != ESlateVisibility::Collapsed
        && CraftBronzeAxeButton && CraftBronzeAxeButton->GetVisibility() != ESlateVisibility::Collapsed
        && UpgradeIronAxeButton && UpgradeIronAxeButton->GetVisibility() == ESlateVisibility::Collapsed;
    for (const TObjectPtr<UWidget>& ExcludedWidget : StationCraftExcludedWidgets)
    {
        const UWidget* Excluded = ExcludedWidget.Get();
        bWorkbenchUiScope &= Excluded && Excluded->GetVisibility() == ESlateVisibility::Collapsed;
    }
    StationFilterKit = TEXT("ForgeKit");
    RecipeQuery.Reset();
    RecipeCategory = 0;
    bRecipeNameSort = false;
    const TArray<int32> ForgeIndices = GetVisibleRecipeIndices();
    bool bHasFryingPan = false;
    bool bFryingPanMaterials = false;
    bool bFryingPanStation = false;
    bool bFryingPanRequirements = false;
    bool bHasForgeAnvil = false;
    bool bForgeOnlyRecipes = true;
    for (const int32 RecipeIndex : ForgeIndices)
    {
        if (!Recipes.IsValidIndex(RecipeIndex)) { bForgeOnlyRecipes = false; continue; }
        const FKalmalaRecipe& Recipe = Recipes[RecipeIndex];
        const bool bMatchingAttachment = FKalmalaToolProgressionContract::IsStationAttachmentKit(Recipe.Output)
            && FKalmalaToolProgressionContract::GetAttachmentStationKit(Recipe.Output) == TEXT("ForgeKit");
        bForgeOnlyRecipes &= Recipe.RequiredStation.Contains(TEXT("ForgeKit")) || bMatchingAttachment;
        bHasForgeAnvil |= Recipe.Output == TEXT("ForgeAnvilKit") && bMatchingAttachment;
        if (Recipe.Output == TEXT("FryingPanKit"))
        {
            bHasFryingPan = true;
            const FKalmalaInventoryStack* IronCost = Recipe.Ingredients.FindByPredicate(
                [](const FKalmalaInventoryStack& Ingredient) { return Ingredient.ItemId == TEXT("Iron"); });
            bFryingPanMaterials = IronCost && IronCost->Quantity == 5;
            bFryingPanStation = Recipe.RequiredStation.Contains(TEXT("ForgeKit"));
            bFryingPanRequirements = bFryingPanMaterials && bFryingPanStation;
        }
    }
    const FName ForgeKit(TEXT("ForgeKit"));
    const FString CraftSection(TEXT("Craft"));
    const bool bForgeInteractionRoute = IsStationContextShellKit(ForgeKit)
        && IsStationContextSectionSupported(ForgeKit, CraftSection)
        && GetInitialStationContextSection(ForgeKit) == CraftSection;
    const bool bPreviousForgeContext = bForgeCraftContext;
    const bool bPreviousForgeUpgradeContext = bForgeUpgradeContext;
    const bool bPreviousRepairContext = bWorkbenchRepairContext;
    bWorkbenchCraftContext = false;
    bWorkbenchRepairContext = false;
    bForgeCraftContext = true;
    bForgeUpgradeContext = false;
    bForgeRepairContext = false;
    ApplyStationCraftLayout();
    bool bForgeUiScope = StationContextStatusText
        && StationContextStatusText->GetVisibility() == ESlateVisibility::Visible
        && CraftButton && CraftButton->GetVisibility() != ESlateVisibility::Collapsed
        && RecipeGrid && RecipeGrid->GetVisibility() != ESlateVisibility::Collapsed
        && Ingredients && Ingredients->GetVisibility() != ESlateVisibility::Collapsed
        && RequirementText && RequirementText->GetVisibility() != ESlateVisibility::Collapsed
        && ToolProgressionText && ToolProgressionText->GetVisibility() == ESlateVisibility::Collapsed
        && ToolProgressionActions && ToolProgressionActions->GetVisibility() == ESlateVisibility::Collapsed
        && StationSectionSwitcher && StationSectionSwitcher->GetVisibility() == ESlateVisibility::Visible
        && ForgeCraftSectionButton && ForgeCraftSectionButton->GetVisibility() == ESlateVisibility::Visible
        && ForgeUpgradeSectionButton && ForgeUpgradeSectionButton->GetVisibility() == ESlateVisibility::Visible
        && ForgeRepairSectionButton && ForgeRepairSectionButton->GetVisibility() == ESlateVisibility::Visible
        && WorkbenchCraftSectionButton && WorkbenchCraftSectionButton->GetVisibility() == ESlateVisibility::Collapsed
        && WorkbenchRepairSectionButton && WorkbenchRepairSectionButton->GetVisibility() == ESlateVisibility::Collapsed;
    for (const TObjectPtr<UWidget>& ExcludedWidget : StationCraftExcludedWidgets)
        bForgeUiScope &= ExcludedWidget && ExcludedWidget->GetVisibility() == ESlateVisibility::Collapsed;
    bForgeCraftContext = bPreviousForgeContext;
    bForgeUpgradeContext = bPreviousForgeUpgradeContext;
    bWorkbenchCraftContext = bPreviousWorkbenchContext;
    bWorkbenchRepairContext = bPreviousRepairContext;
    bForgeRepairContext = bPreviousWorkbenchForgeRepairContext;
    ApplyStationCraftLayout();
    StationFilterKit = PreviousStationFilterKit;
    RecipeQuery = PreviousRecipeQuery;
    RecipeCategory = PreviousRecipeCategory;
    bRecipeNameSort = bPreviousNameSort;
    Refresh();
    const bool bWorkbenchRepairScope = VerifyWorkbenchRepairScopeForTest();
    const bool bForgeUpgradeScope = VerifyForgeUpgradeScopeForTest();
    const bool bForgeRepairScope = VerifyForgeRepairScopeForTest();
    const bool bCookingRackScope = VerifyCookingRackScopeForTest();
    const bool bCauldronScope = VerifyCauldronScopeForTest();
    const bool bFryingPanScope = VerifyFryingPanScopeForTest();
    const bool bChestScope = VerifyStorageContextScopeForTest();
    UE_LOG(LogTemp, Display, TEXT("Workbench Craft scope: BronzeAxe=%d GrindingStone=%d ToolRack=%d NoUnrelated=%d ToolPrerequisites=%d UiScope=%d"),
        bBronzeAxeScoped, bHasGrindingStone, bHasToolRack, bNoUnrelatedRecipes, bToolPrerequisites, bWorkbenchUiScope);
    UE_LOG(LogTemp, Display, TEXT("Forge Craft scope: FryingPan=%d ForgeAnvil=%d Materials=%d Station=%d NoUnrelated=%d Route=%d UiScope=%d"),
        bHasFryingPan, bHasForgeAnvil, bFryingPanMaterials, bFryingPanStation,
        bForgeOnlyRecipes, bForgeInteractionRoute, bForgeUiScope);
    return bKeyboardAdvanced && bKeyboardRestored && bControllerAdvanced && bControllerRestored && bScrollable
        && bSelectionKept && bCategoryWorked && bNoResults && bRestored && bSearchFocusSafe
        && bBuildGroups && bBuildSelection && bBuildKeys && bBuildEmpty
        && bBronzeAxeScoped && bHasGrindingStone && bHasToolRack && bNoUnrelatedRecipes
        && bToolPrerequisites && bWorkbenchUiScope && bWorkbenchRepairScope && bForgeUpgradeScope
        && bForgeRepairScope
        && bHasFryingPan && bHasForgeAnvil && bFryingPanRequirements && bForgeOnlyRecipes
        && bForgeInteractionRoute && bForgeUiScope && bCookingRackScope && bCauldronScope && bFryingPanScope
        && bChestScope;
}

bool UKalmalaCraftingWidget::VerifyCookingRackScopeForTest()
{
    const auto* Catalogue = UKalmalaRecipeCatalogue::Get();
    if (!bOpen || !Catalogue || !RecipeGrid || !Ingredients || !RequirementText || !CraftButton || !StationContextStatusText)
        return false;

    const FName RackKit(TEXT("CookingRackKit"));
    const FName BoarRecipeId(TEXT("CookedBoarMeatRecipe"));
    const FName DeerRecipeId(TEXT("CookedDeerMeatRecipe"));
    const FName CookedBoarId(TEXT("CookedBoarMeat"));
    const FName CookedDeerId(TEXT("CookedDeerMeat"));
    const FKalmalaRecipe* BoarRecipe = Catalogue->Find(BoarRecipeId);
    const FKalmalaRecipe* DeerRecipe = Catalogue->Find(DeerRecipeId);
    const bool bRecipesAuthored = BoarRecipe && DeerRecipe
        && IsCookingRackMenuRecipe(*BoarRecipe) && IsCookingRackMenuRecipe(*DeerRecipe)
        && BoarRecipe->Output == CookedBoarId && DeerRecipe->Output == CookedDeerId
        && BoarRecipe->RequiredStation.Contains(TEXT("CookingRack"))
        && DeerRecipe->RequiredStation.Contains(TEXT("CookingRack"));
    const bool bIngredientDefinitions = BoarRecipe && DeerRecipe
        && BoarRecipe->Ingredients.Num() == 1 && BoarRecipe->Ingredients[0].ItemId == TEXT("BoarMeat")
        && BoarRecipe->Ingredients[0].Quantity == 1
        && DeerRecipe->Ingredients.Num() == 1 && DeerRecipe->Ingredients[0].ItemId == TEXT("DeerMeat")
        && DeerRecipe->Ingredients[0].Quantity == 1;
    const bool bQuantityDefinitions = BoarRecipe && DeerRecipe
        && BoarRecipe->MaxBatch == 5 && DeerRecipe->MaxBatch == 5
        && BoarRecipe->OutputCount == 1 && DeerRecipe->OutputCount == 1;
    const FString CookSection(TEXT("Cook"));
    const bool bInteractionRoute = IsStationContextShellKit(RackKit)
        && IsStationContextSectionSupported(RackKit, CookSection)
        && GetInitialStationContextSection(RackKit) == CookSection;

    const bool bPreviousEmbedded = bEmbeddedContext;
    const bool bPreviousCookingRack = bCookingRackContext;
    const bool bPreviousCauldron = bCauldronContext;
    const bool bPreviousWorkbenchCraft = bWorkbenchCraftContext;
    const bool bPreviousForgeCraft = bForgeCraftContext;
    const bool bPreviousForgeUpgrade = bForgeUpgradeContext;
    const bool bPreviousForgeRepair = bForgeRepairContext;
    const bool bPreviousWorkbenchRepair = bWorkbenchRepairContext;
    const FName PreviousStationKit = StationFilterKit;
    const FString PreviousSection = StationContextSection;
    const TWeakObjectPtr<AKalmalaConstructionActor> PreviousStation = ContextStationActor;
    const TWeakObjectPtr<APawn> PreviousOwnerPawn = ContextOwnerPawn;
    const FString PreviousConstructionId = ContextConstructionId;
    const FString PreviousQuery = RecipeQuery;
    const int32 PreviousCategory = RecipeCategory;
    const bool bPreviousNameSort = bRecipeNameSort;
    const int32 PreviousSelection = Selected;

    bEmbeddedContext = true;
    StationFilterKit = RackKit;
    bCauldronContext = false;
    StationContextSection = CookSection;
    ContextStationActor.Reset();
    ContextOwnerPawn.Reset();
    ContextConstructionId.Reset();
    RecipeQuery.Reset();
    RecipeCategory = 0;
    bRecipeNameSort = false;
    Selected = 0;
    ConfigureStationContextPresentation(CookSection);
    const TArray<int32> RackIndices = GetVisibleRecipeIndices();
    const auto& Recipes = Catalogue->Recipes;
    const int32 BoarIndex = RackIndices.IndexOfByPredicate([&Recipes, BoarRecipeId](const int32 Index)
    {
        return Recipes.IsValidIndex(Index) && Recipes[Index].RecipeId == BoarRecipeId;
    });
    const int32 DeerIndex = RackIndices.IndexOfByPredicate([&Recipes, DeerRecipeId](const int32 Index)
    {
        return Recipes.IsValidIndex(Index) && Recipes[Index].RecipeId == DeerRecipeId;
    });
    Selected = BoarIndex == INDEX_NONE ? 0 : BoarIndex;
    Refresh();

    bool bOnlyRackRecipes = RackIndices.Num() == 2 && bRecipesAuthored && bInteractionRoute;
    for (const int32 Index : RackIndices)
        bOnlyRackRecipes &= Recipes.IsValidIndex(Index) && IsCookingRackMenuRecipe(Recipes[Index])
            && Recipes[Index].RequiredStation.Contains(TEXT("CookingRack"));
    const FString BoarRequirements = RequirementText->GetText().ToString();
    const FString BoarIngredients = Ingredients->GetPresentationText();
    const bool bBoarCounts = BoarIngredients.Contains(TEXT("Boar")) && BoarIngredients.Contains(TEXT("1"));
    const bool bBoarQuantity = BoarRequirements.Contains(TEXT("one batch per press"))
        && BoarRequirements.Contains(TEXT("up to 5 batches per request"));
    const bool bBoarHeat = BoarRequirements.Contains(TEXT("Cooking heat: usable lit hearth"));
    Selected = DeerIndex == INDEX_NONE ? Selected : DeerIndex;
    Refresh();
    const FString DeerRequirements = RequirementText->GetText().ToString();
    const FString DeerIngredients = Ingredients->GetPresentationText();
    const bool bDeerCounts = DeerIngredients.Contains(TEXT("Deer")) && DeerIngredients.Contains(TEXT("1"));
    const bool bDeerQuantity = DeerRequirements.Contains(TEXT("one batch per press"))
        && DeerRequirements.Contains(TEXT("up to 5 batches per request"));
    const bool bDeerHeat = DeerRequirements.Contains(TEXT("Cooking heat: usable lit hearth"));
    Selected = BoarIndex == INDEX_NONE ? 0 : BoarIndex;
    Refresh();
    const FString Status = StationContextStatusText->GetText().ToString();
    const bool bIngredientCounts = bIngredientDefinitions && bBoarCounts && bDeerCounts;
    const bool bQuantity = bQuantityDefinitions && bBoarQuantity && bDeerQuantity;
    const FKalmalaItemDefinition* BoarOutput = UKalmalaItemCatalogue::Get()->FindItem(CookedBoarId);
    const bool bSelectedDescription = BoarOutput && DetailText
        && DetailText->GetText().ToString() == BoarOutput->Description;
    const bool bLiveHeatFeedback = bBoarHeat && bDeerHeat
        && Status.Contains(TEXT("hearth heat"), ESearchCase::IgnoreCase);
    bool bNoUnrelatedRecipesOrActions = true;
    for (const TObjectPtr<UWidget>& ExcludedWidget : StationCraftExcludedWidgets)
        bNoUnrelatedRecipesOrActions &= ExcludedWidget
            && ExcludedWidget->GetVisibility() == ESlateVisibility::Collapsed;
    for (const TObjectPtr<UWidget>& ExcludedWidget : CookingRackExcludedWidgets)
        bNoUnrelatedRecipesOrActions &= ExcludedWidget
            && ExcludedWidget->GetVisibility() == ESlateVisibility::Collapsed;
    bNoUnrelatedRecipesOrActions &= ToolProgressionText
        && ToolProgressionText->GetVisibility() == ESlateVisibility::Collapsed
        && ToolProgressionActions && ToolProgressionActions->GetVisibility() == ESlateVisibility::Collapsed
        && CraftBronzeAxeButton && CraftBronzeAxeButton->GetVisibility() == ESlateVisibility::Collapsed
        && UpgradeIronAxeButton && UpgradeIronAxeButton->GetVisibility() == ESlateVisibility::Collapsed
        && StationSectionSwitcher && StationSectionSwitcher->GetVisibility() == ESlateVisibility::Collapsed;
    const bool bMenuContents = HeaderText && HeaderText->GetText().ToString() == TEXT("Cooking Rack — Cook")
        && InstructionsText && InstructionsText->GetText().ToString().Contains(TEXT("Cooked boar meat and Cooked deer meat only"))
        && RecipeGrid->GetVisibility() != ESlateVisibility::Collapsed
        && Ingredients->GetVisibility() != ESlateVisibility::Collapsed
        && DetailText && DetailText->GetVisibility() != ESlateVisibility::Collapsed
        && bSelectedDescription
        && RequirementText->GetVisibility() != ESlateVisibility::Collapsed
        && CraftButton->GetVisibility() != ESlateVisibility::Collapsed
        && StationContextStatusText->GetVisibility() != ESlateVisibility::Collapsed
        && !CraftButton->GetIsEnabled()
        && Status.Contains(TEXT("Cooking Rack context unavailable"));
    const uint32 RequestsBeforeStaleCraft = CookingRackCraftRequestCountForTest;
    Craft();
    const bool bStaleNoRequest = CookingRackCraftRequestCountForTest == RequestsBeforeStaleCraft;

    bEmbeddedContext = bPreviousEmbedded;
    bCookingRackContext = bPreviousCookingRack;
    bCauldronContext = bPreviousCauldron;
    bWorkbenchCraftContext = bPreviousWorkbenchCraft;
    bForgeCraftContext = bPreviousForgeCraft;
    bForgeUpgradeContext = bPreviousForgeUpgrade;
    bForgeRepairContext = bPreviousForgeRepair;
    bWorkbenchRepairContext = bPreviousWorkbenchRepair;
    StationFilterKit = PreviousStationKit;
    StationContextSection = PreviousSection;
    ContextStationActor = PreviousStation;
    ContextOwnerPawn = PreviousOwnerPawn;
    ContextConstructionId = PreviousConstructionId;
    SetRecipeBrowse(PreviousQuery, PreviousCategory, bPreviousNameSort);
    const TArray<int32> RestoredIndices = GetVisibleRecipeIndices();
    Selected = RestoredIndices.IsEmpty() ? 0 : FMath::Clamp(PreviousSelection, 0, RestoredIndices.Num() - 1);
    ConfigureStationContextPresentation(PreviousSection);
    Refresh();

    const bool bUiScope = bMenuContents && bNoUnrelatedRecipesOrActions;
    UE_LOG(LogTemp, Display, TEXT("Cooking Rack scope: Recipes=%d Ingredients=%d Quantity=%d Description=%d Heat=%d NoUnrelated=%d StaleNoRequest=%d UiScope=%d"),
        bOnlyRackRecipes, bIngredientCounts, bQuantity, bSelectedDescription, bLiveHeatFeedback,
        bNoUnrelatedRecipesOrActions, bStaleNoRequest, bUiScope);
    return bOnlyRackRecipes && bIngredientCounts && bQuantity && bSelectedDescription && bLiveHeatFeedback
        && bNoUnrelatedRecipesOrActions && bStaleNoRequest && bUiScope;
}

bool UKalmalaCraftingWidget::VerifyCauldronScopeForTest()
{
    const auto* Catalogue = UKalmalaRecipeCatalogue::Get();
    if (!bOpen || !Catalogue || !RecipeGrid || !Ingredients || !RequirementText || !CraftButton || !StationContextStatusText)
        return false;

    const FName CauldronKit(TEXT("CauldronKit"));
    const FName StewRecipeId(TEXT("MeatStewRecipe"));
    const FName SoupRecipeId(TEXT("RootVegetableSoupRecipe"));
    const FName StewOutputId(TEXT("MeatStew"));
    const FName SoupOutputId(TEXT("RootVegetableSoup"));
    const FKalmalaRecipe* StewRecipe = Catalogue->Find(StewRecipeId);
    const FKalmalaRecipe* SoupRecipe = Catalogue->Find(SoupRecipeId);
    const bool bRecipesAuthored = StewRecipe && SoupRecipe
        && IsCauldronMenuRecipe(*StewRecipe) && IsCauldronMenuRecipe(*SoupRecipe)
        && StewRecipe->Output == StewOutputId && SoupRecipe->Output == SoupOutputId
        && StewRecipe->RequiredStation.Contains(TEXT("Cauldron"))
        && SoupRecipe->RequiredStation.Contains(TEXT("Cauldron"));
    const auto HasIngredient = [](const FKalmalaRecipe* Recipe, const FName ItemId, const int32 Quantity)
    {
        const FKalmalaInventoryStack* Ingredient = Recipe
            ? Recipe->Ingredients.FindByPredicate([ItemId](const FKalmalaInventoryStack& Stack) { return Stack.ItemId == ItemId; })
            : nullptr;
        return Ingredient && Ingredient->Quantity == Quantity;
    };
    const bool bIngredientDefinitions = StewRecipe && SoupRecipe
        && StewRecipe->Ingredients.Num() == 4 && SoupRecipe->Ingredients.Num() == 4
        && HasIngredient(StewRecipe, TEXT("BoarMeat"), 1) && HasIngredient(StewRecipe, TEXT("DeerMeat"), 1)
        && HasIngredient(StewRecipe, TEXT("Carrot"), 2) && HasIngredient(StewRecipe, TEXT("Potato"), 2)
        && HasIngredient(SoupRecipe, TEXT("Carrot"), 1) && HasIngredient(SoupRecipe, TEXT("Potato"), 1)
        && HasIngredient(SoupRecipe, TEXT("Rutabaga"), 1) && HasIngredient(SoupRecipe, TEXT("Onion"), 1);
    const bool bQuantityDefinitions = StewRecipe && SoupRecipe
        && StewRecipe->MaxBatch == 5 && SoupRecipe->MaxBatch == 5
        && StewRecipe->OutputCount == 1 && SoupRecipe->OutputCount == 1;
    const FString CookSection(TEXT("Cook"));
    const bool bInteractionRoute = IsStationContextShellKit(CauldronKit)
        && IsStationContextSectionSupported(CauldronKit, CookSection)
        && GetInitialStationContextSection(CauldronKit) == CookSection;

    const bool bPreviousEmbedded = bEmbeddedContext;
    const bool bPreviousCookingRack = bCookingRackContext;
    const bool bPreviousCauldron = bCauldronContext;
    const bool bPreviousWorkbenchCraft = bWorkbenchCraftContext;
    const bool bPreviousForgeCraft = bForgeCraftContext;
    const bool bPreviousForgeUpgrade = bForgeUpgradeContext;
    const bool bPreviousForgeRepair = bForgeRepairContext;
    const bool bPreviousWorkbenchRepair = bWorkbenchRepairContext;
    const FName PreviousStationKit = StationFilterKit;
    const FString PreviousSection = StationContextSection;
    const TWeakObjectPtr<AKalmalaConstructionActor> PreviousStation = ContextStationActor;
    const TWeakObjectPtr<APawn> PreviousOwnerPawn = ContextOwnerPawn;
    const FString PreviousConstructionId = ContextConstructionId;
    const FString PreviousQuery = RecipeQuery;
    const int32 PreviousCategory = RecipeCategory;
    const bool bPreviousNameSort = bRecipeNameSort;
    const int32 PreviousSelection = Selected;

    bEmbeddedContext = true;
    StationFilterKit = CauldronKit;
    StationContextSection = CookSection;
    ContextStationActor.Reset();
    ContextOwnerPawn.Reset();
    ContextConstructionId.Reset();
    RecipeQuery.Reset();
    RecipeCategory = 0;
    bRecipeNameSort = false;
    Selected = 0;
    ConfigureStationContextPresentation(CookSection);
    const TArray<int32> CauldronIndices = GetVisibleRecipeIndices();
    const auto& Recipes = Catalogue->Recipes;
    const int32 StewIndex = CauldronIndices.IndexOfByPredicate([&Recipes, StewRecipeId](const int32 Index)
    {
        return Recipes.IsValidIndex(Index) && Recipes[Index].RecipeId == StewRecipeId;
    });
    const int32 SoupIndex = CauldronIndices.IndexOfByPredicate([&Recipes, SoupRecipeId](const int32 Index)
    {
        return Recipes.IsValidIndex(Index) && Recipes[Index].RecipeId == SoupRecipeId;
    });
    Selected = StewIndex == INDEX_NONE ? 0 : StewIndex;
    Refresh();

    bool bOnlyCauldronRecipes = CauldronIndices.Num() == 2 && bRecipesAuthored && bInteractionRoute;
    for (const int32 Index : CauldronIndices)
        bOnlyCauldronRecipes &= Recipes.IsValidIndex(Index) && IsCauldronMenuRecipe(Recipes[Index])
            && Recipes[Index].RequiredStation.Contains(TEXT("Cauldron"));
    const FString StewRequirements = RequirementText->GetText().ToString();
    const FString StewIngredients = Ingredients->GetPresentationText();
    const bool bStewCounts = StewIngredients.Contains(TEXT("BoarMeat |")) && StewIngredients.Contains(TEXT("DeerMeat |"))
        && StewIngredients.Contains(TEXT("Carrot |")) && StewIngredients.Contains(TEXT("Potato |"))
        && StewIngredients.Contains(TEXT("owned ")) && StewIngredients.Contains(TEXT("/ required 1"))
        && StewIngredients.Contains(TEXT("/ required 2"));
    const bool bStewQuantity = StewRequirements.Contains(TEXT("one batch per press"))
        && StewRequirements.Contains(TEXT("up to 5 batches per request"));
    const bool bStewHeat = StewRequirements.Contains(TEXT("Cooking heat: usable lit hearth"));
    Selected = SoupIndex == INDEX_NONE ? Selected : SoupIndex;
    Refresh();
    const FString SoupRequirements = RequirementText->GetText().ToString();
    const FString SoupIngredients = Ingredients->GetPresentationText();
    const bool bSoupCounts = SoupIngredients.Contains(TEXT("Carrot |")) && SoupIngredients.Contains(TEXT("Potato |"))
        && SoupIngredients.Contains(TEXT("Rutabaga |")) && SoupIngredients.Contains(TEXT("Onion |"))
        && SoupIngredients.Contains(TEXT("owned ")) && SoupIngredients.Contains(TEXT("/ required 1"));
    const bool bSoupQuantity = SoupRequirements.Contains(TEXT("one batch per press"))
        && SoupRequirements.Contains(TEXT("up to 5 batches per request"));
    const bool bSoupHeat = SoupRequirements.Contains(TEXT("Cooking heat: usable lit hearth"));
    const FKalmalaItemDefinition* SoupOutput = UKalmalaItemCatalogue::Get()->FindItem(SoupOutputId);
    const bool bSoupDescription = SoupOutput && DetailText
        && DetailText->GetText().ToString() == SoupOutput->Description;
    Selected = StewIndex == INDEX_NONE ? 0 : StewIndex;
    Refresh();
    const FString Status = StationContextStatusText->GetText().ToString();
    const bool bIngredientCounts = bIngredientDefinitions && bStewCounts && bSoupCounts;
    const bool bQuantity = bQuantityDefinitions && bStewQuantity && bSoupQuantity;
    const FKalmalaItemDefinition* StewOutput = UKalmalaItemCatalogue::Get()->FindItem(StewOutputId);
    const bool bSelectedDescription = bSoupDescription && StewOutput && DetailText
        && DetailText->GetText().ToString() == StewOutput->Description;
    const bool bLiveHeatFeedback = bStewHeat && bSoupHeat
        && Status.Contains(TEXT("hearth heat"), ESearchCase::IgnoreCase);
    bool bNoUnrelatedRecipesOrActions = true;
    for (const TObjectPtr<UWidget>& ExcludedWidget : StationCraftExcludedWidgets)
        bNoUnrelatedRecipesOrActions &= ExcludedWidget
            && ExcludedWidget->GetVisibility() == ESlateVisibility::Collapsed;
    for (const TObjectPtr<UWidget>& ExcludedWidget : CookingRackExcludedWidgets)
        bNoUnrelatedRecipesOrActions &= ExcludedWidget
            && ExcludedWidget->GetVisibility() == ESlateVisibility::Collapsed;
    bNoUnrelatedRecipesOrActions &= ToolProgressionText
        && ToolProgressionText->GetVisibility() == ESlateVisibility::Collapsed
        && ToolProgressionActions && ToolProgressionActions->GetVisibility() == ESlateVisibility::Collapsed
        && CraftBronzeAxeButton && CraftBronzeAxeButton->GetVisibility() == ESlateVisibility::Collapsed
        && UpgradeIronAxeButton && UpgradeIronAxeButton->GetVisibility() == ESlateVisibility::Collapsed
        && StationSectionSwitcher && StationSectionSwitcher->GetVisibility() == ESlateVisibility::Collapsed;
    const bool bMenuContents = HeaderText && HeaderText->GetText().ToString() == TEXT("Cauldron — Cook")
        && InstructionsText && InstructionsText->GetText().ToString().Contains(TEXT("Meat stew and Root vegetable soup only"))
        && RecipeGrid->GetVisibility() != ESlateVisibility::Collapsed
        && Ingredients->GetVisibility() != ESlateVisibility::Collapsed
        && DetailText && DetailText->GetVisibility() != ESlateVisibility::Collapsed
        && bSelectedDescription
        && RequirementText->GetVisibility() != ESlateVisibility::Collapsed
        && CraftButton->GetVisibility() != ESlateVisibility::Collapsed
        && StationContextStatusText->GetVisibility() != ESlateVisibility::Collapsed
        && !CraftButton->GetIsEnabled()
        && Status.Contains(TEXT("Cauldron context unavailable"));
    const uint32 RequestsBeforeStaleCraft = CauldronCraftRequestCountForTest;
    Craft();
    const bool bStaleNoRequest = CauldronCraftRequestCountForTest == RequestsBeforeStaleCraft;

    bEmbeddedContext = bPreviousEmbedded;
    bCookingRackContext = bPreviousCookingRack;
    bCauldronContext = bPreviousCauldron;
    bWorkbenchCraftContext = bPreviousWorkbenchCraft;
    bForgeCraftContext = bPreviousForgeCraft;
    bForgeUpgradeContext = bPreviousForgeUpgrade;
    bForgeRepairContext = bPreviousForgeRepair;
    bWorkbenchRepairContext = bPreviousWorkbenchRepair;
    StationFilterKit = PreviousStationKit;
    StationContextSection = PreviousSection;
    ContextStationActor = PreviousStation;
    ContextOwnerPawn = PreviousOwnerPawn;
    ContextConstructionId = PreviousConstructionId;
    SetRecipeBrowse(PreviousQuery, PreviousCategory, bPreviousNameSort);
    const TArray<int32> RestoredIndices = GetVisibleRecipeIndices();
    Selected = RestoredIndices.IsEmpty() ? 0 : FMath::Clamp(PreviousSelection, 0, RestoredIndices.Num() - 1);
    ConfigureStationContextPresentation(PreviousSection);
    Refresh();

    const bool bUiScope = bMenuContents && bNoUnrelatedRecipesOrActions;
    UE_LOG(LogTemp, Display, TEXT("Cauldron scope: Recipes=%d Ingredients=%d Quantity=%d Description=%d Heat=%d NoUnrelated=%d StaleNoRequest=%d UiScope=%d"),
        bOnlyCauldronRecipes, bIngredientCounts, bQuantity, bSelectedDescription, bLiveHeatFeedback,
        bNoUnrelatedRecipesOrActions, bStaleNoRequest, bUiScope);
    return bOnlyCauldronRecipes && bIngredientCounts && bQuantity && bSelectedDescription && bLiveHeatFeedback
        && bNoUnrelatedRecipesOrActions && bStaleNoRequest && bUiScope;
}

bool UKalmalaCraftingWidget::VerifyFryingPanScopeForTest()
{
    const UKalmalaRecipeCatalogue* Catalogue = UKalmalaRecipeCatalogue::Get();
    if (!bOpen || !Catalogue || !RecipeGrid || !Ingredients || !RequirementText || !DetailText
        || !HeaderText || !InstructionsText || !CraftButton || !StationContextStatusText)
        return false;

    const FName PanKit(TEXT("FryingPanKit"));
    const FName RootsRecipeId(TEXT("RoastedRootVegetablesRecipe"));
    const FName RoastRecipeId(TEXT("DeerRootRoastRecipe"));
    const FName RootsOutputId(TEXT("RoastedRootVegetables"));
    const FName RoastOutputId(TEXT("DeerRootRoast"));
    const FKalmalaRecipe* RootsRecipe = Catalogue->Find(RootsRecipeId);
    const FKalmalaRecipe* RoastRecipe = Catalogue->Find(RoastRecipeId);
    const bool bRecipesAuthored = RootsRecipe && RoastRecipe
        && IsFryingPanMenuRecipe(*RootsRecipe) && IsFryingPanMenuRecipe(*RoastRecipe)
        && RootsRecipe->Output == RootsOutputId && RoastRecipe->Output == RoastOutputId
        && RootsRecipe->RequiredStation.Contains(TEXT("FryingPan"))
        && RoastRecipe->RequiredStation.Contains(TEXT("FryingPan"));
    const auto HasIngredient = [](const FKalmalaRecipe* Recipe, const FName ItemId, const int32 Quantity)
    {
        const FKalmalaInventoryStack* Ingredient = Recipe
            ? Recipe->Ingredients.FindByPredicate([ItemId](const FKalmalaInventoryStack& Stack) { return Stack.ItemId == ItemId; })
            : nullptr;
        return Ingredient && Ingredient->Quantity == Quantity;
    };
    const bool bIngredientDefinitions = RootsRecipe && RoastRecipe
        && RootsRecipe->Ingredients.Num() == 3 && RoastRecipe->Ingredients.Num() == 3
        && HasIngredient(RootsRecipe, TEXT("Carrot"), 1) && HasIngredient(RootsRecipe, TEXT("Potato"), 1)
        && HasIngredient(RootsRecipe, TEXT("Onion"), 1)
        && HasIngredient(RoastRecipe, TEXT("DeerMeat"), 1) && HasIngredient(RoastRecipe, TEXT("Rutabaga"), 1)
        && HasIngredient(RoastRecipe, TEXT("Onion"), 1);
    const bool bQuantityDefinitions = RootsRecipe && RoastRecipe
        && RootsRecipe->MaxBatch == 5 && RoastRecipe->MaxBatch == 5
        && RootsRecipe->OutputCount == 1 && RoastRecipe->OutputCount == 1;
    const FKalmalaRecipe* ForgePanRecipe = Catalogue->Find(TEXT("FryingPanRecipe"));
    const FKalmalaInventoryStack* ForgeIronCost = ForgePanRecipe
        ? ForgePanRecipe->Ingredients.FindByPredicate([](const FKalmalaInventoryStack& Ingredient)
            { return Ingredient.ItemId == TEXT("Iron"); })
        : nullptr;
    const bool bForgeProductionSeparate = ForgePanRecipe && ForgePanRecipe->Output == PanKit
        && ForgePanRecipe->RequiredStation.Contains(TEXT("ForgeKit")) && ForgeIronCost && ForgeIronCost->Quantity == 5;
    const bool bBuildPlacementSupported = FKalmalaPlacementPreview::IsSupportedKit(PanKit)
        && GetBuildBrowseGroup(PanKit) == 5;
    const FString CookSection(TEXT("Cook"));
    const bool bInteractionRoute = IsStationContextShellKit(PanKit)
        && IsStationContextSectionSupported(PanKit, CookSection)
        && GetInitialStationContextSection(PanKit) == CookSection;

    const bool bPreviousEmbedded = bEmbeddedContext;
    const bool bPreviousCookingRack = bCookingRackContext;
    const bool bPreviousCauldron = bCauldronContext;
    const bool bPreviousFryingPan = bFryingPanContext;
    const bool bPreviousWorkbenchCraft = bWorkbenchCraftContext;
    const bool bPreviousForgeCraft = bForgeCraftContext;
    const bool bPreviousForgeUpgrade = bForgeUpgradeContext;
    const bool bPreviousForgeRepair = bForgeRepairContext;
    const bool bPreviousWorkbenchRepair = bWorkbenchRepairContext;
    const FName PreviousStationKit = StationFilterKit;
    const FString PreviousSection = StationContextSection;
    const TWeakObjectPtr<AKalmalaConstructionActor> PreviousStation = ContextStationActor;
    const TWeakObjectPtr<APawn> PreviousOwnerPawn = ContextOwnerPawn;
    const FString PreviousConstructionId = ContextConstructionId;
    const FString PreviousQuery = RecipeQuery;
    const int32 PreviousCategory = RecipeCategory;
    const bool bPreviousNameSort = bRecipeNameSort;
    const int32 PreviousSelection = Selected;

    StationFilterKit = NAME_None;
    RecipeQuery.Reset();
    RecipeCategory = 5;
    bRecipeNameSort = false;
    const int32 ForgePanRecipeIndex = Catalogue->Recipes.IndexOfByPredicate(
        [](const FKalmalaRecipe& Recipe) { return Recipe.RecipeId == TEXT("FryingPanRecipe"); });
    const bool bBuildPlacementSeparate = bBuildPlacementSupported
        && ForgePanRecipeIndex != INDEX_NONE && GetVisibleRecipeIndices().Contains(ForgePanRecipeIndex);

    bEmbeddedContext = true;
    StationFilterKit = PanKit;
    StationContextSection = CookSection;
    ContextStationActor.Reset();
    ContextOwnerPawn.Reset();
    ContextConstructionId.Reset();
    RecipeQuery.Reset();
    RecipeCategory = 0;
    bRecipeNameSort = false;
    Selected = 0;
    ConfigureStationContextPresentation(CookSection);
    const TArray<int32> PanIndices = GetVisibleRecipeIndices();
    const auto& Recipes = Catalogue->Recipes;
    const int32 RootsIndex = PanIndices.IndexOfByPredicate([&Recipes, RootsRecipeId](const int32 Index)
    {
        return Recipes.IsValidIndex(Index) && Recipes[Index].RecipeId == RootsRecipeId;
    });
    const int32 RoastIndex = PanIndices.IndexOfByPredicate([&Recipes, RoastRecipeId](const int32 Index)
    {
        return Recipes.IsValidIndex(Index) && Recipes[Index].RecipeId == RoastRecipeId;
    });
    Selected = RootsIndex == INDEX_NONE ? 0 : RootsIndex;
    Refresh();

    bool bOnlyPanRecipes = PanIndices.Num() == 2 && bRecipesAuthored && bInteractionRoute
        && bForgeProductionSeparate && bBuildPlacementSeparate;
    for (const int32 Index : PanIndices)
        bOnlyPanRecipes &= Recipes.IsValidIndex(Index) && IsFryingPanMenuRecipe(Recipes[Index])
            && Recipes[Index].RequiredStation.Contains(TEXT("FryingPan"));
    const FString RootsRequirements = RequirementText->GetText().ToString();
    const FString RootsIngredients = Ingredients->GetPresentationText();
    const bool bRootsCounts = RootsIngredients.Contains(TEXT("Carrot |")) && RootsIngredients.Contains(TEXT("Potato |"))
        && RootsIngredients.Contains(TEXT("Onion |")) && RootsIngredients.Contains(TEXT("owned "))
        && RootsIngredients.Contains(TEXT("/ required 1"));
    const bool bRootsQuantity = RootsRequirements.Contains(TEXT("one batch per press"))
        && RootsRequirements.Contains(TEXT("up to 5 batches per request"));
    const bool bRootsHeat = RootsRequirements.Contains(TEXT("Cooking heat: usable lit hearth"));
    Selected = RoastIndex == INDEX_NONE ? Selected : RoastIndex;
    Refresh();
    const FString RoastRequirements = RequirementText->GetText().ToString();
    const FString RoastIngredients = Ingredients->GetPresentationText();
    const bool bRoastCounts = RoastIngredients.Contains(TEXT("DeerMeat |")) && RoastIngredients.Contains(TEXT("Rutabaga |"))
        && RoastIngredients.Contains(TEXT("Onion |")) && RoastIngredients.Contains(TEXT("owned "))
        && RoastIngredients.Contains(TEXT("/ required 1"));
    const bool bRoastQuantity = RoastRequirements.Contains(TEXT("one batch per press"))
        && RoastRequirements.Contains(TEXT("up to 5 batches per request"));
    const bool bRoastHeat = RoastRequirements.Contains(TEXT("Cooking heat: usable lit hearth"));
    const FKalmalaItemDefinition* RootsOutput = UKalmalaItemCatalogue::Get()->FindItem(RootsOutputId);
    const FKalmalaItemDefinition* RoastOutput = UKalmalaItemCatalogue::Get()->FindItem(RoastOutputId);
    const bool bRoastDescription = RootsOutput && RoastOutput && DetailText
        && DetailText->GetText().ToString() == RoastOutput->Description;
    Selected = RootsIndex == INDEX_NONE ? 0 : RootsIndex;
    Refresh();
    const FString Status = StationContextStatusText->GetText().ToString();
    const bool bIngredientCounts = bIngredientDefinitions && bRootsCounts && bRoastCounts;
    const bool bQuantity = bQuantityDefinitions && bRootsQuantity && bRoastQuantity;
    const bool bSelectedDescription = bRoastDescription && RootsOutput && DetailText
        && DetailText->GetText().ToString() == RootsOutput->Description;
    const bool bLiveHeatFeedback = bRootsHeat && bRoastHeat
        && Status.Contains(TEXT("hearth heat"), ESearchCase::IgnoreCase);
    bool bNoUnrelatedRecipesOrActions = true;
    for (const TObjectPtr<UWidget>& ExcludedWidget : StationCraftExcludedWidgets)
        bNoUnrelatedRecipesOrActions &= ExcludedWidget
            && ExcludedWidget->GetVisibility() == ESlateVisibility::Collapsed;
    for (const TObjectPtr<UWidget>& ExcludedWidget : CookingRackExcludedWidgets)
        bNoUnrelatedRecipesOrActions &= ExcludedWidget
            && ExcludedWidget->GetVisibility() == ESlateVisibility::Collapsed;
    bNoUnrelatedRecipesOrActions &= ToolProgressionText
        && ToolProgressionText->GetVisibility() == ESlateVisibility::Collapsed
        && ToolProgressionActions && ToolProgressionActions->GetVisibility() == ESlateVisibility::Collapsed
        && CraftBronzeAxeButton && CraftBronzeAxeButton->GetVisibility() == ESlateVisibility::Collapsed
        && UpgradeIronAxeButton && UpgradeIronAxeButton->GetVisibility() == ESlateVisibility::Collapsed
        && StationSectionSwitcher && StationSectionSwitcher->GetVisibility() == ESlateVisibility::Collapsed;
    const bool bMenuContents = HeaderText && HeaderText->GetText().ToString() == TEXT("Frying Pan — Cook")
        && InstructionsText && InstructionsText->GetText().ToString().Contains(TEXT("Roasted root vegetables and Deer and rutabaga roast only"))
        && RecipeGrid->GetVisibility() != ESlateVisibility::Collapsed
        && Ingredients->GetVisibility() != ESlateVisibility::Collapsed
        && DetailText && DetailText->GetVisibility() != ESlateVisibility::Collapsed
        && bSelectedDescription
        && RequirementText->GetVisibility() != ESlateVisibility::Collapsed
        && CraftButton->GetVisibility() != ESlateVisibility::Collapsed
        && StationContextStatusText->GetVisibility() != ESlateVisibility::Collapsed
        && !CraftButton->GetIsEnabled()
        && Status.Contains(TEXT("Frying Pan context unavailable"));
    const uint32 RequestsBeforeStaleCraft = FryingPanCraftRequestCountForTest;
    Craft();
    const bool bStaleNoRequest = FryingPanCraftRequestCountForTest == RequestsBeforeStaleCraft;

    bEmbeddedContext = bPreviousEmbedded;
    bCookingRackContext = bPreviousCookingRack;
    bCauldronContext = bPreviousCauldron;
    bFryingPanContext = bPreviousFryingPan;
    bWorkbenchCraftContext = bPreviousWorkbenchCraft;
    bForgeCraftContext = bPreviousForgeCraft;
    bForgeUpgradeContext = bPreviousForgeUpgrade;
    bForgeRepairContext = bPreviousForgeRepair;
    bWorkbenchRepairContext = bPreviousWorkbenchRepair;
    StationFilterKit = PreviousStationKit;
    StationContextSection = PreviousSection;
    ContextStationActor = PreviousStation;
    ContextOwnerPawn = PreviousOwnerPawn;
    ContextConstructionId = PreviousConstructionId;
    SetRecipeBrowse(PreviousQuery, PreviousCategory, bPreviousNameSort);
    const TArray<int32> RestoredIndices = GetVisibleRecipeIndices();
    Selected = RestoredIndices.IsEmpty() ? 0 : FMath::Clamp(PreviousSelection, 0, RestoredIndices.Num() - 1);
    ConfigureStationContextPresentation(PreviousSection);
    Refresh();

    const bool bUiScope = bMenuContents && bNoUnrelatedRecipesOrActions;
    UE_LOG(LogTemp, Display, TEXT("Frying Pan scope: Recipes=%d Ingredients=%d Quantity=%d Description=%d Heat=%d NoUnrelated=%d StaleNoRequest=%d UiScope=%d"),
        bOnlyPanRecipes, bIngredientCounts, bQuantity, bSelectedDescription, bLiveHeatFeedback,
        bNoUnrelatedRecipesOrActions, bStaleNoRequest, bUiScope);
    return bOnlyPanRecipes && bIngredientCounts && bQuantity && bSelectedDescription && bLiveHeatFeedback
        && bNoUnrelatedRecipesOrActions && bStaleNoRequest && bUiScope;
}

bool UKalmalaCraftingWidget::VerifyForgeUpgradeScopeForTest()
{
    const FName ForgeKit(TEXT("ForgeKit"));
    const FName IronAxe(TEXT("IronAxe"));
    const FString CraftSection(TEXT("Craft"));
    const FString UpgradeSection(TEXT("Upgrade"));
    const FString Details = BuildForgeUpgradePresentationText(Model());
    const FKalmalaToolProgressionEntry* Entry = FKalmalaToolProgressionContract::FindEntry(IronAxe);
    const bool bComparison = Details.Contains(TEXT("Comparison: Bronze Axe"))
        && Details.Contains(TEXT("Iron Axe 2")) && Details.Contains(TEXT("→"))
        && Details.Contains(TEXT("24/24 condition"));
    const bool bRequirements = Entry && Entry->RequiredStation == EKalmalaToolStationKind::Forge
        && Entry->RequiredStationLevel == 2 && Entry->RequiredSkill == EKalmalaSkill::Crafting
        && Entry->RequiredSkillLevel == 5
        && Details.Contains(TEXT("Requirements: Forge level 2, Crafting level 5 + second-tier unlock"));
    bool bMaterials = Entry && !Entry->MaterialCosts.IsEmpty();
    const UKalmalaItemCatalogue* Items = UKalmalaItemCatalogue::Get();
    if (Entry && Items)
    {
        for (const FKalmalaToolMaterialCost& Cost : Entry->MaterialCosts)
        {
            const FKalmalaItemDefinition* Item = Items->FindItem(Cost.ItemId);
            const FString RequiredMaterial = Item
                ? FString::Printf(TEXT("%d %s"), Cost.Quantity, *Item->DisplayName)
                : FString::Printf(TEXT("%d %s"), Cost.Quantity, *Cost.ItemId.ToString());
            bMaterials &= Details.Contains(RequiredMaterial);
        }
    }
    else bMaterials = false;
    const bool bStatus = Details.Contains(TEXT("Ready to upgrade."))
        || Details.Contains(TEXT("Unavailable: "))
        || Details.Contains(TEXT("already carried."))
        || Details.Contains(TEXT("details unavailable."))
        || Details.Contains(TEXT("requirements unavailable."));
    const bool bRoute = IsStationContextShellKit(ForgeKit)
        && IsStationContextSectionSupported(ForgeKit, CraftSection)
        && IsStationContextSectionSupported(ForgeKit, UpgradeSection)
        && GetInitialStationContextSection(ForgeKit) == CraftSection;

    const bool bPreviousOpen = bOpen;
    const bool bPreviousEmbedded = bEmbeddedContext;
    const bool bPreviousWorkbenchCraft = bWorkbenchCraftContext;
    const bool bPreviousForgeCraft = bForgeCraftContext;
    const bool bPreviousForgeUpgrade = bForgeUpgradeContext;
    const bool bPreviousForgeRepair = bForgeRepairContext;
    const bool bPreviousWorkbenchRepair = bWorkbenchRepairContext;
    const FName PreviousStationKit = StationFilterKit;
    const FString PreviousSection = StationContextSection;
    const TWeakObjectPtr<AKalmalaConstructionActor> PreviousStation = ContextStationActor;
    const TWeakObjectPtr<APawn> PreviousOwnerPawn = ContextOwnerPawn;
    const FString PreviousConstructionId = ContextConstructionId;

    bOpen = true;
    bEmbeddedContext = true;
    bWorkbenchCraftContext = false;
    bForgeCraftContext = false;
    bForgeUpgradeContext = true;
    bForgeRepairContext = false;
    bWorkbenchRepairContext = false;
    StationFilterKit = ForgeKit;
    StationContextSection = UpgradeSection;
    ContextStationActor.Reset();
    ContextOwnerPawn.Reset();
    ContextConstructionId.Reset();
    ConfigureStationContextPresentation(UpgradeSection);
    RefreshForgeUpgradeState(Model());
    const bool bContextUnavailable = ToolProgressionText
        && ToolProgressionText->GetText().ToString().Contains(TEXT("Forge context unavailable"))
        && UpgradeIronAxeButton && !UpgradeIronAxeButton->GetIsEnabled()
        && StationContextStatusText
        && StationContextStatusText->GetText().ToString().Contains(TEXT("Forge level unavailable"));
    const uint32 RequestsBeforeStaleClick = ForgeUpgradeRequestCountForTest;
    UpgradeIronAxe();
    const bool bStaleNoRequest = ForgeUpgradeRequestCountForTest == RequestsBeforeStaleClick;
    bool bUiScope = HeaderText && HeaderText->GetText().ToString() == TEXT("Forge — Upgrade")
        && StationSectionSwitcher && StationSectionSwitcher->GetVisibility() == ESlateVisibility::Visible
        && StationContextStatusText && StationContextStatusText->GetVisibility() == ESlateVisibility::Visible
        && ForgeCraftSectionButton && ForgeCraftSectionButton->GetVisibility() == ESlateVisibility::Visible
        && ForgeUpgradeSectionButton && ForgeUpgradeSectionButton->GetVisibility() == ESlateVisibility::Visible
        && ForgeRepairSectionButton && ForgeRepairSectionButton->GetVisibility() == ESlateVisibility::Visible
        && WorkbenchCraftSectionButton && WorkbenchCraftSectionButton->GetVisibility() == ESlateVisibility::Collapsed
        && WorkbenchRepairSectionButton && WorkbenchRepairSectionButton->GetVisibility() == ESlateVisibility::Collapsed
        && ToolProgressionText && ToolProgressionText->GetVisibility() == ESlateVisibility::Visible
        && ToolProgressionActions && ToolProgressionActions->GetVisibility() == ESlateVisibility::Visible
        && UpgradeIronAxeButton && UpgradeIronAxeButton->GetVisibility() == ESlateVisibility::Visible
        && !UpgradeIronAxeButton->GetIsEnabled()
        && CraftBronzeAxeButton && CraftBronzeAxeButton->GetVisibility() == ESlateVisibility::Collapsed
        && CraftButton && CraftButton->GetVisibility() == ESlateVisibility::Collapsed
        && RecipeGrid && RecipeGrid->GetVisibility() == ESlateVisibility::Collapsed
        && WorkbenchRepairButton && WorkbenchRepairButton->GetVisibility() == ESlateVisibility::Collapsed
        && WorkbenchRepairInspector && WorkbenchRepairInspector->GetVisibility() == ESlateVisibility::Collapsed;
    for (const TObjectPtr<UWidget>& ExcludedWidget : StationCraftExcludedWidgets)
        bUiScope &= ExcludedWidget && ExcludedWidget->GetVisibility() == ESlateVisibility::Collapsed;
    for (const TObjectPtr<UWidget>& ExcludedWidget : WorkbenchRepairExcludedWidgets)
        bUiScope &= ExcludedWidget && ExcludedWidget->GetVisibility() == ESlateVisibility::Collapsed;

    bOpen = bPreviousOpen;
    bEmbeddedContext = bPreviousEmbedded;
    bWorkbenchCraftContext = bPreviousWorkbenchCraft;
    bForgeCraftContext = bPreviousForgeCraft;
    bForgeUpgradeContext = bPreviousForgeUpgrade;
    bForgeRepairContext = bPreviousForgeRepair;
    bWorkbenchRepairContext = bPreviousWorkbenchRepair;
    StationFilterKit = PreviousStationKit;
    StationContextSection = PreviousSection;
    ContextStationActor = PreviousStation;
    ContextOwnerPawn = PreviousOwnerPawn;
    ContextConstructionId = PreviousConstructionId;
    ConfigureStationContextPresentation(PreviousSection);
    Refresh();

    UE_LOG(LogTemp, Display, TEXT("Forge Upgrade scope: Comparison=%d Requirements=%d Materials=%d Status=%d Route=%d UiScope=%d StaleNoRequest=%d ContextUnavailable=%d"),
        bComparison, bRequirements, bMaterials, bStatus, bRoute, bUiScope, bStaleNoRequest, bContextUnavailable);
    return bComparison && bRequirements && bMaterials && bStatus && bRoute
        && bContextUnavailable && bStaleNoRequest && bUiScope;
}

bool UKalmalaCraftingWidget::VerifyForgeRepairScopeForTest()
{
    const APlayerController* PC = GetOwningPlayer();
    const AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(GetOwningPlayerPawn());
    TArray<FKalmalaCatalogueRow> RepairRows;
    if (Character)
    {
        for (const FKalmalaToolState& Tool : Character->GetCarriedToolInventory())
        {
            const FKalmalaToolDefinition* Definition = FKalmalaToolLifecycleContract::FindDefinition(Tool.ToolId);
            if (!Definition || Tool.ToolId == TEXT("ConstructionHammer")) continue;
            const bool bValidCondition = Tool.ToolLevel >= 1 && Tool.Durability >= 0
                && Tool.Durability <= Definition->MaxDurability;
            const FString Detail = bValidCondition
                ? FString::Printf(TEXT("Level %d\nCondition %d/%d"), Tool.ToolLevel,
                    Tool.Durability, Definition->MaxDurability)
                : TEXT("Condition unavailable");
            RepairRows.Add({Tool.ToolId, GetReadableToolName(Tool.ToolId), Detail, true});
        }
    }

    const int32 TextScale = UKalmalaSettingsWidget::ClampTextScale(UKalmalaSettingsWidget::GetTextScalePercent());
    const int32 Contrast = UKalmalaSettingsWidget::ClampContrastMode(UKalmalaSettingsWidget::GetContrastMode());
    const FName PreviousInventorySelection = InventoryInspector ? InventoryInspector->GetSelectedItem() : NAME_None;
    const int32 PreviousRecipeSelection = Selected;
    const FString PreviousUpgradeState = BuildForgeUpgradePresentationText(Model());
    if (WorkbenchRepairInspector)
    {
        WorkbenchRepairInspector->SetRows(RepairRows, TextScale, Contrast);
        WorkbenchRepairInspector->SetCategory(2);
    }
    const FName SelectedToolId = WorkbenchRepairInspector
        ? WorkbenchRepairInspector->GetSelectedItem() : NAME_None;
    const FKalmalaCatalogueRow* SelectedRow = RepairRows.FindByPredicate([SelectedToolId](const FKalmalaCatalogueRow& Row)
        { return Row.Id == SelectedToolId; });
    const bool bOwnerOnlySource = PC && PC->IsLocalController() && Character == PC->GetPawn();
    const bool bToolRows = !RepairRows.IsEmpty() && WorkbenchRepairInspector
        && WorkbenchRepairInspector->GetVisibleCount() == RepairRows.Num()
        && RepairRows.ContainsByPredicate([](const FKalmalaCatalogueRow& Row) { return !Row.bCarriedTool; }) == false;
    const bool bCondition = SelectedRow && SelectedRow->Detail.Contains(TEXT("Level"))
        && SelectedRow->Detail.Contains(TEXT("Condition"));
    const bool bSelected = !SelectedToolId.IsNone() && SelectedRow != nullptr;
    const FName ForgeKit(TEXT("ForgeKit"));
    const FString RepairSection(TEXT("Repair"));
    const bool bRoute = IsStationContextShellKit(ForgeKit)
        && IsStationContextSectionSupported(ForgeKit, RepairSection)
        && GetInitialStationContextSection(ForgeKit) == TEXT("Craft");

    const bool bPreviousOpen = bOpen;
    const bool bPreviousEmbedded = bEmbeddedContext;
    const bool bPreviousWorkbenchCraft = bWorkbenchCraftContext;
    const bool bPreviousForgeCraft = bForgeCraftContext;
    const bool bPreviousForgeUpgrade = bForgeUpgradeContext;
    const bool bPreviousForgeRepair = bForgeRepairContext;
    const bool bPreviousWorkbenchRepair = bWorkbenchRepairContext;
    const FName PreviousStationKit = StationFilterKit;
    const FString PreviousSection = StationContextSection;
    const TWeakObjectPtr<AKalmalaConstructionActor> PreviousStation = ContextStationActor;
    const TWeakObjectPtr<APawn> PreviousOwnerPawn = ContextOwnerPawn;
    const FString PreviousConstructionId = ContextConstructionId;

    bOpen = true;
    bEmbeddedContext = true;
    bWorkbenchCraftContext = false;
    bForgeCraftContext = false;
    bForgeUpgradeContext = false;
    bForgeRepairContext = true;
    bWorkbenchRepairContext = false;
    StationFilterKit = ForgeKit;
    StationContextSection = RepairSection;
    ContextStationActor.Reset();
    ContextOwnerPawn.Reset();
    ContextConstructionId.Reset();
    ConfigureStationContextPresentation(RepairSection);
    RefreshWorkbenchRepairState(Model());
    const bool bContextUnavailable = WorkbenchRepairStatusText
        && WorkbenchRepairStatusText->GetText().ToString().Contains(TEXT("Forge context unavailable"))
        && WorkbenchRepairButton && !WorkbenchRepairButton->GetIsEnabled();
    const uint32 RequestsBeforeStaleClick = WorkbenchRepairRequestCountForTest;
    RepairWorkbenchSelectedTool();
    const bool bStaleNoRequest = WorkbenchRepairRequestCountForTest == RequestsBeforeStaleClick;
    const bool bUiScope = HeaderText && HeaderText->GetText().ToString() == TEXT("Forge — Repair")
        && StationSectionSwitcher && StationSectionSwitcher->GetVisibility() == ESlateVisibility::Visible
        && StationContextStatusText && StationContextStatusText->GetVisibility() == ESlateVisibility::Visible
        && ForgeCraftSectionButton && ForgeCraftSectionButton->GetVisibility() == ESlateVisibility::Visible
        && ForgeUpgradeSectionButton && ForgeUpgradeSectionButton->GetVisibility() == ESlateVisibility::Visible
        && ForgeRepairSectionButton && ForgeRepairSectionButton->GetVisibility() == ESlateVisibility::Visible
        && WorkbenchCraftSectionButton && WorkbenchCraftSectionButton->GetVisibility() == ESlateVisibility::Collapsed
        && WorkbenchRepairSectionButton && WorkbenchRepairSectionButton->GetVisibility() == ESlateVisibility::Collapsed
        && WorkbenchRepairContextText && WorkbenchRepairContextText->GetVisibility() == ESlateVisibility::Visible
        && WorkbenchRepairInspector && WorkbenchRepairInspector->GetVisibility() == ESlateVisibility::Visible
        && WorkbenchRepairButton && WorkbenchRepairButton->GetVisibility() == ESlateVisibility::Visible
        && WorkbenchRepairStatusText && WorkbenchRepairStatusText->GetVisibility() == ESlateVisibility::Visible
        && RecipeGrid && RecipeGrid->GetVisibility() == ESlateVisibility::Collapsed
        && DetailText && DetailText->GetVisibility() == ESlateVisibility::Collapsed
        && CraftButton && CraftButton->GetVisibility() == ESlateVisibility::Collapsed
        && ToolProgressionText && ToolProgressionText->GetVisibility() == ESlateVisibility::Collapsed
        && ToolProgressionActions && ToolProgressionActions->GetVisibility() == ESlateVisibility::Collapsed
        && StateText && StateText->GetVisibility() == ESlateVisibility::Collapsed
        && FoodText && FoodText->GetVisibility() == ESlateVisibility::Collapsed
        && RepairText && RepairText->GetVisibility() == ESlateVisibility::Collapsed;
    bool bRepairUiIsolated = bUiScope;
    for (const TObjectPtr<UWidget>& ExcludedWidget : WorkbenchRepairExcludedWidgets)
        bRepairUiIsolated &= ExcludedWidget && ExcludedWidget->GetVisibility() == ESlateVisibility::Collapsed;
    const bool bUpgradeSelectionSeparate = Selected == PreviousRecipeSelection
        && (!InventoryInspector || InventoryInspector->GetSelectedItem() == PreviousInventorySelection)
        && BuildForgeUpgradePresentationText(Model()) == PreviousUpgradeState;

    bOpen = bPreviousOpen;
    bEmbeddedContext = bPreviousEmbedded;
    bWorkbenchCraftContext = bPreviousWorkbenchCraft;
    bForgeCraftContext = bPreviousForgeCraft;
    bForgeUpgradeContext = bPreviousForgeUpgrade;
    bForgeRepairContext = bPreviousForgeRepair;
    bWorkbenchRepairContext = bPreviousWorkbenchRepair;
    StationFilterKit = PreviousStationKit;
    StationContextSection = PreviousSection;
    ContextStationActor = PreviousStation;
    ContextOwnerPawn = PreviousOwnerPawn;
    ContextConstructionId = PreviousConstructionId;
    ConfigureStationContextPresentation(PreviousSection);
    Refresh();

    UE_LOG(LogTemp, Display, TEXT("Forge Repair scope: OwnerOnly=%d ToolRows=%d Condition=%d Selected=%d UpgradeSelectionSeparate=%d Route=%d InvalidContextNoRequest=%d ContextUnavailable=%d UiScope=%d"),
        bOwnerOnlySource, bToolRows, bCondition, bSelected, bUpgradeSelectionSeparate, bRoute,
        bStaleNoRequest, bContextUnavailable, bRepairUiIsolated);
    return bOwnerOnlySource && bToolRows && bCondition && bSelected && bUpgradeSelectionSeparate
        && bRoute && bContextUnavailable && bStaleNoRequest && bRepairUiIsolated;
}

bool UKalmalaCraftingWidget::VerifyWorkbenchRepairScopeForTest()
{
    const APlayerController* PC = GetOwningPlayer();
    const AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(GetOwningPlayerPawn());
    TArray<FKalmalaCatalogueRow> RepairRows;
    if (Character)
    {
        for (const FKalmalaToolState& Tool : Character->GetCarriedToolInventory())
        {
            const FKalmalaToolDefinition* Definition = FKalmalaToolLifecycleContract::FindDefinition(Tool.ToolId);
            if (!Definition || Tool.ToolId == TEXT("ConstructionHammer")) continue;
            const bool bValidCondition = Tool.ToolLevel >= 1 && Tool.Durability >= 0
                && Tool.Durability <= Definition->MaxDurability;
            const FString Detail = bValidCondition
                ? FString::Printf(TEXT("Level %d\nCondition %d/%d"), Tool.ToolLevel,
                    Tool.Durability, Definition->MaxDurability)
                : TEXT("Condition unavailable");
            RepairRows.Add({Tool.ToolId, GetReadableToolName(Tool.ToolId), Detail, true});
        }
    }

    const int32 TextScale = UKalmalaSettingsWidget::ClampTextScale(UKalmalaSettingsWidget::GetTextScalePercent());
    const int32 Contrast = UKalmalaSettingsWidget::ClampContrastMode(UKalmalaSettingsWidget::GetContrastMode());
    const FName PreviousInventorySelection = InventoryInspector ? InventoryInspector->GetSelectedItem() : NAME_None;
    const int32 PreviousRecipeSelection = Selected;
    if (WorkbenchRepairInspector)
    {
        WorkbenchRepairInspector->SetRows(RepairRows, TextScale, Contrast);
        WorkbenchRepairInspector->SetCategory(2);
    }
    const FName SelectedToolId = WorkbenchRepairInspector
        ? WorkbenchRepairInspector->GetSelectedItem() : NAME_None;
    const FKalmalaCatalogueRow* SelectedRow = RepairRows.FindByPredicate([SelectedToolId](const FKalmalaCatalogueRow& Row)
        { return Row.Id == SelectedToolId; });
    const bool bOwnerOnlySource = PC && PC->IsLocalController() && Character == PC->GetPawn();
    const bool bToolRows = !RepairRows.IsEmpty() && WorkbenchRepairInspector
        && WorkbenchRepairInspector->GetVisibleCount() == RepairRows.Num()
        && RepairRows.ContainsByPredicate([](const FKalmalaCatalogueRow& Row) { return !Row.bCarriedTool; }) == false;
    const bool bCondition = SelectedRow && SelectedRow->Detail.Contains(TEXT("Level"))
        && SelectedRow->Detail.Contains(TEXT("Condition"));
    const bool bSelected = !SelectedToolId.IsNone() && SelectedRow != nullptr;

    const bool bPreviousCraftContext = bWorkbenchCraftContext;
    const bool bPreviousRepairContext = bWorkbenchRepairContext;
    const bool bPreviousForgeRepairContext = bForgeRepairContext;
    const bool bPreviousEmbeddedContext = bEmbeddedContext;
    bWorkbenchCraftContext = false;
    bForgeRepairContext = false;
    bWorkbenchRepairContext = true;
    bEmbeddedContext = false;
    ApplyStationCraftLayout();
    const uint32 RepairRequestsBeforeInvalidClick = WorkbenchRepairRequestCountForTest;
    RepairWorkbenchSelectedTool();
    const bool bInvalidContextNoRequest = WorkbenchRepairRequestCountForTest == RepairRequestsBeforeInvalidClick;
    bool bUiScope = WorkbenchRepairContextText && WorkbenchRepairContextText->GetVisibility() == ESlateVisibility::Visible
        && WorkbenchRepairInspector && WorkbenchRepairInspector->GetVisibility() == ESlateVisibility::Visible
        && WorkbenchRepairButton && WorkbenchRepairButton->GetVisibility() == ESlateVisibility::Visible
        && WorkbenchRepairStatusText && WorkbenchRepairStatusText->GetVisibility() == ESlateVisibility::Visible
        && InventoryInspector && InventoryInspector->GetVisibility() == ESlateVisibility::Collapsed
        && RecipeSearchBox && RecipeSearchBox->GetVisibility() == ESlateVisibility::Collapsed
        && RecipeGrid && RecipeGrid->GetVisibility() == ESlateVisibility::Collapsed
        && DetailText && DetailText->GetVisibility() == ESlateVisibility::Collapsed
        && CraftButton && CraftButton->GetVisibility() == ESlateVisibility::Collapsed
        && CraftBronzeAxeButton && CraftBronzeAxeButton->GetVisibility() == ESlateVisibility::Collapsed;
    for (const TObjectPtr<UWidget>& ExcludedWidget : WorkbenchRepairExcludedWidgets)
        bUiScope &= ExcludedWidget && ExcludedWidget->GetVisibility() == ESlateVisibility::Collapsed;
    const bool bCraftSelectionSeparate = Selected == PreviousRecipeSelection
        && (!InventoryInspector || InventoryInspector->GetSelectedItem() == PreviousInventorySelection);
    bWorkbenchCraftContext = bPreviousCraftContext;
    bWorkbenchRepairContext = bPreviousRepairContext;
    bForgeRepairContext = bPreviousForgeRepairContext;
    bEmbeddedContext = bPreviousEmbeddedContext;
    ApplyStationCraftLayout();
    Refresh();

    UE_LOG(LogTemp, Display, TEXT("Workbench Repair scope: OwnerOnly=%d ToolRows=%d Condition=%d Selected=%d CraftSelectionSeparate=%d InvalidContextNoRequest=%d UiScope=%d"),
        bOwnerOnlySource, bToolRows, bCondition, bSelected, bCraftSelectionSeparate, bInvalidContextNoRequest, bUiScope);
    return bOwnerOnlySource && bToolRows && bCondition && bSelected && bCraftSelectionSeparate
        && bInvalidContextNoRequest && bUiScope;
}

bool UKalmalaCraftingWidget::VerifyStorageContextScopeForTest()
{
    UKalmalaCraftingComponent* Crafting = Model();
    if (!bOpen || !Crafting || !StorageContextPanel || !StoragePackInspector
        || !StorageContentsInspector || !StorageStoreButton || !StorageWithdrawButton) return false;
    const FName PreviousKit = StationFilterKit;
    const bool bPreviousEmbedded = bEmbeddedContext;
    const bool bPreviousStorage = bStorageContext;
    const TWeakObjectPtr<AKalmalaConstructionActor> PreviousActor = ContextStationActor;
    const TWeakObjectPtr<APawn> PreviousOwner = ContextOwnerPawn;
    const FString PreviousConstructionId = ContextConstructionId;
    const FString PreviousSection = StationContextSection;
    const FName StorageKit(TEXT("StorageKit"));
    const FString StoreSection(TEXT("Store"));
    const bool bRoute = IsStationContextShellKit(StorageKit)
        && IsStationContextSectionSupported(StorageKit, StoreSection)
        && GetInitialStationContextSection(StorageKit) == StoreSection;
    StationFilterKit = TEXT("StorageKit");
    StationContextSection = TEXT("Store");
    bEmbeddedContext = true;
    bStorageContext = true;
    ContextStationActor.Reset();
    ContextOwnerPawn.Reset();
    ContextConstructionId.Reset();
    ApplyStationCraftLayout();
    Refresh();

    const auto* Inventory = GetOwningPlayerPawn()
        ? GetOwningPlayerPawn()->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    const int32 ExpectedChestRows = Crafting->HasStorageView() ? Crafting->GetStorageView().Num() : 0;
    const bool bOwnerPackRows = Inventory
        && StoragePackInspector->GetVisibleCount() == Inventory->GetStacks().Num();
    const bool bOwnerChestRows = StorageContentsInspector->GetVisibleCount() == ExpectedChestRows;
    const FString CapacityStatus = StorageContextStatusText ? StorageContextStatusText->GetText().ToString() : FString();
    const bool bCapacityFeedback = CapacityStatus.Contains(TEXT("Pack stacks:"))
        && CapacityStatus.Contains(TEXT("Chest stacks:")) && CapacityStatus.Contains(TEXT("/ 16"));
    const bool bUiScope = StorageContextPanel->GetVisibility() == ESlateVisibility::Visible
        && StoragePackInspector->GetVisibility() == ESlateVisibility::Visible
        && StorageContentsInspector->GetVisibility() == ESlateVisibility::Visible
        && InventoryInspector && InventoryInspector->GetVisibility() == ESlateVisibility::Collapsed
        && RecipeSearchBox && RecipeSearchBox->GetVisibility() == ESlateVisibility::Collapsed
        && RecipeGrid && RecipeGrid->GetVisibility() == ESlateVisibility::Collapsed
        && CraftButton && CraftButton->GetVisibility() == ESlateVisibility::Collapsed
        && WorkbenchRepairInspector && WorkbenchRepairInspector->GetVisibility() == ESlateVisibility::Collapsed
        && StationContextStatusText && StationContextStatusText->GetVisibility() == ESlateVisibility::Collapsed
        && StationSectionSwitcher && StationSectionSwitcher->GetVisibility() == ESlateVisibility::Collapsed
        && UpgradeIronAxeButton && UpgradeIronAxeButton->GetVisibility() == ESlateVisibility::Collapsed
        && CraftBronzeAxeButton && CraftBronzeAxeButton->GetVisibility() == ESlateVisibility::Collapsed
        && ToolProgressionText && ToolProgressionText->GetVisibility() == ESlateVisibility::Collapsed
        && ToolProgressionActions && ToolProgressionActions->GetVisibility() == ESlateVisibility::Collapsed
        && StorageStoreButton->GetVisibility() == ESlateVisibility::Visible && !StorageStoreButton->GetIsEnabled()
        && StorageWithdrawButton->GetVisibility() == ESlateVisibility::Visible && !StorageWithdrawButton->GetIsEnabled();
    bool bOtherControlsHidden = true;
    for (const TObjectPtr<UWidget>& ExcludedWidget : WorkbenchRepairExcludedWidgets)
        bOtherControlsHidden &= ExcludedWidget && ExcludedWidget->GetVisibility() == ESlateVisibility::Collapsed;
    const uint32 RequestsBeforeStaleActions = StorageTransferRequestCountForTest;
    DepositStorage();
    WithdrawStorage();
    const bool bStaleNoRequest = !IsStationContextValid()
        && StorageTransferRequestCountForTest == RequestsBeforeStaleActions;

    StationFilterKit = PreviousKit;
    StationContextSection = PreviousSection;
    bEmbeddedContext = bPreviousEmbedded;
    bStorageContext = bPreviousStorage;
    ContextStationActor = PreviousActor;
    ContextOwnerPawn = PreviousOwner;
    ContextConstructionId = PreviousConstructionId;
    ApplyStationCraftLayout();
    Refresh();

    UE_LOG(LogTemp, Display, TEXT("Chest scope: OwnerPack=%d OwnerChest=%d Capacity=%d Route=%d StaleNoRequest=%d UiScope=%d"),
        bOwnerPackRows, bOwnerChestRows, bCapacityFeedback, bRoute, bStaleNoRequest, bUiScope && bOtherControlsHidden);
    return bOwnerPackRows && bOwnerChestRows && bCapacityFeedback && bRoute && bStaleNoRequest
        && bUiScope && bOtherControlsHidden;
}
#endif

void UKalmalaCraftingWidget::Refresh()
{
    auto* M=Model(); if (!M) { Close(); return; }
    if (ToolProgressionText)
    {
        const FName ProgressionStation = bWorkbenchCraftContext ? FName(TEXT("WorkbenchKit"))
            : (bForgeCraftContext ? FName(TEXT("ForgeKit")) : NAME_None);
        ToolProgressionText->SetText(FText::FromString(M->GetToolProgressionText(ProgressionStation)));
    }
    RefreshStationContextState();
    RefreshForgeUpgradeState(M);
    const auto& Recipes=UKalmalaRecipeCatalogue::Get()->Recipes;
    const TArray<int32> VisibleIndices = GetVisibleRecipeIndices();
    const int32 TextScalePercent = UKalmalaSettingsWidget::ClampTextScale(
        UKalmalaSettingsWidget::GetTextScalePercent());
    const int32 ContrastMode = UKalmalaSettingsWidget::ClampContrastMode(
        UKalmalaSettingsWidget::GetContrastMode());
    TArray<FKalmalaCatalogueRow> InspectionRows;
    TArray<FKalmalaCatalogueRow> WorkbenchRepairRows;
    TArray<FKalmalaCatalogueRow> StoragePackRows;
    TArray<FKalmalaCatalogueRow> StorageContentsRows;
    const auto* OwnerPawn = GetOwningPlayerPawn();
    const auto* OwnerInventory = OwnerPawn ? OwnerPawn->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    if (OwnerInventory)
        for (const auto& Stack : OwnerInventory->GetStacks())
        {
            const auto* Item = UKalmalaItemCatalogue::Get()->FindItem(Stack.ItemId);
            const FKalmalaCatalogueRow Row{Stack.ItemId, Item ? Item->DisplayName : Stack.ItemId.ToString(),
                FString::Printf(TEXT("Count %d"), Stack.Quantity), false};
            InspectionRows.Add(Row);
            StoragePackRows.Add(Row);
        }
    if (M->HasStorageView())
        for (const auto& Stack : M->GetStorageView())
        {
            const auto* Item = UKalmalaItemCatalogue::Get()->FindItem(Stack.ItemId);
            StorageContentsRows.Add({Stack.ItemId, Item ? Item->DisplayName : Stack.ItemId.ToString(),
                FString::Printf(TEXT("Count %d"), Stack.Quantity), false});
        }
    if (const auto* Character = Cast<AKalmalaCharacter>(OwnerPawn))
        for (const auto& Tool : Character->GetCarriedToolInventory())
        {
            const auto* Definition = FKalmalaToolLifecycleContract::FindDefinition(Tool.ToolId);
            const bool bValidCondition = Definition && Tool.ToolLevel >= 1 && Tool.Durability >= 0
                && Tool.Durability <= Definition->MaxDurability;
            const FString VisibleState = bValidCondition
                ? FString::Printf(TEXT("Level %d\nCondition %d/%d — %s"), Tool.ToolLevel,
                    Tool.Durability, Definition->MaxDurability, Tool.Durability == 0 ? TEXT("Broken") : TEXT("Usable"))
                : TEXT("Condition unavailable");
            const FKalmalaCatalogueRow ToolRow{Tool.ToolId, GetReadableToolName(Tool.ToolId), VisibleState, true};
            InspectionRows.Add(ToolRow);
            if (Tool.ToolId != TEXT("ConstructionHammer") && Definition)
                WorkbenchRepairRows.Add(ToolRow);
        }
    if (LastDetailTextScalePercent != TextScalePercent || LastDetailContrastMode != ContrastMode)
    {
        const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
        Theme.ApplyMenu(*WidgetTree, HeaderText, TextScalePercent, ContrastMode);
        if (bEmbeddedContext)
        {
            MenuBackground->SetPadding(FMargin(0.0f));
            MenuBackground->SetBrushColor(FLinearColor::Transparent);
        }
        else Theme.ApplyPanel(*MenuBackground, ContrastMode, &Theme.BuildPanelImage);
        if (RecipeSearchBox)
        {
            RecipeSearchStyle.SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), FMath::RoundToInt(Theme.BodySize * TextScalePercent / 100.f)));
            RecipeSearchBox->SetWidgetStyle(RecipeSearchStyle);
        }
        LastDetailTextScalePercent = TextScalePercent;
        LastDetailContrastMode = ContrastMode;
    }
    if (InventoryInspector) InventoryInspector->SetRows(InspectionRows, TextScalePercent, ContrastMode);
    if (StoragePackInspector)
        StoragePackInspector->SetRows(StoragePackRows, TextScalePercent, ContrastMode,
            TEXT("Your pack"), TEXT("Your pack is empty."), false);
    if (StorageContentsInspector)
        StorageContentsInspector->SetRows(StorageContentsRows, TextScalePercent, ContrastMode,
            TEXT("This chest"), M->HasStorageView() ? TEXT("This chest is empty.") : TEXT("Chest contents are unavailable."), false);
    if (WorkbenchRepairInspector)
    {
        WorkbenchRepairInspector->SetRows(WorkbenchRepairRows, TextScalePercent, ContrastMode);
        RefreshWorkbenchRepairState(M);
    }
    if (StorageContextStatusText)
    {
        const TArray<FKalmalaInventoryStack>& ChestStacks = M->GetStorageView();
        const bool bStorageContextValid = bStorageContext && IsStationContextValid();
        FString ContextStatus = FString::Printf(TEXT("Pack stacks: %d / %d · Chest stacks: %d / %d\nOne item moves per action."),
            OwnerInventory ? OwnerInventory->GetStacks().Num() : 0, UKalmalaInventoryComponent::MaxSlots,
            M->HasStorageView() ? ChestStacks.Num() : 0, UKalmalaInventoryComponent::MaxSlots);
        const FName DepositId = StoragePackInspector ? StoragePackInspector->GetSelectedItem() : NAME_None;
        const FName WithdrawId = StorageContentsInspector ? StorageContentsInspector->GetSelectedItem() : NAME_None;
        const UKalmalaItemCatalogue* ItemsCatalogue = UKalmalaItemCatalogue::Get();
        const auto* DepositStack = OwnerInventory ? OwnerInventory->GetStacks().FindByPredicate(
            [DepositId](const FKalmalaInventoryStack& Stack) { return Stack.ItemId == DepositId; }) : nullptr;
        const auto* ChestDepositStack = ChestStacks.FindByPredicate(
            [DepositId](const FKalmalaInventoryStack& Stack) { return Stack.ItemId == DepositId; });
        const bool bCanStoreSelected = DepositStack && ItemsCatalogue && ItemsCatalogue->FindItem(DepositId)
            && M->HasStorageView()
            && (ChestDepositStack
                ? ItemsCatalogue->CanAddToStack(DepositId, ChestDepositStack->Quantity, 1)
                : ChestStacks.Num() < UKalmalaInventoryComponent::MaxSlots);
        const auto* ChestWithdrawStack = ChestStacks.FindByPredicate(
            [WithdrawId](const FKalmalaInventoryStack& Stack) { return Stack.ItemId == WithdrawId; });
        const auto* PackWithdrawStack = OwnerInventory ? OwnerInventory->GetStacks().FindByPredicate(
            [WithdrawId](const FKalmalaInventoryStack& Stack) { return Stack.ItemId == WithdrawId; }) : nullptr;
        const bool bCanTakeSelected = ChestWithdrawStack && OwnerInventory && ItemsCatalogue
            && ItemsCatalogue->FindItem(WithdrawId)
            && (PackWithdrawStack
                ? ItemsCatalogue->CanAddToStack(WithdrawId, PackWithdrawStack->Quantity, 1)
                : OwnerInventory->GetStacks().Num() < UKalmalaInventoryComponent::MaxSlots);
        if (!M->HasStorageView()) ContextStatus += TEXT("\nChest view unavailable.");
        else if (!bStorageContextValid) ContextStatus += TEXT("\nChest context expired. Interact with a nearby Chest again.");
        else if (!bCanStoreSelected && !DepositId.IsNone()) ContextStatus += TEXT("\nStore unavailable: chest stack or slot capacity reached.");
        else if (!bCanTakeSelected && !WithdrawId.IsNone()) ContextStatus += TEXT("\nTake unavailable: pack stack or slot capacity reached.");
        else ContextStatus += TEXT("\nSelect a pack item to store or a chest item to take.");
        StorageContextStatusText->SetText(FText::FromString(ContextStatus));
        FKalmalaUITheme::Get().ApplyText(*StorageContextStatusText, FKalmalaUITheme::Get().BodySize,
            false, TextScalePercent, ContrastMode);
        if (StorageStoreButton) StorageStoreButton->SetIsEnabled(bStorageContextValid && bCanStoreSelected);
        if (StorageWithdrawButton) StorageWithdrawButton->SetIsEnabled(bStorageContextValid && bCanTakeSelected);
    }
    if (!VisibleIndices.IsEmpty()) Selected=FMath::Clamp(Selected,0,VisibleIndices.Num()-1);
    RefreshRecipeGrid(VisibleIndices, M, TextScalePercent, ContrastMode);
    if (VisibleIndices.IsEmpty())
    {
        RecipesText->SetText(FText::FromString(TEXT("No matching recipes. Clear recipe search or choose All. Station scope still applies.\n")));
        DetailText->SetText(FText::GetEmpty());
        Ingredients->SetIngredients({}, nullptr, TextScalePercent, ContrastMode);
        RequirementText->SetText(FText::GetEmpty());
        if (SelectedIcon) SelectedIcon->SetVisibility(ESlateVisibility::Collapsed);
        if (CraftButton && CraftButton->GetIsEnabled())
        {
            CraftButton->SetIsEnabled(false);
            FKalmalaUITheme::Get().ApplyButton(*CraftButton, ContrastMode);
        }
        return;
    }
    Selected=FMath::Clamp(Selected,0,VisibleIndices.Num()-1);
    const int32 RecipeIndex = VisibleIndices[Selected];
    const FKalmalaRecipe& SelectedRecipe = Recipes[RecipeIndex];
    TArray<FKalmalaInventoryStack> IngredientCosts = SelectedRecipe.Ingredients;
    if (UKalmalaRecipeCatalogue::IsDirectMaterialBuildable(SelectedRecipe.Output))
    {
        FString Failure;
        UKalmalaRecipeCatalogue::BuildDirectMaterialCost(SelectedRecipe.Output, IngredientCosts, Failure);
    }
    Ingredients->SetIngredients(IngredientCosts,
        OwnerPawn ? OwnerPawn->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr,
        TextScalePercent, ContrastMode);
    EKalmalaIcon Kind; int32 IconVariant;
    UKalmalaIconWidget::FindCatalogueIcon(SelectedRecipe.Output, Kind, IconVariant);
    SelectedIcon->SetIcon(Kind, IconVariant);
    SelectedIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
    const FString StationPrefix = StationFilterKit.IsNone() ? TEXT("")
        : (UKalmalaItemCatalogue::Get()->FindItem(StationFilterKit)
            ? UKalmalaItemCatalogue::Get()->FindItem(StationFilterKit)->DisplayName : StationFilterKit.ToString()) + TEXT(" recipes: ");
    RecipesText->SetText(FText::FromString(FString::Printf(TEXT("> %s%d of %d: %s\n"),
        *StationPrefix, Selected + 1, VisibleIndices.Num(), *SelectedRecipe.DisplayName)));
    const FString Availability = M->GetRecipeAvailability(SelectedRecipe.RecipeId);
    const auto* RequirementOwner = Cast<AKalmalaCharacter>(OwnerPawn);
    RequirementText->SetText(FText::FromString(FKalmalaRecipeRequirements::Describe(SelectedRecipe,
        OwnerPawn ? OwnerPawn->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr,
        RequirementOwner ? RequirementOwner->GetCarriedToolLevel(TEXT("ConstructionHammer")) : -1,
        Availability)));
    const bool bDirectBuild = UKalmalaRecipeCatalogue::IsDirectMaterialBuildable(SelectedRecipe.Output);
    const FString RecipeDescription = M->GetRecipeDescription(SelectedRecipe.RecipeId);
    DetailText->SetText(FText::FromString(RecipeDescription));
    if (CraftButton)
    {
        if (UTextBlock* ButtonLabel = Cast<UTextBlock>(CraftButton->GetContent()))
            ButtonLabel->SetText(FText::FromString(bDirectBuild ? TEXT("Build selected") : TEXT("Craft one")));
        const FString ButtonToolTip = bDirectBuild
            ? TEXT("Build the selected structure from its shown materials.")
            : TEXT("Craft one batch of the selected recipe.");
        CraftButton->SetToolTipText(FText::FromString(ButtonToolTip));
        const bool bCookingContext = bCookingRackContext || bCauldronContext || bFryingPanContext;
        const bool bCookingContextCanCraft = !bCookingContext
            || (IsStationContextValid() && IsCookingMenuRecipe(StationFilterKit, SelectedRecipe));
        const bool bCanCraft = SelectedRecipe.bEnabled && bCookingContextCanCraft;
        if (CraftButton->GetIsEnabled() != bCanCraft)
        {
            CraftButton->SetIsEnabled(bCanCraft);
            FKalmalaUITheme::Get().ApplyButton(*CraftButton, ContrastMode);
        }
    }
    FString PreviewText;
    if (bPlacementPreviewEnabled)
    {
        const FKalmalaPlacementPreview Preview = FKalmalaPlacementPreview::Evaluate(GetWorld(), GetOwningPlayerPawn(), SelectedRecipe.Output);
        PreviewText = TEXT("\n") + Preview.Message + (Preview.bIsValid ? FString::Printf(TEXT(" (%.0f, %.0f)"), Preview.Location.X, Preview.Location.Y) : TEXT("")) + TEXT("\n");
    }
    FString ToolConditionText = TEXT("\nTool condition — owner-only:");
    const auto* Character = Cast<AKalmalaCharacter>(GetOwningPlayerPawn());
    int32 CarriedToolConditionCount = 0;
    const auto AppendToolCondition = [&ToolConditionText, &CarriedToolConditionCount, Character](const FKalmalaToolDefinition& Definition)
    {
        const int32 ToolLevel = Character ? Character->GetCarriedToolLevel(Definition.ToolId) : 0;
        if (ToolLevel <= 0) return;
        ++CarriedToolConditionCount;
        const int32 Condition = Character->GetToolDurability(Definition.ToolId);
        if (Condition < 0 || Condition > Definition.MaxDurability)
        {
            ToolConditionText += FString::Printf(TEXT("\n%s: condition unavailable"), *GetReadableToolName(Definition.ToolId));
            return;
        }
        ToolConditionText += FString::Printf(TEXT("\n%s level %d: %d/%d · %s"), *GetReadableToolName(Definition.ToolId),
            ToolLevel, Condition, Definition.MaxDurability, Condition < Definition.MaxDurability ? TEXT("damaged") : TEXT("full"));
    };
    for (const FKalmalaToolDefinition& Definition : FKalmalaToolLifecycleContract::GetDefinitions()) AppendToolCondition(Definition);
    for (const FKalmalaToolDefinition& Definition : FKalmalaToolLifecycleContract::GetTieredAxeDefinitions()) AppendToolCondition(Definition);
    AppendToolCondition(FKalmalaToolLifecycleContract::GetConstructionHammerDefinition());
    if (CarriedToolConditionCount == 0) ToolConditionText += TEXT("\nNo carried tools.");
    StateText->SetText(FText::FromString(TEXT("\nNearby hearth (replicated shared state; text does not rely on colour):\n")
        + ToolConditionText + TEXT("\n") + M->GetNearbyFireText()+TEXT("\n")+M->GetNearbyConstructionText()+TEXT("\n")+M->GetNearbyWorkbenchText()+TEXT("\n")+M->GetLastResult()+TEXT("\n")+PreviewText));
    FoodText->SetText(FText::FromString(M->GetFoodText()));
    const auto* Catalogue = UKalmalaItemCatalogue::Get();
    SelectedStorageItem = FMath::Clamp(SelectedStorageItem, 0, FMath::Max(0, Catalogue->Items.Num()-1));
    FString ChestText = TEXT("No item catalogue\n");
    if (Catalogue->Items.IsValidIndex(SelectedStorageItem))
    {
        const FKalmalaItemDefinition& SelectedItem = Catalogue->Items[SelectedStorageItem];
        ChestText = FString::Printf(TEXT("Selected item: %s\nDescription: %s\n"),
            *SelectedItem.DisplayName, *SelectedItem.Description);
    }
    ChestText += M->HasStorageView() ? TEXT("Inspected chest (16 stack maximum):\n") : TEXT("Chest contents unavailable; inspect nearby first.\n");
    if (M->HasStorageView())
    {
        if (M->GetStorageView().IsEmpty()) ChestText += TEXT("Empty\n");
        for (const auto& Stack : M->GetStorageView())
        {
            const auto* Item = Catalogue->FindItem(Stack.ItemId);
            ChestText += FString::Printf(TEXT("%s: %d\n"), Item ? *Item->DisplayName : TEXT("Unknown item"), Stack.Quantity);
        }
    }
    StorageText->SetText(FText::FromString(ChestText));
}

FString UKalmalaCraftingWidget::GetPresentationText() const
{
    return InstructionsText && RecipesText && DetailText && StateText
        ? InstructionsText->GetText().ToString()+RecipesText->GetText().ToString()+DetailText->GetText().ToString()
            + (Ingredients ? Ingredients->GetPresentationText() : FString())
            + StateText->GetText().ToString() + (FoodText ? FoodText->GetText().ToString() : FString())
            + (RepairText ? RepairText->GetText().ToString() : FString())
            + (ToolProgressionText ? ToolProgressionText->GetText().ToString() : FString())
            + (RequirementText ? RequirementText->GetText().ToString() : FString())
            + (CraftButton ? CraftButton->GetToolTipText().ToString() : FString())
            + (StorageText ? StorageText->GetText().ToString() : FString()) : FString();
}
void UKalmalaCraftingWidget::NativeTick(const FGeometry& G,float D) { Super::NativeTick(G,D); if(bOpen) Refresh(); }
void UKalmalaCraftingWidget::Previous() { const int32 N=GetVisibleRecipeIndices().Num(); if(N) Selected=(Selected+N-1)%N; Refresh(); }
void UKalmalaCraftingWidget::Next() { const int32 N=GetVisibleRecipeIndices().Num(); if(N) Selected=(Selected+1)%N; Refresh(); }
void UKalmalaCraftingWidget::Craft()
{
    const auto& Recipes = UKalmalaRecipeCatalogue::Get()->Recipes;
    const TArray<int32> VisibleIndices = GetVisibleRecipeIndices();
    if (!VisibleIndices.IsValidIndex(Selected) || !Recipes.IsValidIndex(VisibleIndices[Selected])) return;
    const FKalmalaRecipe& Recipe = Recipes[VisibleIndices[Selected]];
    if ((bCookingRackContext || bCauldronContext || bFryingPanContext)
        && (!IsStationContextValid() || !IsCookingMenuRecipe(StationFilterKit, Recipe))) return;
    if (UKalmalaRecipeCatalogue::IsDirectMaterialBuildable(Recipe.Output)) { Place(); return; }
    if (auto* M = Model())
    {
#if !UE_BUILD_SHIPPING
        if (bCookingRackContext) ++CookingRackCraftRequestCountForTest;
        if (bCauldronContext) ++CauldronCraftRequestCountForTest;
        if (bFryingPanContext) ++FryingPanCraftRequestCountForTest;
#endif
        M->ServerCraft(Recipe.RecipeId, 1);
    }
}
void UKalmalaCraftingWidget::EnablePlacementPreview() { bPlacementPreviewEnabled = true; Refresh(); }
void UKalmalaCraftingWidget::Preview() { EnablePlacementPreview(); }
void UKalmalaCraftingWidget::Place()
{
    const auto& Recipes = UKalmalaRecipeCatalogue::Get()->Recipes;
    const TArray<int32> VisibleIndices = GetVisibleRecipeIndices();
    if (auto* M = Model(); M && VisibleIndices.IsValidIndex(Selected) && Recipes.IsValidIndex(VisibleIndices[Selected]))
    {
        const FName Kit = Recipes[VisibleIndices[Selected]].Output;
        if (Kit == TEXT("CampfireKit")) M->ServerPlaceCampfire(); else M->ServerPlaceConstruction(Kit);
    }
}
void UKalmalaCraftingWidget::Refuel() { if(auto* M=Model()) M->ServerRefuel(); }
void UKalmalaCraftingWidget::Light() { if(auto* M=Model()) M->ServerLight(); }
void UKalmalaCraftingWidget::EatFood() { if(auto* M=Model()) M->ServerConsumeFood(TEXT("RoastedFieldMeat")); }
void UKalmalaCraftingWidget::EatBroth() { if(auto* M=Model()) M->ServerConsumeFood(TEXT("HearthBroth")); }
void UKalmalaCraftingWidget::EatSmokedMeat() { if(auto* M=Model()) M->ServerConsumeFood(TEXT("SmokedFieldMeat")); }
void UKalmalaCraftingWidget::RepairReedKnife() { if(auto* M=Model()) M->ServerRepairTool(TEXT("ReedKnife")); }
void UKalmalaCraftingWidget::RepairFieldHatchet() { if(auto* M=Model()) M->ServerRepairTool(TEXT("FieldHatchet")); }
void UKalmalaCraftingWidget::RepairStonePick() { if(auto* M=Model()) M->ServerRepairTool(TEXT("StonePick")); }
void UKalmalaCraftingWidget::RepairBronzeAxe() { if(auto* M=Model()) M->ServerRepairTool(TEXT("BronzeAxe")); }
void UKalmalaCraftingWidget::RepairIronAxe() { if(auto* M=Model()) M->ServerRepairTool(TEXT("IronAxe")); }
void UKalmalaCraftingWidget::RepairWorkbenchSelectedTool()
{
    APlayerController* PC = GetOwningPlayer();
    AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(PC ? PC->GetPawn() : nullptr);
    UKalmalaCraftingComponent* Crafting = Model();
    const FName SelectedToolId = WorkbenchRepairInspector
        ? WorkbenchRepairInspector->GetSelectedItem() : NAME_None;
    const FKalmalaToolState* Tool = Character
        ? Character->GetCarriedToolInventory().FindByPredicate([SelectedToolId](const FKalmalaToolState& Candidate)
            { return Candidate.ToolId == SelectedToolId; })
        : nullptr;
    const FKalmalaToolDefinition* Definition = SelectedToolId == TEXT("ConstructionHammer")
        ? nullptr : FKalmalaToolLifecycleContract::FindDefinition(SelectedToolId);
    const bool bRepairContext = bWorkbenchRepairContext || bForgeRepairContext;
    const bool bContextKitMatches = (bWorkbenchRepairContext && StationFilterKit == TEXT("WorkbenchKit"))
        || (bForgeRepairContext && StationFilterKit == TEXT("ForgeKit"));
    if (!PC || !PC->IsLocalController() || !Character || !Crafting || !bRepairContext
        || !bContextKitMatches || !IsStationContextValid() || !Tool || !Definition
        || Tool->ToolLevel < 1 || Tool->Durability < 0 || Tool->Durability >= Definition->MaxDurability
        || bWorkbenchRepairPending) return;

    WorkbenchRepairResultSerial = Crafting->GetResultSerial();
    WorkbenchRepairPendingToolId = SelectedToolId;
    WorkbenchRepairPending = true;
    WorkbenchRepairResultToolId = NAME_None;
    WorkbenchRepairResultText.Reset();
#if !UE_BUILD_SHIPPING
    ++WorkbenchRepairRequestCountForTest;
#endif
    Crafting->ServerRepairTool(SelectedToolId);
    RefreshWorkbenchRepairState(Crafting);
}
void UKalmalaCraftingWidget::CraftBronzeAxe() { if(auto* M=Model()) M->ServerProgressTool(TEXT("BronzeAxe")); }
void UKalmalaCraftingWidget::UpgradeIronAxe()
{
    UKalmalaCraftingComponent* Crafting = Model();
    if (!Crafting) return;
    if (bForgeUpgradeContext)
    {
        RefreshForgeUpgradeState(Crafting);
        if (!UpgradeIronAxeButton || !UpgradeIronAxeButton->GetIsEnabled()) return;
#if !UE_BUILD_SHIPPING
        ++ForgeUpgradeRequestCountForTest;
#endif
    }
    Crafting->ServerProgressTool(TEXT("IronAxe"));
}
void UKalmalaCraftingWidget::InspectStorage() { if (auto* M=Model()) M->ServerOpenStorage(); }
void UKalmalaCraftingWidget::PreviousStorageItem()
{
    const int32 Count = UKalmalaItemCatalogue::Get()->Items.Num();
    if (Count) SelectedStorageItem = (SelectedStorageItem + Count - 1) % Count;
    Refresh();
}
void UKalmalaCraftingWidget::NextStorageItem()
{
    const int32 Count = UKalmalaItemCatalogue::Get()->Items.Num();
    if (Count) SelectedStorageItem = (SelectedStorageItem + 1) % Count;
    Refresh();
}
void UKalmalaCraftingWidget::DepositStorage()
{
    if (bStorageContext)
    {
        auto* M = Model();
        const FName ItemId = StoragePackInspector ? StoragePackInspector->GetSelectedItem() : NAME_None;
        if (!M || !IsStationContextValid() || !M->HasStorageView() || ItemId.IsNone()
            || !StorageStoreButton || !StorageStoreButton->GetIsEnabled()) return;
#if !UE_BUILD_SHIPPING
        ++StorageTransferRequestCountForTest;
#endif
        M->ServerDepositStorage(ItemId);
        return;
    }
    const auto& Items = UKalmalaItemCatalogue::Get()->Items;
    if (auto* M=Model(); M && Items.IsValidIndex(SelectedStorageItem)) M->ServerDepositStorage(Items[SelectedStorageItem].ItemId);
}
void UKalmalaCraftingWidget::WithdrawStorage()
{
    if (bStorageContext)
    {
        auto* M = Model();
        const FName ItemId = StorageContentsInspector ? StorageContentsInspector->GetSelectedItem() : NAME_None;
        if (!M || !IsStationContextValid() || !M->HasStorageView() || ItemId.IsNone()
            || !StorageWithdrawButton || !StorageWithdrawButton->GetIsEnabled()) return;
#if !UE_BUILD_SHIPPING
        ++StorageTransferRequestCountForTest;
#endif
        M->ServerWithdrawStorage(ItemId);
        return;
    }
    const auto& Items = UKalmalaItemCatalogue::Get()->Items;
    if (auto* M=Model(); M && Items.IsValidIndex(SelectedStorageItem)) M->ServerWithdrawStorage(Items[SelectedStorageItem].ItemId);
}
void UKalmalaCraftingWidget::CloseClicked() { Close(); }
void UKalmalaCraftingWidget::FocusInventoryDetails()
{
    if (!bOpen || !InventoryInspector) return;
    InventoryInspector->SetKeyboardFocus();
    if (CraftingScrollBox)
        CraftingScrollBox->ScrollToEnd();
}
FReply UKalmalaCraftingWidget::NativeOnPreviewKeyDown(const FGeometry& G,const FKeyEvent& E)
{
    const FKey K=E.GetKey();
    if(K==EKeys::Escape || K==EKeys::Gamepad_FaceButton_Right) { Close(); return FReply::Handled(); }
    if ((InventoryInspector && (InventoryInspector->HasKeyboardFocus() || InventoryInspector->HasFocusedDescendants()))
        || (WorkbenchRepairInspector && (WorkbenchRepairInspector->HasKeyboardFocus()
            || WorkbenchRepairInspector->HasFocusedDescendants())))
        return Super::NativeOnPreviewKeyDown(G,E);
    if (RecipeSearchBox && (RecipeSearchBox->HasKeyboardFocus() || RecipeSearchBox->HasFocusedDescendants())) return Super::NativeOnPreviewKeyDown(G,E);
    if (HasKeyboardFocus() && K == EKeys::PageUp) { CycleRecipeCategory(); return FReply::Handled(); }
    if (HasKeyboardFocus() && K == EKeys::PageDown) { CycleRecipeSort(); return FReply::Handled(); }
    if(K==EKeys::Gamepad_FaceButton_Top) { if(!E.IsRepeat()) Place(); return FReply::Handled(); }
    if(K==EKeys::Gamepad_FaceButton_Left) { if(!E.IsRepeat()) Refuel(); return FReply::Handled(); }
    if(K==EKeys::Gamepad_RightShoulder) { if(!E.IsRepeat()) Light(); return FReply::Handled(); }
    if(K==EKeys::P) { if(!E.IsRepeat()) Preview(); return FReply::Handled(); }
    // Reserve arrows for recipe selection only while the panel itself has focus;
    // focused buttons keep ordinary keyboard/controller navigation and activation.
    if (HasKeyboardFocus())
    {
        if(K==EKeys::Up || K==EKeys::Gamepad_DPad_Up) { Previous(); return FReply::Handled(); }
        if(K==EKeys::Down || K==EKeys::Gamepad_DPad_Down) { Next(); return FReply::Handled(); }
        if(K==EKeys::Enter || K==EKeys::Gamepad_FaceButton_Bottom) { if(!E.IsRepeat()) Craft(); return FReply::Handled(); }
    }
    return Super::NativeOnPreviewKeyDown(G,E);
}

void UKalmalaCraftingSubsystem::Tick(float DeltaTime)
{
    if(!GetWorld() || !GetWorld()->IsGameWorld() || !GetLocalPlayer()) return;
    auto* PC=GetLocalPlayer()->GetPlayerController(GetWorld()); if(Controller!=PC) Release();
    if(!PC || !PC->IsLocalController()) return; Controller=PC;
    if(PC->InputComponent && BoundInput.Get()!=PC->InputComponent)
    {
        PC->InputComponent->BindAction(TEXT("CraftMenu"),IE_Pressed,this,&ThisClass::Toggle).bConsumeInput=true;
        BoundInput=PC->InputComponent;
    }
    if (StationContextWidget && StationContextWidget->IsOpen() && !StationContextWidget->IsTargetValid())
        StationContextWidget->Close();
    if (const auto* Character = Cast<AKalmalaCharacter>(PC->GetPawn()))
    {
        UKalmalaCraftingComponent* Crafting = Character->FindComponentByClass<UKalmalaCraftingComponent>();
        if (Crafting != StationInteractionModel.Get())
        {
            StationInteractionModel = Crafting;
            LastStationInteractionSerial = Crafting ? Crafting->GetCookingStationInteractionSerial() : 0;
            bHasSeenStationInteraction = Crafting != nullptr;
            LastStationContextSerial = Crafting ? Crafting->GetStationContextInteractionSerial() : 0;
            bHasSeenStationContext = Crafting != nullptr;
        }
        else if (Crafting)
        {
            const uint32 Serial = Crafting->GetCookingStationInteractionSerial();
            if (bHasSeenStationInteraction && Serial != LastStationInteractionSerial)
            {
                LastStationInteractionSerial = Serial;
                const FName StationKit = Crafting->GetLastInteractedCookingStationKit();
                if (IsInWorldCookingStation(StationKit) && !IsStationContextShellKit(StationKit))
                {
                    if (!Widget)
                    {
                        Widget = CreateWidget<UKalmalaCraftingWidget>(PC);
                        if (Widget) Widget->AddToPlayerScreen(160);
                    }
                    if (Widget) Widget->OpenForStation(StationKit);
                }
            }

            const uint32 ContextSerial = Crafting->GetStationContextInteractionSerial();
            if (bHasSeenStationContext && ContextSerial != LastStationContextSerial)
            {
                AKalmalaConstructionActor* Station = Crafting->GetLastStationContextActor();
                const FName ContextKit = Crafting->GetLastStationContextKit();
                const FString& ConstructionId = Crafting->GetLastStationContextConstructionId();
                if (Crafting->IsStationContextTargetCurrent(Station, ContextKit, ConstructionId))
                {
                    LastStationContextSerial = ContextSerial;
                    if (IsStationContextShellKit(ContextKit) && !IsOpen())
                    {
                        if (!Widget)
                        {
                            Widget = CreateWidget<UKalmalaCraftingWidget>(PC);
                            if (Widget) Widget->AddToPlayerScreen(160);
                        }
                        if (!StationContextWidget)
                        {
                            StationContextWidget = CreateWidget<UKalmalaStationContextWidget>(PC);
                            if (StationContextWidget) StationContextWidget->AddToPlayerScreen(170);
                        }
                        const FString Section = GetInitialStationContextSection(ContextKit);
                        if (Widget && StationContextWidget)
                            StationContextWidget->OpenForStation(Station, Section, Widget);
                    }
                }
            }
        }
    }
    else
    {
        StationInteractionModel.Reset();
        LastStationInteractionSerial = 0;
        bHasSeenStationInteraction = false;
        LastStationContextSerial = 0;
        bHasSeenStationContext = false;
    }
    UpdateInteractionPrompt(PC);
#if !UE_BUILD_SHIPPING
    UpdateInteractionPromptReview(PC, DeltaTime);
#endif
#if !UE_BUILD_SHIPPING
    if(!bVerified && PC->GetPawn() && FParse::Param(FCommandLine::Get(),TEXT("KalmalaCraftingTest")))
    {
        int32 VisualTextScale = UKalmalaSettingsWidget::GetTextScalePercent();
        int32 VisualContrast = UKalmalaSettingsWidget::GetContrastMode();
        if (FParse::Value(FCommandLine::Get(), TEXT("KalmalaUIDeveloperTextScale="), VisualTextScale))
            UKalmalaSettingsWidget::SetTextScalePercent(VisualTextScale);
        if (FParse::Value(FCommandLine::Get(), TEXT("KalmalaUIDeveloperContrast="), VisualContrast))
            UKalmalaSettingsWidget::SetContrastMode(VisualContrast);
        if(auto* Input=BoundInput.Get()) for(int32 Index=0;Index<Input->GetNumActionBindings();++Index)
        {
            auto& Binding=Input->GetActionBinding(Index);
            if(Binding.GetActionName()==TEXT("CraftMenu") && Binding.KeyEvent==IE_Pressed) Binding.ActionDelegate.Execute(FKey());
        }
        const bool bGridLayoutReady = Widget && Widget->IsOpen()
            && Widget->GetRecipeGridSummary().Contains(TEXT("Scrollable=1"));
        if (Widget && Widget->IsOpen() && !bGridLayoutReady && VerificationLayoutWait < 2.0f)
        {
            VerificationLayoutWait += DeltaTime;
        }
        if(Widget && Widget->IsOpen() && (bGridLayoutReady || VerificationLayoutWait >= 2.0f))
        {
            const auto Text=Widget->GetPresentationText();
            Widget->EnablePlacementPreview();
            const auto PreviewText=Widget->GetPresentationText();
            const bool ToolFeedbackPassed = Text.Contains(TEXT("Tool condition — owner-only"))
                && Text.Contains(TEXT("TOOL UPGRADES — OWNER ONLY"))
                && Text.Contains(TEXT("New tool: Bronze Axe 1")) && Text.Contains(TEXT("Bronze Axe 1"))
                && Text.Contains(TEXT("Iron Axe 2"))
                && Text.Contains(TEXT("Cost: 4 Wood (have "))
                && Text.Contains(TEXT("Requires: Workbench level 1."))
                && Text.Contains(TEXT("Requires: Forge level 2, Crafting level 5 + second-tier unlock."))
                && Text.Contains(TEXT("Status: "))
                && Text.Contains(TEXT("Repair: restore a damaged carried tool to full condition"))
                && Text.Contains(TEXT("Grinding Stone: repair all damaged carried tools"))
                && Text.Contains(TEXT("Field Hatchet level "));
            UE_LOG(LogTemp, Display, TEXT("M9 tool feedback: Passed=%d"), ToolFeedbackPassed);
            const auto* Character = Cast<AKalmalaCharacter>(PC->GetPawn());
            const auto* Crafting = Character ? Character->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
            const FString ChestDescription = Crafting ? Crafting->GetRecipeDescription(TEXT("Storage")) : FString();
            const FString RetiredSmokeFrameDescription = Crafting ? Crafting->GetRecipeDescription(TEXT("SmokeFrame")) : FString();
            const FString RetiredSmokingRecipeDescription = Crafting ? Crafting->GetRecipeDescription(TEXT("SmokeBoarMeat")) : FString();
            const FString DirectBuildDescription = Crafting ? Crafting->GetRecipeDescription(TEXT("Floor")) : FString();
            const FKalmalaRecipe* StorageRecipe = UKalmalaRecipeCatalogue::Get()->Find(TEXT("Storage"));
            const FKalmalaItemDefinition* StorageItem = StorageRecipe
                ? UKalmalaItemCatalogue::Get()->FindItem(StorageRecipe->Output) : nullptr;
            const FKalmalaItemDefinition* FloorItem = UKalmalaItemCatalogue::Get()->FindItem(TEXT("FloorKit"));
            const bool CampFeedbackPassed = StorageItem && ChestDescription == StorageItem->Description
                && RetiredSmokeFrameDescription == TEXT("Unknown recipe")
                && RetiredSmokingRecipeDescription == TEXT("Unknown recipe")
                && FloorItem && DirectBuildDescription == FloorItem->Description
                && !DirectBuildDescription.Contains(TEXT("Raw material cost:"))
                && !DirectBuildDescription.Contains(TEXT("Build quantity:"));
            UE_LOG(LogTemp, Display, TEXT("M9 camp feedback: Passed=%d"), CampFeedbackPassed);
            const bool bInspection = Widget->VerifyInventoryInspectionForTest();
            UE_LOG(LogTemp, Display, TEXT("Inventory inspection: FocusAndKeys=%d"), bInspection);
            const bool bGridNavigation = Widget->VerifyRecipeGridNavigationForTest();
            const FString GridSummary = Widget->GetRecipeGridSummary();
            const bool bGridReady = bGridNavigation && GridSummary.Contains(TEXT("Slots="))
                && GridSummary.Contains(TEXT("Unavailable=")) && GridSummary.Contains(TEXT("Focused=1"))
                && GridSummary.Contains(TEXT("ReadOnly=1")) && GridSummary.Contains(TEXT("Scrollable=1"));
            UE_LOG(LogTemp, Display, TEXT("Build slot grid: %s Navigation=%d"), *GridSummary, bGridNavigation);
            const bool bPromptHidden = !InteractionPrompt || InteractionPrompt->GetVisibility() != ESlateVisibility::Visible;
            UE_LOG(LogTemp, Display, TEXT("Interaction prompt modal: Hidden=%d"), bPromptHidden ? 1 : 0);
            const bool bInputLegendHidden = !Text.Contains(TEXT("Construction hammer menu input:"))
                && !Text.Contains(TEXT("Up/Down")) && !Text.Contains(TEXT("Enter / A"))
                && !Text.Contains(TEXT("Escape / B"));
            const bool Passed=bInspection && bInputLegendHidden
                && Text.Contains(TEXT("Build requirements"))
                && Text.Contains(TEXT("Ingredients — one craft/build"))
                && Text.Contains(TEXT("Stone |")) && Text.Contains(TEXT("Wood |"))
                && Text.Contains(TEXT("Construction Hammer level 1: Present."))
                && Text.Contains(TEXT("Placement: clear, dry ground with a gentle slope and room for the structure."))
                && Text.Contains(TEXT("Hearth fuel: one raw Wood, Lightwood, Densewood, or Coal; starts with 60 seconds."))
                && Text.Contains(TEXT("A low stone-and-wood hearth built in place with a Construction Hammer; raw fuel lights it."))
                && !Text.Contains(TEXT("Raw material cost:"))
                && !Text.Contains(TEXT("Build quantity:"))
                && !Text.Contains(TEXT("Output: Hearth ring construction"))
                && !Text.Contains(TEXT("Skill level: no recipe requirement."))
                && !Text.Contains(TEXT("Unlock: no additional recipe lock."))
                && !Text.Contains(TEXT("server rechecks every request"))
                && !Text.Contains(TEXT("Rejected requests preserve ingredients and tool condition."))
                && !Text.Contains(TEXT("SKILL PROGRESS [PRIVATE TO YOU]"))
                && Text.Contains(TEXT("Selection is marked with >"))
                && Text.Contains(TEXT("Repair: restore a damaged carried tool to full condition"))
                && Text.Contains(TEXT("Tool condition — owner-only")) && Text.Contains(TEXT("Field Hatchet level "))
                && Text.Contains(TEXT("Roasted field meat:"))
                && Text.Contains(TEXT("Build Hearth ring directly from raw materials"))
                && Text.Contains(TEXT("Build the selected structure from its shown materials."))
                && PreviewText.Contains(TEXT("Preview "))
                && bGridReady
                && bPromptHidden
                && PC->IsMoveInputIgnored() && Widget->IsFocusable();
            Widget->Close();
            UE_LOG(LogTemp,Display,TEXT("Crafting presentation: Passed=%d Restored=%d"),Passed,!PC->IsMoveInputIgnored()); bVerified=true;
        }
    }
    FString CapturePath;
    if(bVerified && !bCaptureRequested && PC->GetPawn() && FParse::Value(FCommandLine::Get(),TEXT("KalmalaCraftingCapture="),CapturePath))
    {
        auto* M=PC->GetPawn()->FindComponentByClass<UKalmalaCraftingComponent>();
        if(M && M->GetNearbyFireText().Contains(TEXT("State: SMOULDERING")))
        {
            if(Widget && !Widget->IsOpen()) Widget->Open();
            CaptureWait+=DeltaTime;
            if(CaptureWait>5)
            {
                UE_LOG(LogTemp, Display, TEXT("Construction feedback: Passed=%d"),
                    M->GetNearbyConstructionText().Contains(TEXT("Health: 50.0 / 100"))
                    && M->GetNearbyConstructionText().Contains(TEXT("Rain-wear limit reached")));
                FScreenshotRequest::RequestScreenshot(CapturePath,true,false); bCaptureRequested=true;
                CaptureWait = 0;
            }
        }
    }
    if (bCaptureRequested && ReviewCaptureStage < 26 && Widget
        && FParse::Value(FCommandLine::Get(), TEXT("KalmalaCraftingCapture="), CapturePath))
    {
        CaptureWait += DeltaTime;
        if (CaptureWait > 3.0f)
        {
            CaptureWait = 0;
            if (ReviewCaptureStage >= 18)
            {
                const int32 View = (ReviewCaptureStage - 18) / 2;
                static const TCHAR* Names[] = { TEXT("build-costs"), TEXT("build-requirements"), TEXT("cook-costs"), TEXT("cook-requirements") };
                if (ReviewCaptureStage % 2 == 0)
                {
                    UE_LOG(LogTemp, Display, TEXT("Ingredient review: View=%s Passed=%d"), Names[View], Widget->PrepareIngredientReviewForTest(View));
                }
                else
                {
                    FScreenshotRequest::RequestScreenshot(FPaths::GetBaseFilename(CapturePath, false)
                        + TEXT("-") + Names[View] + TEXT(".png"), true, false);
                }
            }
            else if (ReviewCaptureStage >= 6)
            {
                const int32 View = (ReviewCaptureStage - 6) / 2;
                static const TCHAR* Names[] = { TEXT("cooking"), TEXT("structural"), TEXT("stations"), TEXT("utilities"), TEXT("no-results"), TEXT("inventory-browse") };
                if (ReviewCaptureStage % 2 == 0)
                {
                    UE_LOG(LogTemp, Display, TEXT("Browsing review: View=%s Passed=%d"), Names[View], Widget->PrepareBrowseReviewForTest(View));
                }
                else
                {
                    FScreenshotRequest::RequestScreenshot(FPaths::GetBaseFilename(CapturePath, false)
                        + TEXT("-") + Names[View] + TEXT(".png"), true, false);
                }
            }
            else if (ReviewCaptureStage == 4)
            {
                UE_LOG(LogTemp, Display, TEXT("Inventory detail review: Scrolled=%d"), Widget->ScrollInventoryDetailsForTest());
            }
            else if (ReviewCaptureStage == 0 || ReviewCaptureStage == 2)
            {
                const bool bScrolled = Widget->ScrollReviewSectionForTest(ReviewCaptureStage == 2);
                UE_LOG(LogTemp, Display, TEXT("Crafting review scroll: Section=%s Passed=%d"),
                    ReviewCaptureStage == 0 ? TEXT("Details") : TEXT("Feedback"), bScrolled);
            }
            else
            {
                FScreenshotRequest::RequestScreenshot(
                    FPaths::GetBaseFilename(CapturePath, false)
                        + (ReviewCaptureStage == 1 ? TEXT("-details.png") : ReviewCaptureStage == 3 ? TEXT("-feedback.png") : TEXT("-inspection.png")), true, false);
            }
            ++ReviewCaptureStage;
        }
    }
#endif
}

#if !UE_BUILD_SHIPPING
bool UKalmalaCraftingWidget::PrepareIngredientReviewForTest(const int32 View)
{
    if (!bOpen || !CraftingScrollBox || !Ingredients || !RequirementText || View < 0 || View > 3) return false;
    const auto* Recipe = UKalmalaRecipeCatalogue::Get()->Find(View < 2 ? FName(TEXT("Floor")) : FName(TEXT("CookedBoarMeatRecipe")));
    if (!Recipe) return false;
    SetRecipeBrowse(Recipe->DisplayName, 0, false);
    const auto Indices = GetVisibleRecipeIndices();
    if (Indices.Num() != 1 || UKalmalaRecipeCatalogue::Get()->Recipes[Indices[0]].RecipeId != Recipe->RecipeId) return false;
    const auto* Inventory = GetOwningPlayerPawn() ? GetOwningPlayerPawn()->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    if (!Inventory) return false;
    const FString Costs = Ingredients->GetPresentationText();
    for (const auto& Cost : Recipe->Ingredients)
    {
        if (!Costs.Contains(FString::Printf(TEXT("owned %d / required %d"), Inventory->GetQuantity(Cost.ItemId), Cost.Quantity))) return false;
    }
    const FString Requirements = RequirementText->GetText().ToString();
    if (View < 2)
    {
        if (!Requirements.Contains(TEXT("Build requirements"))
            || !Requirements.Contains(TEXT("Construction Hammer level 1: Present."))
            || !Requirements.Contains(TEXT("Placement: clear, dry ground"))
            || Requirements.Contains(TEXT("Skill level:"))
            || Requirements.Contains(TEXT("server rechecks every request"))) return false;
    }
    else
    {
        const auto* Crafting = GetOwningPlayerPawn()
            ? GetOwningPlayerPawn()->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
        if (!Crafting || !Requirements.Contains(TEXT("Result:"))
            || !Requirements.Contains(TEXT("Quantity: one batch per press"))
            || !Requirements.Contains(TEXT("Cooking heat: usable lit hearth with positive heat"))
            || Requirements.Contains(TEXT("Skill level:")) || Requirements.Contains(TEXT("Unlock:"))
            || Requirements.Contains(TEXT("server rechecks"))
            || Requirements.Contains(TEXT("Rejected requests preserve"))) return false;

        const FString Availability = Crafting->GetRecipeAvailability(Recipe->RecipeId);
        const FString Blocker = !Recipe->bEnabled
            ? FString(TEXT("Recipe unavailable")) : Availability.TrimStartAndEnd();
        const int32 FirstBlocker = Requirements.Find(TEXT("Unavailable:"), ESearchCase::IgnoreCase, ESearchDir::FromStart);
        const int32 LastBlocker = Requirements.Find(TEXT("Unavailable:"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
        if (Blocker.IsEmpty() || Blocker.Equals(TEXT("Ready"), ESearchCase::IgnoreCase))
        {
            if (FirstBlocker != INDEX_NONE) return false;
        }
        else if (FirstBlocker == INDEX_NONE || FirstBlocker != LastBlocker
            || !Requirements.Contains(TEXT("Unavailable: ") + Blocker)) return false;
    }
    CraftingScrollBox->ScrollWidgetIntoView(View % 2 == 0 ? static_cast<UWidget*>(Ingredients.Get())
        : static_cast<UWidget*>(RequirementText.Get()), false, EDescendantScrollDestination::TopOrLeft);
    return true;
}

bool UKalmalaCraftingWidget::PrepareBrowseReviewForTest(const int32 View)
{
    if (!bOpen || !CraftingScrollBox || !RecipeSearchBox || !InventoryInspector) return false;
    if (View == 5)
    {
        InventoryInspector->SetSearch(TEXT(""));
        InventoryInspector->SetCategory(0);
        InventoryInspector->SetSort(2);
        CraftingScrollBox->ScrollWidgetIntoView(InventoryInspector, false, EDescendantScrollDestination::TopOrLeft);
        return InventoryInspector->GetVisibleCount() > 0;
    }
    const int32 Categories[] = { 2, 4, 5, 6, 3 };
    if (View < 0 || View >= UE_ARRAY_COUNT(Categories)) return false;
    SetRecipeBrowse(View == 4 ? TEXT("zz-no-matching-build") : TEXT(""), Categories[View], true);
    RecipeSearchBox->SetText(FText::FromString(View == 4 ? TEXT("zz-no-matching-build") : TEXT("")));
    CraftingScrollBox->ScrollWidgetIntoView(RecipeSearchBox, false, EDescendantScrollDestination::TopOrLeft);
    return View == 4 ? GetVisibleRecipeIndices().IsEmpty() && !CraftButton->GetIsEnabled()
        : !GetVisibleRecipeIndices().IsEmpty();
}

bool UKalmalaCraftingWidget::ScrollInventoryDetailsForTest()
{
    if (!CraftingScrollBox || !InventoryInspector || !bOpen) return false;
    TArray<UWidget*> Children;
    InventoryInspector->WidgetTree->GetAllWidgets(Children);
    for (UWidget* Child : Children)
        if (auto* Detail = Cast<UKalmalaItemDetailWidget>(Child))
        {
            // Inspection is the final scroll child. Resolve the end after layout,
            // rather than using clipped/offscreen descendant cached geometry.
            CraftingScrollBox->ScrollToEnd();
            return Detail->GetVisibility() == ESlateVisibility::Visible
                && Detail->GetCachedGeometry().GetLocalSize().X > 1.0f
                && Detail->GetCachedGeometry().GetLocalSize().Y > 1.0f;
        }
    return false;
}

bool UKalmalaCraftingWidget::ScrollReviewSectionForTest(const bool bFeedback)
{
    UTextBlock* Target = bFeedback ? RepairText.Get() : DetailText.Get();
    if (!CraftingScrollBox || !Target || !bOpen) return false;
    CraftingScrollBox->ScrollWidgetIntoView(Target, false, EDescendantScrollDestination::TopOrLeft);
    return true;
}
#endif

void UKalmalaCraftingSubsystem::UpdateInteractionPrompt(APlayerController* PlayerController)
{
    if (!PlayerController) return;
    if (!InteractionPrompt)
    {
        InteractionPrompt = CreateWidget<UKalmalaInteractionPromptWidget>(PlayerController);
        if (!InteractionPrompt) return;
        InteractionPrompt->AddToPlayerScreen(150);
        InteractionPrompt->SetDesiredSizeInViewport(FVector2D(520.0f, 176.0f));
        InteractionPrompt->SetAlignmentInViewport(FVector2D(0.5f, 0.0f));
    }

    int32 ViewportWidth = 0;
    int32 ViewportHeight = 0;
    PlayerController->GetViewportSize(ViewportWidth, ViewportHeight);
    InteractionPrompt->SetPositionInViewport(FVector2D(ViewportWidth * 0.5f, ViewportHeight * 0.5f + 32.0f), true);
    if (!PlayerController->IsLocalController() || IsOpen()
        || PlayerController->IsMoveInputIgnored())
    {
        InteractionPrompt->SetPrompt(FString());
        return;
    }

    const AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(PlayerController->GetPawn());
    if (!Character || !Character->IsLocallyControlled())
    {
        InteractionPrompt->SetPrompt(FString());
        return;
    }

    FInteractionPromptDescription Description;
    const FHitResult* Candidate = nullptr;
    FHitResult CandidateHit;
    if (const AKalmalaOceanSkiff* AttachedSkiff = Cast<AKalmalaOceanSkiff>(Character->GetAttachParentActor()))
    {
        Description.TargetName = TEXT("Skiff");
        Description.ActionName = TEXT("Disembark");
        if (AttachedSkiff->GetMode() == EKalmalaOceanSkiffMode::Underway)
            Description.UnavailableReason = TEXT("Stop before disembarking");
    }
    else if (Character->GetLocalInteractionCandidate(CandidateHit))
    {
        Candidate = &CandidateHit;
        ResolveInteractionPrompt(Character, CandidateHit.GetActor(), CandidateHit, Description);
    }

    const FString Text = Candidate != nullptr || !Description.TargetName.IsEmpty()
        ? UKalmalaInteractionPromptWidget::BuildPromptText(Description.TargetName, Description.ActionName,
            Description.UnavailableReason)
        : FString();
    InteractionPrompt->SetPrompt(Text);
}

#if !UE_BUILD_SHIPPING
void UKalmalaCraftingSubsystem::UpdateInteractionPromptReview(APlayerController* PlayerController, const float DeltaTime)
{
    if (!FParse::Param(FCommandLine::Get(), TEXT("KalmalaInteractionPromptReview")) || !InteractionPrompt
        || !PlayerController || PlayerController->GetPawn() == nullptr) return;
    if (bInteractionPromptReviewComplete)
    {
        InteractionPrompt->SetPrompt(FString());
        return;
    }

    FString CapturePath;
    FParse::Value(FCommandLine::Get(), TEXT("KalmalaInteractionPromptCapture="), CapturePath);
    InteractionPromptReviewWait += FMath::Clamp(DeltaTime, 0.0f, 0.1f);
    if (InteractionPromptReviewWait < 0.75f) return;
    InteractionPromptReviewWait = 0.0f;

    FString StageName;
    FString Text;
    switch (InteractionPromptReviewStage)
    {
    case 0:
        StageName = TEXT("available");
        Text = UKalmalaInteractionPromptWidget::BuildPromptText(TEXT("Densewood trunk"), TEXT("Chop"));
        break;
    case 1:
        StageName = TEXT("unavailable");
        Text = UKalmalaInteractionPromptWidget::BuildPromptText(TEXT("Mire campfire"), TEXT("Light"), TEXT("Too wet to light"));
        break;
    case 2:
        StageName = TEXT("modal");
        Text = UKalmalaInteractionPromptWidget::BuildPromptText(TEXT("Workbench"), TEXT("Use"), FString(), true);
        break;
    default:
        StageName = TEXT("no-target");
        Text = UKalmalaInteractionPromptWidget::BuildPromptText(FString(), FString());
        break;
    }

    InteractionPrompt->SetPrompt(Text);
    const bool bDisplayed = !Text.IsEmpty();
    UE_LOG(LogTemp, Display, TEXT("Interaction prompt review: Stage=%s Displayed=%d"), *StageName, bDisplayed ? 1 : 0);
    if (!CapturePath.IsEmpty())
    {
        FScreenshotRequest::RequestScreenshot(FPaths::GetBaseFilename(CapturePath, false)
            + TEXT("-") + StageName + TEXT(".png"), true, false);
    }
    if (++InteractionPromptReviewStage >= 4) bInteractionPromptReviewComplete = true;
}
#endif

void UKalmalaCraftingSubsystem::Toggle()
{
    if(!Controller) return;
    if (StationContextWidget && StationContextWidget->IsOpen())
    {
        StationContextWidget->Close();
        return;
    }
    if(!Widget) { Widget=CreateWidget<UKalmalaCraftingWidget>(Controller); if(Widget) Widget->AddToPlayerScreen(160); }
    if (Widget)
    {
        if (Widget->IsOpen()) Widget->Close();
        else
        {
            if (!Widget->IsInViewport()) Widget->AddToPlayerScreen(160);
            Widget->Open();
        }
    }
}
bool UKalmalaCraftingSubsystem::IsOpen() const
{
    return (StationContextWidget && StationContextWidget->IsOpen()) || (Widget && Widget->IsOpen());
}

bool UKalmalaCraftingSubsystem::CloseIfOpen()
{
    if (StationContextWidget && StationContextWidget->IsOpen())
    {
        StationContextWidget->Close();
        return true;
    }
    if (!Widget || !Widget->IsOpen()) return false;
    Widget->Close();
    return true;
}

void UKalmalaCraftingSubsystem::Release()
{
    if(auto* Input=BoundInput.Get()) for(int32 I=Input->GetNumActionBindings()-1;I>=0;--I)
        if(Input->GetActionBinding(I).ActionDelegate.IsBoundToObject(this)) Input->RemoveActionBinding(I);
    BoundInput.Reset();
    if (StationContextWidget)
    {
        StationContextWidget->Close();
        StationContextWidget->RemoveFromParent();
        StationContextWidget = nullptr;
    }
    if(Widget) { Widget->Close(); Widget->RemoveFromParent(); Widget=nullptr; }
    if(InteractionPrompt) { InteractionPrompt->RemoveFromParent(); InteractionPrompt=nullptr; }
    StationInteractionModel.Reset(); LastStationInteractionSerial = 0; bHasSeenStationInteraction = false;
    LastStationContextSerial = 0; bHasSeenStationContext = false;
    Controller=nullptr; bVerified=false;
}
void UKalmalaCraftingSubsystem::Deinitialize() { Release(); Super::Deinitialize(); }
