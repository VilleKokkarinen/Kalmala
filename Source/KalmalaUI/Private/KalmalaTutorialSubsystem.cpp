#include "KalmalaTutorialSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/InputComponent.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "KalmalaCampfire.h"
#include "KalmalaCharacter.h"
#include "KalmalaCraftingSubsystem.h"
#include "KalmalaDiscoveryActor.h"
#include "KalmalaHarvestNode.h"
#include "KalmalaInteractable.h"
#include "KalmalaPlayerStatusComponent.h"
#include "KalmalaSupportMagicComponent.h"
#include "KalmalaWildlifeSpawn.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace
{
constexpr float PromptDurationSeconds = 18.0f;
constexpr float FocusRefreshIntervalSeconds = 0.16f;
constexpr float VisibleTraceDistance = 1400.0f;
constexpr float NormalInteractionDistance = 250.0f;
constexpr float EncounterNoticeDistance = 1200.0f;
constexpr float VisibleCampDistance = 900.0f;
constexpr float ExplorationDistance = 900.0f;

FSlateFontInfo PromptFont(const int32 Size)
{
    return FSlateFontInfo(FCoreStyle::GetDefaultFont(), Size);
}
}

void UKalmalaTutorialPromptWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    SetIsFocusable(false);

    CardSizeBox = WidgetTree->ConstructWidget<USizeBox>();
    CardSizeBox->SetWidthOverride(560.0f);
    CardSizeBox->SetHeightOverride(156.0f);
    CardBorder = WidgetTree->ConstructWidget<UBorder>();
    CardBorder->SetBrushColor(FLinearColor(0.018f, 0.026f, 0.031f, 0.94f));
    CardBorder->SetPadding(FMargin(14.0f));

    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
    USizeBox* SymbolSlot = WidgetTree->ConstructWidget<USizeBox>();
    SymbolSlot->SetWidthOverride(52.0f);
    SymbolSlot->SetHeightOverride(52.0f);
    Row->AddChildToHorizontalBox(SymbolSlot)->SetPadding(FMargin(0.0f, 2.0f, 12.0f, 0.0f));

    UVerticalBox* Copy = WidgetTree->ConstructWidget<UVerticalBox>();
    TitleText = WidgetTree->ConstructWidget<UTextBlock>();
    TitleText->SetFont(PromptFont(20));
    TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.98f, 0.82f, 0.55f, 1.0f)));
    Copy->AddChildToVerticalBox(TitleText)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));

    BodyText = WidgetTree->ConstructWidget<UTextBlock>();
    BodyText->SetFont(PromptFont(16));
    BodyText->SetAutoWrapText(true);
    BodyText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    Copy->AddChildToVerticalBox(BodyText)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 7.0f));

    Row->AddChildToHorizontalBox(Copy)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    CardBorder->SetContent(Row);
    CardSizeBox->SetContent(CardBorder);
    WidgetTree->RootWidget = CardSizeBox;
    SetVisibility(ESlateVisibility::Collapsed);
    SetRenderOpacity(1.0f);
}

int32 UKalmalaTutorialPromptWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, const int32 LayerId,
    const FWidgetStyle& InWidgetStyle, const bool bParentEnabled) const
{
    const int32 DrawLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements,
        LayerId, InWidgetStyle, bParentEnabled) + 1;
    if (CardBorder == nullptr || DisplayedBeat == EKalmalaTutorialBeat::None) return DrawLayer;

    const FVector2D CardSize = AllottedGeometry.GetLocalSize();
    const FVector2D Centre(40.0f, 40.0f);
    const float Radius = FMath::Min(13.0f, FMath::Min(CardSize.X, CardSize.Y) * 0.18f);
    const FLinearColor Ink(0.92f, 0.94f, 0.91f, 1.0f);
    TArray<FVector2D> Points = {
        Centre + FVector2D(0.0f, -Radius),
        Centre + FVector2D(Radius, 0.0f),
        Centre + FVector2D(0.0f, Radius),
        Centre + FVector2D(-Radius, 0.0f),
        Centre + FVector2D(0.0f, -Radius)
    };
    FSlateDrawElement::MakeLines(OutDrawElements, DrawLayer, AllottedGeometry.ToPaintGeometry(), Points,
        ESlateDrawEffect::None, Ink, true, 2.0f);
    TArray<FVector2D> Compass = {
        Centre + FVector2D(0.0f, -Radius - 7.0f), Centre + FVector2D(0.0f, Radius + 7.0f),
        Centre + FVector2D(-Radius - 7.0f, 0.0f), Centre + FVector2D(Radius + 7.0f, 0.0f)
    };
    FSlateDrawElement::MakeLines(OutDrawElements, DrawLayer, AllottedGeometry.ToPaintGeometry(), Compass,
        ESlateDrawEffect::None, Ink, true, 2.0f);
    return DrawLayer;
}

