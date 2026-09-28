#include "KalmalaCraftingSubsystem.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaPlacementPreview.h"
#include "KalmalaRecipeCatalogue.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaCharacter.h"
#include "KalmalaSkillProgressionComponent.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaToolLifecycleContract.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/InputComponent.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/InputSettings.h"
#include "Styling/CoreStyle.h"
#include "InputCoreTypes.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"

namespace
{
FString GetReadableToolName(const FName ToolId)
{
    if (ToolId == TEXT("ReedKnife")) return TEXT("Reed Knife");
    if (ToolId == TEXT("FieldHatchet")) return TEXT("Field Hatchet");
    if (ToolId == TEXT("StonePick")) return TEXT("Stone Pick");
    if (ToolId == TEXT("BronzeAxe")) return TEXT("Bronze Axe");
    if (ToolId == TEXT("IronAxe")) return TEXT("Iron Axe");
    return ToolId.ToString();
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

FString BuildSkillProgressText(const AKalmalaCharacter* Character, const TArray<FKalmalaRecipe>& Recipes)
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

    const FKalmalaRecipe* NextUnlock = nullptr;
    int32 SmallestExperienceGap = MAX_int32;
    for (const FKalmalaRecipe& Recipe : Recipes)
    {
        if (!Recipe.bEnabled || Recipe.RequiredSkill == EKalmalaSkill::None
            || !FKalmalaSkillProgressionContract::IsKnownSkill(Recipe.RequiredSkill)
            || Recipe.RequiredSkillLevel < 2
            || Recipe.RequiredSkillLevel > FKalmalaSkillProgressionContract::MaxLevel)
        {
            continue;
        }

        const FKalmalaSkillState* State = FindState(Recipe.RequiredSkill);
        if (!State || State->Level >= Recipe.RequiredSkillLevel) continue;
        const int32 RequiredExperience = FKalmalaSkillProgressionContract::GetExperienceForLevel(Recipe.RequiredSkillLevel);
        const int32 ExperienceGap = RequiredExperience - State->Experience;
        if (ExperienceGap < SmallestExperienceGap)
        {
            SmallestExperienceGap = ExperienceGap;
            NextUnlock = &Recipe;
        }
    }

    if (!NextUnlock)
    {
        Text += TEXT("Next recipe unlock: all current skill-gated recipes are available.\n");
        return Text;
    }

    const FKalmalaSkillState* UnlockSkillState = FindState(NextUnlock->RequiredSkill);
    if (!UnlockSkillState) return Text;
    FString UnlockNames;
    for (const FKalmalaRecipe& Recipe : Recipes)
    {
        if (!Recipe.bEnabled || Recipe.RequiredSkill != NextUnlock->RequiredSkill
            || Recipe.RequiredSkillLevel != NextUnlock->RequiredSkillLevel
            || UnlockSkillState->Level >= Recipe.RequiredSkillLevel)
        {
            continue;
        }
        if (!UnlockNames.IsEmpty()) UnlockNames += TEXT(" + ");
        UnlockNames += Recipe.DisplayName;
    }

    const int32 RequiredExperience = FKalmalaSkillProgressionContract::GetExperienceForLevel(NextUnlock->RequiredSkillLevel);
    Text += FString::Printf(TEXT("Next recipe unlock: %s at %s level %d (%d/%d XP earned).\n"),
        *UnlockNames, GetReadableSkillName(NextUnlock->RequiredSkill), NextUnlock->RequiredSkillLevel,
        UnlockSkillState->Experience, RequiredExperience);

    const FKalmalaRecipe* EarningRecipe = Recipes.FindByPredicate(
        [NextUnlock, UnlockSkillState](const FKalmalaRecipe& Recipe)
        {
            return Recipe.bEnabled && Recipe.ExperienceSkill == NextUnlock->RequiredSkill
                && Recipe.ExperienceAward > 0
                && (Recipe.RequiredSkill == EKalmalaSkill::None
                    || UnlockSkillState->Level >= Recipe.RequiredSkillLevel);
        });
    if (EarningRecipe)
    {
        Text += FString::Printf(TEXT("Accepted %s requests award +%d %s XP each; one batch still earns once.\n"),
            *EarningRecipe->DisplayName, EarningRecipe->ExperienceAward,
            GetReadableSkillName(NextUnlock->RequiredSkill));
    }
    return Text;
}
}

void UKalmalaCraftingWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized(); SetIsFocusable(true);
    auto* Border = WidgetTree->ConstructWidget<UBorder>(); Border->SetPadding(FMargin(20));
    Border->SetBrushColor(FLinearColor(.025f,.035f,.04f,.98f));
    auto* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
    auto* Column = WidgetTree->ConstructWidget<UVerticalBox>(); Scroll->AddChild(Column);
    auto AddText = [&](const FString& Text, int32 Size) {
        auto* Label = WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetText(FText::FromString(Text)); Label->SetAutoWrapText(true);
        Label->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), Size));
        Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        WrappedTextBlocks.Add(Label);
        Column->AddChild(Label); return Label;
    };
    AddText(TEXT("Camp crafting"), 28);
    FString CraftKey = TEXT("Unbound");
    for (const FInputActionKeyMapping& Mapping : GetDefault<UInputSettings>()->GetActionMappings())
        if (Mapping.ActionName == TEXT("CraftMenu") && !Mapping.Key.IsGamepadKey()) { CraftKey = Mapping.Key.GetDisplayName().ToString(); break; }
    InstructionsText = AddText(FString::Printf(TEXT("Craft menu input: %s. Up/Down or D-pad: choose. Enter / A: craft. P: local preview. Escape / B: close.\nController Y: place hearth. X: add fuel. RB: light. Mouse buttons and focused keyboard/controller buttons also work.\nSelection is marked with >. Requirements and unavailable reasons are written in text; colour is never the only cue.\n"), *CraftKey), 16);
    RecipesText = AddText(TEXT(""), 18);
    DetailText = AddText(TEXT(""), 18);
    auto AddButton = [&](const TCHAR* Label, UHorizontalBox* Row = nullptr, const TCHAR* Help = nullptr) {
        auto* Button = WidgetTree->ConstructWidget<UButton>(); auto* Text = WidgetTree->ConstructWidget<UTextBlock>();
        Text->SetText(FText::FromString(Label)); Text->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(),18));
        Text->SetColorAndOpacity(FSlateColor(FLinearColor::Black)); Button->SetContent(Text);
        Button->SetToolTipText(FText::FromString(Help ? Help : Label));
        if(Row) { auto* Slot=Row->AddChildToHorizontalBox(Button); Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); Slot->SetPadding(FMargin(2,4)); }
        else Column->AddChild(Button); return Button;
    };
    auto* RecipeActions=WidgetTree->ConstructWidget<UHorizontalBox>(); Column->AddChild(RecipeActions);
    AddButton(TEXT("Previous"),RecipeActions,TEXT("Select the previous recipe. Its ingredients, station, unlock, batch limit, and availability are shown above."))->OnClicked.AddDynamic(this, &ThisClass::Previous);
    AddButton(TEXT("Next"),RecipeActions,TEXT("Select the next recipe. Its ingredients, station, unlock, batch limit, and availability are shown above."))->OnClicked.AddDynamic(this, &ThisClass::Next);
    CraftButton = AddButton(TEXT("Craft one"),RecipeActions,TEXT("Craft batch 1 of the selected recipe. The server checks every requirement and rejected requests preserve ingredients."));
    CraftButton->OnClicked.AddDynamic(this, &ThisClass::Craft);
    AddButton(TEXT("Preview kit"),RecipeActions,TEXT("Show a local placement preview for the selected kit. This does not place it or spend ingredients."))->OnClicked.AddDynamic(this, &ThisClass::Preview);
    AddText(TEXT("\nPlace selected kit uses the derived ground ahead. A hearth also needs one ember bundle; every placement is rechecked by the server.\n"),16);
    auto* FireActions=WidgetTree->ConstructWidget<UHorizontalBox>(); Column->AddChild(FireActions);
    AddButton(TEXT("Place selected kit"),FireActions,TEXT("Ask the server to place the selected camp kit. The server checks ground, range, overlap, and materials."))->OnClicked.AddDynamic(this, &ThisClass::Place);
    AddButton(TEXT("Add fuel bundle"),FireActions,TEXT("Add one fuel bundle to a nearby usable hearth if the server confirms access and capacity."))->OnClicked.AddDynamic(this, &ThisClass::Refuel);
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
    AddText(TEXT("Raised rainproof chest — also uses the shared 16-stack chest interface. Its construction and contents last for this server session until the M9 save migration is approved."), 16);
    StorageText = AddText(TEXT(""), 18);
    AddButton(TEXT("Inspect nearby chest"),nullptr,TEXT("Open the owner-only view of a visible nearby chest. The view closes when the chest is closed or out of reach."))->OnClicked.AddDynamic(this, &ThisClass::InspectStorage);
    auto* StorageActions = WidgetTree->ConstructWidget<UHorizontalBox>(); Column->AddChild(StorageActions);
    AddButton(TEXT("Previous item"), StorageActions)->OnClicked.AddDynamic(this, &ThisClass::PreviousStorageItem);
    AddButton(TEXT("Next item"), StorageActions)->OnClicked.AddDynamic(this, &ThisClass::NextStorageItem);
    AddButton(TEXT("Store one"), StorageActions,TEXT("Ask the server to move one selected item from your pack into the nearby chest."))->OnClicked.AddDynamic(this, &ThisClass::DepositStorage);
    AddButton(TEXT("Take one"), StorageActions,TEXT("Ask the server to move one selected item from the nearby chest into your pack."))->OnClicked.AddDynamic(this, &ThisClass::WithdrawStorage);
    auto* CloseButton=AddButton(TEXT("Close")); CloseButton->OnClicked.AddDynamic(this, &ThisClass::CloseClicked);
    CloseButton->RemoveFromParent();
    auto* Outer=WidgetTree->ConstructWidget<UVerticalBox>();
    Outer->AddChildToVerticalBox(Scroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Outer->AddChildToVerticalBox(CloseButton)->SetPadding(FMargin(0,8,0,0));
    Border->SetContent(Outer); WidgetTree->RootWidget = Border;
    SetVisibility(ESlateVisibility::Collapsed);
}

