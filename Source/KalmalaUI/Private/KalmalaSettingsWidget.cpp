#include "KalmalaSettingsWidget.h"
#include "KalmalaUITheme.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Engine/UserInterfaceSettings.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "InputCoreTypes.h"
#include "HAL/PlatformTime.h"
#include "Misc/App.h"
#include "Misc/ConfigCacheIni.h"
#include "Kismet/KismetSystemLibrary.h"

namespace
{
    constexpr FLinearColor BackgroundColour(0.015f, 0.025f, 0.035f, 0.94f);
    constexpr FLinearColor PanelColour(0.055f, 0.08f, 0.10f, 0.98f);
    constexpr TCHAR AudioSettingsSection[] = TEXT("/Script/KalmalaUI.KalmalaSettingsWidget");
    constexpr TCHAR MasterVolumeKey[] = TEXT("LocalMasterVolume");
    constexpr TCHAR RestoreVolumeKey[] = TEXT("LocalRestoreVolume");
    constexpr TCHAR TextScaleKey[] = TEXT("LocalTextScalePercent");
    constexpr TCHAR InterfaceScaleKey[] = TEXT("LocalInterfaceScalePercent");
    constexpr TCHAR ReducedMotionKey[] = TEXT("LocalReducedMotionEnabled");
    constexpr TCHAR ContrastModeKey[] = TEXT("LocalContrastMode");
    constexpr TCHAR FeedbackModeKey[] = TEXT("LocalFeedbackMode");
    constexpr float DefaultMasterVolume = 1.0f;
    constexpr int32 DefaultTextScalePercent = 100;
    constexpr int32 DefaultInterfaceScalePercent = 100;
    constexpr int32 DefaultContrastMode = 0;
    constexpr int32 DefaultFeedbackMode = 0;

    struct FSettingsPalette
    {
        FLinearColor Background;
        FLinearColor Panel;
        FLinearColor Text;
        FLinearColor ButtonBackground;
        FLinearColor ButtonText;
    };

    FSettingsPalette GetSettingsPalette()
    {
        if (UKalmalaSettingsWidget::GetContrastMode() != 0)
        {
            return {
                FLinearColor(0.0f, 0.0f, 0.0f, 0.98f),
                FLinearColor(0.035f, 0.035f, 0.035f, 1.0f),
                FLinearColor::White,
                FLinearColor(0.18f, 0.18f, 0.18f, 1.0f),
                FLinearColor::White
            };
        }

        return {
            BackgroundColour,
            PanelColour,
            FLinearColor(0.86f, 0.92f, 0.90f, 1.0f),
            FLinearColor(0.11f, 0.16f, 0.19f, 1.0f),
            FLinearColor(0.98f, 1.0f, 0.96f, 1.0f)
        };
    }

    void ApplyButtonPalette(UButton* Button)
    {
        if (Button == nullptr) return;
        FKalmalaUITheme::Get().ApplyButton(*Button, UKalmalaSettingsWidget::GetContrastMode());
    }

    enum class ELocalInputMappingKind : uint8
    {
        Action,
        Axis,
        AxisPair
    };

    struct FLocalInputAxisPair
    {
        FKey Positive;
        FKey Negative;
    };

    struct FLocalInputDefinition
    {
        FName Name;
        const TCHAR* Label = TEXT("");
        const TCHAR* PositiveLabel = TEXT("");
        const TCHAR* NegativeLabel = TEXT("");
        ELocalInputMappingKind Kind = ELocalInputMappingKind::Action;
        TArray<FKey> KeyboardKeys;
        TArray<FKey> GamepadKeys;
        TArray<FLocalInputAxisPair> KeyboardPairs;
    };

    FLocalInputDefinition MakeSingleInput(const FName Name, const TCHAR* Label,
        const ELocalInputMappingKind Kind, const TArray<FKey>& KeyboardKeys,
        const TArray<FKey>& GamepadKeys)
    {
        FLocalInputDefinition Definition;
        Definition.Name = Name;
        Definition.Label = Label;
        Definition.Kind = Kind;
        Definition.KeyboardKeys = KeyboardKeys;
        Definition.GamepadKeys = GamepadKeys;
        return Definition;
    }

    FLocalInputDefinition MakeAxisPairInput(const FName Name, const TCHAR* Label,
        const TCHAR* PositiveLabel, const TCHAR* NegativeLabel,
        const TArray<FLocalInputAxisPair>& KeyboardPairs, const TArray<FKey>& GamepadKeys)
    {
        FLocalInputDefinition Definition;
        Definition.Name = Name;
        Definition.Label = Label;
        Definition.PositiveLabel = PositiveLabel;
        Definition.NegativeLabel = NegativeLabel;
        Definition.Kind = ELocalInputMappingKind::AxisPair;
        Definition.KeyboardPairs = KeyboardPairs;
        Definition.GamepadKeys = GamepadKeys;
        return Definition;
    }

    const TArray<FLocalInputDefinition>& GetLocalInputDefinitions()
    {
        static const TArray<FLocalInputDefinition> Definitions = []
        {
            TArray<FLocalInputDefinition> Result;
            Result.Add(MakeAxisPairInput(TEXT("MoveForward"), TEXT("Move forward / back"),
                TEXT("Forward"), TEXT("Backward"),
                { { EKeys::W, EKeys::S }, { EKeys::Up, EKeys::Down }, { EKeys::I, EKeys::K } },
                { EKeys::Gamepad_LeftY }));
            Result.Add(MakeAxisPairInput(TEXT("MoveRight"), TEXT("Move right / left"),
                TEXT("Right"), TEXT("Left"),
                { { EKeys::D, EKeys::A }, { EKeys::Right, EKeys::Left }, { EKeys::L, EKeys::J } },
                { EKeys::Gamepad_LeftX }));
            Result.Add(MakeSingleInput(TEXT("Turn"), TEXT("Look right / left"), ELocalInputMappingKind::Axis,
                { EKeys::MouseX }, { EKeys::Gamepad_RightX }));
            Result.Add(MakeSingleInput(TEXT("LookUp"), TEXT("Look up / down"), ELocalInputMappingKind::Axis,
                { EKeys::MouseY }, { EKeys::Gamepad_RightY }));
            Result.Add(MakeSingleInput(TEXT("Interact"), TEXT("Interact"), ELocalInputMappingKind::Action,
                { EKeys::E, EKeys::F }, { EKeys::Gamepad_FaceButton_Bottom, EKeys::Gamepad_FaceButton_Right }));
            Result.Add(MakeSingleInput(TEXT("Attack"), TEXT("Attack"), ELocalInputMappingKind::Action,
                { EKeys::LeftMouseButton, EKeys::RightMouseButton },
                { EKeys::Gamepad_RightShoulder, EKeys::Gamepad_RightTrigger }));
            Result.Add(MakeSingleInput(TEXT("Jump"), TEXT("Jump"), ELocalInputMappingKind::Action,
                { EKeys::SpaceBar, EKeys::C }, { EKeys::Gamepad_FaceButton_Left, EKeys::Gamepad_FaceButton_Bottom }));
            Result.Add(MakeSingleInput(TEXT("Sprint"), TEXT("Sprint"), ELocalInputMappingKind::Action,
                { EKeys::LeftShift, EKeys::RightShift },
                { EKeys::Gamepad_LeftThumbstick, EKeys::Gamepad_RightThumbstick }));
            Result.Add(MakeSingleInput(TEXT("SettingsMenu"), TEXT("Settings menu"), ELocalInputMappingKind::Action,
                { EKeys::Escape, EKeys::O }, {}));
            Result.Add(MakeSingleInput(TEXT("WorldMap"), TEXT("World map"), ELocalInputMappingKind::Action,
                { EKeys::M, EKeys::N }, { EKeys::Gamepad_Special_Right }));
            Result.Add(MakeSingleInput(TEXT("WorldMapRecenter"), TEXT("Recenter map"), ELocalInputMappingKind::Action,
                { EKeys::R, EKeys::T }, {}));
            Result.Add(MakeSingleInput(TEXT("InventoryMenu"), TEXT("Inventory menu"), ELocalInputMappingKind::Action,
                { EKeys::Tab, EKeys::I }, {}));
            Result.Add(MakeSingleInput(TEXT("CraftMenu"), TEXT("Build and crafting menu"), ELocalInputMappingKind::Action,
                { EKeys::B, EKeys::C }, { EKeys::Gamepad_Special_Left, EKeys::Gamepad_Special_Right }));
            return Result;
        }();
        return Definitions;
    }

    const FLocalInputDefinition* FindLocalInputDefinition(const FName ControlName)
    {
        return GetLocalInputDefinitions().FindByPredicate([ControlName](const FLocalInputDefinition& Definition)
        {
            return Definition.Name == ControlName;
        });
    }

    FString GetLocalInputConfigKey(const FName ControlName, const bool bGamepad)
    {
        return FString::Printf(TEXT("LocalInput_%s_%s"), *ControlName.ToString(),
            bGamepad ? TEXT("Controller") : TEXT("Keyboard"));
    }

    FString GetLocalInputPairKey(const FName ControlName, const bool bGamepad, const bool bPositive)
    {
        return GetLocalInputConfigKey(ControlName, bGamepad) + (bPositive ? TEXT("_Positive") : TEXT("_Negative"));
    }

    bool ReadConfigString(const FString& Key, FString& OutValue)
    {
        return GConfig != nullptr && GConfig->GetString(AudioSettingsSection, *Key, OutValue, GGameUserSettingsIni)
            && !OutValue.IsEmpty();
    }

    FKey ReadStoredKey(const FString& Key)
    {
        FString Value;
        return ReadConfigString(Key, Value) ? FKey(FName(*Value)) : EKeys::Invalid;
    }

    bool IsCandidateKey(const TArray<FKey>& Candidates, const FKey& Key)
    {
        return Key.IsValid() && Candidates.Contains(Key);
    }

    TArray<FKey> GetAxisPairSideCandidates(const FLocalInputDefinition& Definition, const bool bPositiveSide)
    {
        TArray<FKey> Candidates;
        for (const FLocalInputAxisPair& Pair : Definition.KeyboardPairs)
        {
            Candidates.AddUnique(bPositiveSide ? Pair.Positive : Pair.Negative);
        }
        return Candidates;
    }

