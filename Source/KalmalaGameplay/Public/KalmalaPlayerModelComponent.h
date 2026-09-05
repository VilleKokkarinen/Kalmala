#pragma once

#include "Components/SceneComponent.h"
#include "KalmalaPlayerModelComponent.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;

/** Original segmented prototype humanoid; local cosmetics only. */
UCLASS()
class KALMALAGAMEPLAY_API UKalmalaPlayerModelComponent : public USceneComponent
{
    GENERATED_BODY()
public:
    UKalmalaPlayerModelComponent();
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
    int32 GetPartCount() const { return Parts.Num(); }
private:
    UProceduralMeshComponent* AddPart(FName Name, FVector Pivot, FVector Centre, FVector HalfSize, UMaterialInterface* Material);
    UPROPERTY(Transient)
    TArray<TObjectPtr<UProceduralMeshComponent>> Parts;
    UPROPERTY(Transient)
    TObjectPtr<UProceduralMeshComponent> LeftArm;
    UPROPERTY(Transient)
    TObjectPtr<UProceduralMeshComponent> RightArm;
    UPROPERTY(Transient)
    TObjectPtr<UProceduralMeshComponent> LeftLeg;
    UPROPERTY(Transient)
    TObjectPtr<UProceduralMeshComponent> RightLeg;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> ClothMaterial;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> HeadMaterial;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> DarkMaterial;
    float GaitPhase = 0.0f;
};
