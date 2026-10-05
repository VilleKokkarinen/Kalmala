#include "KalmalaThemedButton.h"

#include "Containers/Ticker.h"
#include "KalmalaSettingsWidget.h"

namespace
{
	FSlateBrush BlendBrush(const FSlateBrush& From, const FSlateBrush& To, const float Alpha)
	{
		FSlateBrush Result = To;
		Result.TintColor = FSlateColor(FMath::Lerp(From.TintColor.GetSpecifiedColor(),
			To.TintColor.GetSpecifiedColor(), Alpha));
		Result.OutlineSettings.Width = FMath::Lerp(From.OutlineSettings.Width,
			To.OutlineSettings.Width, Alpha);
		Result.OutlineSettings.Color = FSlateColor(FMath::Lerp(
			From.OutlineSettings.Color.GetSpecifiedColor(), To.OutlineSettings.Color.GetSpecifiedColor(), Alpha));
		return Result;
	}

	FMargin BlendMargin(const FMargin& From, const FMargin& To, const float Alpha)
	{
		return FMargin(FMath::Lerp(From.Left, To.Left, Alpha), FMath::Lerp(From.Top, To.Top, Alpha),
			FMath::Lerp(From.Right, To.Right, Alpha), FMath::Lerp(From.Bottom, To.Bottom, Alpha));
	}

	FButtonStyle BlendStyle(const FButtonStyle& From, const FButtonStyle& To, const float Alpha)
	{
		FButtonStyle Result = To;
		Result.Normal = BlendBrush(From.Normal, To.Normal, Alpha);
		Result.Hovered = BlendBrush(From.Hovered, To.Hovered, Alpha);
		Result.Pressed = BlendBrush(From.Pressed, To.Pressed, Alpha);
		Result.Disabled = BlendBrush(From.Disabled, To.Disabled, Alpha);
		Result.NormalPadding = BlendMargin(From.NormalPadding, To.NormalPadding, Alpha);
		Result.PressedPadding = BlendMargin(From.PressedPadding, To.PressedPadding, Alpha);
		return Result;
	}

	void SetFocusOutline(FSlateBrush& Brush, const FLinearColor& Color, const float Width)
	{
		Brush.OutlineSettings.Color = FSlateColor(Color);
		Brush.OutlineSettings.Width = Width;
	}
}

UKalmalaThemedButton::UKalmalaThemedButton(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	OnHovered.AddDynamic(this, &ThisClass::HandleThemeHovered);
	OnUnhovered.AddDynamic(this, &ThisClass::HandleThemeUnhovered);
	OnReceivedFocus.BindUObject(this, &ThisClass::HandleThemeFocusReceived);
	OnLostFocus.BindUObject(this, &ThisClass::HandleThemeFocusLost);
}

void UKalmalaThemedButton::SetThemeStyle(const FButtonStyle& InBaseStyle, const FLinearColor& InFocusColor,
	const FLinearColor& InSelectedFillColor, const float InFocusBorderWidth,
	const float InSelectedBorderWidth, const float InDisabledBorderWidth,
	const float InTransitionDuration, const bool bInAnimateTransitions)
{
	const bool bAlreadyThemed = bHasThemeStyle;
	BaseThemeStyle = InBaseStyle;
	FocusColor = InFocusColor;
	SelectedFillColor = InSelectedFillColor;
	FocusBorderWidth = FMath::Max(1.0f, InFocusBorderWidth);
	SelectedBorderWidth = FMath::Max(1.0f, InSelectedBorderWidth);
	DisabledBorderWidth = FMath::Max(1.0f, InDisabledBorderWidth);
	TransitionDuration = FMath::Max(0.0f, InTransitionDuration);
	bAnimateTransitions = bInAnimateTransitions;
	bHasThemeStyle = true;
	bInteractionFocused |= HasAnyUserFocus();
	if (!bAlreadyThemed)
	{
		StopInteractionTransition();
		SetStyle(MakeInteractionStyle());
		return;
	}
	RebuildInteractionStyle();
}

void UKalmalaThemedButton::SetInteractionSelected(const bool bSelected)
{
	if (bInteractionSelected == bSelected) return;
	bInteractionSelected = bSelected;
	RebuildInteractionStyle();
}

