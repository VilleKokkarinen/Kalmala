#include "KalmalaCharacter.h"
#include "KalmalaResistanceComponent.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaHarvestNode.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaM9SourceLootContract.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaCombatComponent.h"
#include "KalmalaDiscoveryProgressComponent.h"
#include "KalmalaSupportMagicComponent.h"
#include "KalmalaSkillProgressionComponent.h"
#include "KalmalaPlayerStatusComponent.h"
#include "KalmalaOceanTravelFeedbackComponent.h"
#include "GameFramework/PlayerState.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "KalmalaInteractable.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaIslandLocator.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaGameMode.h"
#include "KalmalaWorldPlayerStartResolver.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Net/UnrealNetwork.h"
#include "OnlineSubsystemTypes.h"
#include "KalmalaCharacterMovementComponent.h"
#include "KalmalaPlayerModelComponent.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "KalmalaGeneratedTerrainPatch.h"
#include "KalmalaTerrainPatchLayout.h"
#include "KalmalaOceanTravelTestFixture.h"
#include "KalmalaOceanSkiff.h"

AKalmalaCharacter::AKalmalaCharacter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UKalmalaCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
    CreateDefaultSubobject<UKalmalaResistanceComponent>(TEXT("Resistances"));
    bReplicates = true;
    Inventory = CreateDefaultSubobject<UKalmalaInventoryComponent>(TEXT("Inventory"));
    Crafting = CreateDefaultSubobject<UKalmalaCraftingComponent>(TEXT("Crafting"));
    Combat = CreateDefaultSubobject<UKalmalaCombatComponent>(TEXT("Combat"));
    DiscoveryProgress = CreateDefaultSubobject<UKalmalaDiscoveryProgressComponent>(TEXT("DiscoveryProgress"));
    SupportMagic = CreateDefaultSubobject<UKalmalaSupportMagicComponent>(TEXT("SupportMagic"));
    SkillProgression = CreateDefaultSubobject<UKalmalaSkillProgressionComponent>(TEXT("SkillProgression"));
    Statuses = CreateDefaultSubobject<UKalmalaPlayerStatusComponent>(TEXT("Statuses"));
    OceanTravelFeedback = CreateDefaultSubobject<UKalmalaOceanTravelFeedbackComponent>(TEXT("OceanTravelFeedback"));
    SetReplicateMovement(true);

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
    BaselineMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
    GetCharacterMovement()->JumpZVelocity = 500.0f;
    GetCharacterMovement()->AirControl = 0.25f;
    JumpMaxCount = 1;

    PlayerModel = CreateDefaultSubobject<UKalmalaPlayerModelComponent>(TEXT("PlayerModel"));
    PlayerModel->SetupAttachment(RootComponent);
    PlayerModel->SetRelativeLocation(FVector(0, 0, -GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()));

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 320.0f;
    CameraBoom->TargetOffset = FVector(0, 0, 45.0f);
    CameraBoom->bUsePawnControlRotation = true;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;
}

bool AKalmalaCharacter::IsExposureUpdateAllowed(const bool bServerAuthority)
{
    return bServerAuthority;
}

void AKalmalaCharacter::SetExposureStateFromServer(const FKalmalaExposureState& NewExposureState)
{
    if (!IsExposureUpdateAllowed(HasAuthority()))
    {
        return;
    }

    ExposureState.Wetness = FMath::Clamp(NewExposureState.Wetness, 0.0f, 100.0f);
    ExposureState.Warmth = FMath::Clamp(NewExposureState.Warmth, 0.0f, 100.0f);
    ExposureState.HeatIntensity = FMath::IsFinite(NewExposureState.HeatIntensity) ? FMath::Clamp(NewExposureState.HeatIntensity, 0.0f, 1.0f) : 0.0f;
    ExposureState.ColdIntensity = FMath::IsFinite(NewExposureState.ColdIntensity) ? FMath::Clamp(NewExposureState.ColdIntensity, 0.0f, 1.0f) : 0.0f;
    ExposureState.TravelSpeedMultiplier = FMath::Clamp(NewExposureState.TravelSpeedMultiplier, 0.68f, 1.0f);
    ApplyExposureTravelPenalty();
    ForceNetUpdate();
}

void AKalmalaCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AKalmalaCharacter, ExposureState);
    DOREPLIFETIME(AKalmalaCharacter, Health);
    DOREPLIFETIME_CONDITION(AKalmalaCharacter, CarriedTools, COND_OwnerOnly);
}

void AKalmalaCharacter::OnRep_Health()
{
#if !UE_BUILD_SHIPPING
    if (!HasAuthority() && (FParse::Param(FCommandLine::Get(), TEXT("KalmalaMirelingPeerTest")) || FParse::Param(FCommandLine::Get(), TEXT("KalmalaBoarPeerTest")) || FParse::Param(FCommandLine::Get(), TEXT("KalmalaDeerPeerTest"))))
    {
        UE_LOG(LogTemp, Display, TEXT("%s verification client observed replicated player health=%.1f."), FParse::Param(FCommandLine::Get(), TEXT("KalmalaBoarPeerTest")) ? TEXT("Boar") : (FParse::Param(FCommandLine::Get(), TEXT("KalmalaDeerPeerTest")) ? TEXT("Deer") : TEXT("Mireling")), Health);
    }
#endif
}

bool AKalmalaCharacter::ApplyWildlifeDamageFromServer(const AActor* SourceActor, const float Damage)
{
    FKalmalaDamageChannels Channels;
    Channels.Crush = Damage;
    return ApplyWildlifeDamageFromServer(SourceActor, Channels);
}

bool AKalmalaCharacter::ApplyWildlifeDamageFromServer(const AActor* SourceActor, const FKalmalaDamageChannels& Damage)
{
    if (!HasAuthority() || !IsValid(SourceActor) || !SourceActor->HasAuthority() || SourceActor->GetWorld() != GetWorld()
        || !Damage.IsValid() || Damage.Total() <= 0.0f || Damage.Total() > 25.0f
        || FVector::DistSquared(SourceActor->GetActorLocation(), GetActorLocation()) > FMath::Square(180.0f)) return false;
    // The first Mireling creates recoverable pressure; player defeat/respawn remains a later policy decision.
    const UKalmalaResistanceComponent* Defenses = FindComponentByClass<UKalmalaResistanceComponent>();
    float ResolvedDamage = 0.0f;
    if (!Defenses) return false;

    int32 GearArmor = 0;
    if (Inventory && !Inventory->GetEquippedGear().IsEmpty())
    {
        const UKalmalaItemCatalogue* ItemCatalogue = UKalmalaItemCatalogue::Get();
        if (!ItemCatalogue) return false;
        for (const FKalmalaEquippedGear& Gear : Inventory->GetEquippedGear())
        {
            FKalmalaGearLevelStats GearStats;
            if (!ItemCatalogue->GetGearLevelStats(Gear.ItemId, 1, GearStats)) return false;
            if (Gear.ConditionSpent < 0 || Gear.ConditionSpent >= GearStats.MaxDurability) continue;
            GearArmor += GearStats.Armor;
        }
    }

    if (!Defenses->Profile.Resolve(Damage, ResolvedDamage, GearArmor)) return false;
    const float Absorbed = SupportMagic ? SupportMagic->AbsorbHearthShieldDamageFromServer(ResolvedDamage) : 0.0f;
    Health = FMath::Clamp(Health - FMath::Max(0.0f, ResolvedDamage - Absorbed), 1.0f, 100.0f);
    if (Inventory) Inventory->WearEquippedGearFromServer();
    ForceNetUpdate();
    return true;
}

bool AKalmalaCharacter::IsMendingReceiveAllowed(const bool bServerAuthority, const bool bValidAlly, const bool bSameWorld,
    const bool bInRange, const bool bLiving, const bool bNeedsHealing, const float HealAmount)
{
    return bServerAuthority && bValidAlly && bSameWorld && bInRange && bLiving && bNeedsHealing
        && FMath::IsFinite(HealAmount) && HealAmount > 0.0f && HealAmount <= 30.0f;
}

bool AKalmalaCharacter::ReceiveMendingFromServer(const AKalmalaCharacter* SourceCharacter, const float HealAmount)
{
    const bool bValidAlly = IsValid(SourceCharacter) && SourceCharacter != this;
    const bool bSameWorld = bValidAlly && SourceCharacter->GetWorld() == GetWorld();
    const bool bInRange = bSameWorld && FVector::DistSquared(SourceCharacter->GetActorLocation(), GetActorLocation()) <= FMath::Square(350.0f);
    if (!IsMendingReceiveAllowed(HasAuthority(), bValidAlly, bSameWorld, bInRange, Health > 1.0f, Health < 100.0f, HealAmount)) return false;
    Health = FMath::Min(100.0f, Health + HealAmount);
    ForceNetUpdate();
    return true;
}

