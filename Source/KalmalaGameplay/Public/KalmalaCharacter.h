#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UKalmalaPlayerModelComponent;
class UKalmalaInventoryComponent;
class UKalmalaCraftingComponent;
class UKalmalaPlayerStatusComponent;
class UKalmalaCombatComponent;
class UKalmalaDiscoveryProgressComponent;
class UKalmalaSupportMagicComponent;
class UKalmalaSkillProgressionComponent;
class UKalmalaOceanTravelFeedbackComponent;
class AKalmalaHarvestNode;
enum class EKalmalaSupportEffect : uint8;

USTRUCT(BlueprintType)
struct FKalmalaExposureState
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Exposure")
    float Wetness = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Exposure")
    float Warmth = 100.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Exposure", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float HeatIntensity = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Exposure", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ColdIntensity = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Exposure")
    float TravelSpeedMultiplier = 1.0f;
};

/**
 * Replicated player pawn for the first multiplayer traversal increment.
 * Camera state is local to the owning player; movement is handled by
 * CharacterMovement's server-authoritative replication path.
 */
UCLASS()
class KALMALAGAMEPLAY_API AKalmalaCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AKalmalaCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    const FKalmalaExposureState& GetExposureState() const { return ExposureState; }
    const UKalmalaPlayerStatusComponent* GetStatusComponent() const { return Statuses; }
    UKalmalaCombatComponent* GetCombatComponent() const { return Combat; }
    UKalmalaDiscoveryProgressComponent* GetDiscoveryProgressComponent() const { return DiscoveryProgress; }
    UKalmalaInventoryComponent* GetInventoryComponent() const { return Inventory; }
    UKalmalaSupportMagicComponent* GetSupportMagicComponent() const { return SupportMagic; }
    UKalmalaSkillProgressionComponent* GetSkillProgressionComponent() const { return SkillProgression; }
    UKalmalaOceanTravelFeedbackComponent* GetOceanTravelFeedbackComponent() const { return OceanTravelFeedback; }
    int32 GetToolDurability(FName ToolId) const;
    int32 GetCarriedToolLevel(FName ToolId) const;
    const TArray<FKalmalaToolState>& GetCarriedToolInventory() const { return CarriedTools; }
    EKalmalaSupportEffect GetSelectedSupportEffect() const;
    float GetHealth() const { return Health; }
    bool ApplyWildlifeDamageFromServer(const AActor* SourceActor, float Damage);
    bool ReceiveMendingFromServer(const AKalmalaCharacter* SourceCharacter, float HealAmount);
    static bool IsMendingReceiveAllowed(bool bServerAuthority, bool bValidAlly, bool bSameWorld, bool bInRange, bool bLiving, bool bNeedsHealing, float HealAmount);
    static bool IsExposureUpdateAllowed(bool bServerAuthority);
    void SetExposureStateFromServer(const FKalmalaExposureState& NewExposureState);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    virtual void OnRep_ReplicatedMovement() override;

