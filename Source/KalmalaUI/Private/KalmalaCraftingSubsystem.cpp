#include "KalmalaCraftingSubsystem.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaRecipeCatalogue.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/InputComponent.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Styling/CoreStyle.h"
#include "InputCoreTypes.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"

void UKalmalaCraftingWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized(); SetIsFocusable(true);
    auto* Border = WidgetTree->ConstructWidget<UBorder>(); Border->SetPadding(FMargin(20));
    Border->SetBrushColor(FLinearColor(.025f,.035f,.04f,.98f));
    auto* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
    auto* Column = WidgetTree->ConstructWidget<UVerticalBox>(); Scroll->AddChild(Column);
    auto AddText = [&](const FString& Text, int32 Size) {
        auto* Label = WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetText(FText::FromString(Text)); Label->SetAutoWrapText(true);
        Label->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), Size));
        Label->SetColorAndOpacity(FSlateColor(FLinearColor::White)); Column->AddChild(Label); return Label;
    };
    AddText(TEXT("Camp crafting"), 28);
    AddText(TEXT("Up/Down or D-pad: choose. Enter / A: craft. Escape / B: close.\nController Y: place hearth. X: add fuel. RB: light.\n"), 16);
    RecipesText = AddText(TEXT(""), 18);
    DetailText = AddText(TEXT(""), 18);
    auto AddButton = [&](const TCHAR* Label, UHorizontalBox* Row = nullptr) {
        auto* Button = WidgetTree->ConstructWidget<UButton>(); auto* Text = WidgetTree->ConstructWidget<UTextBlock>();
        Text->SetText(FText::FromString(Label)); Text->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(),18));
        Text->SetColorAndOpacity(FSlateColor(FLinearColor::Black)); Button->SetContent(Text);
        if(Row) { auto* Slot=Row->AddChildToHorizontalBox(Button); Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); Slot->SetPadding(FMargin(2,4)); }
        else Column->AddChild(Button); return Button;
    };
    auto* RecipeActions=WidgetTree->ConstructWidget<UHorizontalBox>(); Column->AddChild(RecipeActions);
    AddButton(TEXT("Previous"),RecipeActions)->OnClicked.AddDynamic(this, &ThisClass::Previous);
    AddButton(TEXT("Next"),RecipeActions)->OnClicked.AddDynamic(this, &ThisClass::Next);
    AddButton(TEXT("Craft one"),RecipeActions)->OnClicked.AddDynamic(this, &ThisClass::Craft);
    AddText(TEXT("\nHearth placement uses one hearth kit + one ember bundle. Face clear ground before opening this menu. Other kits await construction.\n"),16);
    auto* FireActions=WidgetTree->ConstructWidget<UHorizontalBox>(); Column->AddChild(FireActions);
    AddButton(TEXT("Place hearth"),FireActions)->OnClicked.AddDynamic(this, &ThisClass::Place);
    AddButton(TEXT("Add fuel bundle"),FireActions)->OnClicked.AddDynamic(this, &ThisClass::Refuel);
    AddButton(TEXT("Light hearth"),FireActions)->OnClicked.AddDynamic(this, &ThisClass::Light);
    StateText = AddText(TEXT(""), 18);
    auto* CloseButton=AddButton(TEXT("Close")); CloseButton->OnClicked.AddDynamic(this, &ThisClass::CloseClicked);
    CloseButton->RemoveFromParent();
    auto* Outer=WidgetTree->ConstructWidget<UVerticalBox>();
    Outer->AddChildToVerticalBox(Scroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Outer->AddChildToVerticalBox(CloseButton)->SetPadding(FMargin(0,8,0,0));
    Border->SetContent(Outer); WidgetTree->RootWidget = Border;
    SetVisibility(ESlateVisibility::Collapsed);
}

UKalmalaCraftingComponent* UKalmalaCraftingWidget::Model() const
{
    auto* Pawn = GetOwningPlayerPawn(); return Pawn ? Pawn->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
}

