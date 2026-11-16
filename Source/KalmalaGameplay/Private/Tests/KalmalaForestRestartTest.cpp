#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaGameMode.h"
#include "KalmalaForestLayout.h"
#include "KalmalaForestResourceContract.h"
#include "KalmalaForestTundraTestSupport.h"
#include "KalmalaHarvestNode.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "UObject/UnrealType.h"

namespace
{
    bool SetPendingRewardRowsForRestartTest(UKalmalaWorldPopulationSaveGame* SaveGame, const int32 RowCount)
    {
        if (SaveGame == nullptr || RowCount < 0) return false;
        FArrayProperty* PendingRewardsProperty = FindFProperty<FArrayProperty>(
            UKalmalaWorldPopulationSaveGame::StaticClass(), TEXT("PendingWildlifeRewards"));
        if (PendingRewardsProperty == nullptr) return false;
        TArray<FKalmalaPendingWildlifeReward>* PendingRewards =
            PendingRewardsProperty->ContainerPtrToValuePtr<TArray<FKalmalaPendingWildlifeReward>>(SaveGame);
        if (PendingRewards == nullptr) return false;

        PendingRewards->SetNum(RowCount);
        for (int32 Index = 0; Index < RowCount; ++Index)
        {
            FKalmalaPendingWildlifeReward& Reward = (*PendingRewards)[Index];
            Reward.PersistentSpawnId = FString::Printf(TEXT("rf55-claim-%d"), Index);
            Reward.PlayerIdentity = TEXT("player:rf55");
            Reward.GemItemId = NAME_None;
        }
        return true;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaForestRestartTest,
    "Kalmala.Gameplay.Harvest.ForestProcessRestart",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaForestRestartTest::RunTest(const FString& Parameters)
{
    FString Phase, Root;
    if (!FParse::Value(FCommandLine::Get(), TEXT("KalmalaForestRestart="), Phase))
    {
        AddInfo(TEXT("Real-slot process restart is opt-in; use BiomeExpansion.ps1 -ForestRestartOnly."));
        return true;
    }
    if (!TestTrue(TEXT("Restart phase is Write, Read, or Reject"),
            Phase == TEXT("Write") || Phase == TEXT("Read") || Phase == TEXT("Reject"))
        || !TestTrue(TEXT("Explicit isolated root supplied"),
            FParse::Value(FCommandLine::Get(), TEXT("KalmalaForestRestartRoot="), Root))) return false;
    Root = FPaths::ConvertRelativePathToFull(Root);
    FPaths::NormalizeDirectoryName(Root);
    FString Saved = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
    FPaths::NormalizeDirectoryName(Saved);
    if (!TestTrue(TEXT("Restart root is inside OS temporary storage"),
            FPaths::IsUnderDirectory(Root, FPaths::ConvertRelativePathToFull(FPlatformProcess::UserTempDir())))
        || !TestTrue(TEXT("Saved directory is the explicitly isolated root"), FPaths::IsSamePath(Saved, Root / TEXT("Saved")))
        || !TestTrue(TEXT("Slot path is below MAX_PATH"),
            (Saved / TEXT("SaveGames/KalmalaPopulationDeltas_18446744073709551615.sav")).Len() < 260)) return false;
    const bool bWrite = Phase == TEXT("Write");
    const bool bRead = Phase == TEXT("Read");
    const bool bRejectOversized = Phase == TEXT("Reject");
    FKalmalaWorldGenerationConfig Config;
    Config.WorldSeed = bRejectOversized ? 419ull : 418ull;
    const FString Slot = FString::Printf(TEXT("KalmalaPopulationDeltas_%llu"), Config.WorldSeed);
    if (!TestEqual(TEXT("Only read phase starts with the population slot"),
            UGameplayStatics::DoesSaveGameExist(Slot, 0), bRead)) return false;
    if (bWrite && !TestFalse(TEXT("Write phase requires a fresh save directory"),
        IFileManager::Get().DirectoryExists(*(Saved / TEXT("SaveGames"))))) return false;
    FVector2D Center = FVector2D::ZeroVector;
    TArray<FKalmalaForestTreeDescriptor> Trees;
    if (!bRejectOversized && !TestTrue(TEXT("Stable Tundra patch has collected and uncollected Birch candidates"),
        KalmalaForestTundraTestSupport::FindPatch(Config, 3, Center, Trees))) return false;
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Restart fixture world created"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    auto* Mode = World->SpawnActor<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Authoritative GameMode created"), Mode)) return false;
    Mode->WorldGenerationConfig = Config;
    Mode->TerrainPatchOrigin = Center;
    if (bRejectOversized)
    {
        UKalmalaWorldPopulationSaveGame* OverLimitSave =
            NewObject<UKalmalaWorldPopulationSaveGame>(Mode);
        OverLimitSave->InitializeForWorld(Config);
        if (!TestTrue(TEXT("Over-limit slot fixture contains 4,097 valid owner rows"),
                SetPendingRewardRowsForRestartTest(OverLimitSave,
                    UKalmalaWorldPopulationSaveGame::MaxPendingWildlifeRewards + 1))
            || !TestFalse(TEXT("Over-limit slot fixture fails population-save validation"),
                OverLimitSave->MatchesWorld(Config))
            || !TestTrue(TEXT("Over-limit population record is written to the isolated slot"),
                UGameplayStatics::SaveGameToSlot(OverLimitSave, Slot, 0))) return false;

        auto* OverLimitSlotSave = Cast<UKalmalaWorldPopulationSaveGame>(
            UGameplayStatics::LoadGameFromSlot(Slot, 0));
        if (!TestNotNull(TEXT("The over-limit slot deserializes as the expected save type"), OverLimitSlotSave)) return false;
        TestEqual(TEXT("The production slot contains all 4,097 owner rows before validation"),
            OverLimitSlotSave->GetPendingWildlifeRewards().Num(),
            UKalmalaWorldPopulationSaveGame::MaxPendingWildlifeRewards + 1);

        const FString SlotPath = FPaths::Combine(Saved, TEXT("SaveGames"), Slot + TEXT(".sav"));
        TArray<uint8> BeforeSlotBytes;
        if (!TestTrue(TEXT("Serialized over-limit slot bytes are readable"),
                FFileHelper::LoadFileToArray(&BeforeSlotBytes, *SlotPath))) return false;
        Mode->LoadPopulationSave();
        TestNull(TEXT("Production loader does not retain an over-limit save for retry scans"),
            Mode->PopulationSaveGame.Get());
        TArray<uint8> AfterSlotBytes;
        if (!TestTrue(TEXT("Rejected slot bytes remain readable"),
                FFileHelper::LoadFileToArray(&AfterSlotBytes, *SlotPath))) return false;
        TestTrue(TEXT("Production load rejection leaves the original slot bytes unchanged"),
            BeforeSlotBytes == AfterSlotBytes);
        AddInfo(FString::Printf(TEXT("Forest restart oversized rejection: Phase=Reject Slot=%s Rows=4097 SlotUnchanged=1"), *Slot));
        return true;
    }
    Mode->LoadPopulationSave(); // Actual production slot loader, before patch activation.
    if (!TestNotNull(TEXT("Production loader provides population save"), Mode->PopulationSaveGame.Get())) return false;
    TestTrue(TEXT("Loaded population matches world"), Mode->PopulationSaveGame->MatchesWorld(Config));
    Mode->ActivateTerrainPatch(FIntPoint::ZeroValue);
    const auto* Nodes = Mode->ActiveForestNodes.Find(FIntPoint::ZeroValue);
    if (!TestNotNull(TEXT("Production activation materializes forest"), Nodes)) return false;
    TestEqual(TEXT("Only previously collected candidates are absent"), Nodes->Num(), Trees.Num() - (bWrite ? 0 : 2));
    for (int32 Index = 0; Index < Trees.Num(); ++Index)
    {
        const FString& Id = Trees[Index].PersistentSpawnId;
        TestEqual(TEXT("Tundra persistent identity keeps its original candidate ordinal"), Id,
            FKalmalaForestLayout::GetPersistentSpawnId(Config, Center, Trees[Index].CandidateIndex));
        AKalmalaHarvestNode* Live = nullptr;
        for (const auto& Node : *Nodes)
            if (Node.IsValid() && Node->GetPersistentSpawnId() == Id) Live = Node.Get();
        TestEqual(TEXT("Exact candidate presence matches saved collection"), Live != nullptr, bWrite || Index >= 2);
        TestEqual(TEXT("Exact sparse ID survives process restart"), Mode->PopulationSaveGame->IsHarvested(Id), !bWrite && Index < 2);
        if (Live)
        {
            TestEqual(TEXT("Restarted Tundra Birch keeps the descriptor resource source"),
                Live->GetGatheringSourceId(), FKalmalaForestResourceContract::GetSourceId(Trees[Index].TreeKind));
        }
        TestFalse(TEXT("Forest collection never records creature defeat"), Mode->PopulationSaveGame->IsDefeated(Id));
        if (bWrite && Index < 2)
        {
            if (!TestNotNull(TEXT("Collected candidate is materialized"), Live)) return false;
            TestTrue(TEXT("Node uses real GameMode depletion binding"), Live->OnHarvested.IsBoundToObject(Mode));
            // Pickup transaction coverage is separate; exercise its actual bound
            // persistence callback here without replacing the disk writer.
            Live->OnHarvested.Broadcast(Id);
        }
        AddInfo(FString::Printf(TEXT("Forest restart candidate: Phase=%s Id=%s Collected=%d Live=%d"),
            *Phase, *Id, Index < 2, Live != nullptr));
    }
    auto* Disk = Cast<UKalmalaWorldPopulationSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
    if (!TestNotNull(TEXT("Real production population slot exists and loads"), Disk)) return false;
    TestTrue(TEXT("Disk slot retains world identity"), Disk->MatchesWorld(Config));
    for (int32 Index = 0; Index < Trees.Num(); ++Index)
        TestEqual(TEXT("Disk depletion contains exactly the collected candidates"),
            Disk->IsHarvested(Trees[Index].PersistentSpawnId), Index < 2);
    Mode->RefreshTerrainPatchNeighborhoods();
    TestTrue(TEXT("Eviction removes all forest actors"), Mode->ActiveForestNodes.IsEmpty());
    AddInfo(FString::Printf(TEXT("Forest restart phase: Phase=%s Slot=%s Collected=2 Uncollected=%d"),
        *Phase, *Slot, Trees.Num() - 2));
    return true;
}

#endif
