#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "KalmalaCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;

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
    AKalmalaCharacter();

    const FKalmalaExposureState& GetExposureState() const { return ExposureState; }
    void SetExposureStateFromServer(const FKalmalaExposureState& NewExposureState);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    virtual void OnRep_ReplicatedMovement() override;

private:
    void MoveForward(float Value);
    void MoveRight(float Value);
    void RequestInteract();
    void ConfigureTraversalTestTarget();
    void ApplyExposureTravelPenalty();

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
    bool bTraversalMovementLogged = false;
    bool bTraversalTargetConfigured = false;
    bool bTraversalArrivalLogged = false;
    FVector TraversalStartLocation = FVector::ZeroVector;
    FVector2D TraversalTestTarget = FVector2D::ZeroVector;
};
