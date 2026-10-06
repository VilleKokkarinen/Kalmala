#include "KalmalaCraftingSubsystem.h"
#include "Misc/Paths.h"
#include "KalmalaInventorySubsystem.h"
#include "KalmalaInventoryInspectWidget.h"
#include "KalmalaItemDetailWidget.h"
#include "KalmalaIngredientWidget.h"
#include "KalmalaRecipeRequirements.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaUITheme.h"
#include "KalmalaThemedButton.h"
#include "KalmalaIconWidget.h"
#include "KalmalaSelectedResultWidget.h"
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
#include "KalmalaGeneratedTerrainPatch.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaSkillProgressionComponent.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaToolLifecycleContract.h"
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
#include "GameFramework/InputSettings.h"
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
constexpr int32 MaxFavoriteRecipeCount = 256;
const FString NoPanelImage;

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

EKalmalaCraftingActionKind GetActivityKindForRecipe(const FKalmalaRecipe& Recipe)
{
    if (Recipe.ExperienceSkill == EKalmalaSkill::Cooking) return EKalmalaCraftingActionKind::CookedRecipe;
    if (UKalmalaCraftingWidget::GetBuildBrowseGroup(Recipe.Output) != 0) return EKalmalaCraftingActionKind::BuiltPiece;
    return EKalmalaCraftingActionKind::CraftedItem;
}

const TCHAR* GetReadableSkillName(const EKalmalaSkill Skill)
{
    switch (Skill)
    {
    case EKalmalaSkill::Gathering: return TEXT("Gathering");
    case EKalmalaSkill::Woodcutting: return TEXT("Woodcutting");
    case EKalmalaSkill::Mining: return TEXT("Mining");
    case EKalmalaSkill::Crafting: return TEXT("Crafting");
    case EKalmalaSkill::Cooking: return TEXT("Cooking");
    case EKalmalaSkill::Survival: return TEXT("Survival");
    default: return TEXT("Unknown skill");
    }
}

FString BuildSkillProgressText(const AKalmalaCharacter* Character)
{
    FString Text = TEXT("\nSKILL PROGRESS [PRIVATE TO YOU]\n");
    const UKalmalaSkillProgressionComponent* Progression = Character
        ? Character->GetSkillProgressionComponent() : nullptr;
    if (!Progression)
    {
        return Text + TEXT("Skill progress is not available yet.\n");
    }

    const TArray<EKalmalaSkill> AllowlistedSkills = FKalmalaSkillProgressionContract::GetAllowlistedSkills();
    const TArray<FKalmalaSkillState>& DetailedProgression = Progression->GetDetailedProgression();
    if (DetailedProgression.Num() != AllowlistedSkills.Num())
    {
        return Text + TEXT("Waiting for your complete private skill update.\n");
    }

    const auto FindState = [&DetailedProgression](const EKalmalaSkill Skill) -> const FKalmalaSkillState*
    {
        return DetailedProgression.FindByPredicate([Skill](const FKalmalaSkillState& State)
        {
            return State.Skill == Skill && State.IsValid();
        });
    };

    for (const EKalmalaSkill Skill : AllowlistedSkills)
    {
        const FKalmalaSkillState* State = FindState(Skill);
        if (!State)
        {
            return Text + TEXT("Waiting for your complete private skill update.\n");
        }

        if (State->Level < FKalmalaSkillProgressionContract::MaxLevel)
        {
            const int32 LevelStart = FKalmalaSkillProgressionContract::GetExperienceForLevel(State->Level);
            const int32 NextLevelStart = FKalmalaSkillProgressionContract::GetExperienceForLevel(State->Level + 1);
            const int32 LevelProgress = FMath::Clamp(State->Experience - LevelStart, 0, NextLevelStart - LevelStart);
            Text += FString::Printf(TEXT("%s: Level %d, %d/%d XP to Level %d\n"),
                GetReadableSkillName(Skill), State->Level, LevelProgress,
                NextLevelStart - LevelStart, State->Level + 1);
        }
        else
        {
            Text += FString::Printf(TEXT("%s: Level %d, %d/%d total XP (maximum level)\n"),
                GetReadableSkillName(Skill), State->Level, State->Experience,
                FKalmalaSkillProgressionContract::MaxExperience);
        }
    }

    Text += TEXT("Recipe access depends on materials, stations, and world conditions; skill level does not lock recipes.\n");
    return Text;
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
    const FString& KeyboardBinding, const FString& ControllerBinding, const FString& UnavailableReason,
    const bool bModalOpen)
{
    if (bModalOpen || TargetName.TrimStartAndEnd().IsEmpty() || ActionName.TrimStartAndEnd().IsEmpty()) return FString();

    const bool bKeyboardBound = !KeyboardBinding.TrimStartAndEnd().IsEmpty()
        && KeyboardBinding != TEXT("Not bound") && KeyboardBinding != TEXT("Unbound");
    const bool bControllerBound = !ControllerBinding.TrimStartAndEnd().IsEmpty()
        && ControllerBinding != TEXT("Not bound") && ControllerBinding != TEXT("Unbound");
    const bool bUnavailable = !UnavailableReason.TrimStartAndEnd().IsEmpty() || (!bKeyboardBound && !bControllerBound);

    FString Text = FString::Printf(TEXT("%s\n%s"), *TargetName, *ActionName);
    if (bUnavailable)
    {
        const FString Reason = UnavailableReason.TrimStartAndEnd().IsEmpty()
            ? TEXT("No binding") : UnavailableReason.TrimStartAndEnd();
        Text += FString::Printf(TEXT(" — Unavailable: %s"), *Reason);
    }

    TArray<FString, TInlineAllocator<2>> Bindings;
    if (bKeyboardBound) Bindings.Add(FString::Printf(TEXT("Keyboard: %s"), *KeyboardBinding.TrimStartAndEnd()));
    if (bControllerBound)
    {
        FString DisplayBinding = ControllerBinding.TrimStartAndEnd();
        if (DisplayBinding.StartsWith(TEXT("Gamepad "))) DisplayBinding.RightChopInline(8, EAllowShrinking::No);
        Bindings.Add(FString::Printf(TEXT("Gamepad: %s"), *DisplayBinding));
    }
    if (!Bindings.IsEmpty()) Text += TEXT("\n") + FString::Join(Bindings, TEXT("\n"));
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
    FString CraftKey = TEXT("Unbound");
    for (const FInputActionKeyMapping& Mapping : GetDefault<UInputSettings>()->GetActionMappings())
        if (Mapping.ActionName == TEXT("CraftMenu") && !Mapping.Key.IsGamepadKey()) { CraftKey = Mapping.Key.GetDisplayName().ToString(); break; }
    GeneralInstructions = FString::Printf(TEXT("Construction hammer menu input: %s (Controller View / special-left). Up/Down or D-pad: choose. Enter / A: craft or build. P: local preview. Escape / B: close.\nController Y: build or place selected. X: add fuel. RB: light. Mouse buttons and focused keyboard/controller buttons also work.\nFloor, wall, and roof are built directly from Wood and Fibre; no kit is created. Selection is marked with >. Requirements and unavailable reasons are written in text; colour is never the only cue.\n"), *CraftKey);
    InstructionsText = AddText(GeneralInstructions, 16);
    RecipeGrid = WidgetTree->ConstructWidget<UUniformGridPanel>();
    Column->AddChild(RecipeGrid);
    RecipesText = AddText(TEXT(""), 18);
    SelectedResultPreview = WidgetTree->ConstructWidget<UKalmalaSelectedResultWidget>();
    Column->AddChild(SelectedResultPreview);
    Ingredients = WidgetTree->ConstructWidget<UKalmalaIngredientWidget>();
    Column->AddChild(Ingredients);
    auto AddButton = [&](const TCHAR* Label, UHorizontalBox* Row = nullptr, const TCHAR* Help = nullptr) {
        auto* Button = WidgetTree->ConstructWidget<UKalmalaThemedButton>(); auto* Text = WidgetTree->ConstructWidget<UTextBlock>();
        Text->SetText(FText::FromString(Label)); Text->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(),18));
        Text->SetColorAndOpacity(FSlateColor(FLinearColor::Black)); Button->SetContent(Text);
        Button->SetToolTipText(FText::FromString(Help ? Help : Label));
        if(Row) { auto* Slot=Row->AddChildToHorizontalBox(Button); Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); Slot->SetPadding(FMargin(2,4)); }
        else Column->AddChild(Button); return Button;
    };
    RecipeSearchBox = WidgetTree->ConstructWidget<UEditableTextBox>();
    RecipeSearchBox->SetHintText(FText::FromString(TEXT("Search recipe names")));
    RecipeSearchStyle = FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox");
    RecipeSearchStyle.SetForegroundColor(FSlateColor(FLinearColor::White));
    RecipeSearchStyle.SetBackgroundColor(FSlateColor(FLinearColor(.02f,.025f,.03f,1.f)));
    RecipeSearchBox->SetWidgetStyle(RecipeSearchStyle);
    RecipeSearchBox->OnTextChanged.AddDynamic(this, &ThisClass::RecipeSearchChanged);
    Column->InsertChildAt(2, RecipeSearchBox);
    AddButton(TEXT("Clear recipe search"), nullptr)->OnClicked.AddDynamic(this, &ThisClass::ClearRecipeSearch);
    auto* CategoryButton = AddButton(TEXT("Recipes: All"), nullptr);
    RecipeCategoryLabel = CastChecked<UTextBlock>(CategoryButton->GetContent());
    CategoryButton->OnClicked.AddDynamic(this, &ThisClass::CycleRecipeCategory);
    Column->RemoveChild(CategoryButton); Column->InsertChildAt(3, CategoryButton);
    auto* SortButton = AddButton(TEXT("Recipe order: Catalogue"), nullptr);
    RecipeSortLabel = CastChecked<UTextBlock>(SortButton->GetContent());
    SortButton->OnClicked.AddDynamic(this, &ThisClass::CycleRecipeSort);
    Column->RemoveChild(SortButton); Column->InsertChildAt(4, SortButton);
    FavoriteButton = AddButton(TEXT("Favorite: select a recipe"), nullptr,
        TEXT("Add or remove the selected recipe or build from your local Favorites list."));
    FavoriteActionLabel = CastChecked<UTextBlock>(FavoriteButton->GetContent());
    FavoriteButton->OnClicked.AddDynamic(this, &ThisClass::ToggleSelectedFavorite);
    Column->RemoveChild(FavoriteButton); Column->InsertChildAt(5, FavoriteButton);
    AddText(TEXT("Recipe browsing: Tab reaches search, category, order and Favorite. Page Up/Down cycles category/order while the panel is focused. Controller uses focused buttons. All retains this menu's station scope."), 14);
    auto* InspectButton = AddButton(TEXT("Inspect inventory"), nullptr,
        TEXT("Read your inventory details. Arrows or D-pad select an item; Tab continues to other menu controls."));
    InspectButton->OnClicked.AddDynamic(this, &ThisClass::FocusInventoryDetails);
    auto* RecipeActions=WidgetTree->ConstructWidget<UHorizontalBox>(); Column->AddChild(RecipeActions);
    AddButton(TEXT("Previous"),RecipeActions,TEXT("Select the previous recipe. Its ingredients, station, unlock, batch limit, and availability are shown above."))->OnClicked.AddDynamic(this, &ThisClass::Previous);
    AddButton(TEXT("Next"),RecipeActions,TEXT("Select the next recipe. Its ingredients, station, unlock, batch limit, and availability are shown above."))->OnClicked.AddDynamic(this, &ThisClass::Next);
    CraftButton = AddButton(TEXT("Craft one"),RecipeActions,TEXT("Craft batch 1 of the selected recipe. The server checks every requirement and rejected requests preserve ingredients."));
    CraftButton->OnClicked.AddDynamic(this, &ThisClass::Craft);
    AddButton(TEXT("Preview placement"),RecipeActions,TEXT("Show a local placement preview for the selected buildable. This does not place it or spend ingredients."))->OnClicked.AddDynamic(this, &ThisClass::Preview);
    AddText(TEXT("\nBuild/place selected uses the derived ground ahead. The server checks hammer, raw materials, terrain, and placement before committing. Camp structures are built directly from their listed raw materials.\n"),16);
    auto* FireActions=WidgetTree->ConstructWidget<UHorizontalBox>(); Column->AddChild(FireActions);
    AddButton(TEXT("Build / place selected"),FireActions,TEXT("Ask the server to build the selected structure directly from its listed raw materials with the Construction Hammer. The server validates placement and costs."))->OnClicked.AddDynamic(this, &ThisClass::Place);
    AddButton(TEXT("Add raw fuel"),FireActions,TEXT("Add one Wood, Lightwood, Densewood, or Coal to a nearby usable hearth if the server confirms access and capacity."))->OnClicked.AddDynamic(this, &ThisClass::Refuel);
    AddButton(TEXT("Light hearth"),FireActions,TEXT("Light a nearby usable hearth. The server checks access, dry fuel, and fire state."))->OnClicked.AddDynamic(this, &ThisClass::Light);
    StateText = AddText(TEXT(""), 18);
    FoodText = AddText(TEXT(""), 18);
    AddButton(TEXT("Eat one roasted field meat"),nullptr,TEXT("Consume one roasted field meat for the steady meal effect. Another meal cannot replace an active effect."))->OnClicked.AddDynamic(this, &ThisClass::EatFood);
    AddButton(TEXT("Eat one hearth broth"),nullptr,TEXT("Consume one hearth broth for the steady meal effect. Another meal cannot replace an active effect."))->OnClicked.AddDynamic(this, &ThisClass::EatBroth);
    AddButton(TEXT("Eat one smoked field meat"),nullptr,TEXT("Consume one smoked field meat for the steady meal effect. Another meal cannot replace an active effect."))->OnClicked.AddDynamic(this, &ThisClass::EatSmokedMeat);
    RepairText = AddText(TEXT("\nFree repair: at a visible same-world Workbench or Forge within 2.5 m, select a damaged or broken carried tool to restore it to full condition. Repair uses no materials and awards no Crafting experience; rejected requests leave tool condition unchanged.\nGrinding Stone Repair All: interact with a visible same-world Grinding Stone within 2.5 m to repair every damaged or broken carried tool. The server selects your tools; no materials or Crafting experience are used.\n"), 16);
    auto* RepairActions = WidgetTree->ConstructWidget<UHorizontalBox>(); Column->AddChild(RepairActions);
    AddButton(TEXT("Repair Reed Knife"), RepairActions)->OnClicked.AddDynamic(this, &ThisClass::RepairReedKnife);
    AddButton(TEXT("Repair Field Hatchet"), RepairActions)->OnClicked.AddDynamic(this, &ThisClass::RepairFieldHatchet);
    AddButton(TEXT("Repair Stone Pick"), RepairActions)->OnClicked.AddDynamic(this, &ThisClass::RepairStonePick);
    auto* AxeRepairActions = WidgetTree->ConstructWidget<UHorizontalBox>(); Column->AddChild(AxeRepairActions);
    AddButton(TEXT("Repair Bronze Axe"), AxeRepairActions)->OnClicked.AddDynamic(this, &ThisClass::RepairBronzeAxe);
    AddButton(TEXT("Repair Iron Axe"), AxeRepairActions)->OnClicked.AddDynamic(this, &ThisClass::RepairIronAxe);
    ToolProgressionText = AddText(TEXT(""), 18);
    auto* ToolProgressionActions = WidgetTree->ConstructWidget<UHorizontalBox>(); Column->AddChild(ToolProgressionActions);
    AddButton(TEXT("Craft Bronze Axe"), ToolProgressionActions,
        TEXT("Ask the server to craft the level-one Bronze Axe at a visible same-world level-one Workbench. The server checks materials and private tool inventory."))->OnClicked.AddDynamic(this, &ThisClass::CraftBronzeAxe);
    AddButton(TEXT("Upgrade to Iron Axe"), ToolProgressionActions,
        TEXT("Ask the server to exchange a carried level-one Bronze Axe for a level-two Iron Axe at a visible same-world level-two Forge. The server checks every material and condition."))->OnClicked.AddDynamic(this, &ThisClass::UpgradeIronAxe);
    AddText(TEXT("\nWoven chest — shared nearby storage\nInspect a visible chest, choose an item, then store or take one. Contents clear when closed or out of reach."), 16);
    AddText(TEXT("Chest contents use the shared 16-stack interface. Accepted construction and storage records are saved for this world; rejected transfers leave both inventories unchanged."), 16);
    StorageText = AddText(TEXT(""), 18);
    AddButton(TEXT("Inspect nearby chest"),nullptr,TEXT("Open the owner-only view of a visible nearby chest. The view closes when the chest is closed or out of reach."))->OnClicked.AddDynamic(this, &ThisClass::InspectStorage);
    auto* StorageActions = WidgetTree->ConstructWidget<UHorizontalBox>(); Column->AddChild(StorageActions);
    AddButton(TEXT("Previous item"), StorageActions)->OnClicked.AddDynamic(this, &ThisClass::PreviousStorageItem);
    AddButton(TEXT("Next item"), StorageActions)->OnClicked.AddDynamic(this, &ThisClass::NextStorageItem);
    AddButton(TEXT("Store one"), StorageActions,TEXT("Ask the server to move one selected item from your pack into the nearby chest."))->OnClicked.AddDynamic(this, &ThisClass::DepositStorage);
    AddButton(TEXT("Take one"), StorageActions,TEXT("Ask the server to move one selected item from the nearby chest into your pack."))->OnClicked.AddDynamic(this, &ThisClass::WithdrawStorage);
    InventoryInspector = WidgetTree->ConstructWidget<UKalmalaInventoryInspectWidget>();
    Column->AddChild(InventoryInspector);
    auto* CloseButton=AddButton(TEXT("Close")); CloseButton->OnClicked.AddDynamic(this, &ThisClass::CloseClicked);
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
    case 7: return TEXT("Favorites");
    default: return TEXT("All");
    }
}