void AKalmalaCharacter::BeginPlay()
{
    Super::BeginPlay();
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaM9Schema2PeerTest")) && GetWorld() != nullptr)
    {
        M9Schema2PeerTestStartTime = GetWorld()->GetTimeSeconds();
    }
#endif
    if (HasAuthority())
    {
        CarriedTools = FKalmalaToolLifecycleContract::BuildInitialCarriedTools();
        if (Inventory)
        {
            Inventory->SynchronizeGridFromServer();
            const FName ConstructionHammerId(TEXT("ConstructionHammer"));
            const int32 HammerSlot = Inventory->GetGridSlots().IndexOfByKey(ConstructionHammerId);
            bool bHammerInFirstHotbarSlot = HammerSlot == 0;
            if (HammerSlot > 0)
            {
                bHammerInFirstHotbarSlot = Inventory->MoveSlotFromServer(
                    HammerSlot, 0, ConstructionHammerId, Inventory->GetSlotItem(0));
            }
            if (bHammerInFirstHotbarSlot) Inventory->ServerUseHotbarSlot(0);
        }
        ForceNetUpdate();
    }
    bTraversalTelemetryEnabled = FParse::Param(FCommandLine::Get(), TEXT("KalmalaTraversalTest"));
    bExposureReplicationTelemetryEnabled = FParse::Param(FCommandLine::Get(), TEXT("KalmalaExposureReplicationTest"));
    TraversalStartLocation = GetActorLocation();
    bControlsTestEnabled = FParse::Param(FCommandLine::Get(), TEXT("KalmalaPlayerControlsTest"));
    bSwimmingTestEnabled = FParse::Param(FCommandLine::Get(), TEXT("KalmalaSwimmingTest"));
    bOceanTravelTestEnabled = FParse::Param(FCommandLine::Get(), TEXT("KalmalaOceanTravelTest"));
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaDiscoveryPeerTest")) && GetWorld() != nullptr)
    {
        DiscoveryPeerTestStartTime = GetWorld()->GetTimeSeconds();
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaOceanSkiffFeedbackTest")) && GetWorld() != nullptr)
    {
        OceanTravelFeedbackPeerStartTime = GetWorld()->GetTimeSeconds();
    }
    if (bOceanTravelTestEnabled)
    {
        // The fixture compares terrain streaming and movement agreement, not
        // pawn blocking at a shared generated start.
        GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
        GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);
#if !UE_BUILD_SHIPPING
        if (UKalmalaCharacterMovementComponent* Movement = Cast<UKalmalaCharacterMovementComponent>(GetCharacterMovement()))
        {
            // The current seed's nearest island is a long-distance endpoint;
            // accelerate only this verification route, never normal swimming.
            Movement->MaxFlySpeed = 1800.0f;
        }
#endif
    }
}

void AKalmalaCharacter::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
#if !UE_BUILD_SHIPPING
    if (!bM9Schema2PeerTestLogged && !HasAuthority() && IsLocallyControlled()
        && M9Schema2PeerTestStartTime >= 0.0f && GetWorld() != nullptr)
    {
        FString TestRole;
        FString TestPhase;
        FParse::Value(FCommandLine::Get(), TEXT("KalmalaM9Schema2ClientRole="), TestRole);
        FParse::Value(FCommandLine::Get(), TEXT("KalmalaM9Schema2Phase="), TestPhase);
        int32 OtherPlayerCharacterCount = 0;
        bool bOtherToolsHidden = true;
        bool bExpectedPeerPresent = false;
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
        {
            const AKalmalaCharacter* Other = *It;
            if (Other != this && Other->GetPlayerState() != nullptr)
            {
                ++OtherPlayerCharacterCount;
                bOtherToolsHidden &= Other->GetCarriedToolInventory().IsEmpty();
                const FString OtherPlayerName = Other->GetPlayerState()->GetPlayerName();
                const TCHAR* ExpectedOtherRole = TestRole.Equals(TEXT("Owner"), ESearchCase::IgnoreCase)
                    ? TEXT("M9Schema2-observer") : TEXT("M9Schema2-owner");
                bExpectedPeerPresent |= OtherPlayerName.Equals(ExpectedOtherRole, ESearchCase::IgnoreCase);
            }
        }
        if (!TestRole.IsEmpty() && bExpectedPeerPresent
            && GetWorld()->GetTimeSeconds() - M9Schema2PeerTestStartTime >= 2.0f)
        {
            bool bOwnToolsMatch = false;
            if (TestRole.Equals(TEXT("Owner"), ESearchCase::IgnoreCase))
            {
                const FKalmalaToolState* StoneKnife = CarriedTools.FindByPredicate([](const FKalmalaToolState& State)
                {
                    return State.ItemId == TEXT("StoneKnife");
                });
                const FKalmalaToolState* BronzeAxe = CarriedTools.FindByPredicate([](const FKalmalaToolState& State)
                {
                    return State.ItemId == TEXT("BronzeAxe");
                });
                const FKalmalaToolState* IronAxe = CarriedTools.FindByPredicate([](const FKalmalaToolState& State)
                {
                    return State.ItemId == TEXT("IronAxe");
                });
                bOwnToolsMatch = CarriedTools.Num() == 3 && StoneKnife != nullptr && StoneKnife->Durability == 9
                    && BronzeAxe != nullptr && BronzeAxe->ToolLevel == 1 && BronzeAxe->Durability == 0
                    && IronAxe != nullptr && IronAxe->ToolLevel == 2 && IronAxe->Durability == 20;
            }
            else
            {
                bOwnToolsMatch = !CarriedTools.IsEmpty();
            }
            bM9Schema2PeerTestLogged = true;
            UE_LOG(LogTemp, Display,
                TEXT("M9 schema-2 candidate client: Phase=%s Role=%s Passed=%d OwnedTools=%d OtherOwnersHidden=%d OtherPlayers=%d"),
                *TestPhase, *TestRole, bOwnToolsMatch && bOtherToolsHidden ? 1 : 0,
                CarriedTools.Num(), bOtherToolsHidden ? 1 : 0, OtherPlayerCharacterCount);
        }
    }
#endif
    VerifyPlayerControls(DeltaSeconds);
    VerifyConstructionMovement(DeltaSeconds);
    VerifyForestDelivery(DeltaSeconds);
    VerifySwimming(DeltaSeconds);
    VerifyOceanTravel(DeltaSeconds);

    if (!bCombatPeerTestInvalidAttackSent && !HasAuthority() && IsLocallyControlled()
        && (FParse::Param(FCommandLine::Get(), TEXT("KalmalaCombatPeerTest")) || FParse::Param(FCommandLine::Get(), TEXT("KalmalaMirelingPeerTest")) || FParse::Param(FCommandLine::Get(), TEXT("KalmalaBoarPeerTest")) || FParse::Param(FCommandLine::Get(), TEXT("KalmalaDeerPeerTest"))) && Combat)
    {
        bCombatPeerTestInvalidAttackSent = true;
        // This fixture deliberately supplies no target, damage, or timing data.
        Combat->ServerRequestAttack(1);
    }