    bool IsCandidatePair(const FLocalInputDefinition& Definition, const FLocalInputAxisPair& Pair)
    {
        const bool bHasPositive = Definition.KeyboardPairs.ContainsByPredicate([Pair](const FLocalInputAxisPair& Candidate)
        {
            return Candidate.Positive == Pair.Positive;
        });
        const bool bHasNegative = Definition.KeyboardPairs.ContainsByPredicate([Pair](const FLocalInputAxisPair& Candidate)
        {
            return Candidate.Negative == Pair.Negative;
        });
        return bHasPositive && bHasNegative && Pair.Positive != Pair.Negative;
    }

    FString GetKeyDisplayLabel(const FKey& Key)
    {
        if (!Key.IsValid()) return TEXT("Not bound");
        const FString DisplayName = Key.GetDisplayName().ToString();
        return DisplayName.IsEmpty() ? Key.GetFName().ToString() : DisplayName;
    }

    FKey GetDefaultSingleKey(const FLocalInputDefinition& Definition, const bool bGamepad)
    {
        const UInputSettings* Settings = GetDefault<UInputSettings>();
        if (Settings == nullptr) return EKeys::Invalid;

        if (Definition.Kind == ELocalInputMappingKind::Action)
        {
            TArray<FInputActionKeyMapping> Mappings;
            Settings->GetActionMappingByName(Definition.Name, Mappings);
            for (const FInputActionKeyMapping& Mapping : Mappings)
            {
                if (Mapping.Key.IsGamepadKey() == bGamepad) return Mapping.Key;
            }
        }
        else
        {
            TArray<FInputAxisKeyMapping> Mappings;
            Settings->GetAxisMappingByName(Definition.Name, Mappings);
            for (const FInputAxisKeyMapping& Mapping : Mappings)
            {
                if (Mapping.Key.IsGamepadKey() == bGamepad) return Mapping.Key;
            }
        }
        return EKeys::Invalid;
    }

    float GetDefaultAxisScale(const FLocalInputDefinition& Definition, const FKey& Key)
    {
        const UInputSettings* Settings = GetDefault<UInputSettings>();
        if (Settings != nullptr)
        {
            TArray<FInputAxisKeyMapping> Mappings;
            Settings->GetAxisMappingByName(Definition.Name, Mappings);
            for (const FInputAxisKeyMapping& Mapping : Mappings)
            {
                if (Mapping.Key == Key) return Mapping.Scale;
            }
        }
        return 1.0f;
    }

    int32 GetDefaultPairIndex(const FLocalInputDefinition& Definition)
    {
        const UInputSettings* Settings = GetDefault<UInputSettings>();
        if (Settings != nullptr)
        {
            TArray<FInputAxisKeyMapping> Mappings;
            Settings->GetAxisMappingByName(Definition.Name, Mappings);
            for (int32 Index = 0; Index < Definition.KeyboardPairs.Num(); ++Index)
            {
                const FLocalInputAxisPair& Pair = Definition.KeyboardPairs[Index];
                bool bFoundPositive = false;
                bool bFoundNegative = false;
                for (const FInputAxisKeyMapping& Mapping : Mappings)
                {
                    bFoundPositive |= Mapping.Key == Pair.Positive && Mapping.Scale > 0.0f;
                    bFoundNegative |= Mapping.Key == Pair.Negative && Mapping.Scale < 0.0f;
                }
                if (bFoundPositive && bFoundNegative) return Index;
            }
        }
        return 0;
    }

    bool ReadStoredPair(const FLocalInputDefinition& Definition, FLocalInputAxisPair& OutPair)
    {
        OutPair.Positive = ReadStoredKey(GetLocalInputPairKey(Definition.Name, false, true));
        OutPair.Negative = ReadStoredKey(GetLocalInputPairKey(Definition.Name, false, false));
        return IsCandidatePair(Definition, OutPair);
    }

    bool HasStoredInputOverride(const FLocalInputDefinition& Definition, const bool bGamepad)
    {
        if (Definition.Kind == ELocalInputMappingKind::AxisPair && !bGamepad)
        {
            FLocalInputAxisPair Pair;
            return ReadStoredPair(Definition, Pair);
        }
        const FKey StoredKey = ReadStoredKey(GetLocalInputConfigKey(Definition.Name, bGamepad));
        return IsCandidateKey(bGamepad ? Definition.GamepadKeys : Definition.KeyboardKeys, StoredKey);
    }

    void RemoveActionDeviceMappings(UPlayerInput* PlayerInput, const FName ActionName, const bool bGamepad)
    {
        if (PlayerInput == nullptr) return;
        PlayerInput->ActionMappings.RemoveAll([ActionName, bGamepad](const FInputActionKeyMapping& Mapping)
        {
            return Mapping.ActionName == ActionName && Mapping.Key.IsGamepadKey() == bGamepad;
        });
    }

    void RemoveAxisDeviceMappings(UPlayerInput* PlayerInput, const FName AxisName, const bool bGamepad)
    {
        if (PlayerInput == nullptr) return;
        PlayerInput->AxisMappings.RemoveAll([AxisName, bGamepad](const FInputAxisKeyMapping& Mapping)
        {
            return Mapping.AxisName == AxisName && Mapping.Key.IsGamepadKey() == bGamepad;
        });
    }

    const TCHAR* GetAudioCategoryKey(const EKalmalaAudioCategory Category)
    {
        switch (Category)
        {
        case EKalmalaAudioCategory::Ambient: return TEXT("LocalAmbientVolume");
        case EKalmalaAudioCategory::Music: return TEXT("LocalMusicVolume");
        case EKalmalaAudioCategory::InteractionCombat: return TEXT("LocalInteractionCombatVolume");
        default: return nullptr;
        }
    }

    float ReadStoredVolume(const TCHAR* Key, const float DefaultValue)
    {
        float Value = DefaultValue;
        if (GConfig != nullptr)
        {
            GConfig->GetFloat(AudioSettingsSection, Key, Value, GGameUserSettingsIni);
        }
        return FMath::IsFinite(Value) ? FMath::Clamp(Value, 0.0f, 1.0f) : DefaultValue;
    }

    float GetConfiguredApplicationScale()
    {
        static const float ConfiguredScale = []
        {
            const UUserInterfaceSettings* Settings = GetDefault<UUserInterfaceSettings>();
            return Settings != nullptr && FMath::IsFinite(Settings->ApplicationScale)
                && Settings->ApplicationScale > 0.0f ? Settings->ApplicationScale : 1.0f;
        }();
        return ConfiguredScale;
    }
}

UKalmalaControlButton::UKalmalaControlButton(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    OnClicked.AddDynamic(this, &UKalmalaControlButton::HandleButtonClicked);
}

void UKalmalaControlButton::Configure(const FName InControlName, const bool bInGamepad,
    const bool bInAxisPairSide, const bool bInPositiveAxisSide)
{
    ControlName = InControlName;
    bGamepad = bInGamepad;
    bAxisPairSide = bInAxisPairSide;
    bPositiveAxisSide = bInPositiveAxisSide;
}

void UKalmalaControlButton::SetDisplayText(const FText& Text)
{
    UTextBlock* Label = Cast<UTextBlock>(GetContent());
    if (Label == nullptr)
    {
        Label = NewObject<UTextBlock>(this);
        SetContent(Label);
    }
    Label->SetText(Text);
    Label->SetColorAndOpacity(FSlateColor(GetSettingsPalette().ButtonText));
    Label->SetJustification(ETextJustify::Center);
    Label->SetAutoWrapText(true);
    FKalmalaUITheme::Get().ApplyText(*Label, FKalmalaUITheme::Get().BodySize + 3, false,
        UKalmalaSettingsWidget::GetTextScalePercent(), UKalmalaSettingsWidget::GetContrastMode());
}

void UKalmalaControlButton::HandleButtonClicked()
{
    OnControlBindingClicked.Broadcast(ControlName, bGamepad, bAxisPairSide, bPositiveAxisSide);
}

void UKalmalaSettingsWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (ContentBox != nullptr) return;

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("SettingsCanvas"));
    WidgetTree->RootWidget = Canvas;
    UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SettingsBackdrop"));
    BackdropBorder = Backdrop;
    UCanvasPanelSlot* BackdropSlot = Canvas->AddChildToCanvas(Backdrop);
    BackdropSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
    BackdropSlot->SetOffsets(FMargin(0.0f));

    UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SettingsPanel"));
    PanelBorder = Panel;
    Panel->SetPadding(FMargin(40.0f));
    UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Panel);
    PanelCanvasSlot = PanelSlot;
    PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
    PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
    PanelSlot->SetSize(FVector2D(1000.0f, 980.0f));

    ContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsContent"));
    Panel->SetContent(ContentBox);
    ApplyModalPalette(&FKalmalaUITheme::Get().EscapePanelImage);
    SetVisibility(ESlateVisibility::Collapsed);
}

void UKalmalaSettingsWidget::Open(APlayerController* InOwningPlayer)
{
    if (InOwningPlayer == nullptr) return;
    SetOwningPlayer(InOwningPlayer);
    bMenuOpen = true;
    SetVisibility(ESlateVisibility::Visible);
    InOwningPlayer->SetShowMouseCursor(true);
    FInputModeGameAndUI InputMode;
    InputMode.SetWidgetToFocus(TakeWidget());
    InOwningPlayer->SetInputMode(InputMode);
    InOwningPlayer->SetIgnoreMoveInput(true);
    InOwningPlayer->SetIgnoreLookInput(true);
    ShowMainMenu();
    StartOptionsOpeningAnimation();
}

#if !UE_BUILD_SHIPPING
void UKalmalaSettingsWidget::OpenForVerification(APlayerController* InOwningPlayer, const int32 TabIndex)
{
    Open(InOwningPlayer);
    ShowOptionsMenu();
    SetVerificationTab(TabIndex);
}

void UKalmalaSettingsWidget::SetVerificationTab(const int32 TabIndex)
{
    switch (TabIndex)
    {
    case 0: ShowAudioTab(); break;
    case 1: ShowKeyboardAndMouseTab(); break;
    case 2: ShowAccessibilityTab(); break;
    case 4: ShowOptionsMenu(); break;
    case 6: ShowGraphicsTab(); break;
    case 7: ShowControllerTab(); break;
    default: ShowMainMenu(); break;
    }
}

void UKalmalaSettingsWidget::PrepareMenuRender(const int32 TabIndex)
{
    SetVerificationTab(TabIndex);
    ResetOptionsOpeningAnimation();
    SetVisibility(ESlateVisibility::Visible);
}

FString UKalmalaSettingsWidget::GetPanelImagePathForVerification() const
{
    const UObject* Image = PanelBorder != nullptr ? PanelBorder->Background.GetResourceObject() : nullptr;
    return Image != nullptr ? Image->GetPathName() : FString();
}