void UKalmalaCraftingWidget::Open()
{
    auto* PC = GetOwningPlayer(); if (bOpen || !PC || PC->IsMoveInputIgnored() || !Model()) return;
    bOpen = true; bPreviousCursor = PC->bShowMouseCursor;
    int32 X, Y; PC->GetViewportSize(X,Y);
    const float Scale = FMath::Max(.1f, UWidgetLayoutLibrary::GetViewportScale(this));
    SetDesiredSizeInViewport(FVector2D(FMath::Min(840.0f,X/Scale-32),FMath::Min(980.0f,Y/Scale-32)));
    SetAlignmentInViewport(FVector2D(.5,.5)); SetPositionInViewport(FVector2D(X*.5f,Y*.5f), true);
    SetVisibility(ESlateVisibility::Visible); Refresh();
    PC->SetIgnoreMoveInput(true); PC->SetIgnoreLookInput(true); PC->bShowMouseCursor = true;
    FInputModeGameAndUI Mode; Mode.SetWidgetToFocus(TakeWidget()); Mode.SetHideCursorDuringCapture(false); PC->SetInputMode(Mode);
    SetKeyboardFocus();
}

void UKalmalaCraftingWidget::Close()
{
    if (!bOpen) return; bOpen = false; SetVisibility(ESlateVisibility::Collapsed);
    if (auto* PC=GetOwningPlayer()) { PC->SetIgnoreMoveInput(false); PC->SetIgnoreLookInput(false); PC->bShowMouseCursor=bPreviousCursor; PC->SetInputMode(FInputModeGameOnly()); }
}

void UKalmalaCraftingWidget::Refresh()
{
    auto* M=Model(); if (!M) { Close(); return; }
    const auto& Recipes=GetDefault<UKalmalaRecipeCatalogue>()->Recipes;
    if (Recipes.IsEmpty()) return; Selected=FMath::Clamp(Selected,0,Recipes.Num()-1);
    FString List;
    for (int32 I=0; I<Recipes.Num(); ++I) List+=FString::Printf(TEXT("%s %s\n"), I==Selected ? TEXT(">") : TEXT(" "), *Recipes[I].DisplayName);
    RecipesText->SetText(FText::FromString(List));
    DetailText->SetText(FText::FromString(M->GetRecipeDescription(Recipes[Selected].RecipeId)+TEXT("\n")+M->GetRecipeAvailability(Recipes[Selected].RecipeId)+TEXT("\n")));
    StateText->SetText(FText::FromString(TEXT("\n")+M->GetNearbyFireText()+TEXT("\n")+M->GetLastResult()+TEXT("\n")));
}

FString UKalmalaCraftingWidget::GetPresentationText() const
{
    return RecipesText && DetailText && StateText ? RecipesText->GetText().ToString()+DetailText->GetText().ToString()+StateText->GetText().ToString() : FString();
}
void UKalmalaCraftingWidget::NativeTick(const FGeometry& G,float D) { Super::NativeTick(G,D); if(bOpen) Refresh(); }
void UKalmalaCraftingWidget::Previous() { const int32 N=GetDefault<UKalmalaRecipeCatalogue>()->Recipes.Num(); if(N) Selected=(Selected+N-1)%N; Refresh(); }
void UKalmalaCraftingWidget::Next() { const int32 N=GetDefault<UKalmalaRecipeCatalogue>()->Recipes.Num(); if(N) Selected=(Selected+1)%N; Refresh(); }
void UKalmalaCraftingWidget::Craft() { const auto& R=GetDefault<UKalmalaRecipeCatalogue>()->Recipes; if(auto* M=Model(); M && R.IsValidIndex(Selected)) M->ServerCraft(R[Selected].RecipeId,1); }
void UKalmalaCraftingWidget::Place() { if(auto* M=Model()) M->ServerPlaceCampfire(); }
void UKalmalaCraftingWidget::Refuel() { if(auto* M=Model()) M->ServerRefuel(); }
void UKalmalaCraftingWidget::Light() { if(auto* M=Model()) M->ServerLight(); }
void UKalmalaCraftingWidget::CloseClicked() { Close(); }
FReply UKalmalaCraftingWidget::NativeOnPreviewKeyDown(const FGeometry& G,const FKeyEvent& E)
{
    const FKey K=E.GetKey();
    if(K==EKeys::Escape || K==EKeys::Gamepad_FaceButton_Right) { Close(); return FReply::Handled(); }
    if(K==EKeys::Gamepad_FaceButton_Top) { if(!E.IsRepeat()) Place(); return FReply::Handled(); }
    if(K==EKeys::Gamepad_FaceButton_Left) { if(!E.IsRepeat()) Refuel(); return FReply::Handled(); }
    if(K==EKeys::Gamepad_RightShoulder) { if(!E.IsRepeat()) Light(); return FReply::Handled(); }
    // Reserve arrows for recipe selection only while the panel itself has focus;
    // focused buttons keep ordinary keyboard/controller navigation and activation.
    if (HasKeyboardFocus())
    {
        if(K==EKeys::Up || K==EKeys::Gamepad_DPad_Up) { Previous(); return FReply::Handled(); }
        if(K==EKeys::Down || K==EKeys::Gamepad_DPad_Down) { Next(); return FReply::Handled(); }
        if(K==EKeys::Enter || K==EKeys::Gamepad_FaceButton_Bottom) { if(!E.IsRepeat()) Craft(); return FReply::Handled(); }
    }
    return Super::NativeOnPreviewKeyDown(G,E);
}