void UKalmalaTutorialPromptWidget::SetPrompt(const EKalmalaTutorialBeat Beat, const FString& Title,
    const FString& Body)
{
    DisplayedBeat = Beat;
    if (TitleText) TitleText->SetText(FText::FromString(Title));
    if (BodyText) BodyText->SetText(FText::FromString(Body));
    Invalidate(EInvalidateWidget::LayoutAndVolatility);
}

void UKalmalaTutorialPromptWidget::SetPromptVisible(const bool bVisible)
{
    SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UKalmalaTutorialPromptWidget::SetCardSize(const float Width, const float Height)
{
    if (CardSizeBox != nullptr)
    {
        CardSizeBox->SetWidthOverride(Width);
        CardSizeBox->SetHeightOverride(Height);
    }
}

void UKalmalaTutorialSubsystem::Tick(const float DeltaTime)
{
    UWorld* World = GetWorld();
    ULocalPlayer* Player = GetLocalPlayer();
    if (World == nullptr || !World->IsGameWorld() || Player == nullptr) return;

    APlayerController* Controller = Player->GetPlayerController(World);
    if (Controller != LocalController)
    {
        ReleaseController();
        if (Controller == nullptr || !Controller->IsLocalController()) return;
        LocalController = Controller;
    }
    if (Controller == nullptr || !Controller->IsLocalController()) return;
    BindLocalInput(Controller);

    APawn* Pawn = Controller->GetPawn();
    if (Pawn == nullptr)
    {
        VisibleFocusActor.Reset();
        HideCurrentBeat();
        return;
    }

    if (TrackedPawn.Get() != Pawn)
    {
        TrackedPawn = Pawn;
        if (!bHasInitialPawnLocation)
        {
            InitialPawnLocation = Pawn->GetActorLocation();
            bHasInitialPawnLocation = true;
        }
    }

    FocusRefreshSecondsRemaining -= DeltaTime;
    if (FocusRefreshSecondsRemaining <= 0.0f)
    {
        UpdateVisibleFocus(Pawn);
        FocusRefreshSecondsRemaining = FocusRefreshIntervalSeconds;
    }
    AttackIntentSecondsRemaining = FMath::Max(0.0f, AttackIntentSecondsRemaining - DeltaTime);

    if (CurrentBeat != EKalmalaTutorialBeat::None)
    {
        CurrentBeatSecondsRemaining -= DeltaTime;
        if (CurrentBeatSecondsRemaining <= 0.0f || !IsBeatContextActive(CurrentBeat, Pawn))
        {
            HideCurrentBeat();
        }
    }

    if (CurrentBeat == EKalmalaTutorialBeat::None)
    {
        const EKalmalaTutorialBeat Candidate = FindAvailableBeat(Pawn);
        if (Candidate != EKalmalaTutorialBeat::None && !SeenBeats.Contains(Candidate))
        {
            ShowBeat(Candidate);
        }
    }

    if (PromptWidget != nullptr && Controller->GetLocalPlayer() != nullptr)
    {
        int32 ViewWidth = 0;
        int32 ViewHeight = 0;
        Controller->GetViewportSize(ViewWidth, ViewHeight);
        const float ViewportScale = FMath::Max(0.1f, UWidgetLayoutLibrary::GetViewportScale(PromptWidget));
        const FIntPoint ViewportSize(ViewWidth, ViewHeight);
        if (ViewportSize != LastViewportSize || !FMath::IsNearlyEqual(ViewportScale, LastViewportScale))
        {
            const float PhysicalWidth = FMath::Min(560.0f, FMath::Max(280.0f, static_cast<float>(ViewWidth) - 32.0f));
            const float PhysicalHeight = FMath::Min(156.0f, FMath::Max(136.0f, static_cast<float>(ViewHeight) - 32.0f));
            const FVector2D DesiredSize(PhysicalWidth / ViewportScale, PhysicalHeight / ViewportScale);
            PromptWidget->SetCardSize(DesiredSize.X, DesiredSize.Y);
            PromptWidget->SetDesiredSizeInViewport(DesiredSize);
            // Keep the arrival card clear of the left HUD (24 + 340 logical units).
            // Preserve its centred position whenever that already leaves enough room.
            const float PhysicalLeft = FMath::Clamp(
                FMath::Max((ViewWidth - PhysicalWidth) * 0.5f, 364.0f * ViewportScale + 16.0f),
                16.0f, FMath::Max(16.0f, ViewWidth - PhysicalWidth - 16.0f));
            PromptWidget->SetPositionInViewport(FVector2D(PhysicalLeft / ViewportScale,
                (ViewHeight - PhysicalHeight - 16.0f) / ViewportScale), false);
            LastViewportSize = ViewportSize;
            LastViewportScale = ViewportScale;
        }
    }
}

void UKalmalaTutorialSubsystem::BindLocalInput(APlayerController* InController)
{
    if (InController == nullptr || InController->InputComponent == nullptr
        || BoundInputComponent.Get() == InController->InputComponent) return;

    if (UInputComponent* Previous = BoundInputComponent.Get())
    {
        for (int32 Index = Previous->GetNumActionBindings() - 1; Index >= 0; --Index)
        {
            if (Previous->GetActionBinding(Index).ActionDelegate.IsBoundToObject(this)) Previous->RemoveActionBinding(Index);
        }
    }

    UInputComponent* Input = InController->InputComponent;
    FInputActionBinding& DismissBinding = Input->BindAction(TEXT("TutorialPromptDismiss"), IE_Pressed, this, &ThisClass::DismissPrompt);
    DismissBinding.bConsumeInput = false;
    FInputActionBinding& RevisitBinding = Input->BindAction(TEXT("TutorialPromptRevisit"), IE_Pressed, this, &ThisClass::RevisitPrompt);
    RevisitBinding.bConsumeInput = false;
    FInputActionBinding& AttackBinding = Input->BindAction(TEXT("Attack"), IE_Pressed, this, &ThisClass::NoteAttackIntent);
    AttackBinding.bConsumeInput = false;
    BoundInputComponent = Input;
}

void UKalmalaTutorialSubsystem::ReleaseController()
{
    if (UInputComponent* Input = BoundInputComponent.Get())
    {
        for (int32 Index = Input->GetNumActionBindings() - 1; Index >= 0; --Index)
        {
            if (Input->GetActionBinding(Index).ActionDelegate.IsBoundToObject(this)) Input->RemoveActionBinding(Index);
        }
    }
    BoundInputComponent.Reset();
    if (PromptWidget != nullptr)
    {
        PromptWidget->RemoveFromParent();
        PromptWidget = nullptr;
    }
    LocalController = nullptr;
    TrackedPawn.Reset();
    VisibleFocusActor.Reset();
    LastViewportSize = FIntPoint::ZeroValue;
    LastViewportScale = 0.0f;
    HideCurrentBeat();
}

void UKalmalaTutorialSubsystem::UpdateVisibleFocus(APawn* Pawn)
{
    VisibleFocusActor.Reset();
    if (LocalController == nullptr || Pawn == nullptr || GetWorld() == nullptr) return;

    FVector ViewLocation;
    FRotator ViewRotation;
    LocalController->GetPlayerViewPoint(ViewLocation, ViewRotation);
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(KalmalaTutorialVisibleFocus), true, Pawn);
    FHitResult Hit;
    if (GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, ViewLocation + ViewRotation.Vector() * VisibleTraceDistance,
        ECC_Visibility, QueryParams))
    {
        if (AActor* Actor = Hit.GetActor()) VisibleFocusActor = Actor;
    }
}

