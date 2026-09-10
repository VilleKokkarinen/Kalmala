#include "KalmalaStorageSaveGame.h"
#include "KalmalaItemCatalogue.h"

void UKalmalaStorageSaveGame::InitializeForWorld(const FKalmalaWorldGenerationConfig& Config)
{
    SchemaVersion = CurrentSchemaVersion; WorldConfig = Config; Records.Reset();
}

bool UKalmalaStorageSaveGame::IsValidConstructionId(const FString& Id)
{
    if (Id.IsEmpty() || Id.Len() > 64) return false;
    for (TCHAR C : Id)
        if (!((C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z') || (C >= '0' && C <= '9') || C == '-' || C == '_')) return false;
    return true;
}

bool UKalmalaStorageSaveGame::IsValidStacks(const TArray<FKalmalaInventoryStack>& Stacks)
{
    if (Stacks.Num() > UKalmalaInventoryComponent::MaxSlots) return false;
    TSet<FName> Seen;
    for (const auto& Stack : Stacks)
    {
        if (Seen.Contains(Stack.ItemId) || !GetDefault<UKalmalaItemCatalogue>()->IsValidStack(Stack.ItemId, Stack.Quantity)) return false;
        Seen.Add(Stack.ItemId);
    }
    return true;
}

bool UKalmalaStorageSaveGame::MatchesWorld(const FKalmalaWorldGenerationConfig& Config) const
{
    if (!Config.IsValid() || SchemaVersion != CurrentSchemaVersion || WorldConfig != Config || Records.Num() > MaxRecords) return false;
    TSet<FString> Seen;
    for (const auto& Record : Records)
    {
        if (!IsValidConstructionId(Record.ConstructionId) || Seen.Contains(Record.ConstructionId) || !IsValidStacks(Record.Stacks)) return false;
        Seen.Add(Record.ConstructionId);
    }
    return true;
}

const FKalmalaStorageSaveRecord* UKalmalaStorageSaveGame::FindRecord(const FString& Id) const
{
    return Records.FindByPredicate([&](const auto& Record) { return Record.ConstructionId == Id; });
}

bool UKalmalaStorageSaveGame::UpsertRecord(const FString& Id, const TArray<FKalmalaInventoryStack>& Stacks)
{
    if (!MatchesWorld(WorldConfig) || !IsValidConstructionId(Id) || !IsValidStacks(Stacks)) return false;
    auto* Record = Records.FindByPredicate([&](const auto& Entry) { return Entry.ConstructionId == Id; });
    if (!Record)
    {
        if (Records.Num() >= MaxRecords) return false;
        Record = &Records.AddDefaulted_GetRef(); Record->ConstructionId = Id;
    }
    Record->Stacks = Stacks;
    return true;
}

FString UKalmalaStorageSaveGame::MakeSlotName(const FKalmalaWorldGenerationConfig& Config)
{
    return FString::Printf(TEXT("KalmalaStorage_%llu_%d"), Config.WorldSeed, Config.GeneratorRevision);
}
