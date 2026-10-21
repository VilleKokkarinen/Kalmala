#pragma once

#include "Components/SceneComponent.h"
#include "KalmalaPlayerModelComponent.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;
class UPointLightComponent;
class USceneComponent;

UENUM()
enum class EKalmalaHeldToolVisual : uint8 { None, Hammer, Axe, Knife, Pick, Spear, Mace, Bow, Sword, Club, Torch, Trowel };

/** Original faceted wanderer silhouette and server-selected held-tool cosmetics. */
UCLASS()
class KALMALAGAMEPLAY_API UKalmalaPlayerModelComponent : public USceneComponent
{
    GENERATED_BODY()
public:
    UKalmalaPlayerModelComponent();
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
    int32 GetPartCount() const { return Parts.Num(); }
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    static EKalmalaHeldToolVisual GetHeldToolVisualForItem(FName ItemId);
    EKalmalaHeldToolVisual GetHeldToolVisual() const { return HeldToolVisual; }
    bool StartCampBedRestPose(const FTransform& BedTransform);
    void StopCampBedRestPose();
    bool IsCampBedResting() const { return bCampBedResting; }
    static bool ShouldContinueCampBedRestPose(bool bLocallyControlled, float Speed,
        float CurrentHealth, float StartingHealth);
    static constexpr float AttackSwingDurationSeconds = 0.54f;
    static bool EvaluateAttackArmPose(float ElapsedSeconds, float RestRightArmPitch, float& OutRightArmPitch);
    static constexpr float ToolSharpeningDurationSeconds = 1.2f;
    static bool EvaluateToolSharpeningArmPose(float ElapsedSeconds, float RestLeftArmPitch, float RestRightArmPitch,
        float& OutLeftArmPitch, float& OutRightArmPitch);

    UFUNCTION(NetMulticast, Unreliable)
    void MulticastPlayGrindingStoneSharpening();

    void MulticastPlayGrindingStoneSharpening_Implementation();
private:
    UFUNCTION() void OnRep_HeldToolVisual();
    UPROPERTY(ReplicatedUsing=OnRep_HeldToolVisual)
    EKalmalaHeldToolVisual HeldToolVisual = EKalmalaHeldToolVisual::None;
    UPROPERTY(Transient) TArray<TObjectPtr<UProceduralMeshComponent>> HeldToolParts;
    UPROPERTY(Transient) TObjectPtr<UPointLightComponent> TorchLight;
    UProceduralMeshComponent* AddPart(FName Name, FVector Pivot, FVector Centre, FVector HalfSize,
        UMaterialInterface* Material, bool bTapered = false, USceneComponent* Parent = nullptr);
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
    bool bCampBedResting = false;
    FVector CampBedOriginalRelativeLocation = FVector::ZeroVector;
    FRotator CampBedOriginalRelativeRotation = FRotator::ZeroRotator;
    float CampBedStartingHealth = 100.0f;
    uint32 LastAttackSerial = 0;
    float AttackSwingElapsedSeconds = -1.0f;
    float ToolSharpeningElapsedSeconds = -1.0f;
};
