#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KalmalaSkillProgressionContract.h"
#include "KalmalaSkillProgressionComponent.generated.h"

/**
 * Aggregate badge state that is safe for relevant peers to present. It does
 * not identify a skill and does not expose experience, rewards, or modifiers.
 */
USTRUCT(BlueprintType)
struct KALMALAGAMEPLAY_API FKalmalaSkillPeerPresentationState
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
    uint8 HighestLevel = 1;

    /** Unlock tiers derived from HighestLevel, never submitted by a client. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression")
    uint8 HighestUnlockMask = 0;

    bool IsValid() const;
};

/**
 * Server-owned progression views. Detailed state is owner-only; relevant
 * peers receive only the aggregate presentation badge above.
 */
UCLASS(ClassGroup=(Kalmala), meta=(BlueprintSpawnableComponent))
class KALMALAGAMEPLAY_API UKalmalaSkillProgressionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UKalmalaSkillProgressionComponent();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** Only the server may award experience after an action is accepted. */
    bool AwardExperienceFromAcceptedServerAction(EKalmalaSkill Skill, int32 AwardedExperience);

    const TArray<FKalmalaSkillState>& GetDetailedProgression() const { return DetailedProgression; }
    const FKalmalaSkillPeerPresentationState& GetPeerPresentation() const { return PeerPresentation; }
    const FKalmalaSkillProgressionLedger& GetServerLedger() const { return ServerLedger; }

    static FKalmalaSkillPeerPresentationState BuildPeerPresentation(const FKalmalaSkillProgressionLedger& Ledger);

private:
    void PublishServerState();

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Progression", meta = (AllowPrivateAccess = "true"))
    TArray<FKalmalaSkillState> DetailedProgression;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Progression", meta = (AllowPrivateAccess = "true"))
    FKalmalaSkillPeerPresentationState PeerPresentation;

    UPROPERTY()
    FKalmalaSkillProgressionLedger ServerLedger;
};