private:
    friend class AKalmalaHarvestNode;
    friend class UKalmalaCraftingComponent;
    friend class AKalmalaGameMode;

    bool CommitToolHarvestFromServer(AKalmalaHarvestNode* Node, float TraceDistance, float MaximumRange,
        FName ClientToolId, uint8 ClientAction);
    int32* FindToolDurabilityFromServer(FName ToolId);

    UPROPERTY(VisibleAnywhere, Category="Crafting") TObjectPtr<UKalmalaCraftingComponent> Crafting;
    UPROPERTY(VisibleAnywhere, Category="Combat") TObjectPtr<UKalmalaCombatComponent> Combat;
    UPROPERTY(VisibleAnywhere, Category="Discovery") TObjectPtr<UKalmalaDiscoveryProgressComponent> DiscoveryProgress;
    UPROPERTY(VisibleAnywhere, Category="Support") TObjectPtr<UKalmalaSupportMagicComponent> SupportMagic;
    UPROPERTY(VisibleAnywhere, Category="Progression") TObjectPtr<UKalmalaSkillProgressionComponent> SkillProgression;
    UPROPERTY(VisibleAnywhere, Category="Status") TObjectPtr<UKalmalaPlayerStatusComponent> Statuses;
    UPROPERTY(VisibleAnywhere, Category="Ocean Travel") TObjectPtr<UKalmalaOceanTravelFeedbackComponent> OceanTravelFeedback;
    UPROPERTY(VisibleAnywhere, Category = "Inventory")
    TObjectPtr<UKalmalaInventoryComponent> Inventory;

    void MoveForward(float Value);
    void MoveRight(float Value);
    void SendOceanSkiffSteeringInput();
    void RequestInteract();
    void RequestAttack();
    void SelectMending();
    void SelectHearthShield();
    void SelectBearsVigor();
    void SelectDeerCall();
    void SelectSupportEffect(EKalmalaSupportEffect Effect);
    void ActivateSelectedSupportEffect();
    void ConfigureTraversalTestTarget();
    void ApplyExposureTravelPenalty();
    void StartSprint();
    void StopSprint();
    void VerifyPlayerControls(float DeltaSeconds);
    void VerifyConstructionMovement(float DeltaSeconds);
    bool bConstructionMovementSpawned = false;
    bool bConstructionMovementFinished = false;
    float ConstructionMovementElapsed = 0.0f;
    float ConstructionMovementStartY = 0.0f;
    bool bConstructionWallLogged = false;
    bool bConstructionRoofJumpRequested = false;
    bool bConstructionRoofAirborne = false;
    float ConstructionRoofPeakZ = 0.0f;
    void VerifySwimming(float DeltaSeconds);
    void ConfigureSwimmingTestTarget();
    void VerifyOceanTravel(float DeltaSeconds);
    void ConfigureOceanTravelTarget();
    void AuditOceanTravelTerrain();
    bool bControlsTestEnabled = false;
    bool bWetStaminaReport = false;
    int32 ControlsTestStage = 0;
    float ControlsTestElapsed = 0.0f;
    bool bControlsTestSprintObserved = false;
    bool bControlsTestJumpObserved = false;
    bool bControlsTestReleaseObserved = false;
    bool bControlsTestLocalJumpObserved = false;
    bool bSwimmingTestEnabled = false;
    bool bSwimmingTargetConfigured = false;
    bool bSwimmingEntryLogged = false;
    bool bSwimmingReturnLogged = false;
    FVector2D SwimmingTestTarget = FVector2D::ZeroVector;
    FVector2D SwimmingTestStart = FVector2D::ZeroVector;
    bool bOceanTravelTestEnabled = false;
    bool bCombatPeerTestInvalidAttackSent = false;
    uint8 SelectedSupportEffectValue = 0;
    uint32 LocalSupportRequestSequence = 0;
    uint32 LocalOceanSkiffInputSequence = 0;
    float LocalOceanSkiffThrottle = 0.0f;
    float LocalOceanSkiffRudder = 0.0f;
    double LastOceanSkiffInputSendTime = -1.0;
    bool bDiscoveryPeerPrivacyLogged = false;
    bool bOceanTravelFeedbackPeerPrivacyLogged = false;
    float OceanTravelFeedbackPeerStartTime = -1.0f;
    float DiscoveryPeerTestStartTime = -1.0f;
    float M9Schema2PeerTestStartTime = -1.0f;
    bool bM9Schema2PeerTestLogged = false;
    bool bOceanTravelTargetConfigured = false;
    bool bOceanTravelOceanEntryLogged = false;
    bool bOceanTravelArrivalLogged = false;
    bool bOceanTravelTerrainAuditLogged = false;
    float OceanTravelTerrainAuditNextLogTime = 0.0f;
    bool bOceanTravelHeadingToOcean = true;
    FVector2D OceanTravelTarget = FVector2D::ZeroVector;
    FVector2D OceanTravelWaypoint = FVector2D::ZeroVector;

    UPROPERTY(VisibleAnywhere, Category = "Presentation")
    TObjectPtr<UKalmalaPlayerModelComponent> PlayerModel;

    UFUNCTION()
    void OnRep_ExposureState();

    UFUNCTION()
    void OnRep_Health();

    UFUNCTION(Server, Reliable)
    void ServerRequestInteract(FName ClientToolId, uint8 ClientAction);

    UFUNCTION(Server, Unreliable)
    void ServerSubmitOceanSkiffSteeringInput(float Throttle, float Rudder, uint32 Sequence);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UCameraComponent> FollowCamera;

    UPROPERTY(EditDefaultsOnly, Category = "Interaction", meta = (ClampMin = "1.0"))
    float InteractionRange = 250.0f;

    UPROPERTY(ReplicatedUsing = OnRep_ExposureState, VisibleAnywhere, BlueprintReadOnly, Category = "Exposure", meta = (AllowPrivateAccess = "true"))
    FKalmalaExposureState ExposureState;

    UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
    float Health = 100.0f;

    /** Server-owned carried tools; condition and level are replicated only to the owning player. */
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Tools", meta = (AllowPrivateAccess = "true"))
    TArray<FKalmalaToolState> CarriedTools;

#if WITH_EDITORONLY_DATA
    /** Legacy fixture seed fields; production tool state lives only in CarriedTools. */
    UPROPERTY(Transient)
    int32 ReedKnifeDurability = 0;

    UPROPERTY(Transient)
    int32 FieldHatchetDurability = 0;

    UPROPERTY(Transient)
    int32 StonePickDurability = 0;

    UPROPERTY(Transient)
    int32 BronzeAxeDurability = -1;

    UPROPERTY(Transient)
    int32 IronAxeDurability = -1;
#endif

    float BaselineMaxWalkSpeed = 0.0f;

    bool bTraversalTelemetryEnabled = false;
    bool bExposureReplicationTelemetryEnabled = false;
    bool bTraversalMovementLogged = false;
    bool bTraversalTargetConfigured = false;
    bool bTraversalArrivalLogged = false;
    FVector TraversalStartLocation = FVector::ZeroVector;
    FVector2D TraversalTestTarget = FVector2D::ZeroVector;
};
