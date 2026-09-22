#include "KalmalaSettingsWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "InputCoreTypes.h"
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
    constexpr TCHAR ContrastModeKey[] = TEXT("LocalContrastMode");
    constexpr TCHAR FeedbackModeKey[] = TEXT("LocalFeedbackMode");
    constexpr float DefaultMasterVolume = 1.0f;
    constexpr int32 DefaultTextScalePercent = 100;
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

    float ScaleFontSize(const float BaseSize)
    {
        return BaseSize * (static_cast<float>(UKalmalaSettingsWidget::GetTextScalePercent()) / 100.0f);
    }

    void ApplyButtonPalette(UButton* Button)
    {
        if (Button == nullptr) return;
        const FSettingsPalette Palette = GetSettingsPalette();
        Button->SetBackgroundColor(Palette.ButtonBackground);
        Button->SetColorAndOpacity(Palette.ButtonText);
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
        const TArray<FLocalInputAxisPair>& KeyboardPairs, const TArray<FKey>& GamepadKeys)
    {
        FLocalInputDefinition Definition;
        Definition.Name = Name;
        Definition.Label = Label;
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
                { { EKeys::W, EKeys::S }, { EKeys::Up, EKeys::Down }, { EKeys::I, EKeys::K } },
                { EKeys::Gamepad_LeftY }));
            Result.Add(MakeAxisPairInput(TEXT("MoveRight"), TEXT("Move right / left"),
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
            Result.Add(MakeSingleInput(TEXT("CraftMenu"), TEXT("Crafting menu"), ELocalInputMappingKind::Action,
                { EKeys::B, EKeys::C }, { EKeys::Gamepad_Special_Left, EKeys::Gamepad_Special_Right }));
            Result.Add(MakeSingleInput(TEXT("SupportActivate"), TEXT("Activate support effect"), ELocalInputMappingKind::Action,
                { EKeys::Q, EKeys::E }, { EKeys::Gamepad_FaceButton_Top, EKeys::Gamepad_FaceButton_Bottom }));
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

    bool IsCandidatePair(const FLocalInputDefinition& Definition, const FLocalInputAxisPair& Pair)
    {
        return Definition.KeyboardPairs.ContainsByPredicate([Pair](const FLocalInputAxisPair& Candidate)
        {
            return Candidate.Positive == Pair.Positive && Candidate.Negative == Pair.Negative;
        });
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
}

UKalmalaControlButton::UKalmalaControlButton(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    OnClicked.AddDynamic(this, &UKalmalaControlButton::HandleButtonClicked);
}

void UKalmalaControlButton::Configure(const FName InControlName, const bool bInGamepad)
{
    ControlName = InControlName;
    bGamepad = bInGamepad;
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
    Label->SetAutoWrapText(true);
    FSlateFontInfo Font = Label->GetFont();
    Font.Size = ScaleFontSize(16.0f);
    Label->SetFont(Font);
}

void UKalmalaControlButton::HandleButtonClicked()
{
    OnControlBindingClicked.Broadcast(ControlName, bGamepad);
}

void UKalmalaSettingsWidget::NativeConstruct()
{
    Super::NativeConstruct();
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
    PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
    PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
    PanelSlot->SetSize(FVector2D(760.0f, 660.0f));

    ContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsContent"));
    Panel->SetContent(ContentBox);
    ApplyModalPalette();
    SetVisibility(ESlateVisibility::Collapsed);
}

void UKalmalaSettingsWidget::Open(APlayerController* InOwningPlayer)
{
    if (InOwningPlayer == nullptr) return;
    SetOwningPlayer(InOwningPlayer);
    bMenuOpen = true;
    ShowMainMenu();
    SetVisibility(ESlateVisibility::Visible);
    InOwningPlayer->SetShowMouseCursor(true);
    FInputModeGameAndUI InputMode;
    InputMode.SetWidgetToFocus(TakeWidget());
    InOwningPlayer->SetInputMode(InputMode);
    InOwningPlayer->SetIgnoreMoveInput(true);
    InOwningPlayer->SetIgnoreLookInput(true);
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
    switch (FMath::Clamp(TabIndex, 0, 2))
    {
    case 0: ShowAudioTab(); break;
    case 1: ShowControlsTab(); break;
    default: ShowSettingsTab(); break;
    }
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

bool UKalmalaSettingsWidget::HasFocusableControlsForVerification() const
{
    return ControlButtons.Num() > 0 && ControlButtons.ContainsByPredicate([](const UKalmalaControlButton* Button)
    {
        return Button == nullptr || !Button->GetIsFocusable();
    }) == false;
}
#endif

void UKalmalaSettingsWidget::Close()
{
    if (!bMenuOpen) return;
    bMenuOpen = false;
    SetVisibility(ESlateVisibility::Collapsed);
    if (APlayerController* Controller = GetOwningPlayer())
    {
        Controller->SetShowMouseCursor(false);
        Controller->SetInputMode(FInputModeGameOnly());
        Controller->SetIgnoreMoveInput(false);
        Controller->SetIgnoreLookInput(false);
    }
}

void UKalmalaSettingsWidget::ApplyModalPalette()
{
    const FSettingsPalette Palette = GetSettingsPalette();
    if (BackdropBorder != nullptr) BackdropBorder->SetBrushColor(Palette.Background);
    if (PanelBorder != nullptr) PanelBorder->SetBrushColor(Palette.Panel);
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

void UKalmalaSettingsWidget::SetMasterVolume(const float Volume)
{
    const float ClampedVolume = ClampMasterVolume(Volume);
    if (GConfig != nullptr)
    {
        const float ExistingVolume = GetStoredMasterVolume();
        const float RestoreVolume = ClampedVolume > 0.0f ? ClampedVolume
            : (ExistingVolume > 0.0f ? ExistingVolume : ReadStoredVolume(RestoreVolumeKey, DefaultMasterVolume));
        GConfig->SetFloat(AudioSettingsSection, MasterVolumeKey, ClampedVolume, GGameUserSettingsIni);
        GConfig->SetFloat(AudioSettingsSection, RestoreVolumeKey, RestoreVolume, GGameUserSettingsIni);
        GConfig->Flush(false, GGameUserSettingsIni);
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

void UKalmalaSettingsWidget::SetAudioCategoryVolume(const EKalmalaAudioCategory Category, const float Volume)
{
    const TCHAR* Key = GetAudioCategoryKey(Category);
    if (Key == nullptr || GConfig == nullptr)
    {
        return;
    }

    GConfig->SetFloat(AudioSettingsSection, Key, ClampAudioCategoryVolume(Volume), GGameUserSettingsIni);
    GConfig->Flush(false, GGameUserSettingsIni);
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

FText UKalmalaSettingsWidget::GetLocalInputBindingLabel(const FName ControlName, const bool bGamepad)
{
    const FLocalInputDefinition* Definition = FindLocalInputDefinition(ControlName);
    if (Definition == nullptr) return FText::FromString(TEXT("Not bound"));

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

bool UKalmalaSettingsWidget::SetLocalInputBinding(const FName ControlName, const bool bGamepad, const FKey& Key)
{
    const FLocalInputDefinition* Definition = FindLocalInputDefinition(ControlName);
    if (Definition == nullptr || Definition->Kind == ELocalInputMappingKind::AxisPair || GConfig == nullptr) return false;
    const TArray<FKey>& Candidates = bGamepad ? Definition->GamepadKeys : Definition->KeyboardKeys;
    if (!IsCandidateKey(Candidates, Key)) return false;

    GConfig->SetString(AudioSettingsSection, *GetLocalInputConfigKey(ControlName, bGamepad),
        *Key.GetFName().ToString(), GGameUserSettingsIni);
    GConfig->Flush(false, GGameUserSettingsIni);
    return true;
}

void UKalmalaSettingsWidget::CycleLocalInputBinding(APlayerController* Controller, const FName ControlName,
    const bool bGamepad)
{
    const FLocalInputDefinition* Definition = FindLocalInputDefinition(ControlName);
    if (Definition == nullptr || GConfig == nullptr) return;

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
    FSlateFontInfo Font = Text->GetFont();
    Font.Size = ScaleFontSize(FontSize);
    Text->SetFont(Font);
    UVerticalBoxSlot* BoxSlot = Parent->AddChildToVerticalBox(Text);
    BoxSlot->SetPadding(FMargin(4.0f, 8.0f));
    return Text;
}

UButton* UKalmalaSettingsWidget::AddButton(UVerticalBox* Parent, const FText& Label, const FName Name)
{
    UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
    ApplyButtonPalette(Button);
    UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Text->SetText(Label);
    Text->SetColorAndOpacity(FSlateColor(GetSettingsPalette().ButtonText));
    Text->SetAutoWrapText(true);
    Text->SetJustification(ETextJustify::Center);
    FSlateFontInfo Font = Text->GetFont();
    Font.Size = ScaleFontSize(21.0f);
    Text->SetFont(Font);
    Button->SetContent(Text);
    UVerticalBoxSlot* BoxSlot = Parent->AddChildToVerticalBox(Button);
    BoxSlot->SetPadding(FMargin(4.0f, 7.0f));
    BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    return Button;
}

void UKalmalaSettingsWidget::ShowMainMenu()
{
    ContentBox->ClearChildren();
    AddLabel(ContentBox, FText::FromString(TEXT("KALMALA")), 34.0f)->SetJustification(ETextJustify::Center);
    AddLabel(ContentBox, FText::FromString(TEXT("Settings")), 23.0f)->SetJustification(ETextJustify::Center);
    UButton* Options = AddButton(ContentBox, FText::FromString(TEXT("Options")), TEXT("OptionsButton"));
    Options->OnClicked.AddDynamic(this, &ThisClass::HandleOptionsClicked);
    UButton* Quit = AddButton(ContentBox, FText::FromString(TEXT("Quit")), TEXT("QuitButton"));
    Quit->OnClicked.AddDynamic(this, &ThisClass::HandleQuitClicked);
    AddLabel(ContentBox, FText::FromString(TEXT("Press Esc to return to the game")), 16.0f)->SetJustification(ETextJustify::Center);
}

void UKalmalaSettingsWidget::ShowOptionsMenu()
{
    ContentBox->ClearChildren();
    AddLabel(ContentBox, FText::FromString(TEXT("Options")), 30.0f)->SetJustification(ETextJustify::Center);
    UHorizontalBox* Tabs = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    const auto AddTab = [this, Tabs](const FText& Label, FName Name, FScriptDelegate Delegate)
    {
        UButton* Tab = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
        ApplyButtonPalette(Tab);
        UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        Text->SetText(Label);
        Text->SetColorAndOpacity(FSlateColor(GetSettingsPalette().ButtonText));
        Text->SetJustification(ETextJustify::Center);
        FSlateFontInfo Font = Text->GetFont();
        Font.Size = ScaleFontSize(18.0f);
        Text->SetFont(Font);
        Tab->SetContent(Text);
        Tab->OnClicked.Add(Delegate);
        UHorizontalBoxSlot* Slot = Tabs->AddChildToHorizontalBox(Tab); Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); Slot->SetPadding(FMargin(3.0f));
    };
    FScriptDelegate VideoDelegate; VideoDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ThisClass, HandleVideoClicked)); AddTab(FText::FromString(TEXT("Video")), TEXT("VideoTab"), VideoDelegate);
    FScriptDelegate AudioDelegate; AudioDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ThisClass, HandleAudioClicked)); AddTab(FText::FromString(TEXT("Audio")), TEXT("AudioTab"), AudioDelegate);
    FScriptDelegate ControlsDelegate; ControlsDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ThisClass, HandleControlsClicked)); AddTab(FText::FromString(TEXT("Controls")), TEXT("ControlsTab"), ControlsDelegate);
    FScriptDelegate SettingsDelegate; SettingsDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ThisClass, HandleSettingsClicked)); AddTab(FText::FromString(TEXT("Settings")), TEXT("SettingsTab"), SettingsDelegate);
    ContentBox->AddChildToVerticalBox(Tabs);
    ShowVideoTab();
}