void UKalmalaThemedButton::HandleThemeHovered()
{
	bInteractionHovered = true;
	RebuildInteractionStyle();
}

void UKalmalaThemedButton::HandleThemeUnhovered()
{
	bInteractionHovered = false;
	RebuildInteractionStyle();
}

void UKalmalaThemedButton::HandleThemeFocusReceived()
{
	bInteractionFocused = true;
	RebuildInteractionStyle();
}

void UKalmalaThemedButton::HandleThemeFocusLost()
{
	bInteractionFocused = false;
	RebuildInteractionStyle();
}

FButtonStyle UKalmalaThemedButton::MakeInteractionStyle() const
{
	FButtonStyle Style = BaseThemeStyle;
	if (!GetIsEnabled())
	{
		SetFocusOutline(Style.Disabled,
			BaseThemeStyle.Disabled.OutlineSettings.Color.GetSpecifiedColor(), DisabledBorderWidth);
		return Style;
	}

	if (bInteractionSelected)
	{
		Style.Normal.TintColor = FSlateColor(SelectedFillColor);
		Style.Hovered.TintColor = FSlateColor(SelectedFillColor);
	}

	if (bInteractionFocused)
	{
		SetFocusOutline(Style.Normal, FocusColor, FocusBorderWidth);
		SetFocusOutline(Style.Hovered, FocusColor, FocusBorderWidth);
		SetFocusOutline(Style.Pressed, FocusColor, FocusBorderWidth);
	}
	else if (bInteractionSelected)
	{
		const FLinearColor SelectedBorder = BaseThemeStyle.Normal.OutlineSettings.Color.GetSpecifiedColor();
		SetFocusOutline(Style.Normal, SelectedBorder, SelectedBorderWidth);
		SetFocusOutline(Style.Hovered, SelectedBorder, SelectedBorderWidth);
		SetFocusOutline(Style.Pressed, SelectedBorder, SelectedBorderWidth);
	}
	else if (bInteractionHovered)
	{
		SetFocusOutline(Style.Hovered, FocusColor, FMath::Max(1.0f, FocusBorderWidth - 0.5f));
	}
	return Style;
}

void UKalmalaThemedButton::RebuildInteractionStyle()
{
	if (!bHasThemeStyle) return;
	StopInteractionTransition();
	TransitionStartStyle = GetStyle();
	TransitionTargetStyle = MakeInteractionStyle();
	if (!bAnimateTransitions || TransitionDuration <= 0.0f)
	{
		SetStyle(TransitionTargetStyle);
		return;
	}

	TransitionElapsed = 0.0f;
	const TWeakObjectPtr<UKalmalaThemedButton> WeakThis(this);
	TransitionTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([WeakThis](const float DeltaSeconds)
		{
			UKalmalaThemedButton* Button = WeakThis.Get();
			return Button != nullptr && Button->AdvanceInteractionTransition(DeltaSeconds);
		}), 0.0f);
}

bool UKalmalaThemedButton::AdvanceInteractionTransition(const float DeltaSeconds)
{
	if (UKalmalaSettingsWidget::IsReducedMotionEnabled())
	{
		SetStyle(TransitionTargetStyle);
		TransitionTickerHandle.Reset();
		return false;
	}
	TransitionElapsed += FMath::Max(0.0f, DeltaSeconds);
	const float Progress = FMath::Clamp(TransitionElapsed / TransitionDuration, 0.0f, 1.0f);
	const float Eased = Progress * Progress * (3.0f - 2.0f * Progress);
	SetStyle(BlendStyle(TransitionStartStyle, TransitionTargetStyle, Eased));
	if (Progress < 1.0f) return true;
	SetStyle(TransitionTargetStyle);
	TransitionTickerHandle.Reset();
	return false;
}

void UKalmalaThemedButton::StopInteractionTransition()
{
	if (!TransitionTickerHandle.IsValid()) return;
	FTSTicker::RemoveTicker(TransitionTickerHandle);
	TransitionTickerHandle.Reset();
}

#if !UE_BUILD_SHIPPING
void UKalmalaThemedButton::SetInteractionFocusForVerification(const bool bFocused)
{
	bInteractionFocused = bFocused;
	RebuildInteractionStyle();
}
#endif

void UKalmalaThemedButton::BeginDestroy()
{
	StopInteractionTransition();
	Super::BeginDestroy();
}