#if !UE_BUILD_SHIPPING
    if (IsLocallyControlled() && GetWorld() != nullptr
        && FParse::Param(FCommandLine::Get(), TEXT("KalmalaOceanWeatherPeerTest")))
    {
        static int32 LastOceanWeatherPeerCycle = INDEX_NONE;
        static bool bClientForgeryRejected = false;
        AKalmalaWorldGenerationGameState* WeatherState = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
        if (WeatherState != nullptr)
        {
            const FKalmalaWeatherState& Weather = WeatherState->GetWeatherState();
            if ((Weather.WeatherCycleIndex == 7001 || Weather.WeatherCycleIndex == 7002)
                && LastOceanWeatherPeerCycle != Weather.WeatherCycleIndex)
            {
                LastOceanWeatherPeerCycle = Weather.WeatherCycleIndex;
                if (!HasAuthority() && Weather.WeatherCycleIndex == 7001)
                {
                    FKalmalaWeatherState ForgedWeather = Weather;
                    ForgedWeather.WeatherCycleIndex = 7003;
                    ForgedWeather.WindDirectionDegrees = 270;
                    ForgedWeather.WindStrength = 0.0f;
                    ForgedWeather.RefreshActivityLevel();
                    WeatherState->SetWeatherStateFromServer(ForgedWeather);
                    const FKalmalaWeatherState& AfterForgery = WeatherState->GetWeatherState();
                    bClientForgeryRejected = AfterForgery.WeatherCycleIndex == 7001
                        && AfterForgery.WindDirectionDegrees == 90
                        && FMath::IsNearlyEqual(AfterForgery.WindStrength, 1.0f);
                }

                const FKalmalaWeatherState& AcceptedWeather = WeatherState->GetWeatherState();
                constexpr float VerificationSpeed = 350.0f;
                const float WindRate = AKalmalaOceanSkiff::CalculateWeatherYawRate(0.0f, 0.0f,
                    static_cast<float>(AcceptedWeather.WindDirectionDegrees), AcceptedWeather.WindStrength,
                    VerificationSpeed);
                const float CounterSteerRate = AKalmalaOceanSkiff::CalculateWeatherYawRate(-0.2f, 0.0f,
                    static_cast<float>(AcceptedWeather.WindDirectionDegrees), AcceptedWeather.WindStrength,
                    VerificationSpeed);
                const float ExpectedWindRate = AcceptedWeather.WeatherCycleIndex == 7001 ? 4.0f : 0.0f;
                const bool bWeatherRateMatches = FMath::IsNearlyEqual(WindRate, ExpectedWindRate, 0.001f);
                const bool bForgeryCheckMatches = HasAuthority() || bClientForgeryRejected;
                const bool bPassed = AcceptedWeather.IsValid() && bWeatherRateMatches && bForgeryCheckMatches
                    && (AcceptedWeather.WeatherCycleIndex != 7001 || CounterSteerRate < 0.0f);
                UE_LOG(LogTemp, Display,
                    TEXT("Ocean weather peer result: Authority=%d Cycle=%d Direction=%d Strength=%.3f WindRate=%.3f CounterRate=%.3f ClientForgeryRejected=%d Passed=%d"),
                    HasAuthority() ? 1 : 0, AcceptedWeather.WeatherCycleIndex,
                    AcceptedWeather.WindDirectionDegrees, AcceptedWeather.WindStrength,
                    WindRate, CounterSteerRate, bClientForgeryRejected ? 1 : 0, bPassed ? 1 : 0);
                if (!bPassed)
                {
                    UE_LOG(LogTemp, Error, TEXT("Ocean weather peer verification FAILED: accepted weather or pressure differed from the expected server state."));
                }
            }
        }
    }

    if (IsLocallyControlled() && GetWorld() != nullptr
        && FParse::Param(FCommandLine::Get(), TEXT("KalmalaOceanJourneyPeerTest")))
    {
        static TWeakObjectPtr<AKalmalaOceanSkiff> TestDrivenSkiff;
        static FVector2D TestJourneyStart = FVector2D::ZeroVector;
        AKalmalaOceanSkiff* Skiff = Cast<AKalmalaOceanSkiff>(GetAttachParentActor());
        if (Skiff != nullptr && Skiff->GetHelmOccupant() == this)
        {
            if (TestDrivenSkiff.Get() != Skiff)
            {
                TestDrivenSkiff = Skiff;
                TestJourneyStart = FVector2D(Skiff->GetActorLocation());
            }
            const float Travelled = FVector2D::Distance(
                TestJourneyStart, FVector2D(Skiff->GetActorLocation()));
            LocalOceanSkiffThrottle = Travelled < 240000.0f ? 1.0f : 0.0f;
            LocalOceanSkiffRudder = 0.0f;
            const double Now = GetWorld()->GetTimeSeconds();
            if (LocalOceanSkiffInputSequence != TNumericLimits<uint32>::Max()
                && (LastOceanSkiffInputSendTime < 0.0 || Now - LastOceanSkiffInputSendTime >= 0.1))
            {
                LastOceanSkiffInputSendTime = Now;
                ServerSubmitOceanSkiffSteeringInput(LocalOceanSkiffThrottle, LocalOceanSkiffRudder,
                    ++LocalOceanSkiffInputSequence);
            }
        }
    }

    if (IsLocallyControlled() && GetWorld() != nullptr
        && FParse::Param(FCommandLine::Get(), TEXT("KalmalaOceanJourneyPeerTest")))
    {
        static TWeakObjectPtr<AKalmalaOceanSkiff> ObservedJourneySkiff;
        static FVector2D JourneyStart = FVector2D::ZeroVector;
        static bool bJourneyAttachReported = false;
        static bool bJourneyTravelReported = false;
        static bool bJourneyCompleteReported = false;
        AKalmalaOceanSkiff* Skiff = Cast<AKalmalaOceanSkiff>(GetAttachParentActor());
        if (Skiff != nullptr)
        {
            if (ObservedJourneySkiff.Get() != Skiff)
            {
                ObservedJourneySkiff = Skiff;
                JourneyStart = FVector2D(Skiff->GetActorLocation());
                bJourneyAttachReported = false;
                bJourneyTravelReported = false;
                bJourneyCompleteReported = false;
            }

            const bool bHelm = Skiff->GetHelmOccupant() == this;
            const float JourneyDistance = FVector2D::Distance(JourneyStart, FVector2D(Skiff->GetActorLocation()));
            const AKalmalaWorldGenerationGameState* WorldState = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
            const uint64 Seed = WorldState != nullptr ? WorldState->GetWorldGenerationConfig().WorldSeed : 0;
            if (!bJourneyAttachReported)
            {
                bJourneyAttachReported = true;
                UE_LOG(LogTemp, Display,
                    TEXT("Ocean journey peer replica attached: Authority=%d Seat=%s Mode=%d Seed=%llu."),
                    HasAuthority() ? 1 : 0, bHelm ? TEXT("Helm") : TEXT("Passenger"),
                    static_cast<int32>(Skiff->GetMode()), Seed);
            }
            if (!bJourneyTravelReported && JourneyDistance >= 100000.0f
                && Skiff->GetMode() == EKalmalaOceanSkiffMode::Underway)
            {
                bJourneyTravelReported = true;
                UE_LOG(LogTemp, Display,
                    TEXT("Ocean journey peer replica observed travel: Authority=%d Seat=%s Mode=Underway Distance=%.0f Seed=%llu."),
                    HasAuthority() ? 1 : 0, bHelm ? TEXT("Helm") : TEXT("Passenger"), JourneyDistance, Seed);
            }
            if (!bJourneyCompleteReported && JourneyDistance >= 239000.0f
                && Skiff->GetMode() == EKalmalaOceanSkiffMode::Moored)
            {
                bJourneyCompleteReported = true;
                UE_LOG(LogTemp, Display,
                    TEXT("Ocean journey peer replica observed stop: Authority=%d Seat=%s Mode=Moored Distance=%.0f Seed=%llu."),
                    HasAuthority() ? 1 : 0, bHelm ? TEXT("Helm") : TEXT("Passenger"), JourneyDistance, Seed);
            }
        }
    }

    if (!bOceanTravelFeedbackPeerPrivacyLogged && !HasAuthority() && !IsLocallyControlled()
        && FParse::Param(FCommandLine::Get(), TEXT("KalmalaOceanSkiffFeedbackTest"))
        && OceanTravelFeedbackPeerStartTime >= 0.0f
        && GetWorld()->GetTimeSeconds() - OceanTravelFeedbackPeerStartTime >= 4.0f)
    {
        bOceanTravelFeedbackPeerPrivacyLogged = true;
        const UKalmalaOceanTravelFeedbackComponent* Feedback = GetOceanTravelFeedbackComponent();
        if (Feedback != nullptr && Feedback->GetFeedback() == EKalmalaOceanTravelFeedback::None
            && Feedback->GetFeedbackSerial() == 0)
        {
            UE_LOG(LogTemp, Display, TEXT("Ocean skiff feedback verification client retained no other-owner feedback."));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Ocean skiff feedback verification client received another player's feedback."));
        }
    }

    if (!bDiscoveryPeerPrivacyLogged && !HasAuthority() && IsLocallyControlled()
        && FParse::Param(FCommandLine::Get(), TEXT("KalmalaDiscoveryPeerTest"))
        && DiscoveryPeerTestStartTime >= 0.0f && GetWorld()->GetTimeSeconds() - DiscoveryPeerTestStartTime >= 4.0f)
    {
        bDiscoveryPeerPrivacyLogged = true;
        const UKalmalaDiscoveryProgressComponent* Progress = GetDiscoveryProgressComponent();
        if (Progress != nullptr && Progress->GetFeedback() == EKalmalaDiscoveryFeedback::None && Progress->GetFeedbackSerial() == 0 && Progress->GetFeedbackLabel().IsEmpty())
        {
            UE_LOG(LogTemp, Display, TEXT("Discovery verification client retained no undiscovered remote progress feedback."));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Discovery verification client received undiscovered remote progress feedback."));
        }
    }
#endif

    if (IsLocallyControlled() && Controller && Controller->IsMoveInputIgnored())
    {
        StopSprint();
        StopJumping();
    }

    ConfigureTraversalTestTarget();
    if (!bTraversalTargetConfigured)
    {
        return;
    }

    const FVector2D RemainingOffset = TraversalTestTarget - FVector2D(GetActorLocation());
    if (RemainingOffset.SizeSquared() <= FMath::Square(180.0f))
    {
        if (!bTraversalArrivalLogged)
        {
            bTraversalArrivalLogged = true;
            UE_LOG(LogTemp, Display, TEXT("Traversal-test %s pawn %s reached the Shimmering Lakes target."), HasAuthority() ? TEXT("server") : TEXT("client"), *GetName());
        }
        return;
    }

    if (IsLocallyControlled())
    {
        GetCharacterMovement()->MaxWalkSpeed = 1800.0f;
        AddMovementInput(FVector(RemainingOffset.GetSafeNormal(), 0.0f), 1.0f, true);
    }
}

void AKalmalaCharacter::ConfigureTraversalTestTarget()
{
    if (!bTraversalTelemetryEnabled || bTraversalTargetConfigured || GetWorld() == nullptr)
    {
        return;
    }

    const AKalmalaWorldGenerationGameState* WorldGenerationState = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
    if (WorldGenerationState == nullptr || !WorldGenerationState->GetWorldGenerationConfig().IsValid())
    {
        return;
    }

    const FKalmalaWorldGenerationConfig& Config = WorldGenerationState->GetWorldGenerationConfig();
    const FVector StartLocation = FKalmalaWorldPlayerStartResolver::ResolveStartTransform(Config).GetLocation();
    const FVector2D StartPosition(StartLocation);
    float ClosestDistanceSquared = TNumericLimits<float>::Max();
    for (int32 Y = -12000; Y <= 12000; Y += 250)
    {
        for (int32 X = -12000; X <= 12000; X += 250)
        {
            const FVector2D Candidate = StartPosition + FVector2D(X, Y);
            if (FKalmalaShimmeringLakeSampler::IsWater(Config, Candidate))
            {
                const float DistanceSquared = FVector2D::DistSquared(StartPosition, Candidate);
                if (DistanceSquared < ClosestDistanceSquared)
                {
                    ClosestDistanceSquared = DistanceSquared;
                    TraversalTestTarget = Candidate;
                }
            }
        }
    }

    bTraversalTargetConfigured = ClosestDistanceSquared != TNumericLimits<float>::Max();
}