void UKalmalaSettingsWidget::ShowVideoTab()
{
    while (ContentBox->GetChildrenCount() > 2) ContentBox->RemoveChildAt(2);
    AddLabel(ContentBox, FText::FromString(TEXT("Video")), 24.0f);
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
}

void UKalmalaSettingsWidget::ShowAudioTab()
{
    while (ContentBox->GetChildrenCount() > 2) ContentBox->RemoveChildAt(2);
    AddLabel(ContentBox, FText::FromString(TEXT("Audio")), 24.0f);
    UButton* MasterVolume = AddButton(ContentBox, FText::GetEmpty(), TEXT("MasterVolumeButton"));
    MasterVolume->OnClicked.AddDynamic(this, &ThisClass::HandleMasterVolumeClicked);
    MasterVolumeLabel = Cast<UTextBlock>(MasterVolume->GetContent());
    UButton* Mute = AddButton(ContentBox, FText::GetEmpty(), TEXT("AudioMuteButton"));
    Mute->OnClicked.AddDynamic(this, &ThisClass::HandleAudioMuteClicked);
    AudioMuteLabel = Cast<UTextBlock>(Mute->GetContent());
    UButton* Ambient = AddButton(ContentBox, FText::GetEmpty(), TEXT("AmbientVolumeButton"));
    Ambient->OnClicked.AddDynamic(this, &ThisClass::HandleAmbientVolumeClicked);
    AmbientVolumeLabel = Cast<UTextBlock>(Ambient->GetContent());
    UButton* Music = AddButton(ContentBox, FText::GetEmpty(), TEXT("MusicVolumeButton"));
    Music->OnClicked.AddDynamic(this, &ThisClass::HandleMusicVolumeClicked);
    MusicVolumeLabel = Cast<UTextBlock>(Music->GetContent());
    UButton* InteractionCombat = AddButton(ContentBox, FText::GetEmpty(), TEXT("InteractionCombatVolumeButton"));
    InteractionCombat->OnClicked.AddDynamic(this, &ThisClass::HandleInteractionCombatVolumeClicked);
    InteractionCombatVolumeLabel = Cast<UTextBlock>(InteractionCombat->GetContent());
    AddLabel(ContentBox, FText::FromString(TEXT("Saved locally. Music volume is ready for future music playback.")), 15.0f);
    UpdateAudioLabels();
    MasterVolume->SetUserFocus(GetOwningPlayer());
}