EKalmalaTutorialBeat UKalmalaTutorialSubsystem::FindAvailableBeat(APawn* Pawn) const
{
    if (Pawn == nullptr) return EKalmalaTutorialBeat::None;
    const auto ChooseUnseen = [this](const EKalmalaTutorialBeat Beat)
    {
        return SeenBeats.Contains(Beat) ? EKalmalaTutorialBeat::None : Beat;
    };
    if (const EKalmalaTutorialBeat Arrive = ChooseUnseen(EKalmalaTutorialBeat::Arrive); Arrive != EKalmalaTutorialBeat::None)
        return Arrive;

    const AActor* FocusActor = VisibleFocusActor.Get();
    const float FocusDistance = FocusActor ? FVector::Distance(Pawn->GetActorLocation(), FocusActor->GetActorLocation()) : TNumericLimits<float>::Max();
    if (Cast<AKalmalaDiscoveryActor>(FocusActor) && FocusDistance <= NormalInteractionDistance)
        if (const EKalmalaTutorialBeat Discovery = ChooseUnseen(EKalmalaTutorialBeat::Discovery); Discovery != EKalmalaTutorialBeat::None)
            return Discovery;
    if (Cast<AKalmalaHarvestNode>(FocusActor) && FocusDistance <= NormalInteractionDistance)
        if (const EKalmalaTutorialBeat Gather = ChooseUnseen(EKalmalaTutorialBeat::Gather); Gather != EKalmalaTutorialBeat::None)
            return Gather;
    if (const AKalmalaWildlifeSpawn* Wildlife = Cast<AKalmalaWildlifeSpawn>(FocusActor);
        Wildlife != nullptr && !Wildlife->IsDefeated() && FocusDistance <= EncounterNoticeDistance)
        if (const EKalmalaTutorialBeat Encounter = ChooseUnseen(EKalmalaTutorialBeat::OptionalEncounter); Encounter != EKalmalaTutorialBeat::None)
            return Encounter;
    if (FocusActor != nullptr && FocusActor->GetClass()->ImplementsInterface(UKalmalaInteractable::StaticClass())
        && FocusDistance <= NormalInteractionDistance)
        if (const EKalmalaTutorialBeat Interact = ChooseUnseen(EKalmalaTutorialBeat::Interact); Interact != EKalmalaTutorialBeat::None)
            return Interact;

    const AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(Pawn);
    const UKalmalaSupportMagicComponent* Support = Character ? Character->GetSupportMagicComponent() : nullptr;
    if (HasLearnedSupportEffect(Support))
        if (const EKalmalaTutorialBeat SupportBeat = ChooseUnseen(EKalmalaTutorialBeat::SupportMagic); SupportBeat != EKalmalaTutorialBeat::None)
            return SupportBeat;

    if (const ULocalPlayer* Player = GetLocalPlayer())
    {
        if (const UKalmalaCraftingSubsystem* Crafting = Player->GetSubsystem<UKalmalaCraftingSubsystem>();
            Crafting != nullptr && Crafting->IsOpen())
            if (const EKalmalaTutorialBeat Prepare = ChooseUnseen(EKalmalaTutorialBeat::Prepare); Prepare != EKalmalaTutorialBeat::None)
                return Prepare;
    }

    const UKalmalaPlayerStatusComponent* Status = Pawn->FindComponentByClass<UKalmalaPlayerStatusComponent>();
    if (Status != nullptr && Status->HasStatus(UKalmalaPlayerStatusComponent::WetStatusId))
        if (const EKalmalaTutorialBeat Weather = ChooseUnseen(EKalmalaTutorialBeat::Weather); Weather != EKalmalaTutorialBeat::None)
            return Weather;
    if (AttackIntentSecondsRemaining > 0.0f)
        if (const EKalmalaTutorialBeat Encounter = ChooseUnseen(EKalmalaTutorialBeat::OptionalEncounter); Encounter != EKalmalaTutorialBeat::None)
            return Encounter;
    if (Cast<AKalmalaCampfire>(FocusActor) && FocusDistance <= VisibleCampDistance)
        if (const EKalmalaTutorialBeat Return = ChooseUnseen(EKalmalaTutorialBeat::Return); Return != EKalmalaTutorialBeat::None)
            return Return;
    if (bHasInitialPawnLocation && FVector::DistSquared2D(Pawn->GetActorLocation(), InitialPawnLocation) >= FMath::Square(ExplorationDistance))
        return ChooseUnseen(EKalmalaTutorialBeat::Explore);
    return EKalmalaTutorialBeat::None;
}