UKalmalaCraftingComponent* UKalmalaCraftingWidget::Model() const
{
    auto* Pawn = GetOwningPlayerPawn(); return Pawn ? Pawn->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
}

void UKalmalaCraftingWidget::Open()
{
    auto* PC = GetOwningPlayer(); if (bOpen || !PC || PC->IsMoveInputIgnored() || !Model()) return;
    bOpen = true; bPreviousCursor = PC->bShowMouseCursor;
    int32 X, Y; PC->GetViewportSize(X,Y);
    const float Scale = FMath::Max(.1f, UWidgetLayoutLibrary::GetViewportScale(this));
    const float PanelWidth = FMath::Min(840.0f, X / Scale - 32.0f);
    SetDesiredSizeInViewport(FVector2D(PanelWidth, FMath::Min(980.0f, Y / Scale - 32.0f)));
    const float TextWrapWidth = FMath::Max(240.0f, PanelWidth - 64.0f);
    for (UTextBlock* Label : WrappedTextBlocks)
    {
        if (Label) Label->SetWrapTextAt(TextWrapWidth);
    }
    SetAlignmentInViewport(FVector2D(.5,.5)); SetPositionInViewport(FVector2D(X*.5f,Y*.5f), true);
    SetVisibility(ESlateVisibility::Visible); Refresh();
    PC->SetIgnoreMoveInput(true); PC->SetIgnoreLookInput(true); PC->bShowMouseCursor = true;
    FInputModeGameAndUI Mode; Mode.SetWidgetToFocus(TakeWidget()); Mode.SetHideCursorDuringCapture(false); PC->SetInputMode(Mode);
    SetKeyboardFocus();
}

