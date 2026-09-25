#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KalmalaInteractable.h"
#include "KalmalaOceanSkiff.generated.h"

class AKalmalaCharacter;
class UBoxComponent;
class UProceduralMeshComponent;
struct FKalmalaWorldGenerationConfig;
enum class EKalmalaOceanTravelFeedback : uint8;

UENUM(BlueprintType)
enum class EKalmalaOceanSkiffMode : uint8
{
    Moored,
    Underway,
    Blocked
};

UENUM(BlueprintType)
enum class EKalmalaOceanSkiffBlockReason : uint8
{
	None,
	InvalidOceanFootprint,
	GeneratedTerrainCollision,
	SweepLimit
};

UENUM()
enum class EKalmalaOceanSkiffSeat : uint8
{
    None,
    Helm,
    Passenger
};

/** Transient, server-created two-seat travel actor. It carries no save state. */
UCLASS()
class KALMALAGAMEPLAY_API AKalmalaOceanSkiff : public AActor, public IKalmalaInteractable
{
    GENERATED_BODY()

public:
    AKalmalaOceanSkiff();

    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual bool CanInteract_Implementation(AKalmalaCharacter* Interactor) const override;
    virtual void Interact_Implementation(AKalmalaCharacter* Interactor) override;

    static AKalmalaOceanSkiff* TryLaunchFromServer(AKalmalaCharacter* Interactor, const FHitResult& TerrainHit);
    static EKalmalaOceanTravelFeedback GetLaunchDenialFeedback(bool bGeneratedSurface, bool bInRange,
        bool bDeepOcean, bool bWorldBounded, bool bSessionSlotAvailable, bool bNavigableHull);
    bool TryInteractFromServer(AKalmalaCharacter* Interactor);
    bool TryDisembarkFromServer(AKalmalaCharacter* Interactor);

    AKalmalaCharacter* GetHelmOccupant() const { return HelmOccupant; }
    AKalmalaCharacter* GetPassengerOccupant() const { return PassengerOccupant; }
    EKalmalaOceanSkiffMode GetMode() const { return Mode; }
    EKalmalaOceanSkiffBlockReason GetBlockReason() const { return BlockReason; }
    static bool IsLaunchAllowed(bool bServerAuthority, bool bGeneratedTerrainHit, bool bInRange,
        bool bDeepOcean, bool bWorldBounded, bool bSessionSlotAvailable);
    static bool HasNavigableOceanFootprintForConfig(const FKalmalaWorldGenerationConfig& Config,
        FVector2D Center, float YawDegrees);
    static bool IsSafeExitSurfaceForConfig(const FKalmalaWorldGenerationConfig& Config,
        FVector2D Position, double WorldMargin = 0.0);
    static EKalmalaOceanSkiffSeat ChooseSeat(bool bHelmOccupied, bool bPassengerOccupied);
    static bool IsDisembarkAllowed(bool bServerAuthority, bool bIsOccupant, float Speed, bool bHasSafePlacement);
    static bool IsSteeringIntentAllowed(bool bServerAuthority, bool bIsHelmOccupant, float Throttle,
        float Rudder, uint32 Sequence, uint32 LastAcceptedSequence, double ServerTime,
        double LastAcceptedTime, bool bHasAcceptedInput);
    static bool IsInputFresh(double ServerTime, double LastAcceptedTime, bool bHasAcceptedInput);
    static float AdvanceSpeed(float CurrentSpeed, float Throttle, float DeltaSeconds);
    static float CalculateWeatherYawRate(float Rudder, float HeadingDegrees, float WindDirectionDegrees,
        float WindStrength, float Speed);
    bool AcceptSteeringFromServer(AKalmalaCharacter* Interactor, float Throttle, float Rudder, uint32 Sequence);

private:
    UPROPERTY(VisibleAnywhere, Category = "Travel")
    TObjectPtr<UBoxComponent> HullCollision;

    UPROPERTY(VisibleAnywhere, Category = "Travel")
    TObjectPtr<UProceduralMeshComponent> HullMesh;

    UPROPERTY(ReplicatedUsing = OnRep_SeatOccupants, VisibleAnywhere, Category = "Travel")
    TObjectPtr<AKalmalaCharacter> HelmOccupant;

    UPROPERTY(ReplicatedUsing = OnRep_SeatOccupants, VisibleAnywhere, Category = "Travel")
    TObjectPtr<AKalmalaCharacter> PassengerOccupant;

    UPROPERTY(Replicated, VisibleAnywhere, Category = "Travel")
    EKalmalaOceanSkiffMode Mode = EKalmalaOceanSkiffMode::Moored;

    UPROPERTY(Replicated, VisibleAnywhere, Category = "Travel")
    EKalmalaOceanSkiffBlockReason BlockReason = EKalmalaOceanSkiffBlockReason::None;

    TWeakObjectPtr<AKalmalaCharacter> PresentedHelmOccupant;
    TWeakObjectPtr<AKalmalaCharacter> PresentedPassengerOccupant;

    UFUNCTION()
    void OnRep_SeatOccupants();

    bool FindSafeExitLocation(AKalmalaCharacter* Interactor, FVector& OutLocation) const;
    bool HasDeepOceanFootprint(FVector Location, FRotator Rotation) const;
    void AdvanceServerMovement(float DeltaSeconds);
    void BlockMovementAtLastSafeTransform(const FVector& SafeLocation, const FRotator& SafeRotation,
        EKalmalaOceanSkiffBlockReason Reason);
    void SendFeedback(AKalmalaCharacter* Interactor, EKalmalaOceanTravelFeedback Feedback) const;
    void RefreshSeatOccupancyPresentation();
    void SetSeatOccupant(AKalmalaCharacter* Interactor, EKalmalaOceanSkiffSeat Seat);
    void ClearSeatOccupant(AKalmalaCharacter* Interactor);
    static bool BuildOriginalHull(UProceduralMeshComponent* Mesh);

    float ThrottleInput = 0.0f;
    float RudderInput = 0.0f;
    float CurrentSpeed = 0.0f;
    double LastAcceptedInputTime = 0.0;
    uint32 LastAcceptedInputSequence = 0;
    bool bHasAcceptedInput = false;
};
