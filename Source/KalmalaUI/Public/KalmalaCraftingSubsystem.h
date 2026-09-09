#pragma once
#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Tickable.h"
#include "KalmalaCraftingSubsystem.generated.h"
class UTextBlock;
class UInputComponent;
class UKalmalaCraftingComponent;

UCLASS()
class KALMALAUI_API UKalmalaCraftingWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void Open();
    void Close();
    bool IsOpen() const { return bOpen; }
    FString GetPresentationText() const;
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
private:
    UKalmalaCraftingComponent* Model() const;
    UFUNCTION() void Previous();
    UFUNCTION() void Next();
    UFUNCTION() void Craft();
    UFUNCTION() void Place();
    UFUNCTION() void Refuel();
    UFUNCTION() void Light();
    UFUNCTION() void CloseClicked();
    void Refresh();
    UPROPERTY(Transient) TObjectPtr<UTextBlock> RecipesText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> DetailText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> StateText;
    int32 Selected = 0;
    bool bOpen = false;
    bool bPreviousCursor = false;
};

UCLASS()
class KALMALAUI_API UKalmalaCraftingSubsystem : public ULocalPlayerSubsystem, public FTickableGameObject
{
    GENERATED_BODY()
public:
    virtual void Tick(float DeltaTime) override;
    virtual void Deinitialize() override;
    virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UKalmalaCraftingSubsystem, STATGROUP_Tickables); }
    virtual bool IsTickable() const override { return !IsTemplate(); }
    bool CloseIfOpen();
private:
    void Toggle();
    void Release();
    UPROPERTY(Transient) TObjectPtr<UKalmalaCraftingWidget> Widget;
    UPROPERTY(Transient) TObjectPtr<APlayerController> Controller;
    TWeakObjectPtr<UInputComponent> BoundInput;
    bool bVerified = false;
    bool bCaptureRequested = false;
    float CaptureWait = 0;
};