void UKalmalaCraftingWidget::Close()
{
    if (!bOpen) return; bOpen = false; bPlacementPreviewEnabled = false; SetVisibility(ESlateVisibility::Collapsed);
    if (auto* M = Model()) M->ServerCloseStorage();
    if (auto* PC=GetOwningPlayer()) { PC->SetIgnoreMoveInput(false); PC->SetIgnoreLookInput(false); PC->bShowMouseCursor=bPreviousCursor; PC->SetInputMode(FInputModeGameOnly()); }
}

void UKalmalaCraftingWidget::Refresh()
{
    auto* M=Model(); if (!M) { Close(); return; }
    const auto& Recipes=GetDefault<UKalmalaRecipeCatalogue>()->Recipes;
    if (Recipes.IsEmpty()) return; Selected=FMath::Clamp(Selected,0,Recipes.Num()-1);
    const FKalmalaRecipe& SelectedRecipe = Recipes[Selected];
    RecipesText->SetText(FText::FromString(FString::Printf(TEXT("> Recipe %d of %d: %s\n"),
        Selected + 1, Recipes.Num(), *SelectedRecipe.DisplayName)));
    const FString Availability = M->GetRecipeAvailability(SelectedRecipe.RecipeId);
    DetailText->SetText(FText::FromString(M->GetRecipeDescription(SelectedRecipe.RecipeId)
        + TEXT("\nAvailability: ") + Availability + TEXT("\n")
        + BuildSkillProgressText(Cast<AKalmalaCharacter>(GetOwningPlayerPawn()), Recipes)));
    const int32 TextScalePercent = UKalmalaSettingsWidget::ClampTextScale(
        UKalmalaSettingsWidget::GetTextScalePercent());
    const int32 ContrastMode = UKalmalaSettingsWidget::ClampContrastMode(
        UKalmalaSettingsWidget::GetContrastMode());
    if (LastDetailTextScalePercent != TextScalePercent || LastDetailContrastMode != ContrastMode)
    {
        DetailText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(),
            FMath::RoundToInt(18.0f * TextScalePercent / 100.0f)));
        DetailText->SetColorAndOpacity(FSlateColor(ContrastMode != 0
            ? FLinearColor::White : FLinearColor(0.86f, 0.92f, 0.90f, 1.0f)));
        if (UBorder* Background = Cast<UBorder>(WidgetTree->RootWidget))
        {
            Background->SetBrushColor(ContrastMode != 0
                ? FLinearColor(0.0f, 0.0f, 0.0f, 0.98f)
                : FLinearColor(0.025f, 0.035f, 0.04f, 0.98f));
        }
        LastDetailTextScalePercent = TextScalePercent;
        LastDetailContrastMode = ContrastMode;
    }
    if (CraftButton)
    {
        CraftButton->SetToolTipText(FText::FromString(FString::Printf(
            TEXT("Craft batch 1 of %s. Availability: %s. A rejected request preserves ingredients and tool condition."),
            *SelectedRecipe.DisplayName, *Availability)));
    }
    FString PreviewText;
    if (bPlacementPreviewEnabled)
    {
        const FKalmalaPlacementPreview Preview = FKalmalaPlacementPreview::Evaluate(GetWorld(), GetOwningPlayerPawn(), Recipes[Selected].Output);
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
    StateText->SetText(FText::FromString(TEXT("\nNearby hearth (replicated shared state; text does not rely on colour):\n")
        + ToolConditionText + TEXT("\n") + M->GetNearbyFireText()+TEXT("\n")+M->GetNearbyConstructionText()+TEXT("\n")+M->GetNearbyWorkbenchText()+TEXT("\n")+M->GetLastResult()+TEXT("\n")+PreviewText));
    FoodText->SetText(FText::FromString(M->GetFoodText()));
    if (ToolProgressionText) ToolProgressionText->SetText(FText::FromString(M->GetToolProgressionText()));
    const auto* Catalogue = GetDefault<UKalmalaItemCatalogue>();
    SelectedStorageItem = FMath::Clamp(SelectedStorageItem, 0, FMath::Max(0, Catalogue->Items.Num()-1));
    FString ChestText = Catalogue->Items.IsValidIndex(SelectedStorageItem)
        ? TEXT("Selected item: ") + Catalogue->Items[SelectedStorageItem].DisplayName + TEXT("\n") : TEXT("No item catalogue\n");
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
            + StateText->GetText().ToString() + (FoodText ? FoodText->GetText().ToString() : FString())
            + (RepairText ? RepairText->GetText().ToString() : FString())
            + (ToolProgressionText ? ToolProgressionText->GetText().ToString() : FString())
            + (CraftButton ? CraftButton->GetToolTipText().ToString() : FString()) : FString();
}
void UKalmalaCraftingWidget::NativeTick(const FGeometry& G,float D) { Super::NativeTick(G,D); if(bOpen) Refresh(); }
void UKalmalaCraftingWidget::Previous() { const int32 N=GetDefault<UKalmalaRecipeCatalogue>()->Recipes.Num(); if(N) Selected=(Selected+N-1)%N; Refresh(); }
void UKalmalaCraftingWidget::Next() { const int32 N=GetDefault<UKalmalaRecipeCatalogue>()->Recipes.Num(); if(N) Selected=(Selected+1)%N; Refresh(); }
void UKalmalaCraftingWidget::Craft() { const auto& R=GetDefault<UKalmalaRecipeCatalogue>()->Recipes; if(auto* M=Model(); M && R.IsValidIndex(Selected)) M->ServerCraft(R[Selected].RecipeId,1); }
void UKalmalaCraftingWidget::EnablePlacementPreview() { bPlacementPreviewEnabled = true; Refresh(); }
void UKalmalaCraftingWidget::Preview() { EnablePlacementPreview(); }
void UKalmalaCraftingWidget::Place()
{
    const auto& Recipes = GetDefault<UKalmalaRecipeCatalogue>()->Recipes;
    if (auto* M = Model(); M && Recipes.IsValidIndex(Selected))
    {
        const FName Kit = Recipes[Selected].Output;
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
    const int32 Count = GetDefault<UKalmalaItemCatalogue>()->Items.Num();
    if (Count) SelectedStorageItem = (SelectedStorageItem + Count - 1) % Count;
    Refresh();
}
void UKalmalaCraftingWidget::NextStorageItem()
{
    const int32 Count = GetDefault<UKalmalaItemCatalogue>()->Items.Num();
    if (Count) SelectedStorageItem = (SelectedStorageItem + 1) % Count;
    Refresh();
}
void UKalmalaCraftingWidget::DepositStorage()
{
    const auto& Items = GetDefault<UKalmalaItemCatalogue>()->Items;
    if (auto* M=Model(); M && Items.IsValidIndex(SelectedStorageItem)) M->ServerDepositStorage(Items[SelectedStorageItem].ItemId);
}
void UKalmalaCraftingWidget::WithdrawStorage()
{
    const auto& Items = GetDefault<UKalmalaItemCatalogue>()->Items;
    if (auto* M=Model(); M && Items.IsValidIndex(SelectedStorageItem)) M->ServerWithdrawStorage(Items[SelectedStorageItem].ItemId);
}
void UKalmalaCraftingWidget::CloseClicked() { Close(); }
FReply UKalmalaCraftingWidget::NativeOnPreviewKeyDown(const FGeometry& G,const FKeyEvent& E)
{
    const FKey K=E.GetKey();
    if(K==EKeys::Escape || K==EKeys::Gamepad_FaceButton_Right) { Close(); return FReply::Handled(); }
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
#if !UE_BUILD_SHIPPING
    if(!bVerified && PC->GetPawn() && FParse::Param(FCommandLine::Get(),TEXT("KalmalaCraftingTest")))
    {
        if(auto* Input=BoundInput.Get()) for(int32 Index=0;Index<Input->GetNumActionBindings();++Index)
        {
            auto& Binding=Input->GetActionBinding(Index);
            if(Binding.GetActionName()==TEXT("CraftMenu") && Binding.KeyEvent==IE_Pressed) Binding.ActionDelegate.Execute(FKey());
        }
        if(Widget && Widget->IsOpen())
        {
            const auto Text=Widget->GetPresentationText();
            Widget->EnablePlacementPreview();
            const auto PreviewText=Widget->GetPresentationText();
            const bool ToolFeedbackPassed = Text.Contains(TEXT("Tool condition and free repair status (owner-only)"))
                && Text.Contains(TEXT("TOOL PROGRESSION — OWNER ONLY"))
                && Text.Contains(TEXT("target level 1")) && Text.Contains(TEXT("target level 2"))
                && Text.Contains(TEXT("Cost: 4 Splitwood (have "))
                && (Text.Contains(TEXT("Nearby Workbench level 1; required level 1"))
                    || Text.Contains(TEXT("Need a visible same-world Workbench level 1 within 2.5 m")))
                && (Text.Contains(TEXT("Nearby Forge level 2; required level 2"))
                    || Text.Contains(TEXT("Need a visible same-world Forge level 2 within 2.5 m")))
                && Text.Contains(TEXT("Attachments last only for this session until M9 persistence is approved"))
                && Text.Contains(TEXT("Grinding Stone Repair All: interact with a visible same-world Grinding Stone"));
            UE_LOG(LogTemp, Display, TEXT("M9 tool feedback: Passed=%d"), ToolFeedbackPassed);
            const bool Passed=Text.Contains(TEXT("Craft menu input:")) && Text.Contains(TEXT("Up/Down"))
                && Text.Contains(TEXT("Cost:")) && Text.Contains(TEXT("Output:")) && Text.Contains(TEXT("Handcrafted; no station"))
                && Text.Contains(TEXT("Maximum batch:")) && Text.Contains(TEXT("stack limit"))
                && Text.Contains(TEXT("Failure: the availability text below"))
                && Text.Contains(TEXT("SKILL PROGRESS [PRIVATE TO YOU]"))
                && Text.Contains(TEXT("Cooking: Level 1, 0/100 XP to Level 2"))
                && Text.Contains(TEXT("Next recipe unlock: Smoke boar field meat + Smoke deer field meat at Cooking level 2 (0/100 XP earned)"))
                && Text.Contains(TEXT("Accepted roast boar field meat requests award +10 Cooking XP each; one batch still earns once."))
                && Text.Contains(TEXT("Selection is marked with >"))
                && Text.Contains(TEXT("Need 2 Splitwood")) && Text.Contains(TEXT("Free repair: at a visible same-world Workbench or Forge"))
                && Text.Contains(TEXT("Tool condition and free repair status (owner-only)")) && Text.Contains(TEXT("Bronze Axe:")) && Text.Contains(TEXT("Iron Axe:"))
                && Text.Contains(TEXT("Roasted field meat:"))
                && Text.Contains(TEXT("Craft batch 1 of"))
                && Text.Contains(TEXT("A rejected request preserves ingredients and tool condition"))
                && PreviewText.Contains(TEXT("Preview:"))
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
            if(CaptureWait>2)
            {
                UE_LOG(LogTemp, Display, TEXT("Construction feedback: Passed=%d"),
                    M->GetNearbyConstructionText().Contains(TEXT("Health: 50.0 / 100"))
                    && M->GetNearbyConstructionText().Contains(TEXT("Rain-wear limit reached")));
                FScreenshotRequest::RequestScreenshot(CapturePath,true,false); bCaptureRequested=true;
            }
        }
    }
#endif
}
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
    BoundInput.Reset(); if(Widget) { Widget->Close(); Widget->RemoveFromParent(); Widget=nullptr; } Controller=nullptr; bVerified=false;
}
void UKalmalaCraftingSubsystem::Deinitialize() { Release(); Super::Deinitialize(); }
