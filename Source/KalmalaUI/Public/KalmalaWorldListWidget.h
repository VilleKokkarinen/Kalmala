#pragma once

#include "Blueprint/UserWidget.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "KalmalaWorldDescriptor.h"
#include "KalmalaLocalWorldSaveListing.h"
#include "KalmalaWorldSeedParser.h"
#include "KalmalaWorldListWidget.generated.h"

class UBorder;
class UScrollBox;
class USizeBox;
class UTextBlock;
class UVerticalBox;
class UKalmalaThemedButton;
DECLARE_MULTICAST_DELEGATE(FKalmalaWorldListBack);
DECLARE_MULTICAST_DELEGATE_OneParam(FKalmalaWorldListSelection, uint64);
DECLARE_MULTICAST_DELEGATE_TwoParams(FKalmalaWorldListSoloPlay, uint64, bool);
DECLARE_MULTICAST_DELEGATE_TwoParams(FKalmalaWorldListHostCoop, uint64, bool);
DECLARE_MULTICAST_DELEGATE(FKalmalaWorldListNewWorld);
DECLARE_MULTICAST_DELEGATE(FKalmalaWorldListReturnToWorlds);

/** Read-only world-list entry point backed by filtered platform save names. */
UCLASS()
class KALMALAUI_API UKalmalaWorldListWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    static const TCHAR* EmptyOrUnavailableMessage(EKalmalaWorldMetadataAvailability Availability);
    static FString ReuseConfirmationMessage(uint64 Seed, int32 GeneratorRevision);
    static const TCHAR* SeedCheckResultMessage(EKalmalaWorldSeedParseResult ParseResult,
        EKalmalaLocalWorldSaveListingAvailability Availability, EKalmalaWorldSeedReuseDecision Decision);
    FKalmalaWorldListBack& OnBackRequested() { return BackRequested; }
    FKalmalaWorldListSelection& OnWorldSelected() { return WorldSelected; }
    FKalmalaWorldListSoloPlay& OnSoloPlayRequested() { return SoloPlayRequested; }
    FKalmalaWorldListHostCoop& OnHostCoopRequested() { return HostCoopRequested; }
    FKalmalaWorldListNewWorld& OnNewWorldRequested() { return NewWorldRequested; }
    FKalmalaWorldListReturnToWorlds& OnReturnToWorldsRequested() { return ReturnToWorldsRequested; }
    void FocusBack();
    void FocusPrimary();
    /** Opens consent after the server reports an exact current-revision candidate. */
    bool BeginWorldReuseConfirmation(uint64 Seed);
    bool ShowNewWorldSeedEntry();
    bool ShowWorldDetails(uint64 Seed);
    void ShowWorldList();
    void SetWorldLaunchActionsEnabled(bool bEnabled);
    void ShowWorldLaunchFailure();

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

private:
    void BuildWorldList();
    void BuildNewWorldSeedEntry();
    void BuildWorldDetails();
    void BuildWorldReuseConfirmation();
    void RefreshTheme();
    void RefreshWorldRecords();
    void RequestServerWorldSeedSelection(uint64 Seed, bool bConfirmedExisting);
    void CancelPendingWorldSeedSelectionRequest();
    void HandleServerWorldSeedSelectionResult(uint64 RequestToken, uint64 Seed, bool bAccepted,
        EKalmalaLocalWorldSaveListingAvailability Availability, EKalmalaWorldSeedReuseDecision Decision);
    UFUNCTION() void HandleBack();
    UFUNCTION() void HandleNewWorldRequested();
    UFUNCTION() void HandleSeedCheck();
    UFUNCTION() void HandleSeedTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);
    UFUNCTION() void HandleReuseConfirmed();
    UFUNCTION() void HandleReuseCancelled();
    UFUNCTION() void HandleWorldSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION() void HandleViewDetails();
    UFUNCTION() void HandleSoloPlay();
    UFUNCTION() void HandleHostCoop();
    void FocusWidget(UWidget* Widget);
    const FKalmalaLocalWorldSaveRecord* FindWorld(uint64 Seed) const;
    FString DescribeWorld(const FKalmalaLocalWorldSaveRecord& Record) const;
    UPROPERTY(Transient) TObjectPtr<UBorder> Backdrop;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> Scroll;
    UPROPERTY(Transient) TObjectPtr<USizeBox> Column;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Title;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Message;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> SeedInputLabel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> CreationStatus;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> DetailsText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> HostingInfo;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> Content;
    UPROPERTY(Transient) TObjectPtr<UComboBoxString> WorldPicker;
    UPROPERTY(Transient) TObjectPtr<UEditableTextBox> SeedInput;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> NewWorldButton;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> SeedCheckButton;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> DetailsButton;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> PlaySoloButton;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> HostCoopButton;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> BackButton;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> ReuseConfirmButton;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> ReuseCancelButton;
    FKalmalaWorldListBack BackRequested;
    FKalmalaWorldListSelection WorldSelected;
    FKalmalaWorldListSoloPlay SoloPlayRequested;
    FKalmalaWorldListHostCoop HostCoopRequested;
    FKalmalaWorldListNewWorld NewWorldRequested;
    FKalmalaWorldListReturnToWorlds ReturnToWorldsRequested;
    TArray<FKalmalaLocalWorldSaveRecord> WorldRecords;
    EKalmalaLocalWorldSaveListingAvailability ListingAvailability = EKalmalaLocalWorldSaveListingAvailability::Unsupported;
    uint64 SelectedSeed = 0;
    uint64 PendingReuseSeed = 0;
    uint64 NextWorldSeedRequestToken = 1;
    uint64 PendingWorldSeedRequestToken = 0;
    uint64 PendingWorldSeedRequestSeed = 0;
    bool bHasSelectedWorld = false;
    bool bSelectedSeedIsNew = false;
    bool bShowingNewWorldSeedEntry = false;
    bool bShowingDetails = false;
    bool bShowingReuseConfirmation = false;
    bool bPendingWorldSeedRequestIsConfirmation = false;
    bool bAwaitingWorldSeedRequest = false;
    int32 LastTextScale = -1;
    int32 LastContrast = -2;
    bool bLastReducedMotion = false;
};
