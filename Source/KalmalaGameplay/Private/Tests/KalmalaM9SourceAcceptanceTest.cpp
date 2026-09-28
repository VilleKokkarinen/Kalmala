#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaCharacter.h"
#include "KalmalaHarvestNode.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaM9SourceLootContract.h"
#include "KalmalaSkillProgressionComponent.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaWorldPopulationLayout.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaM9SourceAcceptanceTest,
    "Kalmala.Gameplay.M9.SecondWaveHarvestAcceptance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaM9SourceAcceptanceTest::RunTest(const FString& Parameters)
{
    struct FSourceCase
    {
        FName SourceId;
        FName ItemId;
        FName ToolId;
        EKalmalaToolAction Action;
    };

    const FSourceCase Sources[] =
    {
        { TEXT("meadows-birch-trunk"), TEXT("Lightwood"), TEXT("BronzeAxe"), EKalmalaToolAction::Woodcutting },
        { TEXT("elderwood-ironheart-trunk"), TEXT("Densewood"), TEXT("IronAxe"), EKalmalaToolAction::Woodcutting },
        { TEXT("mire-peat-amber-seam"), TEXT("PeatAmber"), TEXT("StonePick"), EKalmalaToolAction::Mining },
        { TEXT("tundra-frost-salt-deposit"), TEXT("FrostSalt"), TEXT("StonePick"), EKalmalaToolAction::Mining },
    };

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("M9 harvest acceptance world created"), World)) return false;
    World->SetGameState(World->SpawnActor<AKalmalaWorldGenerationGameState>());
    const AKalmalaWorldGenerationGameState* WorldState = World->GetGameState<AKalmalaWorldGenerationGameState>();
    if (!TestNotNull(TEXT("M9 fixture has a server world identity"), WorldState))
    {
        World->DestroyWorld(false);
        return false;
    }

    UWorld* OtherWorld = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Foreign-world rejection fixture created"), OtherWorld))
    {
        World->DestroyWorld(false);
        return false;
    }
    AKalmalaCharacter* ForeignPawn = OtherWorld->SpawnActor<AKalmalaCharacter>();
    if (!TestNotNull(TEXT("Foreign-world pawn spawned"), ForeignPawn))
    {
        OtherWorld->DestroyWorld(false);
        World->DestroyWorld(false);
        return false;
    }

    const auto SetServerToolCondition = [this](AKalmalaCharacter* Pawn, const FName ToolId, const int32 Condition)
    {
        const FName PropertyName(*FString::Printf(TEXT("%sDurability"), *ToolId.ToString()));
        FIntProperty* Property = FindFProperty<FIntProperty>(Pawn->GetClass(), PropertyName);
        if (!TestNotNull(TEXT("Fixture can set owner tool condition"), Property)) return false;
        Property->SetPropertyValue_InContainer(Pawn, Condition);
        return true;
    };

    bool bAllFixturesValid = true;
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Sources); ++Index)
    {
        const FSourceCase& Source = Sources[Index];
        const FVector Location(static_cast<float>(Index) * 2000.0f, 0.0f, 0.0f);
        AKalmalaCharacter* Pawn = World->SpawnActor<AKalmalaCharacter>(Location, FRotator::ZeroRotator);
        AKalmalaHarvestNode* Node = World->SpawnActor<AKalmalaHarvestNode>(Location, FRotator::ZeroRotator);
        if (!Pawn || !Node)
        {
            AddError(FString::Printf(TEXT("M9 source fixture failed to spawn: %s"), *Source.SourceId.ToString()));
            bAllFixturesValid = false;
            continue;
        }

        if (Pawn->GetSkillProgressionComponent()) Pawn->GetSkillProgressionComponent()->BeginPlay();
        if (!SetServerToolCondition(Pawn, Source.ToolId,
            FKalmalaToolLifecycleContract::FindDefinition(Source.ToolId)->MaxDurability))
        {
            bAllFixturesValid = false;
            continue;
        }

        FKalmalaWorldPopulationSpawn Spawn;
        Spawn.Kind = EKalmalaWorldPopulationKind::HarvestNode;
        Spawn.SpatialKey = FIntPoint(Index, -2);
        Spawn.SpawnSeed = 918273ull + static_cast<uint64>(Index);
        Spawn.Location = Location;
        Spawn.ContentId = Source.SourceId;
        Node->InitializeServer(Spawn);
        if (!TestFalse(TEXT("Server fixture node has a stable population identity"), Node->GetPersistentSpawnId().IsEmpty()))
        {
            bAllFixturesValid = false;
            continue;
        }

        const int32 InitialCondition = Pawn->GetToolDurability(Source.ToolId);
        const FString SourceLabel = Source.SourceId.ToString();
        const auto AssertRejectedWithoutChanges = [this, Pawn, Node, Source, InitialCondition, SourceLabel]()
        {
            TestFalse(FString::Printf(TEXT("%s rejected request leaves its source available"), *SourceLabel), Node->IsHarvested());
            TestEqual(FString::Printf(TEXT("%s rejected request leaves owner material unchanged"), *SourceLabel),
                Pawn->GetInventoryComponent()->GetQuantity(Source.ItemId), 0);
            TestEqual(FString::Printf(TEXT("%s rejected request leaves owner pack empty"), *SourceLabel),
                Pawn->GetInventoryComponent()->GetStacks().Num(), 0);
            TestEqual(FString::Printf(TEXT("%s rejected request leaves tool condition unchanged"), *SourceLabel),
                Pawn->GetToolDurability(Source.ToolId), InitialCondition);
        };

        const uint8 ExpectedAction = static_cast<uint8>(Source.Action);
        const EKalmalaToolAction ForgedAction = Source.Action == EKalmalaToolAction::Woodcutting
            ? EKalmalaToolAction::Mining : EKalmalaToolAction::Woodcutting;
        TestFalse(FString::Printf(TEXT("%s rejects a forged tool ID"), *SourceLabel),
            Node->InteractWithToolIntentFromServer(Pawn, 100.0f, 250.0f, TEXT("ForgedTool"), ExpectedAction));
        AssertRejectedWithoutChanges();

        TestFalse(FString::Printf(TEXT("%s rejects a mismatched client action"), *SourceLabel),
            Node->InteractWithToolIntentFromServer(Pawn, 100.0f, 250.0f, Source.ToolId,
                static_cast<uint8>(ForgedAction)));
        AssertRejectedWithoutChanges();

        TestFalse(FString::Printf(TEXT("%s rejects an excessive server trace distance"), *SourceLabel),
            Node->InteractWithToolIntentFromServer(Pawn, 300.0f, 250.0f, Source.ToolId, ExpectedAction));
        AssertRejectedWithoutChanges();

        Pawn->SetActorLocation(Location + FVector(900.0f, 0.0f, 0.0f));
        TestFalse(FString::Printf(TEXT("%s rejects an out-of-range harvester"), *SourceLabel),
            Node->InteractWithToolIntentFromServer(Pawn, 100.0f, 250.0f, Source.ToolId, ExpectedAction));
        AssertRejectedWithoutChanges();
        Pawn->SetActorLocation(Location);

        TestFalse(FString::Printf(TEXT("%s rejects a pawn from another world"), *SourceLabel),
            Node->InteractWithToolIntentFromServer(ForeignPawn, 100.0f, 250.0f, Source.ToolId, ExpectedAction));
        AssertRejectedWithoutChanges();

        Pawn->SetRole(ROLE_SimulatedProxy);
        TestFalse(FString::Printf(TEXT("%s rejects non-authoritative client execution"), *SourceLabel),
            Node->InteractWithToolIntentFromServer(Pawn, 100.0f, 250.0f, Source.ToolId, ExpectedAction));
        Pawn->SetRole(ROLE_Authority);
        AssertRejectedWithoutChanges();

        int32 ExpectedYield = 0;
        TestTrue(FString::Printf(TEXT("%s has a valid bounded server reward"), *SourceLabel),
            FKalmalaM9SourceLootContract::BuildHarvestRewardQuantity(
                WorldState->GetWorldGenerationConfig(), Source.SourceId, Node->GetPersistentSpawnId(), ExpectedYield));
        FString DepletedId;
        Node->OnM9ResourceDepleted.AddLambda([&DepletedId](const FString& ResourceId) { DepletedId = ResourceId; });

        TestTrue(FString::Printf(TEXT("%s accepts its matching in-range server harvest"), *SourceLabel),
            Node->InteractWithToolIntentFromServer(Pawn, 100.0f, 250.0f, Source.ToolId, ExpectedAction));
        TestTrue(FString::Printf(TEXT("%s accepted harvest depletes exactly once"), *SourceLabel), Node->IsHarvested());
        TestEqual(FString::Printf(TEXT("%s grants only its deterministic catalogue reward"), *SourceLabel),
            Pawn->GetInventoryComponent()->GetQuantity(Source.ItemId), ExpectedYield);
        TestEqual(FString::Printf(TEXT("%s accepted use spends one server-owned condition"), *SourceLabel),
            Pawn->GetToolDurability(Source.ToolId), InitialCondition - 1);
        TestEqual(FString::Printf(TEXT("%s depletion callback carries its stable sparse ID"), *SourceLabel),
            DepletedId, FKalmalaM9SourceLootContract::BuildResourceDepletionId(Source.SourceId, Node->GetPersistentSpawnId()));
    }

    OtherWorld->DestroyWorld(false);
    World->DestroyWorld(false);
    return bAllFixturesValid;
}

#endif
