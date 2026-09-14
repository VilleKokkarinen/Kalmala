#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaCampfire.h"
#include "KalmalaCharacter.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaPlayerStatusComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaHearthStateTest, "Kalmala.Gameplay.Hearth.RainState",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaHearthStateTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    auto* Pawn = World->SpawnActor<AKalmalaCharacter>();
    auto* Controller = World->SpawnActor<APlayerController>();
    auto* Fire = World->SpawnActor<AKalmalaCampfire>();
    auto* Roof = World->SpawnActor<AKalmalaConstructionActor>();
    if (!Pawn || !Controller || !Fire || !Roof) { AddError(TEXT("Hearth fixture spawn failed")); World->DestroyWorld(false); return false; }
    Controller->Possess(Pawn);
    Fire->SetActorLocation(Pawn->GetActorLocation() + FVector(120, 0, 0));
    Roof->InitializeFromServer(TEXT("RoofKit"), TEXT("RainStateRoof"));
    Roof->SetActorLocation(Fire->GetActorLocation() + FVector(1000, 0, 300));
    Fire->InitializePaidFromServer(Pawn);
    Fire->AdvanceFromServer(1, 0, 0);
    TestTrue(TEXT("Fuel alone does not self-light"), Fire->GetHearthState() == EKalmalaHearthState::Extinguished);
    TestEqual(TEXT("Unlit fuel is not consumed"), Fire->GetFuelSeconds(), 60.0f);
    Fire->Interact_Implementation(Pawn);
    Fire->AdvanceFromServer(1, 0, 1);
    TestTrue(TEXT("Dry active fire remains lit despite wind"), Fire->IsLit());
    TestEqual(TEXT("Lit heat is normalized"), Fire->GetEffectiveWarmth(), 1.0f);
    Fire->AdvanceFromServer(1, 0.05f, 0);
    TestTrue(TEXT("Rain threshold smoulders immediately"), Fire->GetHearthState() == EKalmalaHearthState::Smouldering);
    TestEqual(TEXT("Smoulder has no heat"), Fire->GetEffectiveWarmth(), 0.0f);
    TestEqual(TEXT("Smoulder has no light"), Fire->FindComponentByClass<UPointLightComponent>()->Intensity, 0.0f);
    TestTrue(TEXT("Text names smouldering"), Fire->GetStatusText().Contains(TEXT("SMOULDERING")));
    auto* Status = Pawn->FindComponentByClass<UKalmalaPlayerStatusComponent>();
    Status->ApplyWetFromServer();
    TestFalse(TEXT("Smoulder cannot remove Wet"), Status->TryRemoveWetAtCampfireFromServer(Fire));
    Fire->AdvanceFromServer(10, 1, 1);
    TestEqual(TEXT("Smoulder consumes normal fuel"), Fire->GetFuelSeconds(), 48.0f);
    Fire->SetRole(ROLE_SimulatedProxy);
    Fire->AdvanceFromServer(10, 0, 0);
    TestEqual(TEXT("Client cannot consume fuel"), Fire->GetFuelSeconds(), 48.0f);
    TestTrue(TEXT("Client cannot reignite"), Fire->GetHearthState() == EKalmalaHearthState::Smouldering);
    Fire->SetRole(ROLE_Authority);
    Roof->SetActorLocation(Fire->GetActorLocation() + FVector(0, 0, 300));
    Fire->AdvanceFromServer(0, 1, 1);
    TestTrue(TEXT("Actual roof restores Lit in rain"), Fire->HasRoof() && Fire->IsLit());
    TestTrue(TEXT("Restored heat removes Wet"), Status->TryRemoveWetAtCampfireFromServer(Fire));
    Roof->SetActorLocation(Fire->GetActorLocation() + FVector(1000, 0, 300));
    Fire->AdvanceFromServer(0, 1, 0);
    TestTrue(TEXT("Loss of roof restores smoulder"), Fire->GetHearthState() == EKalmalaHearthState::Smouldering);
    Fire->AdvanceFromServer(0, 0.049f, 1);
    TestTrue(TEXT("Dry threshold automatically reignites"), Fire->IsLit());
    Fire->AdvanceFromServer(NAN, 1, 0);
    Fire->AdvanceFromServer(10, NAN, 0);
    TestEqual(TEXT("Malformed weather steps preserve fuel"), Fire->GetFuelSeconds(), 48.0f);
    Fire->AdvanceFromServer(48, 1, 0);
    TestTrue(TEXT("Fuel exhaustion extinguishes"), Fire->GetHearthState() == EKalmalaHearthState::Extinguished);
    TestEqual(TEXT("Exhaustion has no heat"), Fire->GetEffectiveWarmth(), 0.0f);
    Fire->AdvanceFromServer(0, 0, 0);
    TestFalse(TEXT("Dry weather cannot revive exhausted fire"), Fire->IsLit());
    World->DestroyWorld(false);
    return true;
}
#endif