void AKalmalaCharacter::OnRep_ExposureState()
{
    ApplyExposureTravelPenalty();
    if (GetPlayerState() && FParse::Param(FCommandLine::Get(), TEXT("KalmalaCampChoiceTest")))
    {
        UE_LOG(LogTemp, Display, TEXT("Camp choice client %s: Wetness=%.2f Warmth=%.2f Heat=%.2f Cold=%.2f Travel=%.2f."), *FString::FromInt(GetPlayerState()->GetPlayerId()), ExposureState.Wetness, ExposureState.Warmth, ExposureState.HeatIntensity, ExposureState.ColdIntensity, ExposureState.TravelSpeedMultiplier);
    }
    if (bExposureReplicationTelemetryEnabled)
    {
        UE_LOG(LogTemp, Display, TEXT("Exposure replication test client received state: Wetness=%.2f Warmth=%.2f Heat=%.2f Cold=%.2f TravelMultiplier=%.2f."), ExposureState.Wetness, ExposureState.Warmth, ExposureState.HeatIntensity, ExposureState.ColdIntensity, ExposureState.TravelSpeedMultiplier);
    }
}

void AKalmalaCharacter::ApplyExposureTravelPenalty()
{
    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        // M3 status definitions now own movement penalties. Legacy exposure is telemetry only.
        Movement->MaxWalkSpeed = BaselineMaxWalkSpeed;
    }
}

void AKalmalaCharacter::OnRep_ReplicatedMovement()
{
    Super::OnRep_ReplicatedMovement();

    if (bTraversalTelemetryEnabled && !HasAuthority() && !bTraversalMovementLogged
        && FVector::DistSquared2D(TraversalStartLocation, GetActorLocation()) >= FMath::Square(3000.0f))
    {
        bTraversalMovementLogged = true;
        UE_LOG(LogTemp, Display, TEXT("Traversal-test client observed replicated pawn movement of at least 3,000 units."));
    }
}

void AKalmalaCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AKalmalaCharacter::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AKalmalaCharacter::MoveRight);
    PlayerInputComponent->BindAxis(TEXT("Turn"), this, &APawn::AddControllerYawInput);
    PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &APawn::AddControllerPitchInput);
    PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &AKalmalaCharacter::RequestInteract);
    PlayerInputComponent->BindAction(TEXT("Attack"), IE_Pressed, this, &AKalmalaCharacter::RequestAttack);
    PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ACharacter::Jump);
    PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
    PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Pressed, this, &AKalmalaCharacter::StartSprint);
    PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Released, this, &AKalmalaCharacter::StopSprint);
    for (int32 Slot = 0; Slot < UKalmalaInventoryComponent::Columns; ++Slot)
    {
        FInputActionBinding Binding(FName(*FString::Printf(TEXT("Hotbar%d"), (Slot + 1) % 10)), IE_Pressed);
        Binding.ActionDelegate.GetDelegateForManualSet().BindWeakLambda(this, [this, Slot]()
        {
            if (IsLocallyControlled() && Controller && !Controller->IsMoveInputIgnored()
                && !Controller->IsLookInputIgnored() && Inventory) Inventory->ServerUseHotbarSlot(Slot);
        });
        PlayerInputComponent->AddActionBinding(Binding);
    }
}

void AKalmalaCharacter::ConfigureSwimmingTestTarget()
{
    if (!bSwimmingTestEnabled || bSwimmingTargetConfigured || GetWorld() == nullptr) return;
    const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
    if (State == nullptr || !State->GetWorldGenerationConfig().IsValid()) return;
    const FVector2D Start(GetActorLocation());
    SwimmingTestStart = Start;
    float ClosestDistanceSquared = TNumericLimits<float>::Max();
    for (int32 Y = -12000; Y <= 12000; Y += 250)
    {
        for (int32 X = -12000; X <= 12000; X += 250)
        {
            const FVector2D Candidate = Start + FVector2D(X, Y);
            if (FKalmalaOceanSampler::Sample(State->GetWorldGenerationConfig(), Candidate).WaterDepth >= 150.0f)
            {
                const float DistanceSquared = FVector2D::DistSquared(Start, Candidate);
                if (DistanceSquared < ClosestDistanceSquared) { ClosestDistanceSquared = DistanceSquared; SwimmingTestTarget = Candidate; }
            }
        }
    }
    bSwimmingTargetConfigured = ClosestDistanceSquared != TNumericLimits<float>::Max();
}

void AKalmalaCharacter::VerifySwimming(const float DeltaSeconds)
{
    ConfigureSwimmingTestTarget();
    if (!bSwimmingTargetConfigured) return;
    if (IsLocallyControlled() && !bSwimmingReturnLogged)
    {
        const FVector2D Goal = bSwimmingEntryLogged ? SwimmingTestStart : SwimmingTestTarget;
        const FVector2D Remaining = Goal - FVector2D(GetActorLocation());
        if (Remaining.SizeSquared() > FMath::Square(100.0f))
        {
            AddMovementInput(FVector(Remaining.GetSafeNormal(), 0.0f), 1.0f, true);
        }
    }
    if (Cast<UKalmalaCharacterMovementComponent>(GetCharacterMovement())->IsSwimmingInGeneratedOcean() && !bSwimmingEntryLogged)
    {
        bSwimmingEntryLogged = true;
        UE_LOG(LogTemp, Display, TEXT("Swimming test %s entered generated ocean. Authority=%d."), IsLocallyControlled() ? TEXT("owner") : TEXT("replica"), HasAuthority() ? 1 : 0);
    }
    if (bSwimmingEntryLogged && !Cast<UKalmalaCharacterMovementComponent>(GetCharacterMovement())->IsSwimmingInGeneratedOcean()
        && FVector2D::DistSquared(FVector2D(GetActorLocation()), SwimmingTestStart) <= FMath::Square(150.0f) && !bSwimmingReturnLogged)
    {
        bSwimmingReturnLogged = true;
        UE_LOG(LogTemp, Display, TEXT("Swimming test %s returned to land. Authority=%d."), IsLocallyControlled() ? TEXT("owner") : TEXT("replica"), HasAuthority() ? 1 : 0);
    }
}

void AKalmalaCharacter::ConfigureOceanTravelTarget()
{
    if (!bOceanTravelTestEnabled || bOceanTravelTargetConfigured || GetWorld() == nullptr)
    {
        return;
    }

    const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
    if (State == nullptr || !State->GetWorldGenerationConfig().IsValid())
    {
        return;
    }

    for (TActorIterator<AKalmalaOceanTravelTestFixture> Iterator(GetWorld()); Iterator; ++Iterator)
    {
        if (Iterator->IsConfigured())
        {
            OceanTravelWaypoint = Iterator->GetEntryPoint();
            OceanTravelTarget = Iterator->GetTargetPoint();
            bOceanTravelTargetConfigured = true;
            UE_LOG(LogTemp, Display, TEXT("Ocean travel test adopted the spawned deep-water fixture at %s toward seeded island %s."),
                *OceanTravelWaypoint.ToString(), *OceanTravelTarget.ToString());
            return;
        }
    }
    UE_LOG(LogTemp, Verbose, TEXT("Ocean travel test is waiting for its spawned deep-water fixture to replicate."));
}

void AKalmalaCharacter::VerifyOceanTravel(const float DeltaSeconds)
{
    ConfigureOceanTravelTarget();
    if (!bOceanTravelTargetConfigured)
    {
        return;
    }

    if (bOceanTravelArrivalLogged)
    {
        AuditOceanTravelTerrain();
        return;
    }

    const FVector2D Goal = bOceanTravelHeadingToOcean ? OceanTravelWaypoint : OceanTravelTarget;
    const FVector2D Remaining = Goal - FVector2D(GetActorLocation());
    if (IsLocallyControlled() && Remaining.SizeSquared() > FMath::Square(260.0f))
    {
        AddMovementInput(FVector(Remaining.GetSafeNormal(), 0.0f), 1.0f, true);
    }

    const UKalmalaCharacterMovementComponent* Movement = Cast<UKalmalaCharacterMovementComponent>(GetCharacterMovement());
    if (IsLocallyControlled() && Movement != nullptr && Movement->IsSwimmingInGeneratedOcean() && !bOceanTravelOceanEntryLogged)
    {
        bOceanTravelOceanEntryLogged = true;
        bOceanTravelHeadingToOcean = false;
        UE_LOG(LogTemp, Display, TEXT("Ocean travel test owner entered open ocean. Authority=%d."), HasAuthority() ? 1 : 0);
    }

    if (IsLocallyControlled() && !bOceanTravelHeadingToOcean && Remaining.SizeSquared() <= FMath::Square(260.0f))
    {
        bOceanTravelArrivalLogged = true;
        UE_LOG(LogTemp, Display, TEXT("Ocean travel test owner reached the seeded island. Authority=%d."), HasAuthority() ? 1 : 0);
        AuditOceanTravelTerrain();
    }
}

