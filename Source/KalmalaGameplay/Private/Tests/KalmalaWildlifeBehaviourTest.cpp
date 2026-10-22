#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaWildlifeSpawn.h"
#include "KalmalaCharacter.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Misc/ScopeExit.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWildlifeBehaviourTest, "Kalmala.Gameplay.WildlifeBehaviour.ServerOwnedCycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaWildlifeBehaviourTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("A client cannot select wildlife behaviour"), AKalmalaWildlifeSpawn::IsBehaviourTransitionAllowed(false, false, EKalmalaWildlifeBehaviour::Idle, EKalmalaWildlifeBehaviour::Flee));
    TestFalse(TEXT("Defeated wildlife cannot resume behaviour"), AKalmalaWildlifeSpawn::IsBehaviourTransitionAllowed(true, true, EKalmalaWildlifeBehaviour::Return, EKalmalaWildlifeBehaviour::Idle));
    TestTrue(TEXT("A server deer sinew roll below 50 percent succeeds"), AKalmalaWildlifeSpawn::IsDeerSinewDropSuccessful(0.499999f));
    TestFalse(TEXT("A server deer sinew roll at 50 percent fails"), AKalmalaWildlifeSpawn::IsDeerSinewDropSuccessful(0.5f));
    TestFalse(TEXT("An invalid deer sinew roll fails closed"), AKalmalaWildlifeSpawn::IsDeerSinewDropSuccessful(-0.01f));
    TestTrue(TEXT("Server damage can enter bounded flee"), AKalmalaWildlifeSpawn::IsBehaviourTransitionAllowed(true, false, EKalmalaWildlifeBehaviour::Idle, EKalmalaWildlifeBehaviour::Flee));
    TestTrue(TEXT("Server flee progresses to investigate"), AKalmalaWildlifeSpawn::IsBehaviourTransitionAllowed(true, false, EKalmalaWildlifeBehaviour::Flee, EKalmalaWildlifeBehaviour::Investigate));
    TestTrue(TEXT("Server investigate progresses to return"), AKalmalaWildlifeSpawn::IsBehaviourTransitionAllowed(true, false, EKalmalaWildlifeBehaviour::Investigate, EKalmalaWildlifeBehaviour::Return));
    TestTrue(TEXT("Server return settles at idle"), AKalmalaWildlifeSpawn::IsBehaviourTransitionAllowed(true, false, EKalmalaWildlifeBehaviour::Return, EKalmalaWildlifeBehaviour::Idle));
    TestFalse(TEXT("The cycle cannot skip directly from flee to idle"), AKalmalaWildlifeSpawn::IsBehaviourTransitionAllowed(true, false, EKalmalaWildlifeBehaviour::Flee, EKalmalaWildlifeBehaviour::Idle));
    TestEqual(TEXT("Archetype selection stays deterministic for one descriptor seed"), uint8(AKalmalaWildlifeSpawn::GetArchetypeForSpawnSeed(6)), uint8(EKalmalaWildlifeArchetype::Boar));
    TestEqual(TEXT("Existing Mireling descriptor seed stays a Mireling"), uint8(AKalmalaWildlifeSpawn::GetArchetypeForSpawnSeed(3308806006119996599ull)), uint8(EKalmalaWildlifeArchetype::Mireling));
    TestEqual(TEXT("A non-boar descriptor deterministically selects deer"), uint8(AKalmalaWildlifeSpawn::GetArchetypeForSpawnSeed(1)), uint8(EKalmalaWildlifeArchetype::Deer));
    TestEqual(TEXT("Boar runtime health matches the revised profile"), AKalmalaWildlifeSpawn::GetMaximumHealthForArchetype(EKalmalaWildlifeArchetype::Boar), 20.0f);
    TestEqual(TEXT("Boar runtime armor matches the revised profile"), AKalmalaWildlifeSpawn::GetArmorForArchetype(EKalmalaWildlifeArchetype::Boar), 5.0f);
    TestEqual(TEXT("Boar charge damage matches the revised Crush profile"), AKalmalaWildlifeSpawn::GetBaseAttackDamageForArchetype(EKalmalaWildlifeArchetype::Boar), 10.0f);
    TestEqual(TEXT("Boar attack uses the Crush channel"), AKalmalaWildlifeSpawn::GetBaseAttackChannelsForArchetype(EKalmalaWildlifeArchetype::Boar).Crush, 10.0f);
    TestEqual(TEXT("Deer runtime health matches the revised profile"), AKalmalaWildlifeSpawn::GetMaximumHealthForArchetype(EKalmalaWildlifeArchetype::Deer), 30.0f);
    TestEqual(TEXT("Deer runtime armor matches the revised profile"), AKalmalaWildlifeSpawn::GetArmorForArchetype(EKalmalaWildlifeArchetype::Deer), 5.0f);
    TestEqual(TEXT("Deer has no authored attack"), AKalmalaWildlifeSpawn::GetBaseAttackDamageForArchetype(EKalmalaWildlifeArchetype::Deer), 0.0f);
    TestEqual(TEXT("Mireling runtime health matches the revised profile"), AKalmalaWildlifeSpawn::GetMaximumHealthForArchetype(EKalmalaWildlifeArchetype::Mireling), 75.0f);
    TestEqual(TEXT("Mireling runtime armor matches the revised profile"), AKalmalaWildlifeSpawn::GetArmorForArchetype(EKalmalaWildlifeArchetype::Mireling), 15.0f);
    TestEqual(TEXT("Mireling melee damage matches the revised Crush profile"), AKalmalaWildlifeSpawn::GetBaseAttackDamageForArchetype(EKalmalaWildlifeArchetype::Mireling), 20.0f);
    TestEqual(TEXT("Mireling attack uses the Crush channel"), AKalmalaWildlifeSpawn::GetBaseAttackChannelsForArchetype(EKalmalaWildlifeArchetype::Mireling).Crush, 20.0f);
    TestEqual(TEXT("Rat runtime health matches the reviewed profile"), AKalmalaWildlifeSpawn::GetMaximumHealthForArchetype(EKalmalaWildlifeArchetype::Rat), 10.0f);
    TestEqual(TEXT("Rat runtime armor matches the reviewed profile"), AKalmalaWildlifeSpawn::GetArmorForArchetype(EKalmalaWildlifeArchetype::Rat), 0.0f);
    const FKalmalaDamageChannels RatAttack = AKalmalaWildlifeSpawn::GetBaseAttackChannelsForArchetype(EKalmalaWildlifeArchetype::Rat);
    TestEqual(TEXT("Rat attack uses reviewed Pierce damage"), RatAttack.Pierce, 5.0f);
    TestEqual(TEXT("Rat attack leaves Crush inactive"), RatAttack.Crush, 0.0f);
    TestEqual(TEXT("Rat base attack total matches its profile"), AKalmalaWildlifeSpawn::GetBaseAttackDamageForArchetype(EKalmalaWildlifeArchetype::Rat), 5.0f);
    TestEqual(TEXT("Badger runtime health matches the reviewed profile"), AKalmalaWildlifeSpawn::GetMaximumHealthForArchetype(EKalmalaWildlifeArchetype::Badger), 100.0f);
    TestEqual(TEXT("Badger runtime armor matches the reviewed profile"), AKalmalaWildlifeSpawn::GetArmorForArchetype(EKalmalaWildlifeArchetype::Badger), 10.0f);
    const FKalmalaDamageChannels BadgerAttack = AKalmalaWildlifeSpawn::GetBaseAttackChannelsForArchetype(EKalmalaWildlifeArchetype::Badger);
    TestEqual(TEXT("Badger attack uses reviewed Slash damage"), BadgerAttack.Slash, 20.0f);
    TestEqual(TEXT("Badger attack leaves Crush inactive"), BadgerAttack.Crush, 0.0f);
    TestEqual(TEXT("Badger base attack total matches its profile"), AKalmalaWildlifeSpawn::GetBaseAttackDamageForArchetype(EKalmalaWildlifeArchetype::Badger), 20.0f);
    TestEqual(TEXT("Moon Bear runtime health matches the reviewed profile"), AKalmalaWildlifeSpawn::GetMaximumHealthForArchetype(EKalmalaWildlifeArchetype::MoonBear), 250.0f);
    TestEqual(TEXT("Moon Bear runtime armor matches the reviewed profile"), AKalmalaWildlifeSpawn::GetArmorForArchetype(EKalmalaWildlifeArchetype::MoonBear), 20.0f);
    const FKalmalaDamageChannels MoonBearAttack = AKalmalaWildlifeSpawn::GetBaseAttackChannelsForArchetype(EKalmalaWildlifeArchetype::MoonBear);
    TestEqual(TEXT("Moon Bear attack uses reviewed Crush damage"), MoonBearAttack.Crush, 75.0f);
    TestEqual(TEXT("Moon Bear base attack total matches its profile"), AKalmalaWildlifeSpawn::GetBaseAttackDamageForArchetype(EKalmalaWildlifeArchetype::MoonBear), 75.0f);
    TestEqual(TEXT("Viper runtime health matches the reviewed profile"),
        AKalmalaWildlifeSpawn::GetMaximumHealthForArchetype(EKalmalaWildlifeArchetype::Viper), 100.0f);
    TestEqual(TEXT("Viper runtime armor matches the reviewed profile"),
        AKalmalaWildlifeSpawn::GetArmorForArchetype(EKalmalaWildlifeArchetype::Viper), 10.0f);
    const FKalmalaDamageChannels ViperAttack = AKalmalaWildlifeSpawn::GetBaseAttackChannelsForArchetype(
        EKalmalaWildlifeArchetype::Viper);
    TestEqual(TEXT("Viper attack uses reviewed Stab damage"), ViperAttack.Stab, 50.0f);
    TestEqual(TEXT("Viper baseline reward is one Bone Fragment"),
        AKalmalaWildlifeSpawn::GetBoneFragmentRewardQuantity(EKalmalaWildlifeArchetype::Viper), 1);
    TestEqual(TEXT("Viper lunge has the reviewed warning"), AKalmalaWildlifeSpawn::ViperTelegraphSeconds, 0.5f);
    TestEqual(TEXT("Viper recovers after its reviewed lunge interval"), AKalmalaWildlifeSpawn::ViperRecoverySeconds, 0.8f);
    TestEqual(TEXT("Viper lunge applies five seconds of weak poison"),
        AKalmalaWildlifeSpawn::ViperWeakPoisonDurationSeconds, 5.0f);
    TestEqual(TEXT("Snake bite reach remains close range"), AKalmalaWildlifeSpawn::SnakeBiteReachCentimetres, 180.0f);
    for (const EKalmalaBiome Biome : { EKalmalaBiome::Meadows, EKalmalaBiome::Elderwood,
             EKalmalaBiome::ShimmeringLakes, EKalmalaBiome::FreezingTundra,
             EKalmalaBiome::ThunderMountains, EKalmalaBiome::Ocean })
    {
        for (uint64 Seed = 0; Seed < 100; ++Seed)
        {
            EKalmalaWildlifeArchetype SelectedArchetype = EKalmalaWildlifeArchetype::Mireling;
            if (AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(Seed, Biome, SelectedArchetype))
            {
                TestFalse(TEXT("Mireling is excluded from every non-Mire biome"),
                    SelectedArchetype == EKalmalaWildlifeArchetype::Mireling);
            }
        }
    }
    const TArray<TPair<FName, EKalmalaWildlifeEcology>> NicheProfiles =
    {
        { TEXT("meadows-open-grazer"), EKalmalaWildlifeEcology::OpenGrazer },
        { TEXT("lakes-shore-forager"), EKalmalaWildlifeEcology::ShoreForager },
        { TEXT("elderwood-canopy-browser"), EKalmalaWildlifeEcology::CanopyBrowser },
        { TEXT("mire-hummock-scavenger"), EKalmalaWildlifeEcology::HummockScavenger },
        { TEXT("tundra-wind-grazer"), EKalmalaWildlifeEcology::WindGrazer },
        { TEXT("mountains-ridge-forager"), EKalmalaWildlifeEcology::RidgeForager }
    };
    for (const TPair<FName, EKalmalaWildlifeEcology>& Profile : NicheProfiles)
    {
        TestEqual(TEXT("Each first-wave niche selects a stable ecological profile"),
            AKalmalaWildlifeSpawn::GetEcologyForNiche(Profile.Key), Profile.Value);
        TestTrue(TEXT("Each niche keeps flee pressure inside the existing bounded wildlife budget"),
            AKalmalaWildlifeSpawn::GetEcologicalFleeDistance(Profile.Key) >= 180.0f
            && AKalmalaWildlifeSpawn::GetEcologicalFleeDistance(Profile.Key) <= 300.0f);
    }
    TestEqual(TEXT("Unknown niche input fails closed to a bounded generalist profile"),
        AKalmalaWildlifeSpawn::GetEcologyForNiche(NAME_None), EKalmalaWildlifeEcology::Generalist);
    TestEqual(TEXT("Mireling pressure uses the tuned server melee interval"), AKalmalaWildlifeSpawn::MirelingMeleeCooldownSeconds, 1.25f);
    TestFalse(TEXT("A client cannot begin a territorial charge"), AKalmalaWildlifeSpawn::IsBoarChargeAllowed(false, false, true, 100.0f));
    TestFalse(TEXT("A defeated boar cannot begin a territorial charge"), AKalmalaWildlifeSpawn::IsBoarChargeAllowed(true, true, true, 100.0f));
    TestFalse(TEXT("A distant player cannot provoke the resting area"), AKalmalaWildlifeSpawn::IsBoarChargeAllowed(true, false, true, 501.0f));
    TestTrue(TEXT("A server-confirmed nearby player can provoke the resting area"), AKalmalaWildlifeSpawn::IsBoarChargeAllowed(true, false, true, 500.0f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaModelSurfaceTest, "Kalmala.Gameplay.Presentation.SurfaceWinding",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaModelSurfaceTest::RunTest(const FString& Parameters)
{
    TArray<FVector> EngineVertices, EngineNormals;
    TArray<int32> EngineTriangles;
    TArray<FVector2D> EngineUV;
    TArray<FProcMeshTangent> EngineTangents;
    UKismetProceduralMeshLibrary::GenerateBoxMesh(FVector(1), EngineVertices, EngineTriangles, EngineNormals, EngineUV, EngineTangents);
    const float EngineSign = FVector::DotProduct(FVector::CrossProduct(
        EngineVertices[EngineTriangles[1]] - EngineVertices[EngineTriangles[0]],
        EngineVertices[EngineTriangles[2]] - EngineVertices[EngineTriangles[0]]), EngineNormals[EngineTriangles[0]]);
    TestTrue(TEXT("Engine reference uses clockwise exterior winding"), EngineSign < 0);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Surface fixture world exists"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    const auto CheckMesh = [this](UProceduralMeshComponent* Mesh, const bool bTetrahedra)
    {
        const FProcMeshSection* Section = Mesh->GetProcMeshSection(0);
        if (!TestNotNull(TEXT("Model mesh has a section"), Section)) return;
        bool bWinding = true, bOutward = true, bNonDegenerate = true;
        FVector WholeCentre = FVector::ZeroVector;
        for (const auto& Vertex : Section->ProcVertexBuffer) WholeCentre += Vertex.Position;
        WholeCentre /= Section->ProcVertexBuffer.Num();
        for (int32 Index = 0; Index < Section->ProcIndexBuffer.Num(); Index += 3)
        {
            const auto& A = Section->ProcVertexBuffer[Section->ProcIndexBuffer[Index]];
            const auto& B = Section->ProcVertexBuffer[Section->ProcIndexBuffer[Index + 1]];
            const auto& C = Section->ProcVertexBuffer[Section->ProcIndexBuffer[Index + 2]];
            const FVector Cross = FVector::CrossProduct(B.Position - A.Position, C.Position - A.Position);
            bNonDegenerate &= !Cross.IsNearlyZero();
            bWinding &= FVector::DotProduct(Cross, A.Normal + B.Normal + C.Normal) < 0;
            FVector Centre = WholeCentre;
            if (bTetrahedra)
            {
                Centre = FVector::ZeroVector;
                const int32 First = (Index / 12) * 12;
                for (int32 Vertex = First; Vertex < First + 12; ++Vertex)
                    Centre += Section->ProcVertexBuffer[Vertex].Position;
                Centre /= 12;
            }
            bOutward &= FVector::DotProduct(A.Normal, (A.Position + B.Position + C.Position) / 3 - Centre) > 0;
        }
        TestTrue(*FString::Printf(TEXT("%s winding matches the engine exterior convention"), *Mesh->GetName()), bWinding);
        TestTrue(*FString::Printf(TEXT("%s normals face away from its solid centre"), *Mesh->GetName()), bOutward);
        TestTrue(*FString::Printf(TEXT("%s has no degenerate faces"), *Mesh->GetName()), bNonDegenerate);
    };
    auto* Player = World->SpawnActor<AKalmalaCharacter>();
    if (!TestNotNull(TEXT("Player fixture exists"), Player)) return false;
    Player->DispatchBeginPlay();
    TArray<UProceduralMeshComponent*> PlayerParts;
    Player->GetComponents(PlayerParts);
    TestEqual(TEXT("All body and held-tool primitives are checked"), PlayerParts.Num(), 12);
    for (auto* Part : PlayerParts) CheckMesh(Part, false);
    for (const uint64 Seed : {uint64(6), uint64(1), uint64(3308806006119996599ull)})
    {
        auto* Creature = World->SpawnActor<AKalmalaWildlifeSpawn>();
        if (!TestNotNull(TEXT("Creature fixture exists"), Creature)) return false;
        FKalmalaWorldPopulationSpawn Spawn;
        Spawn.Kind = EKalmalaWorldPopulationKind::Wildlife;
        Spawn.SpawnSeed = Seed;
        Spawn.Location = FVector(1000, 0, 0);
        Creature->InitializeServer(Spawn);
        TArray<UProceduralMeshComponent*> CreatureParts;
        Creature->GetComponents(CreatureParts);
        TestEqual(TEXT("Creature archetype creates its mesh"), CreatureParts.Num(), 1);
        for (auto* Part : CreatureParts) CheckMesh(Part, true);
    }
    auto* MoonBear = World->SpawnActor<AKalmalaWildlifeSpawn>();
    if (!TestNotNull(TEXT("Moon Bear presentation fixture exists"), MoonBear)) return false;
    FKalmalaWorldPopulationSpawn MoonBearSpawn;
    MoonBearSpawn.Kind = EKalmalaWorldPopulationKind::Wildlife;
    MoonBearSpawn.SpawnSeed = 9;
    MoonBearSpawn.Location = FVector(1000, 0, 0);
    MoonBear->InitializeServer(MoonBearSpawn, EKalmalaWildlifeArchetype::MoonBear);
    TArray<UProceduralMeshComponent*> MoonBearParts;
    MoonBear->GetComponents(MoonBearParts);
    TestEqual(TEXT("Moon Bear creates a procedural model"), MoonBearParts.Num(), 1);
    for (auto* Part : MoonBearParts) CheckMesh(Part, true);
    auto* Viper = World->SpawnActor<AKalmalaWildlifeSpawn>();
    if (!TestNotNull(TEXT("Viper presentation fixture exists"), Viper)) return false;
    FKalmalaWorldPopulationSpawn ViperSpawn;
    ViperSpawn.Kind = EKalmalaWorldPopulationKind::Wildlife;
    ViperSpawn.SpawnSeed = 5;
    ViperSpawn.Location = FVector(1000, 0, 0);
    Viper->InitializeServer(ViperSpawn, EKalmalaWildlifeArchetype::Viper);
    TArray<UProceduralMeshComponent*> ViperParts;
    Viper->GetComponents(ViperParts);
    TestEqual(TEXT("Viper creates its procedural model"), ViperParts.Num(), 1);
    for (auto* Part : ViperParts) CheckMesh(Part, true);
    auto* GiantSnake = World->SpawnActor<AKalmalaWildlifeSpawn>();
    if (!TestNotNull(TEXT("Giant Snake presentation fixture exists"), GiantSnake)) return false;
    FKalmalaWorldPopulationSpawn GiantSnakeSpawn;
    GiantSnakeSpawn.Kind = EKalmalaWorldPopulationKind::Wildlife;
    GiantSnakeSpawn.SpawnSeed = 9;
    GiantSnakeSpawn.Location = FVector(1000, 0, 0);
    GiantSnake->InitializeServer(GiantSnakeSpawn, EKalmalaWildlifeArchetype::GiantSnake);
    TArray<UProceduralMeshComponent*> GiantSnakeParts;
    GiantSnake->GetComponents(GiantSnakeParts);
    TestEqual(TEXT("Giant Snake creates its procedural model"), GiantSnakeParts.Num(), 1);
    for (auto* Part : GiantSnakeParts) CheckMesh(Part, true);
    auto* DeathAdder = World->SpawnActor<AKalmalaWildlifeSpawn>();
    if (!TestNotNull(TEXT("Death Adder presentation fixture exists"), DeathAdder)) return false;
    FKalmalaWorldPopulationSpawn DeathAdderSpawn;
    DeathAdderSpawn.Kind = EKalmalaWorldPopulationKind::Wildlife;
    DeathAdderSpawn.SpawnSeed = 5;
    DeathAdderSpawn.Location = FVector(1000, 0, 0);
    DeathAdder->InitializeServer(DeathAdderSpawn, EKalmalaWildlifeArchetype::DeathAdder);
    TArray<UProceduralMeshComponent*> DeathAdderParts;
    DeathAdder->GetComponents(DeathAdderParts);
    TestEqual(TEXT("Death Adder creates its procedural model"), DeathAdderParts.Num(), 1);
    for (auto* Part : DeathAdderParts) CheckMesh(Part, true);
    auto* PeatHusk = World->SpawnActor<AKalmalaWildlifeSpawn>();
    if (!TestNotNull(TEXT("Peat Husk presentation fixture exists"), PeatHusk)) return false;
    FKalmalaWorldPopulationSpawn PeatHuskSpawn;
    PeatHuskSpawn.Kind = EKalmalaWorldPopulationKind::Wildlife;
    PeatHuskSpawn.SpawnSeed = 9;
    PeatHuskSpawn.Location = FVector(1000, 0, 0);
    PeatHusk->InitializeServer(PeatHuskSpawn, EKalmalaWildlifeArchetype::PeatHusk);
    TArray<UProceduralMeshComponent*> PeatHuskParts;
    PeatHusk->GetComponents(PeatHuskParts);
    TestEqual(TEXT("Peat Husk creates its procedural model"), PeatHuskParts.Num(), 1);
    for (auto* Part : PeatHuskParts) CheckMesh(Part, true);
    auto* Wolf = World->SpawnActor<AKalmalaWildlifeSpawn>();
    if (!TestNotNull(TEXT("Wolf presentation fixture exists"), Wolf)) return false;
    FKalmalaWorldPopulationSpawn WolfSpawn;
    WolfSpawn.Kind = EKalmalaWorldPopulationKind::Wildlife;
    WolfSpawn.SpawnSeed = 5;
    WolfSpawn.Location = FVector(1000, 0, 0);
    Wolf->InitializeServer(WolfSpawn, EKalmalaWildlifeArchetype::Wolf, 0);
    TArray<UProceduralMeshComponent*> WolfParts;
    Wolf->GetComponents(WolfParts);
    TestEqual(TEXT("Wolf creates its original procedural model"), WolfParts.Num(), 1);
    for (auto* Part : WolfParts) CheckMesh(Part, true);
    return true;
}

#endif
