#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KalmalaArrowProjectile.generated.h"
class USphereComponent;
class UProjectileMovementComponent;
UCLASS(NotBlueprintable)
class KALMALAGAMEPLAY_API AKalmalaArrowProjectile : public AActor
{
    GENERATED_BODY()
public:
    AKalmalaArrowProjectile();
    bool InitializeShot(const FVector& Direction, float DrawProgress, FName ArrowItemId);
    static bool ResolveShotDamage(FName ArrowItemId, float DrawProgress, float& OutShotDamage);
    virtual void Tick(float DeltaSeconds) override;
    float GetShotDamage() const { return ShotDamage; }
    FName GetShotItemId() const { return ShotItemId; }
protected:
    virtual void BeginPlay() override;
private:
    UFUNCTION() void Hit(UPrimitiveComponent* Component, AActor* Other, UPrimitiveComponent* OtherComponent,
        FVector Impulse, const FHitResult& Result);
    UPROPERTY() TObjectPtr<USphereComponent> Collision;
    UPROPERTY() TObjectPtr<UProjectileMovementComponent> Movement;
    float ShotDamage = 0;
    FName ShotItemId = NAME_None;
    FVector PreviousPosition = FVector::ZeroVector;
};
