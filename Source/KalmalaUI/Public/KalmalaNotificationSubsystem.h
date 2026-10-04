#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Tickable.h"
#include "KalmalaSkillNotice.h"
#include "KalmalaNotificationSubsystem.generated.h"

UCLASS()
class KALMALAUI_API UKalmalaNotificationWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void SetNotices(const TArray<FKalmalaSkillNotice>& Notices, int32 Scale, int32 Contrast);
    FString GetPresentationText() const { return Presentation; }
protected:
    virtual void NativeOnInitialized() override;
    virtual TSharedRef<SWidget> RebuildWidget() override;
private:
    void BuildPanel();
    UPROPERTY(Transient) TObjectPtr<class UBorder> Panel;
    UPROPERTY(Transient) TObjectPtr<class UVerticalBox> Rows;
    FString Presentation;
    int32 LastScale = INDEX_NONE;
    int32 LastContrast = INDEX_NONE;
};

/** Passive local feedback; reads only the local controller's owning skill details. */
UCLASS()
class KALMALAUI_API UKalmalaNotificationSubsystem : public ULocalPlayerSubsystem, public FTickableGameObject
{
    GENERATED_BODY()
public:
    virtual void Tick(float DeltaTime) override;
    virtual void Deinitialize() override;
    virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
    virtual bool IsTickable() const override { return !IsTemplate(); }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UKalmalaNotificationSubsystem, STATGROUP_Tickables); }
private:
    FKalmalaSkillNoticeQueue Queue;
    TWeakObjectPtr<class APawn> OwnerPawn;
    UPROPERTY(Transient) TObjectPtr<UKalmalaNotificationWidget> Widget;
    bool bOwnerBaselineAudited = false;
#if !UE_BUILD_SHIPPING
    bool bReviewFixtureInitialized = false;
    bool bReviewSettingsApplied = false;
    int32 ReviewCaptureStage = 0;
    float ReviewCaptureElapsed = 0.0f;
    FString ReviewCaptureBasePath;
    bool bReviewPriorMoveInputIgnored = false;
#endif
};