bool UKalmalaSettingsWidget::HasFocusableContentForVerification() const
{
    if (WidgetTree == nullptr) return false;
    bool bHasFocusableButton = false;
    WidgetTree->ForEachWidget([&bHasFocusableButton](UWidget* Widget)
    {
        if (const UButton* Button = Cast<UButton>(Widget)) bHasFocusableButton |= Button->GetIsFocusable();
    });
    return bHasFocusableButton;
}

bool UKalmalaSettingsWidget::HasFocusedContentForVerification() const
{
    if (WidgetTree == nullptr) return false;
    bool bHasFocusedWidget = false;
    WidgetTree->ForEachWidget([&bHasFocusedWidget](UWidget* Widget)
    {
        if (const UButton* Button = Cast<UButton>(Widget))
            bHasFocusedWidget |= Button->GetIsFocusable() && Button->HasAnyUserFocus();
    });
    return bHasFocusedWidget;
}

bool UKalmalaSettingsWidget::IsHomeMenuSurfaceTransparentForVerification() const
{
    return BackdropBorder != nullptr && PanelBorder != nullptr
        && BackdropBorder->GetBrushColor().A <= KINDA_SMALL_NUMBER
        && PanelBorder->GetBrushColor().A <= KINDA_SMALL_NUMBER
        && PanelBorder->Background.DrawAs == ESlateBrushDrawType::NoDrawType
        && PanelBorder->Background.GetResourceObject() == nullptr;
}

bool UKalmalaSettingsWidget::HasOnlyHomeMenuOptionsForVerification() const
{
    if (ContentBox == nullptr || ContentBox->GetChildrenCount() != 2) return false;
    static const FName ExpectedNames[] = { TEXT("OptionsButton"), TEXT("QuitButton") };
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(ExpectedNames); ++Index)
    {
        const UWidget* Child = ContentBox->GetChildAt(Index);
        if (Child == nullptr || Child->GetFName() != ExpectedNames[Index] || !Cast<UButton>(Child)) return false;
    }
    return true;
}

bool UKalmalaSettingsWidget::HasVisibleFocusedHomeMenuOptionForVerification() const
{
    if (!HasOnlyHomeMenuOptionsForVerification()) return false;
    for (int32 Index = 0; Index < ContentBox->GetChildrenCount(); ++Index)
    {
        const UButton* Button = Cast<UButton>(ContentBox->GetChildAt(Index));
        if (Button == nullptr || !Button->HasAnyUserFocus()) continue;
        const FSlateBrush& FocusedBrush = Button->GetStyle().Normal;
        if (FocusedBrush.OutlineSettings.Width <= 0.0f) return false;
        return GetContrastMode() == 0
            || FocusedBrush.OutlineSettings.Color.GetSpecifiedColor().Equals(FLinearColor::White, 0.01f);
    }
    return false;
}

bool UKalmalaSettingsWidget::HasFocusableControlsForVerification() const
{
    return ControlButtons.Num() > 0 && ControlButtons.ContainsByPredicate([](const UKalmalaControlButton* Button)
    {
        return Button == nullptr || !Button->GetIsFocusable();
    }) == false;
}

float UKalmalaSettingsWidget::GetOptionsPanelPositionYForVerification() const
{
    return PanelCanvasSlot != nullptr ? PanelCanvasSlot->GetPosition().Y : 0.0f;
}

FVector2D UKalmalaSettingsWidget::GetOptionsPanelSizeForVerification() const
{
    return PanelCanvasSlot != nullptr ? PanelCanvasSlot->GetSize() : FVector2D::ZeroVector;
}

bool UKalmalaSettingsWidget::DoesOptionsPanelFitViewportForVerification() const
{
    if (PanelCanvasSlot == nullptr || LastLayoutViewportSize.X <= 0.0f || LastLayoutViewportSize.Y <= 0.0f)
    {
        return false;
    }
    const FVector2D PanelSize = PanelCanvasSlot->GetSize();
    return PanelSize.X <= LastLayoutViewportSize.X - 16.0f
        && PanelSize.Y <= LastLayoutViewportSize.Y - 16.0f;
}

bool UKalmalaSettingsWidget::IsOptionsPanelCenterAnchoredForVerification() const
{
    if (PanelCanvasSlot == nullptr) return false;
    const FAnchors Anchors = PanelCanvasSlot->GetAnchors();
    return Anchors.Minimum.Equals(FVector2D(0.5f, 0.5f))
        && Anchors.Maximum.Equals(FVector2D(0.5f, 0.5f));
}
#endif

void UKalmalaSettingsWidget::StartOptionsOpeningAnimation()
{
    const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
    bOptionsOpeningAnimationActive = Theme.ShouldAnimateOptionsOpening();
    OptionsOpeningElapsed = 0.0f;
    if (PanelCanvasSlot != nullptr)
    {
        PanelCanvasSlot->SetPosition(FVector2D(0.0f, Theme.OptionsOpeningOffset(0.0f)));
    }
}

void UKalmalaSettingsWidget::ResetOptionsOpeningAnimation()
{
    bOptionsOpeningAnimationActive = false;
    OptionsOpeningElapsed = 0.0f;
    if (PanelCanvasSlot != nullptr) PanelCanvasSlot->SetPosition(FVector2D::ZeroVector);
}

void UKalmalaSettingsWidget::Close()
{
    FlushAudioSettingsIfDirty();
    if (!bMenuOpen) return;
    bMenuOpen = false;
    ResetOptionsOpeningAnimation();
    SetVisibility(ESlateVisibility::Collapsed);
    if (APlayerController* Controller = GetOwningPlayer())
    {
        Controller->SetShowMouseCursor(false);
        Controller->SetInputMode(FInputModeGameOnly());
        Controller->SetIgnoreMoveInput(false);
        Controller->SetIgnoreLookInput(false);
    }
}

void UKalmalaSettingsWidget::ApplyModalPalette(const FString* ImageOverride)
{
    const FSettingsPalette Palette = GetSettingsPalette();
    if (BackdropBorder != nullptr) BackdropBorder->SetBrushColor(Palette.Background);
    const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
    const FString* SelectedImage = ImageOverride;
    if (SelectedImage == nullptr || SelectedImage->IsEmpty()) SelectedImage = &Theme.EscapePanelImage;
    if (SelectedImage->IsEmpty()) SelectedImage = nullptr;
    if (PanelBorder != nullptr) Theme.ApplyPanel(*PanelBorder, GetContrastMode(), SelectedImage);
}

void UKalmalaSettingsWidget::ApplyHomeMenuPalette()
{
    const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
    if (GetContrastMode() != 0)
    {
        ApplyModalPalette(&Theme.EscapePanelImage);
        return;
    }

    if (BackdropBorder != nullptr) BackdropBorder->SetBrushColor(FLinearColor::Transparent);
    if (PanelBorder != nullptr)
    {
        const FString NoImage;
        Theme.ApplyPanel(*PanelBorder, GetContrastMode(), &NoImage);
        // A transparent tint alone can leave the rounded brush's outline visible.
        FSlateBrush HomeBrush = PanelBorder->Background;
        HomeBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
        PanelBorder->SetBrush(HomeBrush);
        PanelBorder->SetBrushColor(FLinearColor::Transparent);
    }
}

int32 UKalmalaSettingsWidget::ClampViewDistanceQuality(const int32 Quality)
{
    return FMath::Clamp(Quality, 0, 3);
}

float UKalmalaSettingsWidget::ClampMasterVolume(const float Volume)
{
    return FMath::IsFinite(Volume) ? FMath::Clamp(Volume, 0.0f, 1.0f) : DefaultMasterVolume;
}

float UKalmalaSettingsWidget::GetStoredMasterVolume()
{
    return ReadStoredVolume(MasterVolumeKey, DefaultMasterVolume);
}

bool UKalmalaSettingsWidget::IsAudioMuted()
{
    return GetStoredMasterVolume() <= 0.0f;
}

void UKalmalaSettingsWidget::SetMasterVolume(const float Volume, const bool bFlush)
{
    const float ClampedVolume = ClampMasterVolume(Volume);
    if (GConfig != nullptr)
    {
        const float ExistingVolume = GetStoredMasterVolume();
        const float RestoreVolume = ClampedVolume > 0.0f ? ClampedVolume
            : (ExistingVolume > 0.0f ? ExistingVolume : ReadStoredVolume(RestoreVolumeKey, DefaultMasterVolume));
        GConfig->SetFloat(AudioSettingsSection, MasterVolumeKey, ClampedVolume, GGameUserSettingsIni);
        GConfig->SetFloat(AudioSettingsSection, RestoreVolumeKey, RestoreVolume, GGameUserSettingsIni);
        if (bFlush) GConfig->Flush(false, GGameUserSettingsIni);
    }
    FApp::SetVolumeMultiplier(ClampedVolume);
}

void UKalmalaSettingsWidget::ToggleAudioMute()
{
    if (IsAudioMuted())
    {
        const float RestoreVolume = ReadStoredVolume(RestoreVolumeKey, DefaultMasterVolume);
        SetMasterVolume(RestoreVolume > 0.0f ? RestoreVolume : DefaultMasterVolume);
        return;
    }

    SetMasterVolume(0.0f);
}

void UKalmalaSettingsWidget::ApplySavedMasterVolume()
{
    FApp::SetVolumeMultiplier(GetStoredMasterVolume());
}

float UKalmalaSettingsWidget::ClampAudioCategoryVolume(const float Volume)
{
    return FMath::IsFinite(Volume) ? FMath::Clamp(Volume, 0.0f, 1.0f) : DefaultMasterVolume;
}

float UKalmalaSettingsWidget::GetAudioCategoryVolume(const EKalmalaAudioCategory Category)
{
    const TCHAR* Key = GetAudioCategoryKey(Category);
    return Key != nullptr ? ReadStoredVolume(Key, DefaultMasterVolume) : DefaultMasterVolume;
}

void UKalmalaSettingsWidget::SetAudioCategoryVolume(const EKalmalaAudioCategory Category, const float Volume,
    const bool bFlush)
{
    const TCHAR* Key = GetAudioCategoryKey(Category);
    if (Key == nullptr || GConfig == nullptr)
    {
        return;
    }

    GConfig->SetFloat(AudioSettingsSection, Key, ClampAudioCategoryVolume(Volume), GGameUserSettingsIni);
    if (bFlush) GConfig->Flush(false, GGameUserSettingsIni);
}

