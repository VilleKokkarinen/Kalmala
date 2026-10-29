#include "KalmalaWorldSaveInspector.h"

#include "KalmalaConstructionSaveGame.h"
#include "KalmalaM7PersistenceContract.h"
#include "KalmalaOceanTravelPersistenceContract.h"
#include "KalmalaStorageSaveGame.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/StrongObjectPtr.h"

FKalmalaWorldSaveInspection FKalmalaWorldSaveInspector::Evaluate(const uint64 Seed, const FKalmalaWorldSaveInputs& Inputs)
{
    FKalmalaWorldSaveInspection Result;
    Result.Seed = Seed;
    FKalmalaWorldGenerationConfig Config;
    Config.WorldSeed = Seed;
    const auto* Population = Cast<UKalmalaWorldPopulationSaveGame>(Inputs.Population);
    const auto* Construction = Cast<UKalmalaConstructionSaveGameV2>(Inputs.Construction);
    const auto* Storage = Cast<UKalmalaStorageSaveGame>(Inputs.Storage);
    const auto* Ocean = Cast<UKalmalaOceanTravelPersistenceSaveGame>(Inputs.Ocean);
    const auto Status = [](bool bExists, bool bAccepted)
    {
        return !bExists ? EKalmalaWorldSaveComponentStatus::Absent
            : bAccepted ? EKalmalaWorldSaveComponentStatus::Accepted : EKalmalaWorldSaveComponentStatus::Rejected;
    };
    Result.Population = Status(Inputs.bPopulationExists, Inputs.bPopulationExists && Population && Population->MatchesWorld(Config));
    Result.Construction = Status(Inputs.bConstructionExists, Inputs.bConstructionExists && Construction && Construction->MatchesWorld(Config));
    Result.Storage = Status(Inputs.bStorageExists, Inputs.bStorageExists && Storage && Storage->MatchesWorld(Config));
    const FKalmalaM7SaveIdentity Identity = FKalmalaM7SaveIdentity::ForWorld(Seed);
    Result.Ocean = Status(Inputs.bOceanExists, Inputs.bOceanExists && Ocean && Ocean->Matches(Identity)
        && Ocean->HasVesselState() && Ocean->GetVesselState().IsValid());
    return Result;
}

FKalmalaWorldSaveInspection FKalmalaWorldSaveInspector::Inspect(const uint64 Seed)
{
    FKalmalaWorldGenerationConfig Config;
    Config.WorldSeed = Seed;
    const FKalmalaM7SaveIdentity Identity = FKalmalaM7SaveIdentity::ForWorld(Seed);
    // These are the same fixed, numeric identities used by the current server save owners.
    const FString PopulationSlot = FString::Printf(TEXT("KalmalaPopulationDeltas_%llu"), Seed);
    const FString ConstructionSlot = FString::Printf(TEXT("KalmalaConstruction_%llu"), Seed);
    const FString StorageSlot = UKalmalaStorageSaveGame::MakeSlotName(Config);
    const FString OceanSlot = FString::Printf(TEXT("KalmalaOceanTravelWorld_%llu_%d"), Seed, Identity.GeneratorRevision);
    FKalmalaWorldSaveInputs Inputs;
    const auto Read = [](const FString& Slot, bool& bExists)
    {
        bExists = UGameplayStatics::DoesSaveGameExist(Slot, 0);
        return bExists ? UGameplayStatics::LoadGameFromSlot(Slot, 0) : nullptr;
    };
    TStrongObjectPtr<USaveGame> Population(Read(PopulationSlot, Inputs.bPopulationExists));
    TStrongObjectPtr<USaveGame> Construction(Read(ConstructionSlot, Inputs.bConstructionExists));
    TStrongObjectPtr<USaveGame> Storage(Read(StorageSlot, Inputs.bStorageExists));
    TStrongObjectPtr<USaveGame> Ocean(Read(OceanSlot, Inputs.bOceanExists));
    Inputs.Population = Population.Get();
    Inputs.Construction = Construction.Get();
    Inputs.Storage = Storage.Get();
    Inputs.Ocean = Ocean.Get();
    return Evaluate(Seed, Inputs);
}