void AKalmalaCharacter::AuditOceanTravelTerrain()
{
    if (!IsLocallyControlled() || bOceanTravelTerrainAuditLogged || GetWorld() == nullptr)
    {
        return;
    }

    const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
    if (State == nullptr || !State->GetWorldGenerationConfig().IsValid())
    {
        return;
    }

    const FVector2D Origin(FKalmalaWorldPlayerStartResolver::ResolveStartTransform(State->GetWorldGenerationConfig()).GetLocation());
    const FIntPoint Centre = FKalmalaTerrainPatchLayout::GetPatchCoordinate(Origin, FVector2D(GetActorLocation()));
    TSet<FIntPoint> ObservedPatches;
    bool bDuplicateDescriptor = false;
    for (TActorIterator<AKalmalaGeneratedTerrainPatch> Iterator(GetWorld()); Iterator; ++Iterator)
    {
        const AKalmalaGeneratedTerrainPatch* Patch = *Iterator;
        if (Patch == nullptr || !Patch->HasGenerationData())
        {
            continue;
        }
        const FIntPoint Coordinate = FKalmalaTerrainPatchLayout::GetPatchCoordinate(Origin, Patch->GetGeneratedPatchCenter());
        bDuplicateDescriptor |= ObservedPatches.Contains(Coordinate);
        ObservedPatches.Add(Coordinate);
    }

    int32 MissingNeighborhoodPatches = 0;
    for (int32 Y = Centre.Y - 1; Y <= Centre.Y + 1; ++Y)
    {
        for (int32 X = Centre.X - 1; X <= Centre.X + 1; ++X)
        {
            MissingNeighborhoodPatches += ObservedPatches.Contains(FIntPoint(X, Y)) ? 0 : 1;
        }
    }

    if (MissingNeighborhoodPatches > 0)
    {
        if (GetWorld()->GetTimeSeconds() >= OceanTravelTerrainAuditNextLogTime)
        {
            OceanTravelTerrainAuditNextLogTime = GetWorld()->GetTimeSeconds() + 5.0f;
            UE_LOG(LogTemp, Display, TEXT("Ocean travel terrain audit waiting: %d replicated descriptors, %d missing island-neighborhood patches. Authority=%d."),
                ObservedPatches.Num(), MissingNeighborhoodPatches, HasAuthority() ? 1 : 0);
        }
        return;
    }

    bOceanTravelTerrainAuditLogged = true;
    if (bDuplicateDescriptor)
    {
        UE_LOG(LogTemp, Error, TEXT("Ocean travel terrain audit failed: duplicate replicated terrain patch descriptor."));
        return;
    }
    UE_LOG(LogTemp, Display, TEXT("Ocean travel terrain audit passed: %d unique replicated terrain patches and a complete island neighborhood. Authority=%d."),
        ObservedPatches.Num(), HasAuthority() ? 1 : 0);
}

void AKalmalaCharacter::StartSprint()
{
    if (IsLocallyControlled() && Controller && !Controller->IsMoveInputIgnored())
    {
        CastChecked<UKalmalaCharacterMovementComponent>(GetCharacterMovement())->SetSprintRequested(true);
    }
}

void AKalmalaCharacter::StopSprint()
{
    CastChecked<UKalmalaCharacterMovementComponent>(GetCharacterMovement())->SetSprintRequested(false);
}

void AKalmalaCharacter::MoveForward(const float Value)
{
#if !UE_BUILD_SHIPPING
    if (IsLocallyControlled() && (FParse::Param(FCommandLine::Get(), TEXT("KalmalaOceanJourneyPeerTest"))
        || FParse::Param(FCommandLine::Get(), TEXT("KalmalaOceanIntegratedJourneyPeerTest")))) return;
#endif
    if (AKalmalaOceanSkiff* Skiff = Cast<AKalmalaOceanSkiff>(GetAttachParentActor()))
    {
        if (IsLocallyControlled() && Skiff->GetHelmOccupant() == this)
        {
            LocalOceanSkiffThrottle = FMath::IsFinite(Value) ? FMath::Clamp(Value, -1.0f, 1.0f) : 0.0f;
            SendOceanSkiffSteeringInput();
        }
        return;
    }
    if (Controller != nullptr && !FMath::IsNearlyZero(Value))
    {
        const FRotator ControlRotation = Controller->GetControlRotation();
        const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
        AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Value);
    }
}

void AKalmalaCharacter::MoveRight(const float Value)
{
#if !UE_BUILD_SHIPPING
    if (IsLocallyControlled() && (FParse::Param(FCommandLine::Get(), TEXT("KalmalaOceanJourneyPeerTest"))
        || FParse::Param(FCommandLine::Get(), TEXT("KalmalaOceanIntegratedJourneyPeerTest")))) return;
#endif
    if (AKalmalaOceanSkiff* Skiff = Cast<AKalmalaOceanSkiff>(GetAttachParentActor()))
    {
        if (IsLocallyControlled() && Skiff->GetHelmOccupant() == this)
        {
            LocalOceanSkiffRudder = FMath::IsFinite(Value) ? FMath::Clamp(Value, -1.0f, 1.0f) : 0.0f;
            SendOceanSkiffSteeringInput();
        }
        return;
    }
    if (Controller != nullptr && !FMath::IsNearlyZero(Value))
    {
        const FRotator ControlRotation = Controller->GetControlRotation();
        const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
        AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Value);
    }
}

namespace
{
bool TraceInteractionCandidate(const AKalmalaCharacter* Pawn, const FVector& ViewLocation,
    const FRotator& ViewRotation, const float Range, FHitResult& OutHit)
{
    if (ViewLocation.ContainsNaN() || ViewRotation.ContainsNaN() || !FMath::IsFinite(Range) || Range <= 0) return false;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(KalmalaInteraction), false, Pawn);
    const FVector Direction = ViewRotation.Vector();
    if (Pawn->GetWorld()->LineTraceSingleByChannel(OutHit, ViewLocation,
        ViewLocation + Direction * Range, ECC_Visibility, Params)) return true;

    // The third-person boom must not consume physical wood-pickup reach.
    // Extend only the visibility query; every other interaction keeps its trace.
    const float CameraOffset = FVector::Distance(ViewLocation, Pawn->GetActorLocation());
    if (!FMath::IsFinite(CameraOffset) || CameraOffset > 500.0f) return false;
    FHitResult ExtendedHit;
    if (!Pawn->GetWorld()->LineTraceSingleByChannel(ExtendedHit, ViewLocation,
        ViewLocation + Direction * (CameraOffset + Range), ECC_Visibility, Params)) return false;
    const auto* Drop = Cast<AKalmalaHarvestNode>(ExtendedHit.GetActor());
    if (!Drop || !Drop->IsWoodDrop() || Drop->IsHarvested()) return false;
    const float PhysicalDistance = FVector::Distance(Pawn->GetActorLocation(), Drop->GetActorLocation());
    if (!FMath::IsFinite(PhysicalDistance) || PhysicalDistance > Range) return false;
    OutHit = ExtendedHit;
    OutHit.Distance = PhysicalDistance;
    return true;
}
}

bool AKalmalaCharacter::GetLocalInteractionCandidate(FHitResult& OutHit) const
{
    OutHit = FHitResult();
    if (!IsLocallyControlled() || Controller == nullptr || GetWorld() == nullptr) return false;

    FVector ViewLocation;
    FRotator ViewRotation;
    Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
    if (ViewLocation.ContainsNaN() || ViewRotation.ContainsNaN()
        || !FMath::IsFinite(InteractionRange) || InteractionRange <= 0.0f) return false;

    return TraceInteractionCandidate(this, ViewLocation, ViewRotation, InteractionRange, OutHit);
}