int32 UKalmalaSettingsWidget::ClampTextScale(const int32 Percent)
{
    constexpr int32 Choices[] = { 100, 125, 150 };
    int32 ClosestChoice = Choices[0];
    int32 ClosestDistance = FMath::Abs(Percent - ClosestChoice);
    for (const int32 Choice : Choices)
    {
        const int32 Distance = FMath::Abs(Percent - Choice);
        if (Distance < ClosestDistance)
        {
            ClosestChoice = Choice;
            ClosestDistance = Distance;
        }
    }
    return ClosestChoice;
}

int32 UKalmalaSettingsWidget::GetTextScalePercent()
{
    int32 Value = DefaultTextScalePercent;
    if (GConfig != nullptr)
    {
        GConfig->GetInt(AudioSettingsSection, TextScaleKey, Value, GGameUserSettingsIni);
    }
    return ClampTextScale(Value);
}

void UKalmalaSettingsWidget::SetTextScalePercent(const int32 Percent)
{
    if (GConfig == nullptr) return;
    GConfig->SetInt(AudioSettingsSection, TextScaleKey, ClampTextScale(Percent), GGameUserSettingsIni);
    GConfig->Flush(false, GGameUserSettingsIni);
}

int32 UKalmalaSettingsWidget::ClampContrastMode(const int32 Mode)
{
    return FMath::Clamp(Mode, 0, 1);
}

int32 UKalmalaSettingsWidget::GetContrastMode()
{
    int32 Value = DefaultContrastMode;
    if (GConfig != nullptr)
    {
        GConfig->GetInt(AudioSettingsSection, ContrastModeKey, Value, GGameUserSettingsIni);
    }
    return ClampContrastMode(Value);
}

void UKalmalaSettingsWidget::SetContrastMode(const int32 Mode)
{
    if (GConfig == nullptr) return;
    GConfig->SetInt(AudioSettingsSection, ContrastModeKey, ClampContrastMode(Mode), GGameUserSettingsIni);
    GConfig->Flush(false, GGameUserSettingsIni);
}

int32 UKalmalaSettingsWidget::ClampFeedbackMode(const int32 Mode)
{
    return FMath::Clamp(Mode, 0, 1);
}

int32 UKalmalaSettingsWidget::GetFeedbackMode()
{
    int32 Value = DefaultFeedbackMode;
    if (GConfig != nullptr)
    {
        GConfig->GetInt(AudioSettingsSection, FeedbackModeKey, Value, GGameUserSettingsIni);
    }
    return ClampFeedbackMode(Value);
}

void UKalmalaSettingsWidget::SetFeedbackMode(const int32 Mode)
{
    if (GConfig == nullptr) return;
    GConfig->SetInt(AudioSettingsSection, FeedbackModeKey, ClampFeedbackMode(Mode), GGameUserSettingsIni);
    GConfig->Flush(false, GGameUserSettingsIni);
}

int32 UKalmalaSettingsWidget::GetRemappableControlCount()
{
    return GetLocalInputDefinitions().Num();
}

FName UKalmalaSettingsWidget::GetRemappableControlName(const int32 Index)
{
    const TArray<FLocalInputDefinition>& Definitions = GetLocalInputDefinitions();
    return Definitions.IsValidIndex(Index) ? Definitions[Index].Name : NAME_None;
}

FText UKalmalaSettingsWidget::GetRemappableControlLabel(const FName ControlName)
{
    const FLocalInputDefinition* Definition = FindLocalInputDefinition(ControlName);
    return Definition != nullptr ? FText::FromString(Definition->Label) : FText::FromString(TEXT("Unknown control"));
}

FText UKalmalaSettingsWidget::GetLocalInputBindingLabel(const FName ControlName, const bool bGamepad,
    const bool bAxisPairSide, const bool bPositiveAxisSide)
{
    const FLocalInputDefinition* Definition = FindLocalInputDefinition(ControlName);
    if (Definition == nullptr) return FText::FromString(TEXT("Not bound"));

    if (bAxisPairSide)
    {
        if (bGamepad || Definition->Kind != ELocalInputMappingKind::AxisPair)
            return FText::FromString(TEXT("Not bound"));

        FLocalInputAxisPair Pair;
        if (!ReadStoredPair(*Definition, Pair))
        {
            const int32 DefaultIndex = GetDefaultPairIndex(*Definition);
            if (Definition->KeyboardPairs.IsValidIndex(DefaultIndex)) Pair = Definition->KeyboardPairs[DefaultIndex];
        }
        const FKey Key = bPositiveAxisSide ? Pair.Positive : Pair.Negative;
        return IsCandidateKey(GetAxisPairSideCandidates(*Definition, bPositiveAxisSide), Key)
            ? FText::FromString(GetKeyDisplayLabel(Key)) : FText::FromString(TEXT("Not bound"));
    }

    if (Definition->Kind == ELocalInputMappingKind::AxisPair && !bGamepad)
    {
        FLocalInputAxisPair Pair;
        if (!ReadStoredPair(*Definition, Pair))
        {
            const int32 DefaultIndex = GetDefaultPairIndex(*Definition);
            if (Definition->KeyboardPairs.IsValidIndex(DefaultIndex)) Pair = Definition->KeyboardPairs[DefaultIndex];
        }
        if (IsCandidatePair(*Definition, Pair))
        {
            return FText::FromString(FString::Printf(TEXT("%s / %s"),
                *GetKeyDisplayLabel(Pair.Positive), *GetKeyDisplayLabel(Pair.Negative)));
        }
        return FText::FromString(TEXT("Not bound"));
    }

    const TArray<FKey>& Candidates = bGamepad ? Definition->GamepadKeys : Definition->KeyboardKeys;
    FKey Key = ReadStoredKey(GetLocalInputConfigKey(ControlName, bGamepad));
    if (!IsCandidateKey(Candidates, Key)) Key = GetDefaultSingleKey(*Definition, bGamepad);
    if (!IsCandidateKey(Candidates, Key) && Candidates.Num() > 0) Key = Candidates[0];
    return FText::FromString(GetKeyDisplayLabel(Key));
}

bool UKalmalaSettingsWidget::SetLocalInputBinding(const FName ControlName, const bool bGamepad, const FKey& Key,
    const bool bAxisPairSide, const bool bPositiveAxisSide)
{
    const FLocalInputDefinition* Definition = FindLocalInputDefinition(ControlName);
    if (Definition == nullptr || GConfig == nullptr) return false;

    if (Definition->Kind == ELocalInputMappingKind::AxisPair)
    {
        if (bGamepad || !bAxisPairSide
            || !IsCandidateKey(GetAxisPairSideCandidates(*Definition, bPositiveAxisSide), Key)) return false;

        FLocalInputAxisPair Pair;
        if (!ReadStoredPair(*Definition, Pair))
        {
            const int32 DefaultIndex = GetDefaultPairIndex(*Definition);
            if (!Definition->KeyboardPairs.IsValidIndex(DefaultIndex)) return false;
            Pair = Definition->KeyboardPairs[DefaultIndex];
        }
        if (bPositiveAxisSide) Pair.Positive = Key;
        else Pair.Negative = Key;
        if (!IsCandidatePair(*Definition, Pair)) return false;

        GConfig->SetString(AudioSettingsSection, *GetLocalInputPairKey(ControlName, false, true),
            *Pair.Positive.GetFName().ToString(), GGameUserSettingsIni);
        GConfig->SetString(AudioSettingsSection, *GetLocalInputPairKey(ControlName, false, false),
            *Pair.Negative.GetFName().ToString(), GGameUserSettingsIni);
        GConfig->Flush(false, GGameUserSettingsIni);
        return true;
    }

    if (bAxisPairSide) return false;
    const TArray<FKey>& Candidates = bGamepad ? Definition->GamepadKeys : Definition->KeyboardKeys;
    if (!IsCandidateKey(Candidates, Key)) return false;

    GConfig->SetString(AudioSettingsSection, *GetLocalInputConfigKey(ControlName, bGamepad),
        *Key.GetFName().ToString(), GGameUserSettingsIni);
    GConfig->Flush(false, GGameUserSettingsIni);
    return true;
}

void UKalmalaSettingsWidget::CycleLocalInputBinding(APlayerController* Controller, const FName ControlName,
    const bool bGamepad, const bool bAxisPairSide, const bool bPositiveAxisSide)
{
    const FLocalInputDefinition* Definition = FindLocalInputDefinition(ControlName);
    if (Definition == nullptr || GConfig == nullptr) return;

    if (Definition->Kind == ELocalInputMappingKind::AxisPair && bAxisPairSide && !bGamepad)
    {
        const TArray<FKey> Candidates = GetAxisPairSideCandidates(*Definition, bPositiveAxisSide);
        if (Candidates.Num() < 2) return;

        FLocalInputAxisPair CurrentPair;
        if (!ReadStoredPair(*Definition, CurrentPair))
        {
            const int32 DefaultIndex = GetDefaultPairIndex(*Definition);
            if (!Definition->KeyboardPairs.IsValidIndex(DefaultIndex)) return;
            CurrentPair = Definition->KeyboardPairs[DefaultIndex];
        }
        const FKey CurrentKey = bPositiveAxisSide ? CurrentPair.Positive : CurrentPair.Negative;
        int32 CurrentIndex = Candidates.IndexOfByKey(CurrentKey);
        if (CurrentIndex == INDEX_NONE) CurrentIndex = 0;
        if (SetLocalInputBinding(ControlName, false, Candidates[(CurrentIndex + 1) % Candidates.Num()],
            true, bPositiveAxisSide))
        {
            ApplySavedInputBindings(Controller);
        }
        return;
    }

    if (Definition->Kind == ELocalInputMappingKind::AxisPair && !bGamepad)
    {
        if (Definition->KeyboardPairs.Num() < 2) return;
        FLocalInputAxisPair CurrentPair;
        int32 CurrentIndex = GetDefaultPairIndex(*Definition);
        if (ReadStoredPair(*Definition, CurrentPair))
        {
            CurrentIndex = Definition->KeyboardPairs.IndexOfByPredicate([CurrentPair](const FLocalInputAxisPair& Pair)
            {
                return Pair.Positive == CurrentPair.Positive && Pair.Negative == CurrentPair.Negative;
            });
            if (CurrentIndex == INDEX_NONE) CurrentIndex = 0;
        }
        const FLocalInputAxisPair& NextPair = Definition->KeyboardPairs[(CurrentIndex + 1) % Definition->KeyboardPairs.Num()];
        GConfig->SetString(AudioSettingsSection, *GetLocalInputPairKey(ControlName, false, true),
            *NextPair.Positive.GetFName().ToString(), GGameUserSettingsIni);
        GConfig->SetString(AudioSettingsSection, *GetLocalInputPairKey(ControlName, false, false),
            *NextPair.Negative.GetFName().ToString(), GGameUserSettingsIni);
        GConfig->Flush(false, GGameUserSettingsIni);
        ApplySavedInputBindings(Controller);
        return;
    }

    const TArray<FKey>& Candidates = bGamepad ? Definition->GamepadKeys : Definition->KeyboardKeys;
    if (Candidates.Num() < 2) return;
    FKey CurrentKey = ReadStoredKey(GetLocalInputConfigKey(ControlName, bGamepad));
    if (!IsCandidateKey(Candidates, CurrentKey)) CurrentKey = GetDefaultSingleKey(*Definition, bGamepad);
    int32 CurrentIndex = Candidates.IndexOfByKey(CurrentKey);
    if (CurrentIndex == INDEX_NONE) CurrentIndex = 0;
    SetLocalInputBinding(ControlName, bGamepad, Candidates[(CurrentIndex + 1) % Candidates.Num()]);
    ApplySavedInputBindings(Controller);
}

