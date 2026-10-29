#pragma once

#include "Blueprint/UserWidget.h"
#include "Components/ComboBoxString.h"
#include "KalmalaWorldDescriptor.h"
#include "KalmalaLocalWorldSaveListing.h"
#include "KalmalaWorldListWidget.generated.h"

class UBorder;
class UScrollBox;
class USizeBox;
class UTextBlock;
class UVerticalBox;
class UKalmalaThemedButton;
DECLARE_MULTICAST_DELEGATE(FKalmalaWorldListBack);
DECLARE_MULTICAST_DELEGATE_OneParam(FKalmalaWorldListSelection, uint64);

/** Read-only world-list entry point backed by filtered platform save names. */
UCLASS()
class KALMALAUI_API UKalmalaWorldListWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    static const TCHAR* EmptyOrUnavailableMessage(EKalmalaWorldMetadataAvailability Availability);
    FKalmalaWorldListBack& OnBackRequested() { return BackRequested; }
    FKalmalaWorldListSelection& OnWorldSelected() { return WorldSelected; }
    void FocusBack();
    void FocusPrimary();
    bool ShowWorldDetails(uint64 Seed);
    void ShowWorldList();

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

private:
    void BuildWorldList();
    void BuildWorldDetails();
    void RefreshTheme();
    UFUNCTION() void HandleBack();
    UFUNCTION() void HandleWorldSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION() void HandleViewDetails();
    void FocusWidget(UWidget* Widget);
    const FKalmalaLocalWorldSaveRecord* FindWorld(uint64 Seed) const;
    FString DescribeWorld(const FKalmalaLocalWorldSaveRecord& Record) const;
    UPROPERTY(Transient) TObjectPtr<UBorder> Backdrop;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> Scroll;
    UPROPERTY(Transient) TObjectPtr<USizeBox> Column;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Title;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Message;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> CreationStatus;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> DetailsText;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> Content;
    UPROPERTY(Transient) TObjectPtr<UComboBoxString> WorldPicker;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> NewWorldButton;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> DetailsButton;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> BackButton;
    FKalmalaWorldListBack BackRequested;
    FKalmalaWorldListSelection WorldSelected;
    TArray<FKalmalaLocalWorldSaveRecord> WorldRecords;
    EKalmalaLocalWorldSaveListingAvailability ListingAvailability = EKalmalaLocalWorldSaveListingAvailability::Unsupported;
    uint64 SelectedSeed = 0;
    bool bHasSelectedWorld = false;
    bool bShowingDetails = false;
    int32 LastTextScale = -1;
    int32 LastContrast = -2;
    bool bLastReducedMotion = false;
};