bool AKalmalaCharacter::GetLocalHarvestInteractionIntent(const AKalmalaHarvestNode* Node,
    FName& OutItemId, uint8& OutAction, bool& bOutHasUsableTool) const
{
    OutItemId = NAME_None;
    OutAction = 0;
    bOutHasUsableTool = false;
    if (!IsLocallyControlled() || !IsValid(Node)) return false;

    if (Node->IsTreeResource())
    {
        bOutHasUsableTool = Node->IsWoodDrop() && !Node->IsHarvested();
        return true;
    }

    if (Node->GetGatheringSourceId().IsNone())
    {
        bOutHasUsableTool = !Node->IsHarvested();
        return true;
    }

    FKalmalaToolServerSelection Selection;
    if (!FKalmalaToolLifecycleContract::BuildServerSelection(Node->GetGatheringSourceId(), Selection)) return false;

    const FKalmalaToolDefinition* Definition = nullptr;
    int32 DefinitionLevel = 0;
    for (const FKalmalaToolDefinition& Candidate : FKalmalaToolLifecycleContract::GetTieredToolDefinitions())
    {
        const int32 CandidateLevel = GetCarriedToolLevel(Candidate.ItemId);
        if (CandidateLevel < 1 || !FKalmalaToolLifecycleContract::IsToolSuitableForSelection(
                Candidate, CandidateLevel, Selection)
            || GetToolDurability(Candidate.ItemId) <= 0) continue;
        if (Definition == nullptr || FKalmalaToolLifecycleContract::GetToolTier(Candidate.Kind, CandidateLevel)
            > FKalmalaToolLifecycleContract::GetToolTier(Definition->Kind, DefinitionLevel))
        {
            Definition = &Candidate;
            DefinitionLevel = CandidateLevel;
        }
    }
    if (Definition == nullptr)
    {
        Definition = FKalmalaToolLifecycleContract::FindMinimumQualifiedTool(Selection);
        DefinitionLevel = Definition ? GetCarriedToolLevel(Definition->ItemId) : 0;
    }
    if (Inventory && FKalmalaToolLifecycleContract::FindDefinition(Inventory->GetActiveItem()))
    {
        Definition = FKalmalaToolLifecycleContract::FindDefinition(Inventory->GetActiveItem());
        DefinitionLevel = GetCarriedToolLevel(Definition->ItemId);
    }
    if (Definition == nullptr) return false;

    OutItemId = Definition->ItemId;
    OutAction = static_cast<uint8>(Selection.Action);
    bOutHasUsableTool = GetToolDurability(Definition->ItemId) > 0
        && FKalmalaToolLifecycleContract::IsToolSuitableForSelection(*Definition, DefinitionLevel, Selection);
    return true;
}

void AKalmalaCharacter::RequestInteract()
{
    if (IsLocallyControlled() && Controller && !Controller->IsMoveInputIgnored())
    {
        FName ClientItemId = NAME_None;
        uint8 ClientAction = 0;
        FHitResult Hit;
        if (GetLocalInteractionCandidate(Hit))
        {
            if (const AKalmalaHarvestNode* Node = Cast<AKalmalaHarvestNode>(Hit.GetActor()))
            {
                bool bHasUsableTool = false;
                GetLocalHarvestInteractionIntent(Node, ClientItemId, ClientAction, bHasUsableTool);
                (void)bHasUsableTool;
            }
        }
        ServerRequestInteract(ClientItemId, ClientAction);
    }
}

void AKalmalaCharacter::SendOceanSkiffSteeringInput()
{
    if (!IsLocallyControlled() || Controller == nullptr || Controller->IsMoveInputIgnored()
        || Cast<AKalmalaOceanSkiff>(GetAttachParentActor()) == nullptr || GetWorld() == nullptr
        || LocalOceanSkiffInputSequence == TNumericLimits<uint32>::Max())
    {
        return;
    }

    const double Now = GetWorld()->GetTimeSeconds();
    if (LastOceanSkiffInputSendTime >= 0.0 && Now - LastOceanSkiffInputSendTime < 0.1) return;
    LastOceanSkiffInputSendTime = Now;
    ServerSubmitOceanSkiffSteeringInput(LocalOceanSkiffThrottle, LocalOceanSkiffRudder,
        ++LocalOceanSkiffInputSequence);
}

void AKalmalaCharacter::RequestAttack()
{
    static uint32 LocalAttackSequence = 0;
    if (IsLocallyControlled() && Controller && !Controller->IsMoveInputIgnored() && Combat)
    {
        Combat->ServerRequestAttack(++LocalAttackSequence);
    }
}





void AKalmalaCharacter::ServerRequestInteract_Implementation(const FName ClientItemId, const uint8 ClientAction)
{
    if (AKalmalaOceanSkiff* CurrentSkiff = Cast<AKalmalaOceanSkiff>(GetAttachParentActor()))
    {
        CurrentSkiff->TryDisembarkFromServer(this);
        return;
    }

    if (Controller == nullptr || GetWorld() == nullptr)
    {
        return;
    }

    FVector ViewLocation;
    FRotator ViewRotation;
    Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);

    FHitResult Hit;
    const bool bInteractionHit = TraceInteractionCandidate(this, ViewLocation, ViewRotation, InteractionRange, Hit);
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaForestPickup"))
        && !IsLocallyControlled() && bForestDropRangeStarted && ForestDropRangeRequests < 3
        && ClientItemId.IsNone() && ClientAction == 0)
    {
        AKalmalaHarvestNode* Probe = nullptr;
        for (TActorIterator<AKalmalaHarvestNode> Node(GetWorld()); Node; ++Node)
            if (Node->GetPersistentSpawnId() == ForestRemoteRangeId) { Probe = *Node; break; }
        const int32 Delta = Inventory->GetQuantity(ForestPickupItem) - ForestPickupBefore;
        if (!Probe || bInteractionHit || Delta != 0 || Probe->IsHarvested() || !Probe->IsWoodDrop()
            || FVector::DistSquared(GetActorLocation(), Probe->GetActorLocation()) <= FMath::Square(500.0f))
        {
            UE_LOG(LogTemp, Error, TEXT("Forest delivery: Passed=0 RemoteDropRange Id=%s Delta=%d Hit=%d"),
                *ForestRemoteRangeId, Delta, bInteractionHit);
        }
        else
        {
            // Exercise the independent pawn-distance guard from this remote
            // RPC, with a trusted fixture short trace rather than the miss above.
            // This does not change the production trace or client payload.
            const bool bAccepted = Probe->InteractWithToolIntentFromServer(this,
                0.0f, InteractionRange, NAME_None, 0);
            const int32 After = Inventory->GetQuantity(ForestPickupItem) - ForestPickupBefore;
            if (bAccepted || After != 0 || Probe->IsHarvested() || !Probe->IsWoodDrop())
            {
                UE_LOG(LogTemp, Error, TEXT("Forest delivery: Passed=0 RemotePhysicalRange Id=%s Delta=%d Accepted=%d"),
                    *ForestRemoteRangeId, After, bAccepted);
                return;
            }
            ++ForestDropRangeRequests;
            UE_LOG(LogTemp, Display, TEXT("Forest remote physical rejection: Passed=1 Authority=1 Id=%s Item=%s Delta=0 LiveDrop=1 FixtureTrace=0 Requests=%d"),
                *ForestRemoteRangeId, *ForestPickupItem.ToString(), ForestDropRangeRequests);
            UE_LOG(LogTemp, Display, TEXT("Forest remote drop range rejection: Passed=1 Authority=1 Id=%s Item=%s Delta=0 LiveDrop=1 TraceMiss=1 Requests=%d"),
                *ForestRemoteRangeId, *ForestPickupItem.ToString(), ForestDropRangeRequests);
        }
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaForestPickup"))
        && !IsLocallyControlled() && !ForestRemoteRangeId.IsEmpty()
        && ForestRemoteRangeRequests < 3 && ClientItemId.IsNone() && ClientAction == 0)
    {
        AKalmalaHarvestNode* Probe = nullptr;
        for (TActorIterator<AKalmalaHarvestNode> Node(GetWorld()); Node; ++Node)
            if (Node->GetPersistentSpawnId() == ForestRemoteRangeId) { Probe = *Node; break; }
        const int32 Delta = Inventory->GetQuantity(ForestPickupItem) - ForestPickupBefore;
        if (!Probe || bInteractionHit || Delta != 0 || Probe->IsHarvested()
            || !Probe->CanReceiveAxeAttack() || Probe->IsFelledTrunk() || Probe->IsWoodDrop()
            || !FMath::IsFinite(ForestRemoteRangeHealth) || ForestRemoteRangeHealth <= 0
            || Probe->TreeHealth != ForestRemoteRangeHealth
            || FVector::DistSquared(GetActorLocation(), Probe->GetActorLocation()) <= FMath::Square(500.0f))
        {
            UE_LOG(LogTemp, Error, TEXT("Forest delivery: Passed=0 RemoteRange Id=%s Delta=%d Hit=%d"),
                *ForestRemoteRangeId, Delta, bInteractionHit);
        }
        else
        {
            ++ForestRemoteRangeRequests;
            UE_LOG(LogTemp, Display, TEXT("Forest remote range rejection: Passed=1 Authority=1 Id=%s Item=%s Delta=0 Standing=1 HealthUnchanged=1 TraceMiss=1 Requests=%d"),
                *ForestRemoteRangeId, *ForestPickupItem.ToString(), ForestRemoteRangeRequests);
        }
    }