void UKalmalaSettingsWidget::ApplySavedInputBindings(APlayerController* Controller)
{
    if (Controller == nullptr || !Controller->IsLocalController() || Controller->PlayerInput == nullptr) return;

    UPlayerInput* PlayerInput = Controller->PlayerInput;
    PlayerInput->ForceRebuildingKeyMaps(true);
    for (const FLocalInputDefinition& Definition : GetLocalInputDefinitions())
    {
        if (Definition.Kind == ELocalInputMappingKind::AxisPair)
        {
            FLocalInputAxisPair Pair;
            if (ReadStoredPair(Definition, Pair))
            {
                RemoveAxisDeviceMappings(PlayerInput, Definition.Name, false);
                PlayerInput->AxisMappings.Add(FInputAxisKeyMapping(Definition.Name, Pair.Positive, 1.0f));
                PlayerInput->AxisMappings.Add(FInputAxisKeyMapping(Definition.Name, Pair.Negative, -1.0f));
            }
        }

        if (!HasStoredInputOverride(Definition, true) && !HasStoredInputOverride(Definition, false)) continue;
        if (Definition.Kind == ELocalInputMappingKind::Action)
        {
            for (const bool bGamepad : { false, true })
            {
                if (!HasStoredInputOverride(Definition, bGamepad)) continue;
                const FKey Key = ReadStoredKey(GetLocalInputConfigKey(Definition.Name, bGamepad));
                RemoveActionDeviceMappings(PlayerInput, Definition.Name, bGamepad);
                PlayerInput->ActionMappings.Add(FInputActionKeyMapping(Definition.Name, Key));
                if (!bGamepad && Definition.Name == TEXT("SettingsMenu"))
                {
                    // Keep the modal safety path available even after remapping the alternate key.
                    PlayerInput->ActionMappings.Add(FInputActionKeyMapping(Definition.Name, EKeys::Escape));
                }
            }
        }
        else if (Definition.Kind == ELocalInputMappingKind::Axis)
        {
            for (const bool bGamepad : { false, true })
            {
                if (!HasStoredInputOverride(Definition, bGamepad)) continue;
                const FKey Key = ReadStoredKey(GetLocalInputConfigKey(Definition.Name, bGamepad));
                RemoveAxisDeviceMappings(PlayerInput, Definition.Name, bGamepad);
                PlayerInput->AxisMappings.Add(FInputAxisKeyMapping(Definition.Name, Key,
                    GetDefaultAxisScale(Definition, Key)));
            }
        }
        else if (HasStoredInputOverride(Definition, true))
        {
            const FKey Key = ReadStoredKey(GetLocalInputConfigKey(Definition.Name, true));
            RemoveAxisDeviceMappings(PlayerInput, Definition.Name, true);
            PlayerInput->AxisMappings.Add(FInputAxisKeyMapping(Definition.Name, Key,
                GetDefaultAxisScale(Definition, Key)));
        }
    }
    PlayerInput->ForceRebuildingKeyMaps(false);
}

void UKalmalaSettingsWidget::RestoreDefaultInputBindings(APlayerController* Controller)
{
    if (GConfig != nullptr)
    {
        for (const FLocalInputDefinition& Definition : GetLocalInputDefinitions())
        {
            GConfig->RemoveKey(AudioSettingsSection, *GetLocalInputConfigKey(Definition.Name, false), GGameUserSettingsIni);
            GConfig->RemoveKey(AudioSettingsSection, *GetLocalInputConfigKey(Definition.Name, true), GGameUserSettingsIni);
            GConfig->RemoveKey(AudioSettingsSection, *GetLocalInputPairKey(Definition.Name, false, true), GGameUserSettingsIni);
            GConfig->RemoveKey(AudioSettingsSection, *GetLocalInputPairKey(Definition.Name, false, false), GGameUserSettingsIni);
        }
        GConfig->Flush(false, GGameUserSettingsIni);
    }
    ApplySavedInputBindings(Controller);
}

UTextBlock* UKalmalaSettingsWidget::AddLabel(UVerticalBox* Parent, const FText& Label, const float FontSize)
{
    UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Text->SetText(Label);
    Text->SetColorAndOpacity(FSlateColor(GetSettingsPalette().Text));
    Text->SetAutoWrapText(true);
    const auto& Theme = FKalmalaUITheme::Get();
    const bool bHeading = FontSize >= 23;
    Theme.ApplyText(*Text, FMath::RoundToInt(FontSize) + (bHeading ? Theme.EmphasisSize - 17 : Theme.BodySize - 13),
        bHeading, GetTextScalePercent(), GetContrastMode());
    UVerticalBoxSlot* BoxSlot = Parent->AddChildToVerticalBox(Text);
    BoxSlot->SetPadding(FMargin(4.0f, 8.0f));
    return Text;
}

UButton* UKalmalaSettingsWidget::AddButton(UVerticalBox* Parent, const FText& Label, const FName Name)
{
    UButton* Button = WidgetTree->ConstructWidget<UKalmalaThemedButton>(UKalmalaThemedButton::StaticClass(), Name);
    ApplyButtonPalette(Button);
    UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Text->SetText(Label);
    Text->SetColorAndOpacity(FSlateColor(GetSettingsPalette().ButtonText));
    Text->SetAutoWrapText(true);
    Text->SetJustification(ETextJustify::Center);
    FKalmalaUITheme::Get().ApplyText(*Text, FKalmalaUITheme::Get().BodySize + 8, false, GetTextScalePercent(), GetContrastMode());
    Button->SetContent(Text);
    UVerticalBoxSlot* BoxSlot = Parent->AddChildToVerticalBox(Button);
    BoxSlot->SetPadding(FMargin(4.0f, 7.0f));
    BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    FKalmalaUITheme::Get().SizeButtonToContent(*Button);
    return Button;
}

int32 UKalmalaSettingsWidget::ClampInterfaceScalePercent(const int32 Percent)
{
    constexpr int32 Choices[] = { 80, 90, 100, 110, 120 };
    int32 ClosestChoice = Choices[0];
    int32 ClosestDistance = FMath::Abs(Percent - ClosestChoice);
    for (const int32 Choice : Choices)
    {
        const int32 Distance = FMath::Abs(Percent - Choice);
        if (Distance < ClosestDistance)
        {
            ClosestChoice = Choice;
            ClosestDistance = Distance;
        }
    }
    return ClosestChoice;
}

int32 UKalmalaSettingsWidget::GetInterfaceScalePercent()
{
    int32 Value = DefaultInterfaceScalePercent;
    if (GConfig != nullptr)
    {
        GConfig->GetInt(AudioSettingsSection, InterfaceScaleKey, Value, GGameUserSettingsIni);
    }
    return ClampInterfaceScalePercent(Value);
}

void UKalmalaSettingsWidget::SetInterfaceScalePercent(const int32 Percent)
{
    if (GConfig == nullptr) return;
    GConfig->SetInt(AudioSettingsSection, InterfaceScaleKey, ClampInterfaceScalePercent(Percent), GGameUserSettingsIni);
    GConfig->Flush(false, GGameUserSettingsIni);
    ApplySavedInterfaceScale();
}

void UKalmalaSettingsWidget::ApplySavedInterfaceScale()
{
    UUserInterfaceSettings* Settings = GetMutableDefault<UUserInterfaceSettings>();
    if (Settings == nullptr) return;
    Settings->ApplicationScale = GetConfiguredApplicationScale() * GetInterfaceScalePercent() / 100.0f;
}

float UKalmalaSettingsWidget::GetThemeDefaultApplicationScale()
{
    return GetConfiguredApplicationScale();
}

float UKalmalaSettingsWidget::GetAppliedApplicationScale()
{
    const UUserInterfaceSettings* Settings = GetDefault<UUserInterfaceSettings>();
    return Settings != nullptr ? Settings->ApplicationScale : 1.0f;
}

bool UKalmalaSettingsWidget::IsReducedMotionEnabled()
{
    bool bEnabled = false;
    if (GConfig != nullptr)
    {
        GConfig->GetBool(AudioSettingsSection, ReducedMotionKey, bEnabled, GGameUserSettingsIni);
    }
    return bEnabled;
}

void UKalmalaSettingsWidget::SetReducedMotionEnabled(const bool bEnabled)
{
    if (GConfig == nullptr) return;
    GConfig->SetBool(AudioSettingsSection, ReducedMotionKey, bEnabled, GGameUserSettingsIni);
    GConfig->Flush(false, GGameUserSettingsIni);
}

void UKalmalaSettingsWidget::SelectOptionsTab(const int32 SelectedIndex)
{
    LastOptionsTabIndex = FMath::Clamp(SelectedIndex, 0, 4);
    static const TCHAR* TabNames[] = {
        TEXT("Keyboard & Mouse"), TEXT("Controller"), TEXT("Graphics"), TEXT("Audio"), TEXT("Accessibility")
    };
    for (int32 Index = 0; Index < OptionsTabButtons.Num(); ++Index)
    {
        UKalmalaThemedButton* Tab = OptionsTabButtons[Index];
        if (!Tab) continue;
        const bool bSelected = Index == SelectedIndex;
        Tab->SetInteractionSelected(bSelected);
        if (UTextBlock* Label = Cast<UTextBlock>(Tab->GetContent()))
        {
            Label->SetText(FText::FromString(FString(bSelected ? TEXT("> ") : TEXT("")) + TabNames[Index]));
        }
    }
}

