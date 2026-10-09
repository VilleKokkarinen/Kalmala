#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Tickable.h"
#include "KalmalaInventoryMenuSubsystem.generated.h"

class APlayerController;
class UInputComponent;
class UKalmalaInventoryMenuWidget;

/** Owns one local inventory-menu instance and input binding for each local player. */
UCLASS()
class KALMALAUI_API UKalmalaInventoryMenuSubsystem : public ULocalPlayerSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual void Deinitialize() override;
    virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UKalmalaInventoryMenuSubsystem, STATGROUP_Tickables); }
    virtual bool IsTickable() const override { return !IsTemplate(); }

    bool IsMenuOpen() const;
    bool CloseIfOpen();

private:
    void BindLocalInput(APlayerController* InLocalController);
    void ReleaseController();
    void ToggleInventoryMenu();

    UPROPERTY(Transient)
    TObjectPtr<UKalmalaInventoryMenuWidget> InventoryWidget;
    UPROPERTY(Transient)
    TObjectPtr<APlayerController> LocalController;
    TWeakObjectPtr<UInputComponent> BoundInputComponent;
};