#endif
    if (!bInteractionHit)
    {
        return;
    }

    AActor* Target = Hit.GetActor();
    if (Cast<AKalmalaGeneratedTerrainPatch>(Target) != nullptr
        && AKalmalaOceanSkiff::TryLaunchFromServer(this, Hit) != nullptr)
    {
        return;
    }

    if (AKalmalaOceanSkiff* Skiff = Cast<AKalmalaOceanSkiff>(Target))
    {
        Skiff->TryInteractFromServer(this);
        return;
    }

    if (AKalmalaHarvestNode* HarvestNode = Cast<AKalmalaHarvestNode>(Target);
        HarvestNode != nullptr && !HarvestNode->GetGatheringSourceId().IsNone())
    {
#if !UE_BUILD_SHIPPING
        const bool bForestForgedProbe = FParse::Param(FCommandLine::Get(), TEXT("KalmalaForestPickup"))
            && !IsLocallyControlled() && ClientItemId == TEXT("ForestForgedItem") && ClientAction == 255
            && HarvestNode->GetPersistentSpawnId().StartsWith(TEXT("forest:v1:"));
        const bool bStandingBefore = HarvestNode->CanReceiveAxeAttack() && !HarvestNode->IsFelledTrunk();
        const float ForestHealthBefore = HarvestNode->TreeHealth;
        FKalmalaToolServerSelection ForestSelection;
        const bool bForestSelection = bForestForgedProbe
            && FKalmalaToolLifecycleContract::BuildServerSelection(HarvestNode->GetGatheringSourceId(), ForestSelection);
        const int32 WoodBefore = bForestSelection ? Inventory->GetQuantity(ForestSelection.RewardItemId) : 0;
        // Queue the second collector against a live drop, then dispatch both
        // transactions without a tick or replication update between them.
        AKalmalaCharacter* ForestContender = nullptr;
        FVector ForestContenderLocation = FVector::ZeroVector;
        int32 ForestContenderBefore = 0;
        if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaForestPickup"))
            && !IsLocallyControlled() && ClientItemId.IsNone() && ClientAction == 0
            && HarvestNode->GetPersistentSpawnId() == ForestRemoteRangeId
            && HarvestNode->IsWoodDrop() && !HarvestNode->IsHarvested())
        {
            // Exercise capacity through the real remote-owned RPC's selected
            // target and trusted trace before allowing its ordinary collection.
            // Restore exact stacks before the winner/contender transactions.
            const auto OriginalStacks = Inventory->GetStacks();
            const int32 PackedTools = Inventory->CountPackedTools(GetCarriedToolInventory());
            TArray<FKalmalaInventoryStack> FullPack;
            for (int32 Cell = PackedTools; Cell < UKalmalaInventoryComponent::MaxSlots; ++Cell)
                FullPack.Add({TEXT("String"), 1});
            const bool bInstalled = ForestPickupItem != TEXT("String")
                && Inventory->TryCommitStacksFromServer(OriginalStacks, FullPack);
            const auto InstalledStacks = Inventory->GetStacks();
            const bool bInRange = Hit.Distance <= InteractionRange
                && AKalmalaHarvestNode::IsHarvestAllowed(true, false,
                    GetActorLocation(), HarvestNode->GetActorLocation());
            const int32 Before = Inventory->GetQuantity(ForestPickupItem);
            const bool bAccepted = bInstalled && HarvestNode->InteractWithToolIntentFromServer(
                this, Hit.Distance, InteractionRange, ClientItemId, ClientAction);
            const int32 Delta = Inventory->GetQuantity(ForestPickupItem) - Before;
            const bool bLiveDrop = HarvestNode->IsWoodDrop() && !HarvestNode->IsHarvested();
            const bool bRestored = bInstalled
                && Inventory->TryCommitStacksFromServer(InstalledStacks, OriginalStacks);
            if (!bInstalled || !bInRange || bAccepted || Delta != 0 || !bLiveDrop || !bRestored)
            {
                UE_LOG(LogTemp, Error, TEXT("Forest delivery: Passed=0 RemoteFullPack Id=%s Installed=%d InRange=%d Accepted=%d Delta=%d LiveDrop=%d Restored=%d"),
                    *HarvestNode->GetPersistentSpawnId(), bInstalled, bInRange, bAccepted, Delta, bLiveDrop, bRestored);
                return;
            }
            UE_LOG(LogTemp, Display, TEXT("Forest remote full pack rejection: Passed=1 Authority=1 Id=%s Item=%s Delta=0 LiveDrop=1 InRange=1 Slots=40 Restored=1"),
                *HarvestNode->GetPersistentSpawnId(), *ForestPickupItem.ToString());
            for (TActorIterator<AKalmalaCharacter> Pawn(GetWorld()); Pawn; ++Pawn)
            {
                if (*Pawn != this && Pawn->IsLocallyControlled() && Pawn->GetInventoryComponent())
                {
                    ForestContender = *Pawn;
                    ForestContenderLocation = Pawn->GetActorLocation();
                    ForestContenderBefore = Pawn->GetInventoryComponent()->GetQuantity(ForestPickupItem);
                    Pawn->SetActorLocation(HarvestNode->GetActorLocation() + FVector(150, 0, 88));
                    break;
                }
            }
        }
#endif
        [[maybe_unused]] const bool bHarvestAccepted = HarvestNode->InteractWithToolIntentFromServer(this, Hit.Distance, InteractionRange, ClientItemId, ClientAction);
#if !UE_BUILD_SHIPPING
        if (ForestContender)
        {
            const bool bInRange = AKalmalaHarvestNode::IsHarvestAllowed(true, false,
                ForestContender->GetActorLocation(), HarvestNode->GetActorLocation());
            const bool bContenderAccepted = HarvestNode->InteractWithToolIntentFromServer(
                ForestContender, 150.0f, InteractionRange, NAME_None, 0);
            const int32 ContenderDelta = ForestContender->GetInventoryComponent()->GetQuantity(ForestPickupItem) - ForestContenderBefore;
            ForestContender->SetActorLocation(ForestContenderLocation);
            if (!bInRange || !bHarvestAccepted || bContenderAccepted || ContenderDelta != 0 || !HarvestNode->IsHarvested())
            {
                UE_LOG(LogTemp, Error, TEXT("Forest delivery: Passed=0 SameDispatchCollector Id=%s Winner=%d Loser=%d Delta=%d InRange=%d"),
                    *HarvestNode->GetPersistentSpawnId(), bHarvestAccepted, bContenderAccepted, ContenderDelta, bInRange);
            }
            else
            {
                UE_LOG(LogTemp, Display, TEXT("Forest same dispatch collector: Passed=1 Authority=1 Id=%s Item=%s LiveBefore=1 Winner=1 Loser=0 Delta=0 InRange=1"),
                    *HarvestNode->GetPersistentSpawnId(), *ForestPickupItem.ToString());
            }
        }
        if (bForestForgedProbe)
        {
            if (ForestForgedId.IsEmpty()) ForestForgedId = HarvestNode->GetPersistentSpawnId();
            ++ForestForgedRequests;
            const int32 Delta = bForestSelection ? Inventory->GetQuantity(ForestSelection.RewardItemId) - WoodBefore : 0;
            if (ForestForgedId != HarvestNode->GetPersistentSpawnId()
                || !bForestSelection || bHarvestAccepted || !bStandingBefore || !HarvestNode->CanReceiveAxeAttack() || HarvestNode->IsFelledTrunk()
                || HarvestNode->IsWoodDrop() || HarvestNode->IsHarvested() || Delta != 0
                || !FMath::IsFinite(ForestHealthBefore) || ForestHealthBefore <= 0.0f
                || HarvestNode->TreeHealth != ForestHealthBefore)
            {
                UE_LOG(LogTemp, Error, TEXT("Forest delivery: Passed=0 ForgedIntent Id=%s Delta=%d"),
                    *HarvestNode->GetPersistentSpawnId(), Delta);
            }
            else
            {
                UE_LOG(LogTemp, Display, TEXT("Forest forged rejection: Passed=1 Authority=1 Id=%s Item=ForestForgedItem Action=255 Delta=0 Standing=1 Requests=%d HealthUnchanged=1"),
                    *HarvestNode->GetPersistentSpawnId(), ForestForgedRequests);
            }
        }
#endif
        return;
    }

    if (IsValid(Target) && Target->Implements<UKalmalaInteractable>()
        && IKalmalaInteractable::Execute_CanInteract(Target, this))
    {
        IKalmalaInteractable::Execute_Interact(Target, this);
    }
}

void AKalmalaCharacter::ServerSubmitOceanSkiffSteeringInput_Implementation(const float Throttle,
    const float Rudder, const uint32 Sequence)
{
    AKalmalaOceanSkiff* CurrentSkiff = Cast<AKalmalaOceanSkiff>(GetAttachParentActor());
    const bool bAccepted = CurrentSkiff != nullptr
        && CurrentSkiff->AcceptSteeringFromServer(this, Throttle, Rudder, Sequence);
#if !UE_BUILD_SHIPPING
    static bool bLoggedFirstJourneySteeringRpc = false;
    if (!bLoggedFirstJourneySteeringRpc
        && FParse::Param(FCommandLine::Get(), TEXT("KalmalaOceanJourneyPeerTest")))
    {
        bLoggedFirstJourneySteeringRpc = true;
        UE_LOG(LogTemp, Display,
            TEXT("Ocean journey steering RPC reached server: Authority=%d Character=%s AttachedSkiff=%d Helm=%d Accepted=%d Sequence=%u Throttle=%.2f Rudder=%.2f."),
            HasAuthority() ? 1 : 0, *GetName(), CurrentSkiff != nullptr ? 1 : 0,
            CurrentSkiff != nullptr && CurrentSkiff->GetHelmOccupant() == this ? 1 : 0,
            bAccepted ? 1 : 0, Sequence, Throttle, Rudder);
    }
#endif
}

