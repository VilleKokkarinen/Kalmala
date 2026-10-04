#include "KalmalaCraftingSubsystem.h"
#include "Misc/Paths.h"
#include "KalmalaInventorySubsystem.h"
#include "KalmalaInventoryInspectWidget.h"
#include "KalmalaItemDetailWidget.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaUITheme.h"
#include "KalmalaIconWidget.h"
#include "Components/SizeBox.h"
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
}

void UKalmalaStationPromptWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!WidgetTree) return;
    UBorder* Border = WidgetTree->ConstructWidget<UBorder>();
    Border->SetPadding(FMargin(18.0f, 10.0f));
    Border->SetBrushColor(FLinearColor(0.015f, 0.025f, 0.03f, 0.92f));
    PromptText = WidgetTree->ConstructWidget<UTextBlock>();
    PromptText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 20));
    PromptText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    Border->SetContent(PromptText);
    WidgetTree->RootWidget = Border;
    SetVisibility(ESlateVisibility::Collapsed);
}

void UKalmalaStationPromptWidget::SetPrompt(const FString& Text)
{
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
    Column->RemoveChild(RecipesText);
    auto* RecipeRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    auto* IconBox = WidgetTree->ConstructWidget<USizeBox>();
    IconBox->SetWidthOverride(32); IconBox->SetHeightOverride(32);
    SelectedIcon = WidgetTree->ConstructWidget<UKalmalaIconWidget>();
    IconBox->SetContent(SelectedIcon); RecipeRow->AddChild(IconBox);
    RecipeRow->AddChildToHorizontalBox(RecipesText)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Column->AddChild(RecipeRow);
    DetailText = AddText(TEXT(""), 18);
    auto AddButton = [&](const TCHAR* Label, UHorizontalBox* Row = nullptr, const TCHAR* Help = nullptr) {
        auto* Button = WidgetTree->ConstructWidget<UButton>(); auto* Text = WidgetTree->ConstructWidget<UTextBlock>();
        Text->SetText(FText::FromString(Label)); Text->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(),18));
        Text->SetColorAndOpacity(FSlateColor(FLinearColor::Black)); Button->SetContent(Text);
        Button->SetToolTipText(FText::FromString(Help ? Help : Label));
        if(Row) { auto* Slot=Row->AddChildToHorizontalBox(Button); Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); Slot->SetPadding(FMargin(2,4)); }
        else Column->AddChild(Button); return Button;
    };
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

TArray<int32> UKalmalaCraftingWidget::GetVisibleRecipeIndices() const
{
    TArray<int32> Indices;
    const auto& Recipes = UKalmalaRecipeCatalogue::Get()->Recipes;
    for (int32 Index = 0; Index < Recipes.Num(); ++Index)
        if (StationFilterKit.IsNone() || Recipes[Index].RequiredStation.Contains(StationFilterKit)) Indices.Add(Index);
    return Indices;
}

void UKalmalaCraftingWidget::Open() { OpenInternal(NAME_None); }

void UKalmalaCraftingWidget::OpenForStation(const FName StationKit) { OpenInternal(StationKit); }