void UKalmalaSettingsWidget::ShowControlsTab()
{
    while (ContentBox->GetChildrenCount() > 2) ContentBox->RemoveChildAt(2);
    ControlButtons.Reset();
    AddLabel(ContentBox, FText::FromString(TEXT("Controls")), 24.0f);
    UButton* Restore = AddButton(ContentBox, FText::FromString(TEXT("Restore default controls")), TEXT("RestoreControlsButton"));
    Restore->OnClicked.AddDynamic(this, &ThisClass::HandleRestoreControlsClicked);
    AddLabel(ContentBox, FText::FromString(TEXT("Activate a keyboard or controller row to cycle its local binding.")), 15.0f);

    UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ControlsScroll"));
    UVerticalBox* Rows = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ControlRows"));
    Scroll->AddChild(Rows);
    ContentBox->AddChildToVerticalBox(Scroll);

    for (int32 Index = 0; Index < GetRemappableControlCount(); ++Index)
    {
        const FName ControlName = GetRemappableControlName(Index);
        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        Label->SetText(GetRemappableControlLabel(ControlName));
        Label->SetColorAndOpacity(FSlateColor(GetSettingsPalette().Text));
        Label->SetAutoWrapText(true);
        FSlateFontInfo Font = Label->GetFont();
        Font.Size = ScaleFontSize(16.0f);
        Label->SetFont(Font);
        UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(Label);
        LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        LabelSlot->SetPadding(FMargin(4.0f, 3.0f));

        const auto AddControlButton = [this, Row, ControlName](const bool bGamepad)
        {
            UKalmalaControlButton* Button = WidgetTree->ConstructWidget<UKalmalaControlButton>(
                UKalmalaControlButton::StaticClass());
            ApplyButtonPalette(Button);
            Button->Configure(ControlName, bGamepad);
            Button->SetDisplayText(FText::GetEmpty());
            Button->OnControlBindingClicked.AddDynamic(this, &ThisClass::HandleControlBindingClicked);
            UHorizontalBoxSlot* ButtonSlot = Row->AddChildToHorizontalBox(Button);
            ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            ButtonSlot->SetPadding(FMargin(3.0f, 2.0f));
            ControlButtons.Add(Button);
        };
        AddControlButton(false);
        AddControlButton(true);
        Rows->AddChildToVerticalBox(Row);
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
        const TCHAR* DeviceLabel = Button->IsGamepadBinding() ? TEXT("Controller") : TEXT("Keyboard");
        Button->SetDisplayText(FText::FromString(FString::Printf(TEXT("%s: %s"), DeviceLabel,
            *GetLocalInputBindingLabel(Button->GetControlName(), Button->IsGamepadBinding()).ToString())));
    }
}