void UKalmalaSettingsWidget::ShowMainMenu()
{
    OptionsTabButtons.Reset();
    ApplyHomeMenuPalette();
    ContentBox->ClearChildren();
    UButton* Options = AddButton(ContentBox, FText::FromString(TEXT("Options")), TEXT("OptionsButton"));
    Options->OnClicked.AddDynamic(this, &ThisClass::HandleOptionsClicked);
    UButton* Quit = AddButton(ContentBox, FText::FromString(TEXT("Quit")), TEXT("QuitButton"));
    Quit->OnClicked.AddDynamic(this, &ThisClass::HandleQuitClicked);
    Options->SetUserFocus(GetOwningPlayer());
    Options->SetKeyboardFocus();
}

void UKalmalaSettingsWidget::ShowOptionsMenu()
{
    OptionsTabButtons.Reset();
    ContentBox->ClearChildren();
    ApplyModalPalette(&FKalmalaUITheme::Get().InventoryPanelImage);
    AddLabel(ContentBox, FText::FromString(TEXT("Options")), 30.0f)->SetJustification(ETextJustify::Center);
    UHorizontalBox* Tabs = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    const auto AddTab = [this, Tabs](const FText& Label, FName Name, FScriptDelegate Delegate)
    {
        UKalmalaThemedButton* Tab = WidgetTree->ConstructWidget<UKalmalaThemedButton>(UKalmalaThemedButton::StaticClass(), Name);
        ApplyButtonPalette(Tab);
        UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        Text->SetText(Label);
        Text->SetColorAndOpacity(FSlateColor(GetSettingsPalette().ButtonText));
        Text->SetJustification(ETextJustify::Center);
        Text->SetAutoWrapText(true);
        FKalmalaUITheme::Get().ApplyText(*Text, FKalmalaUITheme::Get().BodySize + 5, false, GetTextScalePercent(), GetContrastMode());
        Tab->SetContent(Text);
        OptionsTabButtons.Add(Tab);
        Tab->OnClicked.Add(Delegate);
        UHorizontalBoxSlot* Slot = Tabs->AddChildToHorizontalBox(Tab); Slot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic)); Slot->SetPadding(FMargin(3.0f));
        FKalmalaUITheme::Get().SizeButtonToContent(*Tab);
    };
    FScriptDelegate KeyboardDelegate; KeyboardDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ThisClass, HandleKeyboardAndMouseClicked)); AddTab(FText::FromString(TEXT("Keyboard & Mouse")), TEXT("KeyboardMouseTab"), KeyboardDelegate);
    FScriptDelegate ControllerDelegate; ControllerDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ThisClass, HandleControllerClicked)); AddTab(FText::FromString(TEXT("Controller")), TEXT("ControllerTab"), ControllerDelegate);
    FScriptDelegate GraphicsDelegate; GraphicsDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ThisClass, HandleGraphicsClicked)); AddTab(FText::FromString(TEXT("Graphics")), TEXT("GraphicsTab"), GraphicsDelegate);
    FScriptDelegate AudioDelegate; AudioDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ThisClass, HandleAudioClicked)); AddTab(FText::FromString(TEXT("Audio")), TEXT("AudioTab"), AudioDelegate);
    FScriptDelegate AccessibilityDelegate; AccessibilityDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ThisClass, HandleAccessibilityClicked)); AddTab(FText::FromString(TEXT("Accessibility")), TEXT("AccessibilityTab"), AccessibilityDelegate);
    ContentBox->AddChildToVerticalBox(Tabs);
    // Restore presentation and focus without invoking option-changing handlers.
    switch (LastOptionsTabIndex)
    {
    case 1: ShowControllerTab(); break;
    case 2: ShowGraphicsTab(); break;
    case 3: ShowAudioTab(); break;
    case 4: ShowAccessibilityTab(); break;
    default: ShowKeyboardAndMouseTab(); break;
    }
}

void UKalmalaSettingsWidget::ShowGraphicsTab()
{
    SelectOptionsTab(2);
    ApplyModalPalette(&FKalmalaUITheme::Get().InventoryPanelImage);
    while (ContentBox->GetChildrenCount() > 2) ContentBox->RemoveChildAt(2);
    AddLabel(ContentBox, FText::FromString(TEXT("Graphics")), 24.0f);
    UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
    if (Settings == nullptr) return;
    ResolutionChoices = { FIntPoint(1280, 720), FIntPoint(1600, 900), FIntPoint(1920, 1080), FIntPoint(2560, 1440), FIntPoint(3840, 2160) };
    const FIntPoint CurrentResolution = Settings->GetScreenResolution();
    ResolutionChoiceIndex = ResolutionChoices.IndexOfByPredicate([CurrentResolution](const FIntPoint& Choice) { return Choice == CurrentResolution; });
    if (ResolutionChoiceIndex == INDEX_NONE) { ResolutionChoices.Insert(CurrentResolution, 0); ResolutionChoiceIndex = 0; }
    UButton* Resolution = AddButton(ContentBox, FText::GetEmpty(), TEXT("ResolutionButton")); Resolution->OnClicked.AddDynamic(this, &ThisClass::HandleResolutionClicked); ResolutionLabel = Cast<UTextBlock>(Resolution->GetContent());
    UButton* VSync = AddButton(ContentBox, FText::GetEmpty(), TEXT("VSyncButton")); VSync->OnClicked.AddDynamic(this, &ThisClass::HandleVSyncClicked); VSyncLabel = Cast<UTextBlock>(VSync->GetContent());
    UButton* WindowMode = AddButton(ContentBox, FText::GetEmpty(), TEXT("WindowModeButton")); WindowMode->OnClicked.AddDynamic(this, &ThisClass::HandleWindowModeClicked); WindowModeLabel = Cast<UTextBlock>(WindowMode->GetContent());
    UButton* ViewDistance = AddButton(ContentBox, FText::GetEmpty(), TEXT("ViewDistanceButton")); ViewDistance->OnClicked.AddDynamic(this, &ThisClass::HandleViewDistanceClicked); ViewDistanceLabel = Cast<UTextBlock>(ViewDistance->GetContent());
    AddLabel(ContentBox, FText::FromString(TEXT("Changes are applied and saved immediately.")), 15.0f)->SetJustification(ETextJustify::Center);
    UpdateVideoLabels();
    Resolution->SetUserFocus(GetOwningPlayer());
    Resolution->SetKeyboardFocus();
}

void UKalmalaSettingsWidget::ShowAudioTab()
{
    SelectOptionsTab(3);
    ApplyModalPalette(&FKalmalaUITheme::Get().InventoryPanelImage);
    while (ContentBox->GetChildrenCount() > 2) ContentBox->RemoveChildAt(2);
    AddLabel(ContentBox, FText::FromString(TEXT("Audio")), 24.0f);

    struct FVolumeSliderWidgets
    {
        USlider* Slider = nullptr;
        UTextBlock* Value = nullptr;
    };
    const auto AddVolumeSlider = [this](const TCHAR* LabelText, const FName Name, const float InitialValue)
        -> FVolumeSliderWidgets
    {
        const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
        auto* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

        auto* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        Label->SetText(FText::FromString(LabelText));
        Label->SetAutoWrapText(true);
        Theme.ApplyText(*Label, Theme.BodySize + 3, false, GetTextScalePercent(), GetContrastMode());
        auto* LabelSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        LabelSize->SetWidthOverride(230.0f);
        LabelSize->SetContent(Label);
        Row->AddChildToHorizontalBox(LabelSize)->SetPadding(FMargin(4.0f, 6.0f, 10.0f, 6.0f));

        auto* Slider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), Name);
        Slider->SetMinValue(0.0f);
        Slider->SetMaxValue(1.0f);
        Slider->SetStepSize(0.005f);
        Slider->MouseUsesStep = false;
        Slider->SetValue(InitialValue);
        Slider->SetToolTipText(FText::FromString(FString::Printf(TEXT("Adjust %s volume from 0 to 100 percent."), LabelText)));
        Theme.ApplySlider(*Slider, GetContrastMode());
        UHorizontalBoxSlot* SliderSlot = Row->AddChildToHorizontalBox(Slider);
        SliderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        SliderSlot->SetPadding(FMargin(4.0f, 6.0f));
        SliderSlot->SetVerticalAlignment(VAlign_Center);

        auto* Value = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        Value->SetAutoWrapText(false);
        Value->SetJustification(ETextJustify::Center);
        Value->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(InitialValue * 100.0f))));
        Theme.ApplyText(*Value, Theme.BodySize + 3, false, GetTextScalePercent(), GetContrastMode());
        auto* ValueSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        ValueSize->SetWidthOverride(64.0f);
        ValueSize->SetContent(Value);
        Row->AddChildToHorizontalBox(ValueSize)->SetPadding(FMargin(8.0f, 6.0f, 4.0f, 6.0f));
        ContentBox->AddChildToVerticalBox(Row)->SetPadding(FMargin(4.0f, 3.0f));
        return { Slider, Value };
    };

    const FVolumeSliderWidgets Master = AddVolumeSlider(TEXT("Master volume"), TEXT("MasterVolumeSlider"),
        GetStoredMasterVolume());
    MasterVolumeSlider = Master.Slider;
    MasterVolumeValueLabel = Master.Value;
    MasterVolumeSlider->OnValueChanged.AddDynamic(this, &ThisClass::HandleMasterVolumeChanged);

    UButton* Mute = AddButton(ContentBox, FText::GetEmpty(), TEXT("AudioMuteButton"));
    Mute->OnClicked.AddDynamic(this, &ThisClass::HandleAudioMuteClicked);
    AudioMuteLabel = Cast<UTextBlock>(Mute->GetContent());

    const FVolumeSliderWidgets Ambient = AddVolumeSlider(TEXT("Ambient volume"), TEXT("AmbientVolumeSlider"),
        GetAudioCategoryVolume(EKalmalaAudioCategory::Ambient));
    AmbientVolumeSlider = Ambient.Slider;
    AmbientVolumeValueLabel = Ambient.Value;
    AmbientVolumeSlider->OnValueChanged.AddDynamic(this, &ThisClass::HandleAmbientVolumeChanged);

    const FVolumeSliderWidgets Music = AddVolumeSlider(TEXT("Music volume"), TEXT("MusicVolumeSlider"),
        GetAudioCategoryVolume(EKalmalaAudioCategory::Music));
    MusicVolumeSlider = Music.Slider;
    MusicVolumeValueLabel = Music.Value;
    MusicVolumeSlider->OnValueChanged.AddDynamic(this, &ThisClass::HandleMusicVolumeChanged);

    const FVolumeSliderWidgets InteractionCombat = AddVolumeSlider(TEXT("Interaction / combat feedback"),
        TEXT("InteractionCombatVolumeSlider"), GetAudioCategoryVolume(EKalmalaAudioCategory::InteractionCombat));
    InteractionCombatVolumeSlider = InteractionCombat.Slider;
    InteractionCombatVolumeValueLabel = InteractionCombat.Value;
    InteractionCombatVolumeSlider->OnValueChanged.AddDynamic(this, &ThisClass::HandleInteractionCombatVolumeChanged);

    AddLabel(ContentBox, FText::FromString(TEXT("Saved locally. Music volume is ready for future music playback.")), 15.0f);
    UpdateAudioLabels();
    MasterVolumeSlider->SetUserFocus(GetOwningPlayer());
    MasterVolumeSlider->SetKeyboardFocus();
}

