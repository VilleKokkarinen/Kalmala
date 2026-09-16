#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaCombatIntentContract.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

namespace
{
    bool SerializePopulationSave(UKalmalaWorldPopulationSaveGame* SaveGame, TArray<uint8>& OutBytes)
    {
        OutBytes.Reset();
        return IsValid(SaveGame) && UGameplayStatics::SaveGameToMemory(SaveGame, OutBytes);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaCombatIntentContractTest,
    "Kalmala.Gameplay.Combat.IntentContract.RejectionDoesNotMutate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaCombatIntentContractTest::RunTest(const FString& Parameters)
{
    FKalmalaWorldGenerationConfig WorldConfig;
    WorldConfig.WorldSeed = 418;
    UKalmalaWorldPopulationSaveGame* SaveGame = NewObject<UKalmalaWorldPopulationSaveGame>();
    SaveGame->InitializeForWorld(WorldConfig);

    const FString ValidCreatureId(TEXT("1/12/-8/418002"));
    float Health = 100.0f;
    bool bDefeated = false;
    int32 RewardCount = 0;
    TArray<uint8> InitialSaveBytes;
    TestTrue(TEXT("The untouched sparse world save serializes"), SerializePopulationSave(SaveGame, InitialSaveBytes));

    const auto TestRejectedMutation = [this, &Health, &bDefeated, &RewardCount, SaveGame, &InitialSaveBytes, &ValidCreatureId](const TCHAR* Label, const bool bAllowed)
    {
        const float HealthBefore = Health;
        const bool bDefeatedBefore = bDefeated;
        const int32 RewardsBefore = RewardCount;
        if (bAllowed)
        {
            Health = 0.0f;
            bDefeated = true;
            ++RewardCount;
            SaveGame->MarkDefeated(ValidCreatureId);
        }

        TArray<uint8> SaveBytesAfter;
        TestFalse(FString::Printf(TEXT("%s is rejected"), Label), bAllowed);
        TestEqual(FString::Printf(TEXT("%s leaves health unchanged"), Label), Health, HealthBefore);
        TestEqual(FString::Printf(TEXT("%s leaves defeat state unchanged"), Label), bDefeated, bDefeatedBefore);
        TestEqual(FString::Printf(TEXT("%s leaves rewards unchanged"), Label), RewardCount, RewardsBefore);
        TestTrue(FString::Printf(TEXT("%s leaves sparse save bytes unchanged"), Label), SerializePopulationSave(SaveGame, SaveBytesAfter) && SaveBytesAfter == InitialSaveBytes);
    };

    TestRejectedMutation(TEXT("Client-only attack"), FKalmalaCombatIntentContract::IsAttackMutationAllowed(false, true, true, true, 1, true, true, true, true));
    TestRejectedMutation(TEXT("Distant attack"), FKalmalaCombatIntentContract::IsAttackMutationAllowed(true, true, true, true, 2, true, true, false, true));
    TestRejectedMutation(TEXT("Duplicate attack"), FKalmalaCombatIntentContract::IsAttackMutationAllowed(true, true, true, false, 3, true, true, true, true));
    TestRejectedMutation(TEXT("Malformed attack"), FKalmalaCombatIntentContract::IsAttackMutationAllowed(true, true, true, true, 0, false, true, true, true));

    TestRejectedMutation(TEXT("Client-only progression"), FKalmalaCombatIntentContract::IsProgressionMutationAllowed(false, true, true, true, 4, true, true, true, true));
    TestRejectedMutation(TEXT("Distant progression"), FKalmalaCombatIntentContract::IsProgressionMutationAllowed(true, true, true, true, 5, true, false, true, true));
    TestRejectedMutation(TEXT("Duplicate progression"), FKalmalaCombatIntentContract::IsProgressionMutationAllowed(true, true, true, true, 6, true, true, false, true));
    TestRejectedMutation(TEXT("Malformed progression"), FKalmalaCombatIntentContract::IsProgressionMutationAllowed(true, true, true, true, 7, false, true, true, true));

    const bool bAcceptedProgression = FKalmalaCombatIntentContract::IsProgressionMutationAllowed(true, true, true, true, 8, true, true, true, true);
    TestTrue(TEXT("A fully server-validated progression transaction is permitted"), bAcceptedProgression);
    if (bAcceptedProgression)
    {
        Health = 0.0f;
        bDefeated = true;
        ++RewardCount;
        SaveGame->MarkDefeated(ValidCreatureId);
    }
    TestEqual(TEXT("Only the accepted transaction changes health"), Health, 0.0f);
    TestTrue(TEXT("Only the accepted transaction records defeat"), bDefeated);
    TestEqual(TEXT("Only the accepted transaction grants one reward"), RewardCount, 1);
    TestTrue(TEXT("Only the accepted transaction changes the sparse save"), SaveGame->IsDefeated(ValidCreatureId));
    return true;
}

#endif
