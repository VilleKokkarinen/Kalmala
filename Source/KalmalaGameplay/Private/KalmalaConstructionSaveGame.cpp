#include "KalmalaConstructionSaveGame.h"
#include "KalmalaPlacementPreview.h"

void UKalmalaConstructionSaveGame::InitializeForWorld(const FKalmalaWorldGenerationConfig& InWorld)
{
    SchemaVersion = CurrentSchemaVersion;
    WorldConfig = InWorld;
    Records.Reset();
}

bool UKalmalaConstructionSaveGame::MatchesWorld(const FKalmalaWorldGenerationConfig& InWorld) const
{
    return SchemaVersion == CurrentSchemaVersion && WorldConfig == InWorld;
}

bool UKalmalaConstructionSaveGame::IsValidRecord(const FKalmalaConstructionSaveRecord& Record)
{
    return Record.ConstructionId.Len() > 0 && Record.ConstructionId.Len() <= 64 && FKalmalaPlacementPreview::IsSupportedKit(Record.KitId)
        && !Record.Transform.GetLocation().ContainsNaN() && !Record.Transform.GetRotation().ContainsNaN();
}

bool UKalmalaConstructionSaveGame::AddRecord(const FKalmalaConstructionSaveRecord& Record)
{
    if (!IsValidRecord(Record) || Records.Num() >= MaxRecords
        || Records.ContainsByPredicate([&Record](const auto& Existing) { return Existing.ConstructionId == Record.ConstructionId; })) return false;
    Records.Add(Record);
    return true;
}

bool UKalmalaConstructionSaveGame::RemoveRecord(const FString& ConstructionId)
{
    return !ConstructionId.IsEmpty() && Records.RemoveAll([&ConstructionId](const auto& Record) { return Record.ConstructionId == ConstructionId; }) == 1;
}