void UKalmalaSettingsWidget::ShowKeyboardAndMouseTab()
{
    ShowInputTab(false);
}

void UKalmalaSettingsWidget::ShowControllerTab()
{
    ShowInputTab(true);
}

void UKalmalaSettingsWidget::ShowInputTab(const bool bGamepad)
{
    SelectOptionsTab(bGamepad ? 1 : 0);
    ApplyModalPalette(&FKalmalaUITheme::Get().InventoryPanelImage);
    while (ContentBox->GetChildrenCount() > 2) ContentBox->RemoveChildAt(2);
    ControlButtons.Reset();
    AddLabel(ContentBox, FText::FromString(bGamepad ? TEXT("Controller") : TEXT("Keyboard & Mouse")), 24.0f);
    UButton* Restore = AddButton(ContentBox, FText::FromString(TEXT("Restore default controls")), TEXT("RestoreControlsButton"));
    Restore->OnClicked.AddDynamic(this, &ThisClass::HandleRestoreControlsClicked);
    AddLabel(ContentBox, FText::FromString(TEXT("Activate a row to cycle its local binding.")), 15.0f);

    UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ControlsScroll"));
    FKalmalaUITheme::Get().ApplyScroll(*Scroll);
    UVerticalBox* Rows = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ControlRows"));
    Scroll->AddChild(Rows);
    UVerticalBoxSlot* ScrollSlot = ContentBox->AddChildToVerticalBox(Scroll);
    ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    const auto AddControlRow = [this, Rows, bGamepad](const FName ControlName, const FText& ControlLabel,
        const bool bAxisPairSide, const bool bPositiveAxisSide)
    {
        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        Label->SetText(ControlLabel);
        Label->SetColorAndOpacity(FSlateColor(GetSettingsPalette().Text));
        Label->SetAutoWrapText(true);
        Label->SetJustification(ETextJustify::Right);
        FKalmalaUITheme::Get().ApplyText(*Label, FKalmalaUITheme::Get().BodySize + 3, false, GetTextScalePercent(), GetContrastMode());
        UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(Label);
        LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        LabelSlot->SetVerticalAlignment(VAlign_Center);
        LabelSlot->SetPadding(FMargin(4.0f, 2.0f, 8.0f, 2.0f));

        USizeBox* ButtonSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        ButtonSize->SetWidthOverride(240.0f);
        UKalmalaControlButton* Button = WidgetTree->ConstructWidget<UKalmalaControlButton>(
            UKalmalaControlButton::StaticClass());
        ApplyButtonPalette(Button);
        Button->Configure(ControlName, bGamepad, bAxisPairSide, bPositiveAxisSide);
        Button->SetDisplayText(FText::GetEmpty());
        Button->OnControlBindingClicked.AddDynamic(this, &ThisClass::HandleControlBindingClicked);
        ButtonSize->SetContent(Button);
        UHorizontalBoxSlot* ButtonSlot = Row->AddChildToHorizontalBox(ButtonSize);
        ButtonSlot->SetHorizontalAlignment(HAlign_Right);
        ButtonSlot->SetVerticalAlignment(VAlign_Center);
        ButtonSlot->SetPadding(FMargin(2.0f, 1.0f, 4.0f, 1.0f));
        ControlButtons.Add(Button);

        UVerticalBoxSlot* RowSlot = Rows->AddChildToVerticalBox(Row);
        RowSlot->SetPadding(FMargin(3.0f, 2.0f));
    };

    for (int32 Index = 0; Index < GetRemappableControlCount(); ++Index)
    {
        const FName ControlName = GetRemappableControlName(Index);
        const FLocalInputDefinition* Definition = FindLocalInputDefinition(ControlName);
        if (!bGamepad && Definition != nullptr && Definition->Kind == ELocalInputMappingKind::AxisPair)
        {
            AddControlRow(ControlName, FText::FromString(Definition->PositiveLabel), true, true);
            AddControlRow(ControlName, FText::FromString(Definition->NegativeLabel), true, false);
            continue;
        }

        AddControlRow(ControlName, GetRemappableControlLabel(ControlName), false, true);
    }
    UpdateControlsLabels();
    if (ControlButtons.Num() > 0)
    {
        ControlButtons[0]->SetUserFocus(GetOwningPlayer());
        ControlButtons[0]->SetKeyboardFocus();
    }
}

void UKalmalaSettingsWidget::UpdateControlsLabels()
{
    for (UKalmalaControlButton* Button : ControlButtons)
    {
        if (Button == nullptr) continue;
        Button->SetDisplayText(GetLocalInputBindingLabel(Button->GetControlName(), Button->IsGamepadBinding(),
            Button->IsAxisPairSideBinding(), Button->IsPositiveAxisSideBinding()));
    }
}

void UKalmalaSettingsWidget::ShowAccessibilityTab()
{
    SelectOptionsTab(4);
    while (ContentBox->GetChildrenCount() > 2) ContentBox->RemoveChildAt(2);
    ApplyModalPalette(&FKalmalaUITheme::Get().InventoryPanelImage);
    AddLabel(ContentBox, FText::FromString(TEXT("Accessibility")), 24.0f);

    UButton* InterfaceScale = AddButton(ContentBox, FText::GetEmpty(), TEXT("InterfaceScaleButton"));
    InterfaceScale->OnClicked.AddDynamic(this, &ThisClass::HandleInterfaceScaleClicked);
    InterfaceScaleLabel = Cast<UTextBlock>(InterfaceScale->GetContent());

    UButton* TextScale = AddButton(ContentBox, FText::GetEmpty(), TEXT("TextScaleButton"));
    TextScale->OnClicked.AddDynamic(this, &ThisClass::HandleTextScaleClicked);
    TextScaleLabel = Cast<UTextBlock>(TextScale->GetContent());

    UButton* ReducedMotion = AddButton(ContentBox, FText::GetEmpty(), TEXT("ReducedMotionButton"));
    ReducedMotion->OnClicked.AddDynamic(this, &ThisClass::HandleReducedMotionClicked);
    ReducedMotionLabel = Cast<UTextBlock>(ReducedMotion->GetContent());

    UButton* Contrast = AddButton(ContentBox, FText::GetEmpty(), TEXT("ContrastButton"));
    Contrast->OnClicked.AddDynamic(this, &ThisClass::HandleContrastClicked);
    ContrastLabel = Cast<UTextBlock>(Contrast->GetContent());

    UButton* Feedback = AddButton(ContentBox, FText::GetEmpty(), TEXT("FeedbackButton"));
    Feedback->OnClicked.AddDynamic(this, &ThisClass::HandleFeedbackClicked);
    FeedbackLabel = Cast<UTextBlock>(Feedback->GetContent());

    AddLabel(ContentBox,
        FText::FromString(TEXT("Changes apply immediately to this local menu and are saved on this device.")),
        15.0f)->SetJustification(ETextJustify::Center);
    // Explicit lines avoid auto-wrap measuring centered button text at a stale width.
    for (UTextBlock* Label : { InterfaceScaleLabel, TextScaleLabel, ReducedMotionLabel, ContrastLabel, FeedbackLabel })
    {
        Label->SetAutoWrapText(false);
        FKalmalaUITheme::Get().ApplyText(*Label, FKalmalaUITheme::Get().BodySize + 3,
            false, GetTextScalePercent(), GetContrastMode());
    }
    UpdateSettingsLabels();
    InterfaceScale->SetUserFocus(GetOwningPlayer());
    InterfaceScale->SetKeyboardFocus();
}

void UKalmalaSettingsWidget::ShowPlaceholderTab(const FText& Title, const FText& Description)
{
    while (ContentBox->GetChildrenCount() > 2) ContentBox->RemoveChildAt(2);
    AddLabel(ContentBox, Title, 24.0f);
    AddLabel(ContentBox, Description, 18.0f);
}

void UKalmalaSettingsWidget::UpdateVideoLabels()
{
    UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
    if (Settings == nullptr) return;
    const FIntPoint Resolution = Settings->GetScreenResolution();
    if (ResolutionLabel) ResolutionLabel->SetText(FText::FromString(FString::Printf(TEXT("Resolution: %d x %d"), Resolution.X, Resolution.Y)));
    if (VSyncLabel) VSyncLabel->SetText(FText::FromString(FString::Printf(TEXT("V-Sync: %s"), Settings->IsVSyncEnabled() ? TEXT("On") : TEXT("Off"))));
    const TCHAR* Mode = Settings->GetFullscreenMode() == EWindowMode::Windowed ? TEXT("Windowed") : Settings->GetFullscreenMode() == EWindowMode::WindowedFullscreen ? TEXT("Borderless") : TEXT("Fullscreen");
    if (WindowModeLabel) WindowModeLabel->SetText(FText::FromString(FString::Printf(TEXT("Window Mode: %s"), Mode)));
    if (ViewDistanceLabel) ViewDistanceLabel->SetText(FText::FromString(FString::Printf(TEXT("Render Distance: %d / 3"), ClampViewDistanceQuality(Settings->ScalabilityQuality.ViewDistanceQuality))));
}

