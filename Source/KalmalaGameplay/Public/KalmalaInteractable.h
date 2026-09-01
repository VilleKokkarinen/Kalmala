#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "KalmalaInteractable.generated.h"

class AKalmalaCharacter;

UINTERFACE(BlueprintType)
class KALMALAGAMEPLAY_API UKalmalaInteractable : public UInterface
{
    GENERATED_BODY()
};

/**
 * Server-executed interaction contract. Implementers decide whether the
 * validated interactor may use them and apply state only from Interact.
 */
class KALMALAGAMEPLAY_API IKalmalaInteractable
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    bool CanInteract(AKalmalaCharacter* Interactor) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    void Interact(AKalmalaCharacter* Interactor);
};
