#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "KalmalaCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;

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

    UFUNCTION(Server, Reliable)
    void ServerRequestInteract();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UCameraComponent> FollowCamera;

    UPROPERTY(EditDefaultsOnly, Category = "Interaction", meta = (ClampMin = "1.0"))
    float InteractionRange = 250.0f;

    bool bTraversalTelemetryEnabled = false;
    bool bTraversalMovementLogged = false;
    bool bTraversalTargetConfigured = false;
    bool bTraversalArrivalLogged = false;
    FVector TraversalStartLocation = FVector::ZeroVector;
    FVector2D TraversalTestTarget = FVector2D::ZeroVector;
};
