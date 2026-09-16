#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KalmalaDiscoveryProgressComponent.generated.h"

/** Owner-only, text-backed acknowledgement for a server-confirmed discovery. */
UENUM(BlueprintType)
enum class EKalmalaDiscoveryFeedback : uint8 { None, LandmarkFound, ScrollFound, AlreadyFound, Unavailable };

UCLASS(ClassGroup=(Kalmala), meta=(BlueprintSpawnableComponent))
class KALMALAGAMEPLAY_API UKalmalaDiscoveryProgressComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UKalmalaDiscoveryProgressComponent();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    EKalmalaDiscoveryFeedback GetFeedback() const { return Feedback; }
    uint32 GetFeedbackSerial() const { return FeedbackSerial; }
    const FString& GetFeedbackLabel() const { return FeedbackLabel; }
    void PublishFeedbackFromServer(EKalmalaDiscoveryFeedback NewFeedback, const FString& NewLabel);
private:
    UPROPERTY(Replicated) EKalmalaDiscoveryFeedback Feedback = EKalmalaDiscoveryFeedback::None;
    UPROPERTY(Replicated) uint32 FeedbackSerial = 0;
    UPROPERTY(Replicated) FString FeedbackLabel;
};
