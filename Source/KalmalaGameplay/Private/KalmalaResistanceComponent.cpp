#include "KalmalaResistanceComponent.h"
#include "GameFramework/Actor.h"

UKalmalaResistanceComponent::UKalmalaResistanceComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UKalmalaResistanceComponent::ResolveFromServer(const FKalmalaDamageChannels& Incoming, float& OutDamage) const
{
    OutDamage = 0.0f;
    return GetOwner() && GetOwner()->HasAuthority() && Profile.Resolve(Incoming, OutDamage);
}