int32 AKalmalaCharacter::GetToolDurability(const FName ItemId) const
{
    const FName CanonicalId = FKalmalaToolLifecycleContract::CanonicalizeItemId(ItemId);
    if (Inventory && Inventory->GetActiveItem() == CanonicalId)
        if (const int32 ActiveDurability = GetToolDurability(Inventory->GetActiveInstanceGuid()); ActiveDurability >= 0)
            return ActiveDurability;
    if (const FKalmalaToolState* State = CarriedTools.FindByPredicate([CanonicalId](const FKalmalaToolState& Candidate)
        { return Candidate.ItemId == CanonicalId; })) return State->Durability;
#if WITH_EDITORONLY_DATA
    if (CanonicalId == TEXT("StoneKnife")) return StoneKnifeDurability;
    if (CanonicalId == TEXT("StoneAxe")) return StoneAxeDurability;
    if (CanonicalId == TEXT("StonePick")) return StonePickDurability;
    if (CanonicalId == TEXT("BronzeAxe")) return BronzeAxeDurability;
    if (CanonicalId == TEXT("IronAxe")) return IronAxeDurability;
#endif
    if (CanonicalId == TEXT("BronzeAxe") || CanonicalId == TEXT("IronAxe")) return -1;
    return 0;
}

int32 AKalmalaCharacter::GetToolDurability(const FGuid& InstanceGuid) const
{
    if (!InstanceGuid.IsValid()) return -1;
    const FKalmalaToolState* State = CarriedTools.FindByPredicate([&InstanceGuid](const FKalmalaToolState& Candidate)
        { return Candidate.InstanceGuid == InstanceGuid; });
    return State ? State->Durability : -1;
}

int32 AKalmalaCharacter::GetCarriedToolLevel(const FName ItemId) const
{
    const FName CanonicalId = FKalmalaToolLifecycleContract::CanonicalizeItemId(ItemId);
    if (Inventory && Inventory->GetActiveItem() == CanonicalId)
        if (const int32 ActiveLevel = GetCarriedToolLevel(Inventory->GetActiveInstanceGuid()); ActiveLevel >= 1)
            return ActiveLevel;
    const FKalmalaToolState* State = CarriedTools.FindByPredicate([CanonicalId](const FKalmalaToolState& Candidate)
    {
        return Candidate.ItemId == CanonicalId;
    });
    return State ? State->ToolLevel : 0;
}

int32 AKalmalaCharacter::GetCarriedToolLevel(const FGuid& InstanceGuid) const
{
    if (!InstanceGuid.IsValid()) return 0;
    const FKalmalaToolState* State = CarriedTools.FindByPredicate([&InstanceGuid](const FKalmalaToolState& Candidate)
    {
        return Candidate.InstanceGuid == InstanceGuid;
    });
    return State ? State->ToolLevel : 0;
}

void AKalmalaCharacter::EnsureToolInstanceGuidsFromServer()
{
    if (!HasAuthority()) return;
    TSet<FGuid> Seen;
    for (FKalmalaToolState& Tool : CarriedTools)
    {
        if (!Tool.InstanceGuid.IsValid() || Seen.Contains(Tool.InstanceGuid))
        {
            do { Tool.InstanceGuid = FGuid::NewGuid(); } while (!Tool.InstanceGuid.IsValid() || Seen.Contains(Tool.InstanceGuid));
        }
        Seen.Add(Tool.InstanceGuid);
    }
}

int32* AKalmalaCharacter::FindToolDurabilityFromServer(const FName ItemId)
{
    if (!HasAuthority()) return nullptr;
    const FName CanonicalId = FKalmalaToolLifecycleContract::CanonicalizeItemId(ItemId);
    if (Inventory && Inventory->GetActiveItem() == CanonicalId)
        if (int32* ActiveDurability = FindToolDurabilityFromServer(Inventory->GetActiveInstanceGuid()))
            return ActiveDurability;
    if (FKalmalaToolState* State = CarriedTools.FindByPredicate([CanonicalId](FKalmalaToolState& Candidate)
        { return Candidate.ItemId == CanonicalId; })) return &State->Durability;
#if WITH_EDITORONLY_DATA
    int32* LegacyCondition = nullptr;
    if (CanonicalId == TEXT("StoneKnife")) LegacyCondition = &StoneKnifeDurability;
    else if (CanonicalId == TEXT("StoneAxe")) LegacyCondition = &StoneAxeDurability;
    else if (CanonicalId == TEXT("StonePick")) LegacyCondition = &StonePickDurability;
    else if (CanonicalId == TEXT("BronzeAxe")) LegacyCondition = &BronzeAxeDurability;
    else if (CanonicalId == TEXT("IronAxe")) LegacyCondition = &IronAxeDurability;

    const FKalmalaToolDefinition* Definition = FKalmalaToolLifecycleContract::FindDefinition(CanonicalId);
    if (LegacyCondition != nullptr && Definition != nullptr && *LegacyCondition >= 0
        && *LegacyCondition <= FKalmalaToolLifecycleContract::GetMaxDurability(CanonicalId, 1)
        && Inventory && Inventory->CanFitContents(Inventory->GetStacks(), Inventory->CountPackedTools(CarriedTools) + 1))
    {
        FKalmalaToolState SeededState;
        SeededState.ItemId = CanonicalId;
        SeededState.ToolLevel = 1;
        SeededState.Durability = *LegacyCondition;
        SeededState.InstanceGuid = FGuid::NewGuid();
        CarriedTools.Add(SeededState);
        return &CarriedTools.Last().Durability;
    }
#endif
    return nullptr;
}

int32* AKalmalaCharacter::FindToolDurabilityFromServer(const FGuid& InstanceGuid)
{
    if (!HasAuthority() || !InstanceGuid.IsValid()) return nullptr;
    if (FKalmalaToolState* State = CarriedTools.FindByPredicate([&InstanceGuid](FKalmalaToolState& Candidate)
        { return Candidate.InstanceGuid == InstanceGuid; })) return &State->Durability;
    return nullptr;
}

bool AKalmalaCharacter::CommitToolHarvestFromServer(AKalmalaHarvestNode* Node, const float TraceDistance,
    const float MaximumRange, const FName ClientItemId, const uint8 ClientAction)
{
    if (!HasAuthority() || !IsValid(Node) || !Node->HasAuthority() || Node->GetWorld() != GetWorld()
        || !Node->CanInteract_Implementation(this) || !SkillProgression || !Inventory) return false;

    FKalmalaToolServerSelection Selection;
    if (!FKalmalaToolLifecycleContract::BuildServerSelection(Node->GetGatheringSourceId(), Selection)) return false;
    if (FKalmalaM9SourceLootContract::IsM9Source(Selection.SourceId))
    {
        const AKalmalaWorldGenerationGameState* WorldState = GetWorld() != nullptr
            ? GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>() : nullptr;
        int32 M9RewardQuantity = 0;
        if (WorldState == nullptr
            || !FKalmalaM9SourceLootContract::BuildHarvestRewardQuantity(
                WorldState->GetWorldGenerationConfig(), Selection.SourceId, Node->GetPersistentSpawnId(), M9RewardQuantity)
            || Node->GetM9ResourceDepletionId().IsEmpty()) return false;
        Selection.RewardQuantity = M9RewardQuantity;
    }
    const TArray<FKalmalaToolState> ExistingTools = CarriedTools;
    const FKalmalaSkillState* Skill = SkillProgression->GetServerLedger().Find(Selection.RequiredSkill);
    int32* CurrentDurability = FindToolDurabilityFromServer(ClientItemId);
    if (Skill == nullptr || CurrentDurability == nullptr) return false;

    FKalmalaToolServerContext Context;
    Context.bServerAuthority = HasAuthority() && Node->HasAuthority();
    Context.bTraceHit = true; // Called only for the actor returned by the server's interaction trace.
    Context.bSameWorld = Node->GetWorld() == GetWorld();
    Context.bNodeAvailable = !Node->IsHarvested();
    Context.TraceDistance = TraceDistance;
    Context.MaximumRange = MaximumRange;

    FKalmalaToolState CandidateToolState{ClientItemId, *CurrentDurability, GetCarriedToolLevel(ClientItemId)};
    FName RewardItemId = NAME_None;
    int32 RewardQuantity = 0;
    if (!FKalmalaToolLifecycleContract::ApplyServerUse(CandidateToolState, Context, ClientItemId,
        static_cast<EKalmalaToolAction>(ClientAction), *Skill, Selection, RewardItemId, RewardQuantity)) return false;

    TArray<FKalmalaInventoryStack> CandidateInventory;
    FString Reason;
    const TArray<FKalmalaInventoryStack> ExistingInventory = Inventory->GetStacks();
    if (!UKalmalaInventoryComponent::BuildGrant(ExistingInventory, RewardItemId, RewardQuantity, CandidateInventory, Reason)
        || !Inventory->TryCommitStacksFromServer(ExistingInventory, CandidateInventory)) return false;

    *CurrentDurability = CandidateToolState.Durability;
    AKalmalaGameMode* GameMode = GetWorld() != nullptr
        ? GetWorld()->GetAuthGameMode<AKalmalaGameMode>() : nullptr;
    if (GameMode == nullptr || !GameMode->PersistPlayerStateFromServer(this))
    {
        CarriedTools = ExistingTools;
        if (!Inventory->TryCommitStacksFromServer(CandidateInventory, ExistingInventory))
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to roll back inventory after player tool persistence rejected harvesting."));
        }
        ForceNetUpdate();
        return false;
    }

    Node->CommitHarvestedStateFromServer();
    ForceNetUpdate();
    return true;
}