void UKalmalaCraftingSubsystem::Tick(float DeltaTime)
{
    if(!GetWorld() || !GetWorld()->IsGameWorld() || !GetLocalPlayer()) return;
    auto* PC=GetLocalPlayer()->GetPlayerController(GetWorld()); if(Controller!=PC) Release();
    if(!PC || !PC->IsLocalController()) return; Controller=PC;
    if(PC->InputComponent && BoundInput.Get()!=PC->InputComponent)
    {
        PC->InputComponent->BindAction(TEXT("CraftMenu"),IE_Pressed,this,&ThisClass::Toggle).bConsumeInput=true;
        BoundInput=PC->InputComponent;
    }
#if !UE_BUILD_SHIPPING
    if(!bVerified && PC->GetPawn() && FParse::Param(FCommandLine::Get(),TEXT("KalmalaCraftingTest")))
    {
        if(auto* Input=BoundInput.Get()) for(int32 Index=0;Index<Input->GetNumActionBindings();++Index)
        {
            auto& Binding=Input->GetActionBinding(Index);
            if(Binding.GetActionName()==TEXT("CraftMenu") && Binding.KeyEvent==IE_Pressed) Binding.ActionDelegate.Execute(FKey());
        }
        if(Widget && Widget->IsOpen())
        {
            const auto Text=Widget->GetPresentationText();
            const bool Passed=Text.Contains(TEXT("Cost:")) && Text.Contains(TEXT("Ember"),ESearchCase::IgnoreCase) && PC->IsMoveInputIgnored() && Widget->IsFocusable();
            Widget->Close();
            UE_LOG(LogTemp,Display,TEXT("Crafting presentation: Passed=%d Restored=%d"),Passed,!PC->IsMoveInputIgnored()); bVerified=true;
        }
    }
    FString CapturePath;
    if(bVerified && !bCaptureRequested && PC->GetPawn() && FParse::Value(FCommandLine::Get(),TEXT("KalmalaCraftingCapture="),CapturePath))
    {
        auto* M=PC->GetPawn()->FindComponentByClass<UKalmalaCraftingComponent>();
        if(M && M->GetNearbyFireText().Contains(TEXT("96% wet")))
        {
            if(Widget && !Widget->IsOpen()) Widget->Open();
            CaptureWait+=DeltaTime;
            if(CaptureWait>2) { FScreenshotRequest::RequestScreenshot(CapturePath,true,false); bCaptureRequested=true; }
        }
    }
#endif
}
void UKalmalaCraftingSubsystem::Toggle()
{
    if(!Controller) return;
    if(!Widget) { Widget=CreateWidget<UKalmalaCraftingWidget>(Controller); if(Widget) Widget->AddToPlayerScreen(160); }
    if(Widget) { if(Widget->IsOpen()) Widget->Close(); else Widget->Open(); }
}
bool UKalmalaCraftingSubsystem::CloseIfOpen() { if(!Widget || !Widget->IsOpen()) return false; Widget->Close(); return true; }
void UKalmalaCraftingSubsystem::Release()
{
    if(auto* Input=BoundInput.Get()) for(int32 I=Input->GetNumActionBindings()-1;I>=0;--I)
        if(Input->GetActionBinding(I).ActionDelegate.IsBoundToObject(this)) Input->RemoveActionBinding(I);
    BoundInput.Reset(); if(Widget) { Widget->Close(); Widget->RemoveFromParent(); Widget=nullptr; } Controller=nullptr; bVerified=false;
}
void UKalmalaCraftingSubsystem::Deinitialize() { Release(); Super::Deinitialize(); }
