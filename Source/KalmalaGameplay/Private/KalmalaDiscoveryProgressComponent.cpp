#include "KalmalaDiscoveryProgressComponent.h"
#include "Net/UnrealNetwork.h"

UKalmalaDiscoveryProgressComponent::UKalmalaDiscoveryProgressComponent() { SetIsReplicatedByDefault(true); }
void UKalmalaDiscoveryProgressComponent::PublishFeedbackFromServer(const EKalmalaDiscoveryFeedback NewFeedback, const FString& NewLabel)
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;
    Feedback = NewFeedback;
    FeedbackLabel = NewLabel.Left(48);
    ++FeedbackSerial;
    GetOwner()->ForceNetUpdate();
}
void UKalmalaDiscoveryProgressComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UKalmalaDiscoveryProgressComponent, Feedback, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UKalmalaDiscoveryProgressComponent, FeedbackSerial, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UKalmalaDiscoveryProgressComponent, FeedbackLabel, COND_OwnerOnly);
}
