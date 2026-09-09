#pragma once
#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Tickable.h"
#include "KalmalaInventorySubsystem.generated.h"

class UTextBlock;

UCLASS()
class KALMALAUI_API UKalmalaInventoryWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void SetPackText(const FString& Text);
protected:
    virtual void NativeOnInitialized() override;
private:
    UPROPERTY(Transient) TObjectPtr<UTextBlock> PackText;
};

/** Read-only local inventory presentation, with no input bindings or network requests. */
UCLASS()
class KALMALAUI_API UKalmalaInventorySubsystem : public ULocalPlayerSubsystem, public FTickableGameObject
{
    GENERATED_BODY()
public:
    virtual void Tick(float DeltaTime) override;
    virtual void Deinitialize() override;
    virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UKalmalaInventorySubsystem, STATGROUP_Tickables); }
    virtual bool IsTickable() const override { return !IsTemplate(); }
private:
    UPROPERTY(Transient) TObjectPtr<UKalmalaInventoryWidget> Widget;
    bool bVerified = false;
};