UKalmalaCraftingSubsystem* UKalmalaCraftingWidget::GetLocalCraftingSubsystem() const
{
    APlayerController* LocalController = GetOwningPlayer();
    ULocalPlayer* LocalPlayer = LocalController ? LocalController->GetLocalPlayer() : nullptr;
    return LocalPlayer ? LocalPlayer->GetSubsystem<UKalmalaCraftingSubsystem>() : nullptr;
}

bool UKalmalaCraftingWidget::IsRecipeFavorite(const FName RecipeId) const
{
    const UKalmalaCraftingSubsystem* FavoriteState = GetLocalCraftingSubsystem();
    return FavoriteState && FavoriteState->IsRecipeFavorite(RecipeId);
}

bool UKalmalaCraftingWidget::IsRecipeRecent(const FName RecipeId) const
{
    const UKalmalaCraftingSubsystem* ActivityState = GetLocalCraftingSubsystem();
    if (!ActivityState || RecipeId.IsNone()) return false;
    return ActivityState->GetRecentRecipeActivity(EKalmalaCraftingActionKind::BuiltPiece) == RecipeId
        || ActivityState->GetRecentRecipeActivity(EKalmalaCraftingActionKind::CookedRecipe) == RecipeId
        || ActivityState->GetRecentRecipeActivity(EKalmalaCraftingActionKind::CraftedItem) == RecipeId;
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
            || (RecipeCategory == 7 && (IsRecipeFavorite(Recipe.RecipeId) || IsRecipeRecent(Recipe.RecipeId)))
            || (RecipeCategory >= 4 && RecipeCategory <= 6 && BuildGroup == RecipeCategory);
        if ((StationFilterKit.IsNone() || Recipe.RequiredStation.Contains(StationFilterKit))
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
    RecipeCategory = FMath::Clamp(Category, 0, 7); bRecipeNameSort = bNameSort;
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
void UKalmalaCraftingWidget::CycleRecipeCategory() { SetRecipeBrowse(RecipeQuery, (RecipeCategory + 1) % 8, bRecipeNameSort); }
void UKalmalaCraftingWidget::CycleRecipeSort() { SetRecipeBrowse(RecipeQuery, RecipeCategory, !bRecipeNameSort); }
void UKalmalaCraftingWidget::ClearRecipeSearch() { RecipeSearchBox->SetText(FText::GetEmpty()); SetRecipeBrowse(TEXT(""), RecipeCategory, bRecipeNameSort); }

void UKalmalaCraftingWidget::ToggleSelectedFavorite()
{
    const TArray<int32> VisibleIndices = GetVisibleRecipeIndices();
    const auto& Recipes = UKalmalaRecipeCatalogue::Get()->Recipes;
    if (!VisibleIndices.IsValidIndex(Selected) || !Recipes.IsValidIndex(VisibleIndices[Selected])) return;
    UKalmalaCraftingSubsystem* FavoriteState = GetLocalCraftingSubsystem();
    if (!FavoriteState) return;
    const FName RecipeId = Recipes[VisibleIndices[Selected]].RecipeId;
    if (!FavoriteState->SetRecipeFavorite(RecipeId, !FavoriteState->IsRecipeFavorite(RecipeId))) return;
    SetRecipeBrowse(RecipeQuery, RecipeCategory, bRecipeNameSort);
}

void UKalmalaCraftingWidget::Open() { OpenInternal(NAME_None); }

void UKalmalaCraftingWidget::OpenForStation(const FName StationKit) { OpenInternal(StationKit); }

void UKalmalaCraftingWidget::UpdateMenuHeader(const FName StationKit)
{
    if (StationKit.IsNone())
    {
        if (HeaderText) HeaderText->SetText(FText::FromString(TEXT("Construction hammer — Build and craft")));
        if (InstructionsText) InstructionsText->SetText(FText::FromString(GeneralInstructions));
        return;
    }

    const FKalmalaItemDefinition* StationItem = UKalmalaItemCatalogue::Get()->FindItem(StationKit);
    const FString StationName = StationItem ? StationItem->DisplayName : StationKit.ToString();
    if (HeaderText) HeaderText->SetText(FText::FromString(StationName + TEXT(" — Cook")));
    if (InstructionsText) InstructionsText->SetText(FText::FromString(
        TEXT("Station recipes. Up/Down or D-pad: choose. Enter / A: cook one. Escape / B: close.\n")
        TEXT("The server requires this placed station and a usable, lit hearth with heat at both the station and you. Ingredients and availability are shown in text.")));
}

void UKalmalaCraftingWidget::RememberMenuBrowseState()
{
    FKalmalaMenuBrowseMemory& Memory = MenuBrowseMemory.FindOrAdd(StationFilterKit);
    Memory.Query = RecipeQuery;
    Memory.Category = RecipeCategory;
    Memory.bNameSort = bRecipeNameSort;
    const float CurrentScrollOffset = CraftingScrollBox ? CraftingScrollBox->GetScrollOffset() : 0.0f;
    Memory.ScrollOffset = FMath::IsFinite(CurrentScrollOffset) ? FMath::Max(0.0f, CurrentScrollOffset) : 0.0f;

    const TArray<int32> VisibleIndices = GetVisibleRecipeIndices();
    const auto& Recipes = UKalmalaRecipeCatalogue::Get()->Recipes;
    Memory.SelectedRecipeId = VisibleIndices.IsValidIndex(Selected) && Recipes.IsValidIndex(VisibleIndices[Selected])
        ? Recipes[VisibleIndices[Selected]].RecipeId : NAME_None;
}

bool UKalmalaCraftingWidget::RestoreMenuBrowseState(const FName StationKit)
{
    StationFilterKit = StationKit;
    Selected = 0;
    const FKalmalaMenuBrowseMemory* Memory = MenuBrowseMemory.Find(StationKit);
    if (!Memory)
    {
        RecipeQuery.Reset();
        RecipeCategory = 0;
        bRecipeNameSort = false;
        SetRecipeBrowse(RecipeQuery, RecipeCategory, bRecipeNameSort);
        if (StationKit.IsNone())
        {
            const auto& Recipes = UKalmalaRecipeCatalogue::Get()->Recipes;
            const int32 FirstBuild = Recipes.IndexOfByPredicate([](const FKalmalaRecipe& Recipe)
            {
                return UKalmalaRecipeCatalogue::IsDirectMaterialBuildable(Recipe.Output);
            });
            if (FirstBuild != INDEX_NONE)
            {
                Selected = GetVisibleRecipeIndices().IndexOfByKey(FirstBuild);
                if (Selected == INDEX_NONE) Selected = 0;
            }
        }
        if (RecipeSearchBox && RecipeSearchBox->GetText().ToString() != RecipeQuery)
            RecipeSearchBox->SetText(FText::GetEmpty());
        bPendingMenuScrollRestore = true;
        PendingMenuScrollRestoreOffset = 0.0f;
        return false;
    }

    RecipeQuery = Memory->Query.Left(64).TrimStartAndEnd();
    RecipeCategory = FMath::Clamp(Memory->Category, 0, 7);
    bRecipeNameSort = Memory->bNameSort;
    SetRecipeBrowse(RecipeQuery, RecipeCategory, bRecipeNameSort);
    if (RecipeSearchBox && RecipeSearchBox->GetText().ToString() != RecipeQuery)
        RecipeSearchBox->SetText(FText::FromString(RecipeQuery));

    const TArray<int32> VisibleIndices = GetVisibleRecipeIndices();
    const auto& Recipes = UKalmalaRecipeCatalogue::Get()->Recipes;
    Selected = INDEX_NONE;
    if (!Memory->SelectedRecipeId.IsNone())
    {
        for (int32 VisibleIndex = 0; VisibleIndex < VisibleIndices.Num(); ++VisibleIndex)
        {
            const int32 RecipeIndex = VisibleIndices[VisibleIndex];
            if (Recipes.IsValidIndex(RecipeIndex) && Recipes[RecipeIndex].RecipeId == Memory->SelectedRecipeId)
            {
                Selected = VisibleIndex;
                break;
            }
        }
    }
    if (Selected == INDEX_NONE) Selected = 0;

    bPendingMenuScrollRestore = true;
    PendingMenuScrollRestoreOffset = FMath::IsFinite(Memory->ScrollOffset)
        ? FMath::Max(0.0f, Memory->ScrollOffset) : 0.0f;
    bPlacementPreviewEnabled = false;
    return true;
}

void UKalmalaCraftingWidget::OpenInternal(const FName StationKit)
{
    if (!StationKit.IsNone() && !IsInWorldCookingStation(StationKit)) return;
    if (UKalmalaCraftingSubsystem* FavoriteState = GetLocalCraftingSubsystem())
    {
        FavoriteState->PruneRecipeFavorites();
        FavoriteState->PruneRecipeActivity();
    }
    if (bOpen)
    {
        RememberMenuBrowseState();
        RestoreMenuBrowseState(StationKit);
        UpdateMenuHeader(StationKit);
        bPlacementPreviewEnabled = false;
        Refresh();
        return;
    }
    auto* PC = GetOwningPlayer(); if (!PC || PC->IsMoveInputIgnored() || !Model()) return;
    const auto* Character = Cast<AKalmalaCharacter>(PC->GetPawn());
    if (!Character || (StationKit.IsNone() && Character->GetCarriedToolLevel(TEXT("ConstructionHammer")) < 1)) return;
    RestoreMenuBrowseState(StationKit);
    UpdateMenuHeader(StationKit);
    bOpen = true; bPreviousCursor = PC->bShowMouseCursor;
    if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
        if (auto* InventoryHUD = LocalPlayer->GetSubsystem<UKalmalaInventorySubsystem>())
            InventoryHUD->SetCraftingMenuSuppressed(true);
    int32 X, Y; PC->GetViewportSize(X,Y);
    const float Scale = FMath::Max(.1f, UWidgetLayoutLibrary::GetViewportScale(this));
    const float PanelWidth = FMath::Min(840.0f, X / Scale - 32.0f);
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
    SetAlignmentInViewport(FVector2D(.5,.5)); SetPositionInViewport(FVector2D(X*.5f,Y*.5f), true);
    SetVisibility(ESlateVisibility::Visible); Refresh();
    PC->SetIgnoreMoveInput(true); PC->SetIgnoreLookInput(true); PC->bShowMouseCursor = true;
    FInputModeGameAndUI Mode; Mode.SetWidgetToFocus(TakeWidget()); Mode.SetHideCursorDuringCapture(false); PC->SetInputMode(Mode);
    SetKeyboardFocus();
}

void UKalmalaCraftingWidget::Close()
{
    if (!bOpen) return;
    RememberMenuBrowseState();
    bOpen = false; bPlacementPreviewEnabled = false; SetVisibility(ESlateVisibility::Collapsed);
    if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
        if (auto* InventoryHUD = LocalPlayer->GetSubsystem<UKalmalaInventorySubsystem>())
            InventoryHUD->SetCraftingMenuSuppressed(false);
    if (auto* M = Model()) M->ServerCloseStorage();
    if (auto* PC=GetOwningPlayer()) { PC->SetIgnoreMoveInput(false); PC->SetIgnoreLookInput(false); PC->bShowMouseCursor=bPreviousCursor; PC->SetInputMode(FInputModeGameOnly()); }
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
        RecipeSlotFavoriteFrames.Reset();
        RecipeSlotNames.Reset();
        RecipeSlotStates.Reset();
        RecipeSlotFavoriteMarkers.Reset();
        RecipeSlotRankMarkers.Reset();
        RecipeSlotRecentMarkers.Reset();
        RecipeSlotRecentBadgeFrames.Reset();
        RecipeSlotVisualStates.Reset();

        for (int32 SlotIndex = 0; SlotIndex < VisibleIndices.Num(); ++SlotIndex)
        {
            const int32 RecipeIndex = VisibleIndices[SlotIndex];
            if (!Recipes.IsValidIndex(RecipeIndex)) continue;
            const FKalmalaRecipe& Recipe = Recipes[RecipeIndex];

            UBorder* FavoriteFrame = WidgetTree->ConstructWidget<UBorder>();
            FavoriteFrame->SetPadding(FMargin(FKalmalaUITheme::Get().FavoriteMarkerBorderWidth));
            FavoriteFrame->SetBrushColor(FLinearColor::Transparent);
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
            const auto AddMarker = [this, CardContent]()
            {
                UTextBlock* Marker = WidgetTree->ConstructWidget<UTextBlock>();
                Marker->SetAutoWrapText(true);
                Marker->SetJustification(ETextJustify::Right);
                Marker->SetVisibility(ESlateVisibility::Collapsed);
                UVerticalBoxSlot* MarkerSlot = CardContent->AddChildToVerticalBox(Marker);
                MarkerSlot->SetHorizontalAlignment(HAlign_Fill);
                MarkerSlot->SetPadding(FMargin(0.0f, FKalmalaUITheme::Get().SlotPadding, 0.0f, 0.0f));
                return Marker;
            };
            UTextBlock* RankMarker = AddMarker();
            UBorder* RecentBadge = WidgetTree->ConstructWidget<UBorder>();
            RecentBadge->SetPadding(FMargin(4.0f, 2.0f));
            RecentBadge->SetVisibility(ESlateVisibility::Collapsed);
            UTextBlock* RecentMarker = WidgetTree->ConstructWidget<UTextBlock>();
            RecentMarker->SetJustification(ETextJustify::Right);
            RecentMarker->SetVisibility(ESlateVisibility::Collapsed);
            RecentBadge->SetContent(RecentMarker);
            UVerticalBoxSlot* RecentSlot = CardContent->AddChildToVerticalBox(RecentBadge);
            RecentSlot->SetHorizontalAlignment(HAlign_Right);
            RecentSlot->SetPadding(FMargin(0.0f, FKalmalaUITheme::Get().SlotPadding, 0.0f, 0.0f));
            UTextBlock* FavoriteMarker = AddMarker();
            CardSize->SetContent(CardContent);
            Card->SetContent(CardSize);
            FavoriteFrame->SetContent(Card);
            UBorder* CellMargin = WidgetTree->ConstructWidget<UBorder>();
            CellMargin->SetBrushColor(FLinearColor::Transparent);
            CellMargin->SetPadding(FMargin(3.0f));
            CellMargin->SetContent(FavoriteFrame);
            UUniformGridSlot* GridPanelSlot = RecipeGrid->AddChildToUniformGrid(
                CellMargin, SlotIndex / RecipeGridColumns, SlotIndex % RecipeGridColumns);
            GridPanelSlot->SetHorizontalAlignment(HAlign_Fill);
            GridPanelSlot->SetVerticalAlignment(VAlign_Fill);

            RecipeSlotCards.Add(Card);
            RecipeSlotFavoriteFrames.Add(FavoriteFrame);
            RecipeSlotNames.Add(Name);
            RecipeSlotStates.Add(State);
            RecipeSlotFavoriteMarkers.Add(FavoriteMarker);
            RecipeSlotRankMarkers.Add(RankMarker);
            RecipeSlotRecentMarkers.Add(RecentMarker);
            RecipeSlotRecentBadgeFrames.Add(RecentBadge);
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
    const UKalmalaCraftingSubsystem* ActivityState = GetLocalCraftingSubsystem();
    TMap<FName, int32> ActivityRanks;
    if (ActivityState)
    {
        const EKalmalaCraftingActionKind Kinds[] = { EKalmalaCraftingActionKind::BuiltPiece,
            EKalmalaCraftingActionKind::CookedRecipe, EKalmalaCraftingActionKind::CraftedItem };
        for (const EKalmalaCraftingActionKind Kind : Kinds)
        {
            const TMap<FName, int32> KindRanks = ActivityState->GetRecipeActivityRanks(Kind);
            for (const TPair<FName, int32>& Rank : KindRanks) ActivityRanks.Add(Rank.Key, Rank.Value);
        }
    }

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
        const bool bFavorite = IsRecipeFavorite(Recipe.RecipeId);
        const EKalmalaCraftingActionKind ActivityKind = GetActivityKindForRecipe(Recipe);
        const int32 ActivityRank = ActivityRanks.FindRef(Recipe.RecipeId);
        const bool bRecent = ActivityState
            && ActivityState->GetRecentRecipeActivity(ActivityKind) == Recipe.RecipeId;
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
        const FString FavoriteMarkerText = bFavorite
            ? (Theme.UsesFavoriteMarkerStar() ? TEXT("★ Favorite") : TEXT("Favorite")) : TEXT("");
        static const TCHAR* RankNames[] = { TEXT("Gold"), TEXT("Silver"), TEXT("Bronze") };
        const FString RankMarkerText = ActivityRank >= 1 && ActivityRank <= 3
            ? FString::Printf(TEXT("● %s Rank %d"), RankNames[ActivityRank - 1], ActivityRank) : TEXT("");
        const FString RecentMarkerText = bRecent ? TEXT("◷ Recent") : TEXT("");
        const auto UpdateMarker = [&Theme, TextScalePercent, ContrastMode, bRestyle](
            UTextBlock* Marker, const FString& Label, const FLinearColor& Color)
        {
            if (!Marker) return;
            const bool bTextChanged = Marker->GetText().ToString() != Label;
            if (bTextChanged) Marker->SetText(FText::FromString(Label));
            const ESlateVisibility WantedVisibility = Label.IsEmpty()
                ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible;
            if (Marker->GetVisibility() != WantedVisibility) Marker->SetVisibility(WantedVisibility);
            if (bRestyle || bTextChanged) Theme.ApplyText(*Marker, 9, false, TextScalePercent, ContrastMode);
            if (!Marker->GetColorAndOpacity().GetSpecifiedColor().Equals(Color))
                Marker->SetColorAndOpacity(FSlateColor(Color));
        };
        const FLinearColor FavoriteColor = ContrastMode == 0 ? Theme.FavoriteMarkerColor : FLinearColor::White;
        const FLinearColor RankColor = Theme.RankMarkerColor(ActivityRank, ContrastMode);
        const FLinearColor RecentColor = ContrastMode == 0 ? Theme.RecentMarkerColor : FLinearColor::White;
        UpdateMarker(RecipeSlotFavoriteMarkers.IsValidIndex(SlotIndex) ? RecipeSlotFavoriteMarkers[SlotIndex] : nullptr,
            FavoriteMarkerText, FavoriteColor);
        UpdateMarker(RecipeSlotRankMarkers.IsValidIndex(SlotIndex) ? RecipeSlotRankMarkers[SlotIndex] : nullptr,
            RankMarkerText, RankColor);
        UpdateMarker(RecipeSlotRecentMarkers.IsValidIndex(SlotIndex) ? RecipeSlotRecentMarkers[SlotIndex] : nullptr,
            RecentMarkerText, RecentColor);
        if (RecipeSlotRecentBadgeFrames.IsValidIndex(SlotIndex) && RecipeSlotRecentBadgeFrames[SlotIndex])
        {
            UBorder* RecentBadge = RecipeSlotRecentBadgeFrames[SlotIndex];
            const ESlateVisibility BadgeVisibility = bRecent
                ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
            if (RecentBadge->GetVisibility() != BadgeVisibility) RecentBadge->SetVisibility(BadgeVisibility);
            const FLinearColor BadgeColor = ContrastMode == 0
                ? FLinearColor(RecentColor.R, RecentColor.G, RecentColor.B, 0.16f)
                : FLinearColor(0.12f, 0.12f, 0.12f, 1.0f);
            if (!RecentBadge->GetBrushColor().Equals(BadgeColor)) RecentBadge->SetBrushColor(BadgeColor);
        }
        if (RecipeSlotFavoriteFrames.IsValidIndex(SlotIndex) && RecipeSlotFavoriteFrames[SlotIndex])
        {
            const bool bShowFavoriteBorder = bFavorite && Theme.UsesFavoriteMarkerBorder();
            const FLinearColor FrameColor = bShowFavoriteBorder ? FavoriteColor : FLinearColor::Transparent;
            if (!RecipeSlotFavoriteFrames[SlotIndex]->GetBrushColor().Equals(FrameColor))
                RecipeSlotFavoriteFrames[SlotIndex]->SetBrushColor(FrameColor);
        }
        FString RecipeTooltip = bUnavailable ? Reason : Recipe.DisplayName + TEXT(" — Available");
        if (bFavorite) RecipeTooltip = TEXT("Favorite. ") + RecipeTooltip;
        if (ActivityRank > 0) RecipeTooltip = FString::Printf(TEXT("Rank %d. "), ActivityRank) + RecipeTooltip;
        if (bRecent) RecipeTooltip = TEXT("Recent. ") + RecipeTooltip;
        const FText ToolTip = FText::FromString(RecipeTooltip);
        if (RecipeSlotCards[SlotIndex]->GetToolTipText().ToString() != ToolTip.ToString())
        {
            RecipeSlotCards[SlotIndex]->SetToolTipText(ToolTip);
        }

        const uint8 VisualState = static_cast<uint8>((bSelected ? 1 : 0) | (bFocused ? 2 : 0)
            | (bFavorite ? 16 : 0)
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
bool UKalmalaCraftingWidget::VerifyMenuBrowseMemoryForTest()
{
    const auto& Recipes = UKalmalaRecipeCatalogue::Get()->Recipes;
    FName CookingStation = NAME_None;
    for (const FKalmalaRecipe& Recipe : Recipes)
    {
        if (Recipe.ExperienceSkill == EKalmalaSkill::Cooking && !Recipe.RequiredStation.IsEmpty())
        {
            CookingStation = Recipe.RequiredStation[0];
            break;
        }
    }
    if (CookingStation.IsNone()) return false;

    StationFilterKit = NAME_None;
    Selected = 0;
    SetRecipeBrowse(TEXT(""), 3, true);
    const TArray<int32> BuildIndices = GetVisibleRecipeIndices();
    if (BuildIndices.Num() < 2) return false;
    Selected = 1;
    const FName BuildId = Recipes[BuildIndices[Selected]].RecipeId;
    RememberMenuBrowseState();
    if (FKalmalaMenuBrowseMemory* MainMemory = MenuBrowseMemory.Find(NAME_None))
        MainMemory->ScrollOffset = 37.0f;

    StationFilterKit = CookingStation;
    Selected = 0;
    SetRecipeBrowse(TEXT(""), 2, false);
    const TArray<int32> StationIndices = GetVisibleRecipeIndices();
    if (StationIndices.IsEmpty()) return false;
    Selected = StationIndices.Num() - 1;
    const FName StationRecipeId = Recipes[StationIndices[Selected]].RecipeId;
    RememberMenuBrowseState();
    if (FKalmalaMenuBrowseMemory* StationMemory = MenuBrowseMemory.Find(CookingStation))
        StationMemory->ScrollOffset = 81.0f;

    const bool bMainRestored = RestoreMenuBrowseState(NAME_None)
        && StationFilterKit.IsNone() && RecipeCategory == 3 && bRecipeNameSort
        && GetVisibleRecipeIndices().IsValidIndex(Selected)
        && Recipes[GetVisibleRecipeIndices()[Selected]].RecipeId == BuildId;
    const TArray<int32> RestoredBuildIndices = GetVisibleRecipeIndices();
    const bool bMainScrollQueued = bPendingMenuScrollRestore
        && FMath::IsNearlyEqual(PendingMenuScrollRestoreOffset, 37.0f);

    const bool bStationRestored = RestoreMenuBrowseState(CookingStation)
        && StationFilterKit == CookingStation && RecipeCategory == 2 && !bRecipeNameSort
        && GetVisibleRecipeIndices().IsValidIndex(Selected)
        && Recipes[GetVisibleRecipeIndices()[Selected]].RecipeId == StationRecipeId;
    const bool bStationScrollQueued = bPendingMenuScrollRestore
        && FMath::IsNearlyEqual(PendingMenuScrollRestoreOffset, 81.0f);

    if (FKalmalaMenuBrowseMemory* MainMemory = MenuBrowseMemory.Find(NAME_None))
        MainMemory->SelectedRecipeId = TEXT("RemovedRecipeMemoryFixture");
    const bool bMissingSelectionFallback = RestoreMenuBrowseState(NAME_None)
        && !GetVisibleRecipeIndices().IsEmpty() && Selected == 0;
    const bool bSeparateMenuRecords = MenuBrowseMemory.Contains(NAME_None)
        && MenuBrowseMemory.Contains(CookingStation)
        && RestoredBuildIndices.Num() == BuildIndices.Num();

    StationFilterKit = NAME_None;
    SetRecipeBrowse(TEXT(""), 0, false);
    UE_LOG(LogTemp, Display, TEXT("Menu memory: MainRestored=%d StationRestored=%d MissingSelectionFallback=%d MainScroll=%d StationScroll=%d SeparateMenus=%d"),
        bMainRestored, bStationRestored, bMissingSelectionFallback, bMainScrollQueued,
        bStationScrollQueued, bSeparateMenuRecords);
    return bMainRestored && bStationRestored && bMissingSelectionFallback
        && bMainScrollQueued && bStationScrollQueued && bSeparateMenuRecords;
}

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
    if (!bOpen || !SelectedResultPreview || RecipeSlotCards.IsEmpty() || RecipeGridSelectedIndex < 0 || !bRecipeGridFocused
        || RecipeGridUnavailableCount < 1) return false;
    const auto& Recipes = UKalmalaRecipeCatalogue::Get()->Recipes;
    const int32 InitialActorCount = GetWorld() ? GetWorld()->GetActorCount() : INDEX_NONE;
    TArray<FName> OriginalOutputs;
    OriginalOutputs.Reserve(Recipes.Num());
    for (const FKalmalaRecipe& Recipe : Recipes) OriginalOutputs.Add(Recipe.Output);
    const auto IsPreviewForSelection = [this, &Recipes]()
    {
        const TArray<int32> Indices = GetVisibleRecipeIndices();
        if (!SelectedResultPreview || !Indices.IsValidIndex(Selected) || !Recipes.IsValidIndex(Indices[Selected])) return false;
        const FKalmalaRecipe& Recipe = Recipes[Indices[Selected]];
        EKalmalaIcon ExpectedIcon = EKalmalaIcon::Unknown;
        int32 ExpectedVariant = 0;
        return UKalmalaSelectedResultWidget::FindCanonicalIcon(Recipe.Output, ExpectedIcon, ExpectedVariant)
            && SelectedResultPreview->IsShowingResult()
            && SelectedResultPreview->GetOutputId() == Recipe.Output
            && SelectedResultPreview->HasCanonicalIcon();
    };
    const int32 InitialSelection = Selected;
    const FModifierKeysState NoModifiers;
    const FKeyEvent KeyboardDown(EKeys::Down, NoModifiers, 0, false, 0, 0);
    const FReply KeyboardReply = NativeOnPreviewKeyDown(FGeometry(), KeyboardDown);
    const bool bKeyboardAdvanced = KeyboardReply.IsEventHandled() && Selected != InitialSelection
        && RecipeGridSelectedIndex == Selected;
    const bool bKeyboardPreviewAdvanced = bKeyboardAdvanced && IsPreviewForSelection();
    const FKeyEvent KeyboardUp(EKeys::Up, NoModifiers, 0, false, 0, 0);
    const FReply KeyboardUpReply = NativeOnPreviewKeyDown(FGeometry(), KeyboardUp);
    const bool bKeyboardRestored = KeyboardUpReply.IsEventHandled() && Selected == InitialSelection
        && RecipeGridSelectedIndex == InitialSelection;
    const bool bKeyboardPreviewRestored = bKeyboardRestored && IsPreviewForSelection();
    const FKeyEvent ControllerDown(EKeys::Gamepad_DPad_Down, NoModifiers, 0, false, 0, 0);
    const FReply ControllerDownReply = NativeOnPreviewKeyDown(FGeometry(), ControllerDown);
    const bool bControllerAdvanced = ControllerDownReply.IsEventHandled() && Selected != InitialSelection
        && RecipeGridSelectedIndex == Selected;
    const bool bControllerPreviewAdvanced = bControllerAdvanced && IsPreviewForSelection();
    const FKeyEvent ControllerUp(EKeys::Gamepad_DPad_Up, NoModifiers, 0, false, 0, 0);
    const FReply ControllerUpReply = NativeOnPreviewKeyDown(FGeometry(), ControllerUp);
    const bool bControllerRestored = ControllerUpReply.IsEventHandled() && Selected == InitialSelection
        && RecipeGridSelectedIndex == InitialSelection;
    const bool bControllerPreviewRestored = bControllerRestored && IsPreviewForSelection();
    const auto OriginalIndices = GetVisibleRecipeIndices();
    const int32 OriginalRecipe = OriginalIndices.IsValidIndex(InitialSelection) ? OriginalIndices[InitialSelection] : INDEX_NONE;

    UKalmalaCraftingComponent* const CraftingModel = Model();
    int32 UnavailableSelection = INDEX_NONE;
    FString UnavailableReason;
    if (CraftingModel)
    {
        for (int32 VisibleIndex = 0; VisibleIndex < OriginalIndices.Num(); ++VisibleIndex)
        {
            const int32 RecipeIndex = OriginalIndices[VisibleIndex];
            if (!Recipes.IsValidIndex(RecipeIndex)) continue;
            const FString Reason = CraftingModel->GetRecipeAvailability(Recipes[RecipeIndex].RecipeId);
            if (!Reason.IsEmpty())
            {
                UnavailableSelection = VisibleIndex;
                UnavailableReason = Reason;
                break;
            }
        }
    }
    bool bUnavailablePreview = false;
    if (OriginalIndices.IsValidIndex(UnavailableSelection))
    {
        Selected = UnavailableSelection;
        Refresh();
        bUnavailablePreview = IsPreviewForSelection()
            && RecipeSlotStates.IsValidIndex(UnavailableSelection)
            && RecipeSlotStates[UnavailableSelection]->GetText().ToString().Contains(TEXT("UNAVAILABLE"))
            && SelectedResultPreview->GetPresentationText().Contains(UnavailableReason);
        Selected = InitialSelection;
        Refresh();
    }
    SelectedResultPreview->SetResult(TEXT("KalmalaMissingPreviewFixture"), TEXT("Unmapped output"),
        TEXT("Description fixture"), TEXT("Requirements fixture"),
        UKalmalaSettingsWidget::GetTextScalePercent(), UKalmalaSettingsWidget::GetContrastMode());
    const bool bMissingIconFallback = SelectedResultPreview->IsShowingResult()
        && !SelectedResultPreview->HasCanonicalIcon()
        && SelectedResultPreview->GetPresentationText().Contains(TEXT("Preview icon unavailable"));
    Refresh();
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
        && !CraftButton->GetIsEnabled() && !SelectedResultPreview->IsShowingResult()
        && SelectedResultPreview->GetOutputId().IsNone()
        && SelectedResultPreview->GetPresentationText().IsEmpty()
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
    TArray<FName> CurrentOutputs;
    CurrentOutputs.Reserve(Recipes.Num());
    for (const FKalmalaRecipe& Recipe : Recipes) CurrentOutputs.Add(Recipe.Output);
    const bool bCatalogueStable = OriginalOutputs == CurrentOutputs;
    const bool bNoActorsSpawned = GetWorld() && GetWorld()->GetActorCount() == InitialActorCount;
    const FVector2D ConfiguredPreviewSize = SelectedResultPreview->GetConfiguredPreviewIconSize();
    const bool bLargePreview = UKalmalaSelectedResultWidget::GetPreviewIconExtent() >= 64
        && ConfiguredPreviewSize.X >= 64.0f && ConfiguredPreviewSize.Y >= 64.0f;
    const bool bPreviewKeyboard = bKeyboardPreviewAdvanced && bKeyboardPreviewRestored;
    const bool bPreviewController = bControllerPreviewAdvanced && bControllerPreviewRestored;
    UE_LOG(LogTemp, Display, TEXT("Result preview: Keyboard=%d DPad=%d Unavailable=%d MissingIconFallback=%d NoResultsCleared=%d Large=%d NoActorsSpawned=%d CatalogueStable=%d IconExtent=%d"),
        bPreviewKeyboard, bPreviewController, bUnavailablePreview, bMissingIconFallback,
        bNoResults,
        bLargePreview, bNoActorsSpawned, bCatalogueStable,
        UKalmalaSelectedResultWidget::GetPreviewIconExtent());
    const float ScrollOffsetOfEnd = CraftingScrollBox ? CraftingScrollBox->GetScrollOffsetOfEnd() : 0.0f;
    const bool bScrollable = ScrollOffsetOfEnd > 1.0f;
    UE_LOG(LogTemp, Display, TEXT("Build grid input: KeyboardDown=%d KeyboardUp=%d DPadDown=%d DPadUp=%d Scrollable=%d ScrollEnd=%.1f Focused=%d"),
        bKeyboardAdvanced, bKeyboardRestored, bControllerAdvanced, bControllerRestored,
        bScrollable, ScrollOffsetOfEnd, HasKeyboardFocus());
    return bKeyboardAdvanced && bKeyboardRestored && bControllerAdvanced && bControllerRestored && bScrollable
        && bPreviewKeyboard && bPreviewController && bUnavailablePreview && bMissingIconFallback
        && bNoResults && bLargePreview && bNoActorsSpawned && bCatalogueStable
        && bSelectionKept && bCategoryWorked && bNoResults && bRestored && bSearchFocusSafe
        && bBuildGroups && bBuildSelection && bBuildKeys && bBuildEmpty;
}

bool UKalmalaCraftingWidget::PrepareRecipeActivityReviewForTest()
{
    if (!bOpen || !RecipeGrid) return false;
    if (!FParse::Param(FCommandLine::Get(), TEXT("KalmalaCraftingTest"))) return false;
    UKalmalaCraftingSubsystem* LocalState = GetLocalCraftingSubsystem();
    if (!LocalState) return false;
    const UKalmalaRecipeCatalogue* Catalogue = UKalmalaRecipeCatalogue::Get();
    if (!Catalogue) return false;
    const FName CraftingFixtureId(TEXT("KalmalaActivityCraftFixture"));
    UKalmalaRecipeCatalogue* MutableCatalogue = GetMutableDefault<UKalmalaRecipeCatalogue>();
    if (!MutableCatalogue->Recipes.ContainsByPredicate([CraftingFixtureId](const FKalmalaRecipe& Recipe)
        { return Recipe.RecipeId == CraftingFixtureId; }))
    {
        FKalmalaRecipe& Fixture = MutableCatalogue->Recipes.AddDefaulted_GetRef();
        Fixture.RecipeId = CraftingFixtureId;
        Fixture.DisplayName = TEXT("Activity review crafted item");
        FKalmalaInventoryStack& Cost = Fixture.Ingredients.AddDefaulted_GetRef();
        Cost.ItemId = TEXT("Stone");
        Cost.Quantity = 1;
        Fixture.Output = TEXT("Wood");
        Fixture.OutputCount = 1;
        Fixture.MaxBatch = 1;
        Fixture.ExperienceSkill = EKalmalaSkill::None;
        Fixture.ExperienceAward = 0;
        Fixture.bEnabled = true;
        if (!MutableCatalogue->IsValidCatalogue())
        {
            MutableCatalogue->Recipes.Pop(EAllowShrinking::No);
            return false;
        }
    }
    const auto& Recipes = Catalogue->Recipes;
    TArray<int32> BuildIndices;
    TArray<int32> CookingIndices;
    TArray<int32> CraftingIndices;
    for (int32 Index = 0; Index < Recipes.Num(); ++Index)
    {
        if (Recipes[Index].ExperienceSkill == EKalmalaSkill::Cooking) CookingIndices.Add(Index);
        else if (GetBuildBrowseGroup(Recipes[Index].Output) != 0) BuildIndices.Add(Index);
        else CraftingIndices.Add(Index);
    }
    if (BuildIndices.Num() < 2 || CookingIndices.IsEmpty() || CraftingIndices.IsEmpty()) return false;

    const bool bHostOwner = GetOwningPlayer() && GetOwningPlayer()->HasAuthority();
    const int32 LocalBuildIndex = BuildIndices[bHostOwner ? 0 : BuildIndices.Num() - 1];
    const int32 OtherBuildIndex = BuildIndices[bHostOwner ? BuildIndices.Num() - 1 : 0];
    const int32 CookingIndex = CookingIndices[bHostOwner ? 0 : CookingIndices.Num() - 1];
    const int32 CraftingIndex = CraftingIndices[bHostOwner ? 0 : CraftingIndices.Num() - 1];
    const FName BuildId = Recipes[LocalBuildIndex].RecipeId;
    const FName OtherOwnerBuildId = Recipes[OtherBuildIndex].RecipeId;
    const FName CookingId = Recipes[CookingIndex].RecipeId;
    const FName CraftingId = Recipes[CraftingIndex].RecipeId;

    for (const FKalmalaRecipe& Recipe : Recipes) LocalState->SetRecipeFavorite(Recipe.RecipeId, false);
    LocalState->ResetRecipeActivityForTest();
    LocalState->SetRecipeFavorite(OtherOwnerBuildId, false);
    LocalState->SetRecipeFavorite(CookingId, false);
    LocalState->SetRecipeFavorite(CraftingId, false);
    LocalState->SetRecipeFavorite(BuildId, true);
    LocalState->SetRecipeActivityForTest(EKalmalaCraftingActionKind::BuiltPiece, BuildId, 9, true);
    LocalState->SetRecipeActivityForTest(EKalmalaCraftingActionKind::CookedRecipe, CookingId, 5, true);
    LocalState->SetRecipeActivityForTest(EKalmalaCraftingActionKind::CraftedItem, CraftingId, 7, true);
    LocalState->SetRecipeActivityForTest(EKalmalaCraftingActionKind::BuiltPiece, OtherOwnerBuildId, 0, false);
    LocalState->PruneRecipeActivity();

    SetRecipeBrowse(TEXT(""), 0, false);
    const TArray<int32> OrdinaryVisibleIndices = GetVisibleRecipeIndices();
    const int32 OrdinaryCookingSlot = OrdinaryVisibleIndices.IndexOfByKey(CookingIndex);
    const int32 OrdinaryCraftingSlot = OrdinaryVisibleIndices.IndexOfByKey(CraftingIndex);
    if (OrdinaryCookingSlot == INDEX_NONE || OrdinaryCraftingSlot == INDEX_NONE) return false;
    Refresh();
    const auto HasActivityMarkerWidgets = [this](const int32 MarkerSlotIndex)
    {
        return RecipeSlotFavoriteFrames.IsValidIndex(MarkerSlotIndex) && RecipeSlotFavoriteFrames[MarkerSlotIndex]
            && RecipeSlotFavoriteMarkers.IsValidIndex(MarkerSlotIndex) && RecipeSlotFavoriteMarkers[MarkerSlotIndex]
            && RecipeSlotRankMarkers.IsValidIndex(MarkerSlotIndex) && RecipeSlotRankMarkers[MarkerSlotIndex]
            && RecipeSlotRecentMarkers.IsValidIndex(MarkerSlotIndex) && RecipeSlotRecentMarkers[MarkerSlotIndex]
            && RecipeSlotRecentBadgeFrames.IsValidIndex(MarkerSlotIndex) && RecipeSlotRecentBadgeFrames[MarkerSlotIndex];
    };
    if (!HasActivityMarkerWidgets(OrdinaryCookingSlot) || !HasActivityMarkerWidgets(OrdinaryCraftingSlot))
        return false;
    const bool bOrdinaryRecent = RecipeSlotRecentMarkers[OrdinaryCookingSlot]->GetText().ToString().Contains(TEXT("Recent"))
        && RecipeSlotRecentBadgeFrames[OrdinaryCookingSlot]->GetVisibility() == ESlateVisibility::HitTestInvisible
        && RecipeSlotRecentMarkers[OrdinaryCraftingSlot]->GetText().ToString().Contains(TEXT("Recent"))
        && RecipeSlotRecentBadgeFrames[OrdinaryCraftingSlot]->GetVisibility() == ESlateVisibility::HitTestInvisible;

    SetRecipeBrowse(TEXT(""), 7, false);
    const TArray<int32> VisibleIndices = GetVisibleRecipeIndices();
    const int32 BuildSlot = VisibleIndices.IndexOfByKey(LocalBuildIndex);
    const int32 CookingSlot = VisibleIndices.IndexOfByKey(CookingIndex);
    const int32 CraftingSlot = VisibleIndices.IndexOfByKey(CraftingIndex);
    if (!VisibleIndices.Contains(LocalBuildIndex) || !VisibleIndices.Contains(CookingIndex)
        || !VisibleIndices.Contains(CraftingIndex) || BuildSlot == INDEX_NONE
        || CookingSlot == INDEX_NONE || CraftingSlot == INDEX_NONE) return false;
    Selected = BuildSlot;
    Refresh();
    if (!HasActivityMarkerWidgets(BuildSlot) || !HasActivityMarkerWidgets(CookingSlot)
        || !HasActivityMarkerWidgets(CraftingSlot)) return false;

    const FString FavoriteText = RecipeSlotFavoriteMarkers[BuildSlot]->GetText().ToString();
    const FString RankText = RecipeSlotRankMarkers[BuildSlot]->GetText().ToString();
    const FString RecentText = RecipeSlotRecentMarkers[BuildSlot]->GetText().ToString();
    const bool bFavoriteTreatment = FKalmalaUITheme::Get().UsesFavoriteMarkerStar()
        ? FavoriteText.Contains(TEXT("★ Favorite")) : FavoriteText == TEXT("Favorite");
    const FKalmalaUITheme& MarkerTheme = FKalmalaUITheme::Get();
    const int32 MarkerContrast = UKalmalaSettingsWidget::GetContrastMode();
    const FLinearColor ExpectedFavoriteColor = MarkerContrast == 0
        ? MarkerTheme.FavoriteMarkerColor : FLinearColor::White;
    const FLinearColor ExpectedFavoriteFrame = MarkerTheme.UsesFavoriteMarkerBorder()
        ? ExpectedFavoriteColor : FLinearColor::Transparent;
    const bool bThemeMarkers = RecipeSlotFavoriteFrames[BuildSlot]->GetBrushColor().Equals(ExpectedFavoriteFrame)
        && RecipeSlotFavoriteMarkers[BuildSlot]->GetColorAndOpacity().GetSpecifiedColor().Equals(ExpectedFavoriteColor)
        && RecipeSlotRankMarkers[BuildSlot]->GetColorAndOpacity().GetSpecifiedColor()
            .Equals(MarkerTheme.RankMarkerColor(1, MarkerContrast))
        && RecipeSlotRecentMarkers[BuildSlot]->GetColorAndOpacity().GetSpecifiedColor().Equals(
            MarkerContrast == 0 ? MarkerTheme.RecentMarkerColor : FLinearColor::White);
    const bool bCoexist = bFavoriteTreatment && bThemeMarkers && RankText.Contains(TEXT("Gold Rank 1"))
        && RecentText.Contains(TEXT("Recent"))
        && RecipeSlotFavoriteMarkers[BuildSlot] != RecipeSlotRankMarkers[BuildSlot]
        && RecipeSlotFavoriteMarkers[BuildSlot] != RecipeSlotRecentMarkers[BuildSlot]
        && RecipeSlotRankMarkers[BuildSlot] != RecipeSlotRecentMarkers[BuildSlot]
        && RecipeSlotRecentBadgeFrames[BuildSlot]->GetVisibility() == ESlateVisibility::HitTestInvisible;
    const bool bRecentShortcuts = RecipeSlotRecentMarkers[CookingSlot]->GetText().ToString().Contains(TEXT("Recent"))
        && RecipeSlotRecentBadgeFrames[CookingSlot]->GetVisibility() == ESlateVisibility::HitTestInvisible
        && RecipeSlotRecentMarkers[CraftingSlot]->GetText().ToString().Contains(TEXT("Recent"))
        && RecipeSlotRecentBadgeFrames[CraftingSlot]->GetVisibility() == ESlateVisibility::HitTestInvisible;
    const bool bNoManualBookmarkRequired = !LocalState->IsRecipeFavorite(CookingId)
        && !LocalState->IsRecipeFavorite(CraftingId)
        && RecipeSlotFavoriteMarkers[CookingSlot]->GetText().IsEmpty()
        && RecipeSlotFavoriteMarkers[CraftingSlot]->GetText().IsEmpty()
        && VisibleIndices.Contains(CookingIndex) && VisibleIndices.Contains(CraftingIndex);
    const bool bOwnerIsolation = BuildId != OtherOwnerBuildId
        && LocalState->IsRecipeFavorite(BuildId) && !LocalState->IsRecipeFavorite(OtherOwnerBuildId)
        && LocalState->GetRecipeActivityCount(EKalmalaCraftingActionKind::BuiltPiece, OtherOwnerBuildId) == 0;

    int32 ExpectedReducedMotion = 0;
    const bool bHasExpectedMotion = FParse::Value(FCommandLine::Get(),
        TEXT("KalmalaUIDeveloperReducedMotion="), ExpectedReducedMotion);
    const bool bMotionSettingMatches = !bHasExpectedMotion
        || UKalmalaSettingsWidget::IsReducedMotionEnabled() == (ExpectedReducedMotion != 0);
    const bool bStaticMarkers = RecipeSlotFavoriteMarkers[BuildSlot]->GetVisibility() == ESlateVisibility::HitTestInvisible
        && RecipeSlotRankMarkers[BuildSlot]->GetVisibility() == ESlateVisibility::HitTestInvisible
        && RecipeSlotRecentMarkers[BuildSlot]->GetVisibility() == ESlateVisibility::HitTestInvisible
        && FMath::IsNearlyEqual(RecipeSlotFavoriteMarkers[BuildSlot]->GetRenderOpacity(), 1.0f)
        && FMath::IsNearlyEqual(RecipeSlotRankMarkers[BuildSlot]->GetRenderOpacity(), 1.0f)
        && FMath::IsNearlyEqual(RecipeSlotRecentMarkers[BuildSlot]->GetRenderOpacity(), 1.0f);
    const bool bStaticMotion = bMotionSettingMatches && bStaticMarkers;
    if (CraftingScrollBox)
        CraftingScrollBox->ScrollWidgetIntoView(RecipeGrid, false, EDescendantScrollDestination::TopOrLeft);
    const FString OwnerName = bHostOwner ? TEXT("Host") : TEXT("Client");
    UE_LOG(LogTemp, Display, TEXT("Recipe activity markers: Owner=%s FavoriteId=%s RecentCooking=%s RecentCrafting=%s Coexist=%d OrdinaryRecent=%d RecentShortcuts=%d NoManualBookmark=%d StaticMotion=%d OwnerIsolation=%d"),
        *OwnerName, *BuildId.ToString(), *CookingId.ToString(), *CraftingId.ToString(),
        bCoexist, bOrdinaryRecent, bRecentShortcuts, bNoManualBookmarkRequired, bStaticMotion, bOwnerIsolation);
    return bCoexist && bOrdinaryRecent && bRecentShortcuts && bNoManualBookmarkRequired && bStaticMotion && bOwnerIsolation;
}
#endif

void UKalmalaCraftingWidget::Refresh()
{
    auto* M=Model(); if (!M) { Close(); return; }
    const auto& Recipes=UKalmalaRecipeCatalogue::Get()->Recipes;
    const TArray<int32> VisibleIndices = GetVisibleRecipeIndices();
    const int32 TextScalePercent = UKalmalaSettingsWidget::ClampTextScale(
        UKalmalaSettingsWidget::GetTextScalePercent());
    const int32 ContrastMode = UKalmalaSettingsWidget::ClampContrastMode(
        UKalmalaSettingsWidget::GetContrastMode());
    TArray<FKalmalaCatalogueRow> InspectionRows;
    const auto* OwnerPawn = GetOwningPlayerPawn();
    if (const auto* Inventory = OwnerPawn ? OwnerPawn->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr)
        for (const auto& Stack : Inventory->GetStacks())
        {
            const auto* Item = UKalmalaItemCatalogue::Get()->FindItem(Stack.ItemId);
            InspectionRows.Add({Stack.ItemId, Item ? Item->DisplayName : Stack.ItemId.ToString(),
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
            InspectionRows.Add({Tool.ToolId, GetReadableToolName(Tool.ToolId),
                VisibleState, true});
        }
    if (LastDetailTextScalePercent != TextScalePercent || LastDetailContrastMode != ContrastMode)
    {
        const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
        Theme.ApplyMenu(*WidgetTree, HeaderText, TextScalePercent, ContrastMode);
        Theme.ApplyPanel(*MenuBackground, ContrastMode, &Theme.BuildPanelImage);
        if (RecipeSearchBox)
        {
            RecipeSearchStyle.SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), FMath::RoundToInt(Theme.BodySize * TextScalePercent / 100.f)));
            RecipeSearchBox->SetWidgetStyle(RecipeSearchStyle);
        }
        LastDetailTextScalePercent = TextScalePercent;
        LastDetailContrastMode = ContrastMode;
    }
    if (InventoryInspector) InventoryInspector->SetRows(InspectionRows, TextScalePercent, ContrastMode);
    if (!VisibleIndices.IsEmpty()) Selected=FMath::Clamp(Selected,0,VisibleIndices.Num()-1);
    const int32 FavoriteRecipeIndex = VisibleIndices.IsValidIndex(Selected) ? VisibleIndices[Selected] : INDEX_NONE;
    const FName FavoriteRecipeId = Recipes.IsValidIndex(FavoriteRecipeIndex) ? Recipes[FavoriteRecipeIndex].RecipeId : NAME_None;
    const UKalmalaCraftingSubsystem* FavoriteState = GetLocalCraftingSubsystem();
    const bool bSelectedFavorite = FavoriteState && FavoriteState->IsRecipeFavorite(FavoriteRecipeId);
    const bool bCanBookmark = FavoriteState && !FavoriteRecipeId.IsNone()
        && (bSelectedFavorite || FavoriteState->CanFavoriteRecipe(FavoriteRecipeId));
    if (FavoriteButton) FavoriteButton->SetIsEnabled(bCanBookmark);
    if (FavoriteActionLabel) FavoriteActionLabel->SetText(FText::FromString(FavoriteRecipeId.IsNone()
        ? TEXT("Favorite: no recipe selected")
        : bSelectedFavorite ? TEXT("Remove from Favorites")
        : bCanBookmark ? TEXT("Add to Favorites") : TEXT("Favorites limit reached")));
    RefreshRecipeGrid(VisibleIndices, M, TextScalePercent, ContrastMode);
    if (VisibleIndices.IsEmpty())
    {
        RecipesText->SetText(FText::FromString(TEXT("No matching recipes. Clear recipe search or choose All. Station scope still applies.\n")));
        if (SelectedResultPreview) SelectedResultPreview->ClearResult();
        Ingredients->SetIngredients({}, nullptr, TextScalePercent, ContrastMode);
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
    const FString StationPrefix = StationFilterKit.IsNone() ? TEXT("")
        : (UKalmalaItemCatalogue::Get()->FindItem(StationFilterKit)
            ? UKalmalaItemCatalogue::Get()->FindItem(StationFilterKit)->DisplayName : StationFilterKit.ToString()) + TEXT(" recipes: ");
    RecipesText->SetText(FText::FromString(FString::Printf(TEXT("> %s%d of %d: %s\n"),
        *StationPrefix, Selected + 1, VisibleIndices.Num(), *SelectedRecipe.DisplayName)));
    const FString Availability = M->GetRecipeAvailability(SelectedRecipe.RecipeId);
    const auto* RequirementOwner = Cast<AKalmalaCharacter>(OwnerPawn);
    const FString Requirements = FKalmalaRecipeRequirements::Describe(SelectedRecipe,
        OwnerPawn ? OwnerPawn->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr,
        RequirementOwner ? RequirementOwner->GetCarriedToolLevel(TEXT("ConstructionHammer")) : -1,
        Availability);
    const FString Description = M->GetRecipeDescription(SelectedRecipe.RecipeId)
        + TEXT("\nAvailability: ") + Availability + TEXT("\n")
        + BuildSkillProgressText(Cast<AKalmalaCharacter>(GetOwningPlayerPawn()));
    if (SelectedResultPreview)
        SelectedResultPreview->SetResult(SelectedRecipe.Output, SelectedRecipe.DisplayName,
            Description, Requirements, TextScalePercent, ContrastMode);
    const bool bDirectBuild = UKalmalaRecipeCatalogue::IsDirectMaterialBuildable(SelectedRecipe.Output);
    if (CraftButton)
    {
        if (UTextBlock* ButtonLabel = Cast<UTextBlock>(CraftButton->GetContent()))
            ButtonLabel->SetText(FText::FromString(bDirectBuild ? TEXT("Build selected") : TEXT("Craft one")));
        const FString ButtonToolTip = bDirectBuild
            ? FString::Printf(TEXT("Build %s directly from raw materials with the Construction Hammer. Availability: %s. Rejected requests preserve materials."),
                *SelectedRecipe.DisplayName, *Availability)
            : FString::Printf(TEXT("Craft batch 1 of %s. Availability: %s. A rejected request preserves ingredients and tool condition."),
                *SelectedRecipe.DisplayName, *Availability);
        CraftButton->SetToolTipText(FText::FromString(ButtonToolTip));
        if (CraftButton->GetIsEnabled() != SelectedRecipe.bEnabled)
        {
            CraftButton->SetIsEnabled(SelectedRecipe.bEnabled);
            FKalmalaUITheme::Get().ApplyButton(*CraftButton, ContrastMode);
        }
    }
    FString PreviewText;
    if (bPlacementPreviewEnabled)
    {
        const FKalmalaPlacementPreview Preview = FKalmalaPlacementPreview::Evaluate(GetWorld(), GetOwningPlayerPawn(), SelectedRecipe.Output);
        PreviewText = TEXT("\n") + Preview.Message + (Preview.bIsValid ? FString::Printf(TEXT(" (%.0f, %.0f)"), Preview.Location.X, Preview.Location.Y) : TEXT("")) + TEXT("\n");
    }
    FString ToolConditionText = TEXT("\nTool condition and free repair status (owner-only):");
    const auto* Character = Cast<AKalmalaCharacter>(GetOwningPlayerPawn());
    const auto AppendToolCondition = [&ToolConditionText, Character](const FKalmalaToolDefinition& Definition)
    {
        const int32 ToolLevel = Character ? Character->GetCarriedToolLevel(Definition.ToolId) : 0;
        const int32 Condition = Character && ToolLevel > 0 ? Character->GetToolDurability(Definition.ToolId) : -1;
        const FString RepairState = Condition < 0 || Condition > Definition.MaxDurability
            ? TEXT("; condition unavailable")
            : Condition < Definition.MaxDurability
                ? TEXT("; damaged, repair free at a visible Workbench or Forge")
                : TEXT("; no repair needed");
        ToolConditionText += FString::Printf(TEXT("\n%s: %d/%d%s"), *GetReadableToolName(Definition.ToolId), Condition,
            Definition.MaxDurability, *RepairState);
    };
    for (const FKalmalaToolDefinition& Definition : FKalmalaToolLifecycleContract::GetDefinitions()) AppendToolCondition(Definition);
    for (const FKalmalaToolDefinition& Definition : FKalmalaToolLifecycleContract::GetTieredAxeDefinitions()) AppendToolCondition(Definition);
    AppendToolCondition(FKalmalaToolLifecycleContract::GetConstructionHammerDefinition());
    StateText->SetText(FText::FromString(TEXT("\nNearby hearth (replicated shared state; text does not rely on colour):\n")
        + ToolConditionText + TEXT("\n") + M->GetNearbyFireText()+TEXT("\n")+M->GetNearbyConstructionText()+TEXT("\n")+M->GetNearbyWorkbenchText()+TEXT("\n")+M->GetLastResult()+TEXT("\n")+PreviewText));
    FoodText->SetText(FText::FromString(M->GetFoodText()));
    if (ToolProgressionText) ToolProgressionText->SetText(FText::FromString(M->GetToolProgressionText()));
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
    return InstructionsText && RecipesText && StateText
        ? InstructionsText->GetText().ToString()+RecipesText->GetText().ToString()
            + (SelectedResultPreview ? SelectedResultPreview->GetPresentationText() : FString())
            + StateText->GetText().ToString() + (FoodText ? FoodText->GetText().ToString() : FString())
            + (RepairText ? RepairText->GetText().ToString() : FString())
            + (ToolProgressionText ? ToolProgressionText->GetText().ToString() : FString())
            + (FavoriteActionLabel ? FavoriteActionLabel->GetText().ToString() : FString())
            + (CraftButton ? CraftButton->GetToolTipText().ToString() : FString())
            + (StorageText ? StorageText->GetText().ToString() : FString()) : FString();
}
void UKalmalaCraftingWidget::NativeTick(const FGeometry& G,float D)
{
    Super::NativeTick(G,D);
    if (!bOpen) return;
    Refresh();
    if (!CraftingScrollBox) return;

    const float MaxScrollOffset = FMath::Max(0.0f, CraftingScrollBox->GetScrollOffsetOfEnd());
    const float CurrentScrollOffset = CraftingScrollBox->GetScrollOffset();
    if (bPendingMenuScrollRestore || CurrentScrollOffset > MaxScrollOffset)
    {
        const float TargetOffset = bPendingMenuScrollRestore
            ? PendingMenuScrollRestoreOffset : CurrentScrollOffset;
        CraftingScrollBox->SetScrollOffset(FMath::Clamp(TargetOffset, 0.0f, MaxScrollOffset));
        bPendingMenuScrollRestore = false;
    }
}
void UKalmalaCraftingWidget::Previous() { const int32 N=GetVisibleRecipeIndices().Num(); if(N) Selected=(Selected+N-1)%N; Refresh(); }
void UKalmalaCraftingWidget::Next() { const int32 N=GetVisibleRecipeIndices().Num(); if(N) Selected=(Selected+1)%N; Refresh(); }
void UKalmalaCraftingWidget::Craft()
{
    const auto& Recipes = UKalmalaRecipeCatalogue::Get()->Recipes;
    const TArray<int32> VisibleIndices = GetVisibleRecipeIndices();
    if (!VisibleIndices.IsValidIndex(Selected) || !Recipes.IsValidIndex(VisibleIndices[Selected])) return;
    const FKalmalaRecipe& Recipe = Recipes[VisibleIndices[Selected]];
    if (UKalmalaRecipeCatalogue::IsDirectMaterialBuildable(Recipe.Output)) { Place(); return; }
    if (auto* M = Model()) M->ServerCraft(Recipe.RecipeId, 1);
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
void UKalmalaCraftingWidget::CraftBronzeAxe() { if(auto* M=Model()) M->ServerProgressTool(TEXT("BronzeAxe")); }
void UKalmalaCraftingWidget::UpgradeIronAxe() { if(auto* M=Model()) M->ServerProgressTool(TEXT("IronAxe")); }
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
    const auto& Items = UKalmalaItemCatalogue::Get()->Items;
    if (auto* M=Model(); M && Items.IsValidIndex(SelectedStorageItem)) M->ServerDepositStorage(Items[SelectedStorageItem].ItemId);
}
void UKalmalaCraftingWidget::WithdrawStorage()
{
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
    if (InventoryInspector && (InventoryInspector->HasKeyboardFocus() || InventoryInspector->HasFocusedDescendants()))
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
    if (const auto* Character = Cast<AKalmalaCharacter>(PC->GetPawn()))
    {
        UKalmalaCraftingComponent* Crafting = Character->FindComponentByClass<UKalmalaCraftingComponent>();
        ObserveRecipeActivity(Crafting);
        if (Crafting != StationInteractionModel.Get())
        {
            StationInteractionModel = Crafting;
            LastStationInteractionSerial = Crafting ? Crafting->GetCookingStationInteractionSerial() : 0;
            bHasSeenStationInteraction = Crafting != nullptr;
        }
        else if (Crafting)
        {
            const uint32 Serial = Crafting->GetCookingStationInteractionSerial();
            if (bHasSeenStationInteraction && Serial != LastStationInteractionSerial)
            {
                LastStationInteractionSerial = Serial;
                const FName StationKit = Crafting->GetLastInteractedCookingStationKit();
                if (IsInWorldCookingStation(StationKit))
                {
                    if (!Widget)
                    {
                        Widget = CreateWidget<UKalmalaCraftingWidget>(PC);
                        if (Widget) Widget->AddToPlayerScreen(160);
                    }
                    if (Widget) Widget->OpenForStation(StationKit);
                }
            }
        }
    }
    else
    {
        ObserveRecipeActivity(nullptr);
        StationInteractionModel.Reset();
        LastStationInteractionSerial = 0;
        bHasSeenStationInteraction = false;
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
        int32 DeveloperReducedMotion = 0;
        if (FParse::Value(FCommandLine::Get(), TEXT("KalmalaUIDeveloperReducedMotion="), DeveloperReducedMotion))
            if (UKalmalaSettingsWidget::IsReducedMotionEnabled() != (DeveloperReducedMotion != 0))
                UKalmalaSettingsWidget::SetReducedMotionEnabled(DeveloperReducedMotion != 0);
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
            const bool ToolFeedbackPassed = Text.Contains(TEXT("Tool condition and free repair status (owner-only)"))
                && Text.Contains(TEXT("TOOL PROGRESSION — OWNER ONLY"))
                && Text.Contains(TEXT("target level 1")) && Text.Contains(TEXT("target level 2"))
                && Text.Contains(TEXT("Cost: 4 Wood (have "))
                && (Text.Contains(TEXT("Nearby Workbench level 1; required level 1"))
                    || Text.Contains(TEXT("Need a visible same-world Workbench level 1 within 2.5 m")))
                && (Text.Contains(TEXT("Nearby Forge level 2; required level 2"))
                    || Text.Contains(TEXT("Need a visible same-world Forge level 2 within 2.5 m")))
                && Text.Contains(TEXT("Accepted attachments persist in this world's construction save"))
                && Text.Contains(TEXT("Grinding Stone Repair All: interact with a visible same-world Grinding Stone"));
            UE_LOG(LogTemp, Display, TEXT("M9 tool feedback: Passed=%d"), ToolFeedbackPassed);
            const auto* Character = Cast<AKalmalaCharacter>(PC->GetPawn());
            const auto* Crafting = Character ? Character->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
            const FString ChestDescription = Crafting ? Crafting->GetRecipeDescription(TEXT("Storage")) : FString();
            const FString RetiredSmokeFrameDescription = Crafting ? Crafting->GetRecipeDescription(TEXT("SmokeFrame")) : FString();
            const FString RetiredSmokingRecipeDescription = Crafting ? Crafting->GetRecipeDescription(TEXT("SmokeBoarMeat")) : FString();
            const FString DirectBuildDescription = Crafting ? Crafting->GetRecipeDescription(TEXT("Floor")) : FString();
            const bool CampFeedbackPassed = ChestDescription.Contains(TEXT("6 Wood"))
                && ChestDescription.Contains(TEXT("8 Reed fibre")) && ChestDescription.Contains(TEXT("Chest"))
                && RetiredSmokeFrameDescription == TEXT("Unknown recipe")
                && RetiredSmokingRecipeDescription == TEXT("Unknown recipe")
                && DirectBuildDescription.Contains(TEXT("Build directly with the Construction Hammer; no kit is created."))
                && DirectBuildDescription.Contains(TEXT("6 Wood")) && DirectBuildDescription.Contains(TEXT("4 Reed fibre"));
            UE_LOG(LogTemp, Display, TEXT("M9 camp feedback: Passed=%d"), CampFeedbackPassed);
            const bool bInspection = Widget->VerifyInventoryInspectionForTest();
            UE_LOG(LogTemp, Display, TEXT("Inventory inspection: FocusAndKeys=%d"), bInspection);
            const bool bGridNavigation = Widget->VerifyRecipeGridNavigationForTest();
            const FString GridSummary = Widget->GetRecipeGridSummary();
            const bool bGridReady = bGridNavigation && GridSummary.Contains(TEXT("Slots="))
                && GridSummary.Contains(TEXT("Unavailable=")) && GridSummary.Contains(TEXT("Focused=1"))
                && GridSummary.Contains(TEXT("ReadOnly=1")) && GridSummary.Contains(TEXT("Scrollable=1"));
            const bool bActivityMarkers = Widget->PrepareRecipeActivityReviewForTest();
            UE_LOG(LogTemp, Display, TEXT("Build slot grid: %s Navigation=%d"), *GridSummary, bGridNavigation);
            auto* InventoryHUD = GetLocalPlayer()->GetSubsystem<UKalmalaInventorySubsystem>();
            const bool bHUDHidden = InventoryHUD && InventoryHUD->IsCraftingMenuSuppressed();
            const bool bPromptHidden = !InteractionPrompt || InteractionPrompt->GetVisibility() != ESlateVisibility::Visible;
            UE_LOG(LogTemp, Display, TEXT("Interaction prompt modal: Hidden=%d"), bPromptHidden ? 1 : 0);
            const bool Passed=bInspection && bHUDHidden && Text.Contains(TEXT("Construction hammer menu input:")) && Text.Contains(TEXT("Up/Down"))
                && Text.Contains(TEXT("Requirements — selected recipe"))
                && Text.Contains(TEXT("Tool: carried Construction Hammer level 1 — Present"))
                && Text.Contains(TEXT("Skill level: no recipe requirement."))
                && Text.Contains(TEXT("Unlock: no additional recipe lock."))
                && Text.Contains(TEXT("Raw material cost: 5 Stone, 3 Wood"))
                && Text.Contains(TEXT("Ignition: one raw Wood, Lightwood, Densewood, or Coal is also consumed to start the hearth with 60 seconds of fuel."))
                && Text.Contains(TEXT("Output: Hearth ring construction (no kit item created)"))
                && Text.Contains(TEXT("Description: A low stone-and-wood hearth built in place with a Construction Hammer; raw fuel lights it."))
                && Text.Contains(TEXT("Description: Basic construction material."))
                && Text.Contains(TEXT("Build quantity: one hearth per request"))
                && Text.Contains(TEXT("Failure: the availability text below"))
                && Text.Contains(TEXT("SKILL PROGRESS [PRIVATE TO YOU]"))
                && Text.Contains(TEXT("Cooking: Level 1, 0/100 XP to Level 2"))
                && Text.Contains(TEXT("Recipe access depends on materials, stations, and world conditions; skill level does not lock recipes."))
                && !Text.Contains(TEXT("Next recipe unlock:"))
                && Text.Contains(TEXT("Selection is marked with >"))
                && Text.Contains(TEXT("Free repair: at a visible same-world Workbench or Forge"))
                && Text.Contains(TEXT("Tool condition and free repair status (owner-only)")) && Text.Contains(TEXT("Bronze Axe:")) && Text.Contains(TEXT("Iron Axe:"))
                && Text.Contains(TEXT("Roasted field meat:"))
                && Text.Contains(TEXT("Build Hearth ring directly from raw materials"))
                && Text.Contains(TEXT("Rejected requests preserve materials"))
                && PreviewText.Contains(TEXT("Preview "))
                && bGridReady
                && bActivityMarkers
                && bPromptHidden
                && PC->IsMoveInputIgnored() && Widget->IsFocusable();
            Widget->Close();
            const bool bHUDRestored = InventoryHUD && !InventoryHUD->IsCraftingMenuSuppressed();
            UE_LOG(LogTemp, Display, TEXT("Crafting HUD overlap: Hidden=%d Restored=%d"), bHUDHidden, bHUDRestored);
            UE_LOG(LogTemp,Display,TEXT("Crafting presentation: Passed=%d Restored=%d"),Passed,!PC->IsMoveInputIgnored() && bHUDRestored); bVerified=true;
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
    if (bCaptureRequested && ReviewCaptureStage < 28 && Widget
        && FParse::Value(FCommandLine::Get(), TEXT("KalmalaCraftingCapture="), CapturePath))
    {
        CaptureWait += DeltaTime;
        if (CaptureWait > 3.0f)
        {
            CaptureWait = 0;
            if (ReviewCaptureStage == 26)
            {
                UE_LOG(LogTemp, Display, TEXT("Recipe activity marker review: Prepared=%d"),
                    Widget->PrepareRecipeActivityReviewForTest());
            }
            else if (ReviewCaptureStage == 27)
            {
                FScreenshotRequest::RequestScreenshot(FPaths::GetBaseFilename(CapturePath, false)
                    + TEXT("-activity-markers.png"), true, false);
            }
            else if (ReviewCaptureStage >= 18)
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
    if (!bOpen || !CraftingScrollBox || !Ingredients || !SelectedResultPreview || View < 0 || View > 3) return false;
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
    const FString Requirements = SelectedResultPreview->GetRequirementsText();
    if (!Requirements.Contains(TEXT("Skill level: no recipe requirement."))
        || (View < 2 && !Requirements.Contains(TEXT("carried Construction Hammer level 1 — Present")))
        || (View >= 2 && !Requirements.Contains(TEXT("Cooking heat:")))) return false;
    CraftingScrollBox->ScrollWidgetIntoView(View % 2 == 0 ? static_cast<UWidget*>(Ingredients.Get())
        : static_cast<UWidget*>(SelectedResultPreview.Get()), false, EDescendantScrollDestination::TopOrLeft);
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
    UWidget* Target = bFeedback ? static_cast<UWidget*>(RepairText.Get())
        : static_cast<UWidget*>(SelectedResultPreview.Get());
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
    if (!PlayerController->IsLocalController() || (Widget && Widget->IsOpen())
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
            UKalmalaSettingsWidget::GetLocalInputBindingLabel(TEXT("Interact"), false).ToString(),
            UKalmalaSettingsWidget::GetLocalInputBindingLabel(TEXT("Interact"), true).ToString(),
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

    const FString KeyboardBinding = UKalmalaSettingsWidget::GetLocalInputBindingLabel(TEXT("Interact"), false).ToString();
    const FString ControllerBinding = UKalmalaSettingsWidget::GetLocalInputBindingLabel(TEXT("Interact"), true).ToString();
    FString StageName;
    FString Text;
    switch (InteractionPromptReviewStage)
    {
    case 0:
        StageName = TEXT("available");
        Text = UKalmalaInteractionPromptWidget::BuildPromptText(TEXT("Densewood trunk"), TEXT("Chop"),
            KeyboardBinding, ControllerBinding);
        break;
    case 1:
        StageName = TEXT("unavailable");
        Text = UKalmalaInteractionPromptWidget::BuildPromptText(TEXT("Mire campfire"), TEXT("Light"),
            KeyboardBinding, ControllerBinding, TEXT("Too wet to light"));
        break;
    case 2:
        StageName = TEXT("modal");
        Text = UKalmalaInteractionPromptWidget::BuildPromptText(TEXT("Workbench"), TEXT("Use"),
            KeyboardBinding, ControllerBinding, FString(), true);
        break;
    default:
        StageName = TEXT("no-target");
        Text = UKalmalaInteractionPromptWidget::BuildPromptText(FString(), FString(),
            KeyboardBinding, ControllerBinding);
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

bool UKalmalaCraftingSubsystem::CanFavoriteRecipe(const FName RecipeId) const
{
    const UKalmalaRecipeCatalogue* Catalogue = UKalmalaRecipeCatalogue::Get();
    if (!Catalogue || !Catalogue->Find(RecipeId)) return false;
    if (FavoriteRecipeIds.Contains(RecipeId)) return true;
    return FavoriteRecipeIds.Num() < FMath::Min(MaxFavoriteRecipeCount, Catalogue->Recipes.Num());
}

bool UKalmalaCraftingSubsystem::SetRecipeFavorite(const FName RecipeId, const bool bFavorite)
{
    if (!bFavorite) return FavoriteRecipeIds.Remove(RecipeId) > 0;
    if (!CanFavoriteRecipe(RecipeId)) return false;
    FavoriteRecipeIds.Add(RecipeId);
    return true;
}

void UKalmalaCraftingSubsystem::PruneRecipeFavorites()
{
    const UKalmalaRecipeCatalogue* Catalogue = UKalmalaRecipeCatalogue::Get();
    if (!Catalogue)
    {
        FavoriteRecipeIds.Reset();
        return;
    }
    for (auto It = FavoriteRecipeIds.CreateIterator(); It; ++It)
    {
        if (!Catalogue->Find(*It)) It.RemoveCurrent();
    }
}

void UKalmalaCraftingSubsystem::ObserveRecipeActivity(UKalmalaCraftingComponent* Crafting)
{
    if (!bHasObservedRecipeActivityComponent || ActivityCraftingComponent.Get() != Crafting)
    {
        ActivityCraftingComponent = Crafting;
        LastAcceptedRecipeActivitySequence = 0;
        if (Crafting)
        {
            for (const FKalmalaAcceptedCraftingActionReceipt& Receipt : Crafting->GetAcceptedCraftingActionReceipts())
                if (Receipt.Sequence > LastAcceptedRecipeActivitySequence)
                    LastAcceptedRecipeActivitySequence = Receipt.Sequence;
        }
        bHasObservedRecipeActivityComponent = true;
        PruneRecipeActivity();
        return;
    }
    if (!Crafting) return;

    bool bObservedNewReceipt = false;
    for (const FKalmalaAcceptedCraftingActionReceipt& Receipt : Crafting->GetAcceptedCraftingActionReceipts())
    {
        if (Receipt.Sequence <= LastAcceptedRecipeActivitySequence) continue;
        LastAcceptedRecipeActivitySequence = Receipt.Sequence;
        bObservedNewReceipt = true;
        RecordAcceptedRecipeActivity(Receipt);
    }
    if (bObservedNewReceipt) PruneRecipeActivity();
}

void UKalmalaCraftingSubsystem::RecordAcceptedRecipeActivity(
    const FKalmalaAcceptedCraftingActionReceipt& Receipt)
{
    const UKalmalaRecipeCatalogue* Catalogue = UKalmalaRecipeCatalogue::Get();
    const FKalmalaRecipe* Recipe = Catalogue ? Catalogue->Find(Receipt.RecipeId) : nullptr;
    if (!Recipe) return;

    TMap<FName, uint32>* Counts = nullptr;
    FName* Recent = nullptr;
    switch (Receipt.Kind)
    {
    case EKalmalaCraftingActionKind::BuiltPiece:
        if (UKalmalaCraftingWidget::GetBuildBrowseGroup(Recipe->Output) == 0) return;
        Counts = &BuiltPieceCounts;
        Recent = &RecentBuiltPieceRecipeId;
        break;
    case EKalmalaCraftingActionKind::CookedRecipe:
        if (Recipe->ExperienceSkill != EKalmalaSkill::Cooking) return;
        Counts = &CookedRecipeCounts;
        Recent = &RecentCookedRecipeId;
        break;
    case EKalmalaCraftingActionKind::CraftedItem:
        if (Recipe->ExperienceSkill == EKalmalaSkill::Cooking) return;
        Counts = &CraftedItemCounts;
        Recent = &RecentCraftedItemRecipeId;
        break;
    default:
        return;
    }

    uint32& Count = Counts->FindOrAdd(Receipt.RecipeId);
    if (Count < TNumericLimits<uint32>::Max()) ++Count;
    *Recent = Receipt.RecipeId;
}

uint32 UKalmalaCraftingSubsystem::GetRecipeActivityCount(
    const EKalmalaCraftingActionKind Kind, const FName RecipeId) const
{
    const TMap<FName, uint32>* Counts = nullptr;
    switch (Kind)
    {
    case EKalmalaCraftingActionKind::BuiltPiece: Counts = &BuiltPieceCounts; break;
    case EKalmalaCraftingActionKind::CookedRecipe: Counts = &CookedRecipeCounts; break;
    case EKalmalaCraftingActionKind::CraftedItem: Counts = &CraftedItemCounts; break;
    default: return 0;
    }
    const uint32* Count = Counts->Find(RecipeId);
    return Count ? *Count : 0;
}

int32 UKalmalaCraftingSubsystem::GetRecipeActivityRank(
    const EKalmalaCraftingActionKind Kind, const FName RecipeId) const
{
    return GetRecipeActivityRanks(Kind).FindRef(RecipeId);
}

TMap<FName, int32> UKalmalaCraftingSubsystem::GetRecipeActivityRanks(
    const EKalmalaCraftingActionKind Kind) const
{
    const TMap<FName, uint32>* Counts = nullptr;
    switch (Kind)
    {
    case EKalmalaCraftingActionKind::BuiltPiece: Counts = &BuiltPieceCounts; break;
    case EKalmalaCraftingActionKind::CookedRecipe: Counts = &CookedRecipeCounts; break;
    case EKalmalaCraftingActionKind::CraftedItem: Counts = &CraftedItemCounts; break;
    default: return {};
    }

    TArray<TPair<FName, uint32>> Ranked;
    Ranked.Reserve(Counts->Num());
    for (const TPair<FName, uint32>& Entry : *Counts)
        if (Entry.Value > 0) Ranked.Add(Entry);
    Ranked.StableSort([](const TPair<FName, uint32>& A, const TPair<FName, uint32>& B)
    {
        if (A.Value != B.Value) return A.Value > B.Value;
        return A.Key.LexicalLess(B.Key);
    });
    TMap<FName, int32> Result;
    for (int32 Index = 0; Index < FMath::Min(3, Ranked.Num()); ++Index)
        Result.Add(Ranked[Index].Key, Index + 1);
    return Result;
}

FName UKalmalaCraftingSubsystem::GetRecentRecipeActivity(const EKalmalaCraftingActionKind Kind) const
{
    switch (Kind)
    {
    case EKalmalaCraftingActionKind::BuiltPiece: return RecentBuiltPieceRecipeId;
    case EKalmalaCraftingActionKind::CookedRecipe: return RecentCookedRecipeId;
    case EKalmalaCraftingActionKind::CraftedItem: return RecentCraftedItemRecipeId;
    default: return NAME_None;
    }
}

void UKalmalaCraftingSubsystem::PruneRecipeActivity()
{
    const UKalmalaRecipeCatalogue* Catalogue = UKalmalaRecipeCatalogue::Get();
    if (!Catalogue)
    {
        BuiltPieceCounts.Reset();
        CookedRecipeCounts.Reset();
        CraftedItemCounts.Reset();
        RecentBuiltPieceRecipeId = NAME_None;
        RecentCookedRecipeId = NAME_None;
        RecentCraftedItemRecipeId = NAME_None;
        return;
    }
    const auto Prune = [Catalogue](TMap<FName, uint32>& Counts)
    {
        for (auto It = Counts.CreateIterator(); It; ++It)
            if (It.Value() == 0 || !Catalogue->Find(It.Key())) It.RemoveCurrent();
    };
    Prune(BuiltPieceCounts);
    Prune(CookedRecipeCounts);
    Prune(CraftedItemCounts);
    if (!Catalogue->Find(RecentBuiltPieceRecipeId)) RecentBuiltPieceRecipeId = NAME_None;
    if (!Catalogue->Find(RecentCookedRecipeId)) RecentCookedRecipeId = NAME_None;
    if (!Catalogue->Find(RecentCraftedItemRecipeId)) RecentCraftedItemRecipeId = NAME_None;
}

#if !UE_BUILD_SHIPPING
void UKalmalaCraftingSubsystem::ResetRecipeActivityForTest()
{
    BuiltPieceCounts.Reset();
    CookedRecipeCounts.Reset();
    CraftedItemCounts.Reset();
    RecentBuiltPieceRecipeId = NAME_None;
    RecentCookedRecipeId = NAME_None;
    RecentCraftedItemRecipeId = NAME_None;
}

void UKalmalaCraftingSubsystem::SetRecipeActivityForTest(
    const EKalmalaCraftingActionKind Kind, const FName RecipeId, const uint32 Count, const bool bRecent)
{
    const UKalmalaRecipeCatalogue* Catalogue = UKalmalaRecipeCatalogue::Get();
    const FKalmalaRecipe* Recipe = Catalogue ? Catalogue->Find(RecipeId) : nullptr;
    if (!Recipe) return;
    TMap<FName, uint32>* Counts = nullptr;
    FName* Recent = nullptr;
    switch (Kind)
    {
    case EKalmalaCraftingActionKind::BuiltPiece:
        if (UKalmalaCraftingWidget::GetBuildBrowseGroup(Recipe->Output) == 0) return;
        Counts = &BuiltPieceCounts;
        Recent = &RecentBuiltPieceRecipeId;
        break;
    case EKalmalaCraftingActionKind::CookedRecipe:
        if (Recipe->ExperienceSkill != EKalmalaSkill::Cooking) return;
        Counts = &CookedRecipeCounts;
        Recent = &RecentCookedRecipeId;
        break;
    case EKalmalaCraftingActionKind::CraftedItem:
        if (Recipe->ExperienceSkill == EKalmalaSkill::Cooking
            || UKalmalaCraftingWidget::GetBuildBrowseGroup(Recipe->Output) != 0) return;
        Counts = &CraftedItemCounts;
        Recent = &RecentCraftedItemRecipeId;
        break;
    default:
        return;
    }
    if (Count == 0) Counts->Remove(RecipeId);
    else Counts->Add(RecipeId, Count);
    if (bRecent) *Recent = RecipeId;
    else if (*Recent == RecipeId) *Recent = NAME_None;
}
#endif

void UKalmalaCraftingSubsystem::Toggle()
{
    if(!Controller) return;
    if(!Widget) { Widget=CreateWidget<UKalmalaCraftingWidget>(Controller); if(Widget) Widget->AddToPlayerScreen(160); }
    if(Widget) { if(Widget->IsOpen()) Widget->Close(); else Widget->Open(); }
}
bool UKalmalaCraftingSubsystem::CloseIfOpen() { if(!Widget || !Widget->IsOpen()) return false; Widget->Close(); return true; }
void UKalmalaCraftingSubsystem::Release()
{
    if(auto* Input=BoundInput.Get()) for(int32 I=Input->GetNumActionBindings()-1;I>=0;--I)
        if(Input->GetActionBinding(I).ActionDelegate.IsBoundToObject(this)) Input->RemoveActionBinding(I);
    BoundInput.Reset();
    if(Widget) { Widget->Close(); Widget->RemoveFromParent(); Widget=nullptr; }
    if(InteractionPrompt) { InteractionPrompt->RemoveFromParent(); InteractionPrompt=nullptr; }
    StationInteractionModel.Reset(); LastStationInteractionSerial = 0; bHasSeenStationInteraction = false;
    ActivityCraftingComponent.Reset(); LastAcceptedRecipeActivitySequence = 0;
    bHasObservedRecipeActivityComponent = false;
    Controller=nullptr; bVerified=false;
}
void UKalmalaCraftingSubsystem::Deinitialize() { Release(); Super::Deinitialize(); }