void UKalmalaSettingsWidget::UpdateAudioLabels()
{
    const bool bWasUpdatingAudioControls = bUpdatingAudioControls;
    bUpdatingAudioControls = true;
    const float MasterValue = GetStoredMasterVolume();
    if (MasterVolumeSlider != nullptr) MasterVolumeSlider->SetValue(MasterValue);
    if (MasterVolumeValueLabel != nullptr) MasterVolumeValueLabel->SetText(FText::AsPercent(MasterValue));
    if (AudioMuteLabel != nullptr)
    {
        AudioMuteLabel->SetText(FText::FromString(IsAudioMuted() ? TEXT("Restore audio") : TEXT("Mute audio")));
    }

    const auto UpdateCategory = [](USlider* Slider, UTextBlock* ValueLabel, const EKalmalaAudioCategory Category)
    {
        const float Value = GetAudioCategoryVolume(Category);
        if (Slider != nullptr) Slider->SetValue(Value);
        if (ValueLabel != nullptr) ValueLabel->SetText(FText::AsPercent(Value));
    };
    UpdateCategory(AmbientVolumeSlider, AmbientVolumeValueLabel, EKalmalaAudioCategory::Ambient);
    UpdateCategory(MusicVolumeSlider, MusicVolumeValueLabel, EKalmalaAudioCategory::Music);
    UpdateCategory(InteractionCombatVolumeSlider, InteractionCombatVolumeValueLabel,
        EKalmalaAudioCategory::InteractionCombat);
    bUpdatingAudioControls = bWasUpdatingAudioControls;
}

void UKalmalaSettingsWidget::MarkAudioSettingsDirty()
{
    bAudioSettingsDirty = true;
    AudioSettingsLastChangedTime = FPlatformTime::Seconds();
}

void UKalmalaSettingsWidget::FlushAudioSettingsIfDirty()
{
    if (!bAudioSettingsDirty || GConfig == nullptr) return;
    GConfig->Flush(false, GGameUserSettingsIni);
    bAudioSettingsDirty = false;
}

void UKalmalaSettingsWidget::UpdateSettingsLabels()
{
    if (InterfaceScaleLabel != nullptr)
    {
        InterfaceScaleLabel->SetText(FText::FromString(FString::Printf(
            TEXT("Interface scale\n%d%%"), GetInterfaceScalePercent())));
    }
    if (TextScaleLabel != nullptr)
    {
        TextScaleLabel->SetText(FText::FromString(FString::Printf(
            TEXT("Text scale\n%d%%"), GetTextScalePercent())));
    }
    if (ReducedMotionLabel != nullptr)
    {
        ReducedMotionLabel->SetText(FText::FromString(FString::Printf(
            TEXT("Reduced motion\n%s"), IsReducedMotionEnabled() ? TEXT("On") : TEXT("Off"))));
    }
    if (ContrastLabel != nullptr)
    {
        ContrastLabel->SetText(FText::FromString(FString::Printf(
            TEXT("Contrast\n%s"),
            GetContrastMode() == 0 ? TEXT("Standard") : TEXT("High"))));
    }
    if (FeedbackLabel != nullptr)
    {
        FeedbackLabel->SetText(FText::FromString(FString::Printf(
            TEXT("Colour-independent feedback\n%s"),
            GetFeedbackMode() == 0 ? TEXT("Text only") : TEXT("Text + markers"))));
    }
}

void UKalmalaSettingsWidget::ApplyVideoSettings() { if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings()) { Settings->ApplySettings(false); Settings->SaveSettings(); UpdateVideoLabels(); } }
void UKalmalaSettingsWidget::HandleOptionsClicked() { ShowOptionsMenu(); }
void UKalmalaSettingsWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    if (bAudioSettingsDirty && FPlatformTime::Seconds() - AudioSettingsLastChangedTime >= 0.35)
    {
        FlushAudioSettingsIfDirty();
    }
    const FVector2D ViewportSize = Geometry.GetLocalSize();
    if (PanelCanvasSlot != nullptr && ViewportSize.X > 0.0f && ViewportSize.Y > 0.0f)
    {
        LastLayoutViewportSize = ViewportSize;
        const FVector2D AvailableSize(
            FMath::Max(0.0f, ViewportSize.X - 32.0f), FMath::Max(0.0f, ViewportSize.Y - 32.0f));
        const FVector2D PanelSize(FMath::Min(1000.0f, AvailableSize.X), FMath::Min(980.0f, AvailableSize.Y));
        const FVector2D CurrentPanelSize = PanelCanvasSlot->GetSize();
        if (!FMath::IsNearlyEqual(CurrentPanelSize.X, PanelSize.X, 0.5f)
            || !FMath::IsNearlyEqual(CurrentPanelSize.Y, PanelSize.Y, 0.5f))
        {
            PanelCanvasSlot->SetSize(PanelSize);
        }
        if (PanelBorder != nullptr)
        {
            const float PanelPadding = FMath::Clamp(FMath::Min(PanelSize.X, PanelSize.Y) * 0.04f, 12.0f, 40.0f);
            if (!FMath::IsNearlyEqual(PanelBorder->GetPadding().Left, PanelPadding, 0.5f))
            {
                PanelBorder->SetPadding(FMargin(PanelPadding));
            }
        }
    }
    if (bMenuOpen && bOptionsOpeningAnimationActive && PanelCanvasSlot != nullptr)
    {
        if (IsReducedMotionEnabled())
        {
            ResetOptionsOpeningAnimation();
        }
        else
        {
            const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
            OptionsOpeningElapsed = FMath::Min(OptionsOpeningElapsed + FMath::Clamp(DeltaTime, 0.0f, 1.0f / 30.0f),
                Theme.OptionsOpeningDuration);
            const float Progress = Theme.OptionsOpeningDuration > 0.0f
                ? OptionsOpeningElapsed / Theme.OptionsOpeningDuration : 1.0f;
            PanelCanvasSlot->SetPosition(FVector2D(0.0f, Theme.OptionsOpeningOffset(Progress)));
            if (Progress >= 1.0f) ResetOptionsOpeningAnimation();
        }
    }
}
void UKalmalaSettingsWidget::HandleQuitClicked() { UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false); }
void UKalmalaSettingsWidget::HandleGraphicsClicked() { ShowGraphicsTab(); }
void UKalmalaSettingsWidget::HandleKeyboardAndMouseClicked() { ShowKeyboardAndMouseTab(); }
void UKalmalaSettingsWidget::HandleControllerClicked() { ShowControllerTab(); }
void UKalmalaSettingsWidget::HandleAudioClicked() { ShowAudioTab(); }
void UKalmalaSettingsWidget::HandleAccessibilityClicked() { ShowAccessibilityTab(); }
void UKalmalaSettingsWidget::HandleResolutionClicked() { if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings()) { ResolutionChoiceIndex = (ResolutionChoiceIndex + 1) % ResolutionChoices.Num(); Settings->SetScreenResolution(ResolutionChoices[ResolutionChoiceIndex]); ApplyVideoSettings(); } }
void UKalmalaSettingsWidget::HandleVSyncClicked() { if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings()) { Settings->SetVSyncEnabled(!Settings->IsVSyncEnabled()); ApplyVideoSettings(); } }
void UKalmalaSettingsWidget::HandleWindowModeClicked() { if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings()) { const EWindowMode::Type Mode = Settings->GetFullscreenMode(); Settings->SetFullscreenMode(Mode == EWindowMode::Fullscreen ? EWindowMode::Windowed : Mode == EWindowMode::Windowed ? EWindowMode::WindowedFullscreen : EWindowMode::Fullscreen); ApplyVideoSettings(); } }
void UKalmalaSettingsWidget::HandleViewDistanceClicked() { if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings()) { Settings->ScalabilityQuality.ViewDistanceQuality = (ClampViewDistanceQuality(Settings->ScalabilityQuality.ViewDistanceQuality) + 1) % 4; ApplyVideoSettings(); } }
void UKalmalaSettingsWidget::HandleMasterVolumeChanged(const float Value)
{
    if (bUpdatingAudioControls) return;
    SetMasterVolume(Value, false);
    MarkAudioSettingsDirty();
    UpdateAudioLabels();
}

void UKalmalaSettingsWidget::HandleAudioMuteClicked()
{
    ToggleAudioMute();
    UpdateAudioLabels();
}

void UKalmalaSettingsWidget::HandleAmbientVolumeChanged(const float Value)
{
    if (bUpdatingAudioControls) return;
    SetAudioCategoryVolume(EKalmalaAudioCategory::Ambient, Value, false);
    MarkAudioSettingsDirty();
    UpdateAudioLabels();
}

void UKalmalaSettingsWidget::HandleMusicVolumeChanged(const float Value)
{
    if (bUpdatingAudioControls) return;
    SetAudioCategoryVolume(EKalmalaAudioCategory::Music, Value, false);
    MarkAudioSettingsDirty();
    UpdateAudioLabels();
}

void UKalmalaSettingsWidget::HandleInteractionCombatVolumeChanged(const float Value)
{
    if (bUpdatingAudioControls) return;
    SetAudioCategoryVolume(EKalmalaAudioCategory::InteractionCombat, Value, false);
    MarkAudioSettingsDirty();
    UpdateAudioLabels();
}

void UKalmalaSettingsWidget::HandleControlBindingClicked(const FName ControlName, const bool bGamepad,
    const bool bAxisPairSide, const bool bPositiveAxisSide)
{
    CycleLocalInputBinding(GetOwningPlayer(), ControlName, bGamepad, bAxisPairSide, bPositiveAxisSide);
    UpdateControlsLabels();
}

void UKalmalaSettingsWidget::HandleRestoreControlsClicked()
{
    RestoreDefaultInputBindings(GetOwningPlayer());
    UpdateControlsLabels();
}

void UKalmalaSettingsWidget::HandleTextScaleClicked()
{
    const int32 Current = GetTextScalePercent();
    const int32 Next = Current == 100 ? 125 : Current == 125 ? 150 : 100;
    SetTextScalePercent(Next);
    ShowAccessibilityTab();
}

void UKalmalaSettingsWidget::HandleInterfaceScaleClicked()
{
    const int32 Current = GetInterfaceScalePercent();
    const int32 Next = Current >= 120 ? 80 : Current + 10;
    SetInterfaceScalePercent(Next);
    ShowAccessibilityTab();
}

void UKalmalaSettingsWidget::HandleReducedMotionClicked()
{
    SetReducedMotionEnabled(!IsReducedMotionEnabled());
    ShowAccessibilityTab();
}

void UKalmalaSettingsWidget::HandleContrastClicked()
{
    SetContrastMode(GetContrastMode() == 0 ? 1 : 0);
    ShowAccessibilityTab();
}

void UKalmalaSettingsWidget::HandleFeedbackClicked()
{
    SetFeedbackMode(GetFeedbackMode() == 0 ? 1 : 0);
    ShowAccessibilityTab();
}