bool UKalmalaTutorialSubsystem::IsBeatContextActive(const EKalmalaTutorialBeat Beat, APawn* Pawn) const
{
    if (Pawn == nullptr) return false;
    const AActor* FocusActor = VisibleFocusActor.Get();
    const float FocusDistance = FocusActor ? FVector::Distance(Pawn->GetActorLocation(), FocusActor->GetActorLocation()) : TNumericLimits<float>::Max();

    switch (Beat)
    {
    case EKalmalaTutorialBeat::Arrive:
        return true;
    case EKalmalaTutorialBeat::Interact:
        return FocusActor != nullptr && FocusActor->GetClass()->ImplementsInterface(UKalmalaInteractable::StaticClass())
            && FocusDistance <= NormalInteractionDistance;
    case EKalmalaTutorialBeat::Gather:
        return Cast<AKalmalaHarvestNode>(FocusActor) != nullptr && FocusDistance <= NormalInteractionDistance;
    case EKalmalaTutorialBeat::Prepare:
        if (const ULocalPlayer* Player = GetLocalPlayer())
            if (const UKalmalaCraftingSubsystem* Crafting = Player->GetSubsystem<UKalmalaCraftingSubsystem>()) return Crafting->IsOpen();
        return false;
    case EKalmalaTutorialBeat::Weather:
        if (const UKalmalaPlayerStatusComponent* Status = Pawn->FindComponentByClass<UKalmalaPlayerStatusComponent>())
            return Status->HasStatus(UKalmalaPlayerStatusComponent::WetStatusId);
        return false;
    case EKalmalaTutorialBeat::Explore:
        return bHasInitialPawnLocation && FVector::DistSquared2D(Pawn->GetActorLocation(), InitialPawnLocation) >= FMath::Square(ExplorationDistance);
    case EKalmalaTutorialBeat::OptionalEncounter:
        return AttackIntentSecondsRemaining > 0.0f
            || (Cast<AKalmalaWildlifeSpawn>(FocusActor) != nullptr && !Cast<AKalmalaWildlifeSpawn>(FocusActor)->IsDefeated()
                && FocusDistance <= EncounterNoticeDistance);
    case EKalmalaTutorialBeat::Discovery:
        return Cast<AKalmalaDiscoveryActor>(FocusActor) != nullptr && FocusDistance <= NormalInteractionDistance;
    case EKalmalaTutorialBeat::SupportMagic:
        if (const AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(Pawn)) return HasLearnedSupportEffect(Character->GetSupportMagicComponent());
        return false;
    case EKalmalaTutorialBeat::Return:
        return Cast<AKalmalaCampfire>(FocusActor) != nullptr && FocusDistance <= VisibleCampDistance;
    default:
        return false;
    }
}