void UKalmalaSettingsWidget::ShowSettingsTab()
{
    while (ContentBox->GetChildrenCount() > 2) ContentBox->RemoveChildAt(2);
    ApplyModalPalette();
    AddLabel(ContentBox, FText::FromString(TEXT("Settings")), 24.0f);

    UButton* TextScale = AddButton(ContentBox, FText::GetEmpty(), TEXT("TextScaleButton"));
    TextScale->OnClicked.AddDynamic(this, &ThisClass::HandleTextScaleClicked);
    TextScaleLabel = Cast<UTextBlock>(TextScale->GetContent());

    UButton* Contrast = AddButton(ContentBox, FText::GetEmpty(), TEXT("ContrastButton"));
    Contrast->OnClicked.AddDynamic(this, &ThisClass::HandleContrastClicked);
    ContrastLabel = Cast<UTextBlock>(Contrast->GetContent());

    UButton* Feedback = AddButton(ContentBox, FText::GetEmpty(), TEXT("FeedbackButton"));
    Feedback->OnClicked.AddDynamic(this, &ThisClass::HandleFeedbackClicked);
    FeedbackLabel = Cast<UTextBlock>(Feedback->GetContent());

    AddLabel(ContentBox,
        FText::FromString(TEXT("Changes apply immediately to this local menu and are saved on this device.")),
        15.0f)->SetJustification(ETextJustify::Center);
    UpdateSettingsLabels();
    TextScale->SetUserFocus(GetOwningPlayer());
    TextScale->SetKeyboardFocus();
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
    const int32 Percent = FMath::RoundToInt(GetStoredMasterVolume() * 100.0f);
    if (MasterVolumeLabel != nullptr)
    {
        MasterVolumeLabel->SetText(FText::FromString(FString::Printf(
            TEXT("Master Volume: %d%% (Activate to change)"), Percent)));
    }
    if (AudioMuteLabel != nullptr)
    {
        AudioMuteLabel->SetText(FText::FromString(IsAudioMuted() ? TEXT("Restore audio") : TEXT("Mute audio")));
    }

    const auto UpdateCategoryLabel = [](UTextBlock* Label, const TCHAR* CategoryLabel,
        const EKalmalaAudioCategory Category)
    {
        if (Label != nullptr)
        {
            const int32 CategoryPercent = FMath::RoundToInt(GetAudioCategoryVolume(Category) * 100.0f);
            Label->SetText(FText::FromString(FString::Printf(
                TEXT("%s Volume: %d%% (Activate to change)"), CategoryLabel, CategoryPercent)));
        }
    };
    UpdateCategoryLabel(AmbientVolumeLabel, TEXT("Ambient"), EKalmalaAudioCategory::Ambient);
    UpdateCategoryLabel(MusicVolumeLabel, TEXT("Music"), EKalmalaAudioCategory::Music);
    UpdateCategoryLabel(InteractionCombatVolumeLabel, TEXT("Interaction/Combat Feedback"),
        EKalmalaAudioCategory::InteractionCombat);
}

