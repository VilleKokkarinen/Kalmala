#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Components/Button.h"
#include "KalmalaThemedButton.generated.h"

/** Shared theme-aware local interaction states for buttons, tabs and controls. */
UCLASS()
class KALMALAUI_API UKalmalaThemedButton : public UButton
{
	GENERATED_BODY()

public:
	UKalmalaThemedButton(const FObjectInitializer& ObjectInitializer);

	void SetThemeStyle(const FButtonStyle& InBaseStyle, const FLinearColor& InFocusColor,
		const FLinearColor& InSelectedFillColor,
		float InFocusBorderWidth, float InSelectedBorderWidth, float InDisabledBorderWidth,
		float InTransitionDuration, bool bInAnimateTransitions);
	void SetInteractionSelected(bool bSelected);
	bool IsInteractionSelected() const { return bInteractionSelected; }

#if !UE_BUILD_SHIPPING
	void SetInteractionFocusForVerification(bool bFocused);
	bool HasActiveInteractionTransitionForVerification() const { return TransitionTickerHandle.IsValid(); }
#endif

protected:
	virtual void BeginDestroy() override;

private:
	UFUNCTION()
	void HandleThemeHovered();
	UFUNCTION()
	void HandleThemeUnhovered();
	void HandleThemeFocusReceived();
	void HandleThemeFocusLost();
	void RebuildInteractionStyle();
	FButtonStyle MakeInteractionStyle() const;
	bool AdvanceInteractionTransition(float DeltaSeconds);
	void StopInteractionTransition();

	FButtonStyle BaseThemeStyle;
	FButtonStyle TransitionStartStyle;
	FButtonStyle TransitionTargetStyle;
	FLinearColor FocusColor = FLinearColor::White;
	FLinearColor SelectedFillColor = FLinearColor::White;
	float FocusBorderWidth = 2.0f;
	float SelectedBorderWidth = 2.0f;
	float DisabledBorderWidth = 2.0f;
	float TransitionDuration = 0.12f;
	float TransitionElapsed = 0.0f;
	bool bHasThemeStyle = false;
	bool bAnimateTransitions = true;
	bool bInteractionHovered = false;
	bool bInteractionFocused = false;
	bool bInteractionSelected = false;
	FTSTicker::FDelegateHandle TransitionTickerHandle;
};