bool UKalmalaTutorialSubsystem::HasLearnedSupportEffect(const UKalmalaSupportMagicComponent* Support) const
{
    if (Support == nullptr) return false;
    return Support->HasLearnedEffect(EKalmalaSupportEffect::Mending)
        || Support->HasLearnedEffect(EKalmalaSupportEffect::HearthShield)
        || Support->HasLearnedEffect(EKalmalaSupportEffect::BearsVigor)
        || Support->HasLearnedEffect(EKalmalaSupportEffect::DeerCall);
}

void UKalmalaTutorialSubsystem::ShowBeat(const EKalmalaTutorialBeat Beat, const bool bMarkSeen)
{
    if (Beat == EKalmalaTutorialBeat::None || LocalController == nullptr) return;
    if (PromptWidget == nullptr)
    {
        PromptWidget = CreateWidget<UKalmalaTutorialPromptWidget>(LocalController);
        if (PromptWidget == nullptr) return;
        PromptWidget->AddToPlayerScreen(70);
    }
    CurrentBeat = Beat;
    LastBeat = Beat;
    CurrentBeatSecondsRemaining = PromptDurationSeconds;
    if (bMarkSeen) SeenBeats.Add(Beat);

    const UEnum* BeatNames = StaticEnum<EKalmalaTutorialBeat>();
    const FString Title = (BeatNames != nullptr ? BeatNames->GetDisplayNameTextByValue(static_cast<int64>(Beat)).ToString() : TEXT("Field note")) + TEXT(" · optional");
    PromptWidget->SetPrompt(Beat, Title, BuildBody(Beat));
    PromptWidget->SetPromptVisible(true);
}