void UKalmalaSettingsWidget::UpdateSettingsLabels()
{
    if (TextScaleLabel != nullptr)
    {
        TextScaleLabel->SetText(FText::FromString(FString::Printf(
            TEXT("Text Scale: %d%% (Activate to change)"), GetTextScalePercent())));
    }
    if (ContrastLabel != nullptr)
    {
        ContrastLabel->SetText(FText::FromString(FString::Printf(
            TEXT("Contrast: %s (Activate to change)"),
            GetContrastMode() == 0 ? TEXT("Standard") : TEXT("High"))));
    }
    if (FeedbackLabel != nullptr)
    {
        FeedbackLabel->SetText(FText::FromString(FString::Printf(
            TEXT("Colour-independent feedback: %s (Activate to change)"),
            GetFeedbackMode() == 0 ? TEXT("Text only") : TEXT("Text + markers"))));
    }
}

void UKalmalaSettingsWidget::ApplyVideoSettings() { if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings()) { Settings->ApplySettings(false); Settings->SaveSettings(); UpdateVideoLabels(); } }
void UKalmalaSettingsWidget::HandleOptionsClicked() { ShowOptionsMenu(); }
void UKalmalaSettingsWidget::HandleQuitClicked() { UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false); }
void UKalmalaSettingsWidget::HandleVideoClicked() { ShowVideoTab(); }
void UKalmalaSettingsWidget::HandleAudioClicked() { ShowAudioTab(); }
void UKalmalaSettingsWidget::HandleControlsClicked() { ShowControlsTab(); }
void UKalmalaSettingsWidget::HandleSettingsClicked() { ShowSettingsTab(); }
void UKalmalaSettingsWidget::HandleResolutionClicked() { if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings()) { ResolutionChoiceIndex = (ResolutionChoiceIndex + 1) % ResolutionChoices.Num(); Settings->SetScreenResolution(ResolutionChoices[ResolutionChoiceIndex]); ApplyVideoSettings(); } }
void UKalmalaSettingsWidget::HandleVSyncClicked() { if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings()) { Settings->SetVSyncEnabled(!Settings->IsVSyncEnabled()); ApplyVideoSettings(); } }
void UKalmalaSettingsWidget::HandleWindowModeClicked() { if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings()) { const EWindowMode::Type Mode = Settings->GetFullscreenMode(); Settings->SetFullscreenMode(Mode == EWindowMode::Fullscreen ? EWindowMode::Windowed : Mode == EWindowMode::Windowed ? EWindowMode::WindowedFullscreen : EWindowMode::Fullscreen); ApplyVideoSettings(); } }
void UKalmalaSettingsWidget::HandleViewDistanceClicked() { if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings()) { Settings->ScalabilityQuality.ViewDistanceQuality = (ClampViewDistanceQuality(Settings->ScalabilityQuality.ViewDistanceQuality) + 1) % 4; ApplyVideoSettings(); } }
void UKalmalaSettingsWidget::HandleMasterVolumeClicked()
{
    const int32 CurrentStep = FMath::RoundToInt(GetStoredMasterVolume() * 4.0f);
    const int32 NextStep = (CurrentStep % 4) + 1;
    SetMasterVolume(static_cast<float>(NextStep) * 0.25f);
    UpdateAudioLabels();
}

