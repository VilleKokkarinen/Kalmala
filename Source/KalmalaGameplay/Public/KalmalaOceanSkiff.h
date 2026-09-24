#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KalmalaInteractable.h"
#include "KalmalaOceanSkiff.generated.h"

class AKalmalaCharacter;
class UBoxComponent;
class UProceduralMeshComponent;

UENUM(BlueprintType)
enum class EKalmalaOceanSkiffMode : uint8
{
    Moored,
    Underway,
    Blocked
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

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual bool CanInteract_Implementation(AKalmalaCharacter* Interactor) const override;
    virtual void Interact_Implementation(AKalmalaCharacter* Interactor) override;

    static AKalmalaOceanSkiff* TryLaunchFromServer(AKalmalaCharacter* Interactor, const FHitResult& TerrainHit);
    bool TryDisembarkFromServer(AKalmalaCharacter* Interactor);

    AKalmalaCharacter* GetHelmOccupant() const { return HelmOccupant; }
    AKalmalaCharacter* GetPassengerOccupant() const { return PassengerOccupant; }
    EKalmalaOceanSkiffMode GetMode() const { return Mode; }

    static bool IsLaunchAllowed(bool bServerAuthority, bool bGeneratedTerrainHit, bool bInRange,
        bool bDeepOcean, bool bWorldBounded, bool bSessionSlotAvailable);
    static EKalmalaOceanSkiffSeat ChooseSeat(bool bHelmOccupied, bool bPassengerOccupied);
    static bool IsDisembarkAllowed(bool bServerAuthority, bool bIsOccupant, float Speed, bool bHasSafePlacement);

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

    TWeakObjectPtr<AKalmalaCharacter> PresentedHelmOccupant;
    TWeakObjectPtr<AKalmalaCharacter> PresentedPassengerOccupant;

    UFUNCTION()
    void OnRep_SeatOccupants();

    bool FindSafeExitLocation(AKalmalaCharacter* Interactor, FVector& OutLocation) const;
    void RefreshSeatOccupancyPresentation();
    void SetSeatOccupant(AKalmalaCharacter* Interactor, EKalmalaOceanSkiffSeat Seat);
    void ClearSeatOccupant(AKalmalaCharacter* Interactor);
    static bool BuildOriginalHull(UProceduralMeshComponent* Mesh);
};