void UKalmalaTutorialSubsystem::HideCurrentBeat()
{
    CurrentBeat = EKalmalaTutorialBeat::None;
    CurrentBeatSecondsRemaining = 0.0f;
    if (PromptWidget != nullptr) PromptWidget->SetPromptVisible(false);
}

void UKalmalaTutorialSubsystem::DismissPrompt()
{
    if (CurrentBeat != EKalmalaTutorialBeat::None) LastBeat = CurrentBeat;
    HideCurrentBeat();
}

void UKalmalaTutorialSubsystem::RevisitPrompt()
{
    if (LastBeat != EKalmalaTutorialBeat::None) ShowBeat(LastBeat, false);
}

void UKalmalaTutorialSubsystem::NoteAttackIntent()
{
    AttackIntentSecondsRemaining = 4.0f;
}

FString UKalmalaTutorialSubsystem::BuildBody(const EKalmalaTutorialBeat Beat) const
{
    switch (Beat)
    {
    case EKalmalaTutorialBeat::Arrive:
        return TEXT("Movement, looking, jumping, and sprinting are available whenever you need them.");
    case EKalmalaTutorialBeat::Interact:
        return TEXT("A nearby usable object is in view. Face it and choose its displayed action; the server checks every request.");
    case EKalmalaTutorialBeat::Gather:
        return TEXT("Gather what you need from the wilderness. Your pack shows what was accepted while the node remains visible.");
    case EKalmalaTutorialBeat::Prepare:
        return TEXT("Choose what to make, then place it where the terrain and your materials allow. The server checks materials and placement.");
    case EKalmalaTutorialBeat::Weather:
        return TEXT("Weather changes comfort and travel. Shelter, cover, and a lit hearth are options; your status shows current effects.");
    case EKalmalaTutorialBeat::Explore:
        return TEXT("Pick a heading and see what the generated land offers. The map is for orientation, not a required route.");
    case EKalmalaTutorialBeat::OptionalEncounter:
        return TEXT("You can engage or move on. Hits and results are confirmed by the server.");
    case EKalmalaTutorialBeat::Discovery:
        return TEXT("This visible discovery is optional. Choose Interact if you want to investigate.");
    case EKalmalaTutorialBeat::SupportMagic:
        return TEXT("Learned support effects can help an eligible ally or situation. Choose a valid target and action when the interface allows.");
    case EKalmalaTutorialBeat::Return:
        return TEXT("At a visible camp, choose Use to tend the hearth or open storage. You can shelter here or keep exploring.");
    default:
        return FString();
    }
}

void UKalmalaTutorialSubsystem::Deinitialize()
{
    ReleaseController();
    SeenBeats.Reset();
    bHasInitialPawnLocation = false;
    Super::Deinitialize();
}