void UKalmalaSettingsWidget::HandleAudioMuteClicked()
{
    ToggleAudioMute();
    UpdateAudioLabels();
}

void UKalmalaSettingsWidget::CycleAudioCategory(const EKalmalaAudioCategory Category)
{
    const int32 CurrentStep = FMath::RoundToInt(GetAudioCategoryVolume(Category) * 4.0f);
    const int32 NextStep = (CurrentStep + 1) % 5;
    SetAudioCategoryVolume(Category, static_cast<float>(NextStep) * 0.25f);
    UpdateAudioLabels();
}

void UKalmalaSettingsWidget::HandleAmbientVolumeClicked()
{
    CycleAudioCategory(EKalmalaAudioCategory::Ambient);
}

void UKalmalaSettingsWidget::HandleMusicVolumeClicked()
{
    CycleAudioCategory(EKalmalaAudioCategory::Music);
}

void UKalmalaSettingsWidget::HandleInteractionCombatVolumeClicked()
{
    CycleAudioCategory(EKalmalaAudioCategory::InteractionCombat);
}

void UKalmalaSettingsWidget::HandleControlBindingClicked(const FName ControlName, const bool bGamepad)
{
    CycleLocalInputBinding(GetOwningPlayer(), ControlName, bGamepad);
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
    ShowSettingsTab();
}

void UKalmalaSettingsWidget::HandleContrastClicked()
{
    SetContrastMode(GetContrastMode() == 0 ? 1 : 0);
    ShowSettingsTab();
}

void UKalmalaSettingsWidget::HandleFeedbackClicked()
{
    SetFeedbackMode(GetFeedbackMode() == 0 ? 1 : 0);
    ShowSettingsTab();
}
