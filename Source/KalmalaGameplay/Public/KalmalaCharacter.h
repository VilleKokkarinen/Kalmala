#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "KalmalaCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UKalmalaPlayerModelComponent;
class UKalmalaInventoryComponent;
class UKalmalaCraftingComponent;

USTRUCT(BlueprintType)
struct FKalmalaExposureState
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Exposure")
    float Wetness = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Exposure")
    float Warmth = 100.0f;

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
    static bool IsExposureUpdateAllowed(bool bServerAuthority);
    void SetExposureStateFromServer(const FKalmalaExposureState& NewExposureState);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    virtual void OnRep_ReplicatedMovement() override;

private:
    UPROPERTY(VisibleAnywhere, Category="Crafting") TObjectPtr<UKalmalaCraftingComponent> Crafting;
    UPROPERTY(VisibleAnywhere, Category = "Inventory")
    TObjectPtr<UKalmalaInventoryComponent> Inventory;

    void MoveForward(float Value);
    void MoveRight(float Value);
    void RequestInteract();
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
    void VerifySwimming(float DeltaSeconds);
    void ConfigureSwimmingTestTarget();
    void VerifyOceanTravel(float DeltaSeconds);
    void ConfigureOceanTravelTarget();
    void AuditOceanTravelTerrain();
    bool bControlsTestEnabled = false;
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

    UFUNCTION(Server, Reliable)
    void ServerRequestInteract();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UCameraComponent> FollowCamera;

    UPROPERTY(EditDefaultsOnly, Category = "Interaction", meta = (ClampMin = "1.0"))
    float InteractionRange = 250.0f;

    UPROPERTY(ReplicatedUsing = OnRep_ExposureState, VisibleAnywhere, BlueprintReadOnly, Category = "Exposure", meta = (AllowPrivateAccess = "true"))
    FKalmalaExposureState ExposureState;

    float BaselineMaxWalkSpeed = 0.0f;

    bool bTraversalTelemetryEnabled = false;
    bool bExposureReplicationTelemetryEnabled = false;
    bool bTraversalMovementLogged = false;
    bool bTraversalTargetConfigured = false;
    bool bTraversalArrivalLogged = false;
    FVector TraversalStartLocation = FVector::ZeroVector;
    FVector2D TraversalTestTarget = FVector2D::ZeroVector;
};