void UKalmalaCraftingWidget::OpenInternal(const FName StationKit)
{
    if (!StationKit.IsNone() && !IsInWorldCookingStation(StationKit)) return;
    if (bOpen)
    {
        StationFilterKit = StationKit;
        Selected = 0;
        bPlacementPreviewEnabled = false;
        Refresh();
        return;
    }
    auto* PC = GetOwningPlayer(); if (!PC || PC->IsMoveInputIgnored() || !Model()) return;
    const auto* Character = Cast<AKalmalaCharacter>(PC->GetPawn());
    if (!Character || (StationKit.IsNone() && Character->GetCarriedToolLevel(TEXT("ConstructionHammer")) < 1)) return;
    StationFilterKit = StationKit;
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
    if (StationFilterKit.IsNone())
    {
        if (HeaderText) HeaderText->SetText(FText::FromString(TEXT("Construction hammer — Build and craft")));
        if (InstructionsText) InstructionsText->SetText(FText::FromString(GeneralInstructions));
    }
    else
    {
        const FKalmalaItemDefinition* StationItem = UKalmalaItemCatalogue::Get()->FindItem(StationFilterKit);
        const FString StationName = StationItem ? StationItem->DisplayName : StationFilterKit.ToString();
        if (HeaderText) HeaderText->SetText(FText::FromString(StationName + TEXT(" — Cook")));
        if (InstructionsText) InstructionsText->SetText(FText::FromString(
            TEXT("Station recipes. Up/Down or D-pad: choose. Enter / A: cook one. Escape / B: close.\n")
            TEXT("The server requires this placed station and a usable, lit hearth with heat at both the station and you. Ingredients and availability are shown in text.")));
    }
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
    if (!bOpen) return; bOpen = false; bPlacementPreviewEnabled = false; SetVisibility(ESlateVisibility::Collapsed);
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
            Name->SetText(FText::FromString(Recipe.DisplayName));
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
        if (RecipeSlotNames[SlotIndex]->GetText().ToString() != Recipe.DisplayName)
        {
            RecipeSlotNames[SlotIndex]->SetText(FText::FromString(Recipe.DisplayName));
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
            Theme.ApplyPanel(*RecipeSlotCards[SlotIndex], ContrastMode, &NoPanelImage);
            RecipeSlotCards[SlotIndex]->SetPadding(FMargin(Theme.SlotPadding + (bSelected ? 1.0f : 0.0f)));
            RecipeSlotCards[SlotIndex]->SetBrushColor(ContrastMode != 0 ? FLinearColor::White
                : bSelected ? FLinearColor(0.43f, 0.30f, 0.12f, 1.0f)
                : bUnavailable ? FLinearColor(0.055f, 0.065f, 0.075f, 0.98f)
                : FLinearColor(0.075f, 0.10f, 0.115f, 0.98f));
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
    SetKeyboardFocus();
    return bFocused && bChanged && bReturned && HasKeyboardFocus();
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
    const float ScrollOffsetOfEnd = CraftingScrollBox ? CraftingScrollBox->GetScrollOffsetOfEnd() : 0.0f;
    const bool bScrollable = ScrollOffsetOfEnd > 1.0f;
    UE_LOG(LogTemp, Display, TEXT("Build grid input: KeyboardDown=%d KeyboardUp=%d DPadDown=%d DPadUp=%d Scrollable=%d ScrollEnd=%.1f Focused=%d"),
        bKeyboardAdvanced, bKeyboardRestored, bControllerAdvanced, bControllerRestored,
        bScrollable, ScrollOffsetOfEnd, HasKeyboardFocus());
    return bKeyboardAdvanced && bKeyboardRestored && bControllerAdvanced && bControllerRestored && bScrollable;
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
        LastDetailTextScalePercent = TextScalePercent;
        LastDetailContrastMode = ContrastMode;
    }
    if (InventoryInspector) InventoryInspector->SetRows(InspectionRows, TextScalePercent, ContrastMode);
    if (!VisibleIndices.IsEmpty()) Selected=FMath::Clamp(Selected,0,VisibleIndices.Num()-1);
    RefreshRecipeGrid(VisibleIndices, M, TextScalePercent, ContrastMode);
    if (VisibleIndices.IsEmpty())
    {
        RecipesText->SetText(FText::FromString(TEXT("No recipes are configured for this station.\n")));
        DetailText->SetText(FText::GetEmpty());
        if (SelectedIcon) SelectedIcon->SetVisibility(ESlateVisibility::Collapsed);
        if (CraftButton) CraftButton->SetIsEnabled(false);
        return;
    }
    Selected=FMath::Clamp(Selected,0,VisibleIndices.Num()-1);
    const int32 RecipeIndex = VisibleIndices[Selected];
    const FKalmalaRecipe& SelectedRecipe = Recipes[RecipeIndex];
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
    DetailText->SetText(FText::FromString(M->GetRecipeDescription(SelectedRecipe.RecipeId)
        + TEXT("\nAvailability: ") + Availability + TEXT("\n")
        + BuildSkillProgressText(Cast<AKalmalaCharacter>(GetOwningPlayerPawn()))));
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
        CraftButton->SetIsEnabled(SelectedRecipe.bEnabled);
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
    return InstructionsText && RecipesText && DetailText && StateText
        ? InstructionsText->GetText().ToString()+RecipesText->GetText().ToString()+DetailText->GetText().ToString()
            + StateText->GetText().ToString() + (FoodText ? FoodText->GetText().ToString() : FString())
            + (RepairText ? RepairText->GetText().ToString() : FString())
            + (ToolProgressionText ? ToolProgressionText->GetText().ToString() : FString())
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
        StationInteractionModel.Reset();
        LastStationInteractionSerial = 0;
        bHasSeenStationInteraction = false;
    }
    UpdateStationPrompt(PC);
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
            UE_LOG(LogTemp, Display, TEXT("Build slot grid: %s Navigation=%d"), *GridSummary, bGridNavigation);
            auto* InventoryHUD = GetLocalPlayer()->GetSubsystem<UKalmalaInventorySubsystem>();
            const bool bHUDHidden = InventoryHUD && InventoryHUD->IsCraftingMenuSuppressed();
            const bool Passed=bInspection && bHUDHidden && Text.Contains(TEXT("Construction hammer menu input:")) && Text.Contains(TEXT("Up/Down"))
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
    if (bCaptureRequested && ReviewCaptureStage < 6 && Widget
        && FParse::Value(FCommandLine::Get(), TEXT("KalmalaCraftingCapture="), CapturePath))
    {
        CaptureWait += DeltaTime;
        if (CaptureWait > 3.0f)
        {
            CaptureWait = 0;
            if (ReviewCaptureStage == 4)
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

void UKalmalaCraftingSubsystem::UpdateStationPrompt(APlayerController* PlayerController)
{
    if (!PlayerController) return;
    if (!StationPrompt)
    {
        StationPrompt = CreateWidget<UKalmalaStationPromptWidget>(PlayerController);
        if (!StationPrompt) return;
        StationPrompt->AddToPlayerScreen(150);
        StationPrompt->SetDesiredSizeInViewport(FVector2D(520.0f, 64.0f));
        StationPrompt->SetAlignmentInViewport(FVector2D(0.5f, 1.0f));
    }

    int32 ViewportWidth = 0;
    int32 ViewportHeight = 0;
    PlayerController->GetViewportSize(ViewportWidth, ViewportHeight);
    StationPrompt->SetPositionInViewport(FVector2D(ViewportWidth * 0.5f, ViewportHeight * 0.82f), true);
    if (Widget && Widget->IsOpen())
    {
        StationPrompt->SetPrompt(FString());
        return;
    }

    const AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(PlayerController->GetPawn());
    if (!Character || PlayerController->IsMoveInputIgnored())
    {
        StationPrompt->SetPrompt(FString());
        return;
    }

    const UKalmalaCraftingComponent* Crafting = Character->FindComponentByClass<UKalmalaCraftingComponent>();
    const FName StationKit = Crafting ? Crafting->GetLookedAtCookingStationKit() : NAME_None;
    if (!IsInWorldCookingStation(StationKit))
    {
        StationPrompt->SetPrompt(FString());
        return;
    }
    const FKalmalaItemDefinition* Definition = UKalmalaItemCatalogue::Get()->FindItem(StationKit);
    const FString StationName = Definition ? Definition->DisplayName : StationKit.ToString();
    const FString InteractKey = UKalmalaSettingsWidget::GetLocalInputBindingLabel(TEXT("Interact"), false).ToString();
    StationPrompt->SetPrompt(FString::Printf(TEXT("Press %s to use %s"), *InteractKey, *StationName));
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
    BoundInput.Reset();
    if(Widget) { Widget->Close(); Widget->RemoveFromParent(); Widget=nullptr; }
    if(StationPrompt) { StationPrompt->RemoveFromParent(); StationPrompt=nullptr; }
    StationInteractionModel.Reset(); LastStationInteractionSerial = 0; bHasSeenStationInteraction = false;
    Controller=nullptr; bVerified=false;
}
void UKalmalaCraftingSubsystem::Deinitialize() { Release(); Super::Deinitialize(); }
