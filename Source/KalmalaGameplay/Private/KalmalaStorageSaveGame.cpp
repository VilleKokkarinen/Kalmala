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
    if (Stacks.Num() > MaxStorageSlots) return false;
    TSet<FGuid> SeenStackGuids;
    TSet<FGuid> SeenItemGuids;
    for (const auto& Stack : Stacks)
    {
        if (!UKalmalaItemCatalogue::Get()->IsValidStack(Stack.ItemId, Stack.Quantity)
            || !Stack.InstanceGuid.IsValid() || SeenStackGuids.Contains(Stack.InstanceGuid)
            || Stack.ItemGuids.Num() != Stack.Quantity) return false;
        SeenStackGuids.Add(Stack.InstanceGuid);
        for (const FGuid& ItemGuid : Stack.ItemGuids)
        {
            if (!ItemGuid.IsValid() || SeenItemGuids.Contains(ItemGuid)) return false;
            SeenItemGuids.Add(ItemGuid);
        }
    }
    return true;
}

bool UKalmalaStorageSaveGame::MatchesWorld(const FKalmalaWorldGenerationConfig& Config) const
{
    if (!Config.IsValid() || SchemaVersion != CurrentSchemaVersion || WorldConfig != Config || Records.Num() > MaxRecords) return false;
    TSet<FString> Seen;
    TSet<FGuid> SeenItemGuids;
    for (const auto& Record : Records)
    {
        if (!IsValidConstructionId(Record.ConstructionId) || Seen.Contains(Record.ConstructionId) || !IsValidStacks(Record.Stacks)) return false;
        Seen.Add(Record.ConstructionId);
        for (const FKalmalaInventoryStack& Stack : Record.Stacks)
            for (const FGuid& ItemGuid : Stack.ItemGuids)
            {
                if (SeenItemGuids.Contains(ItemGuid)) return false;
                SeenItemGuids.Add(ItemGuid);
            }
    }
    return true;
}

const FKalmalaStorageSaveRecord* UKalmalaStorageSaveGame::FindRecord(const FString& Id) const
{
    return Records.FindByPredicate([&](const auto& Record) { return Record.ConstructionId == Id; });
}

bool UKalmalaStorageSaveGame::UpsertRecord(const FString& Id, const TArray<FKalmalaInventoryStack>& Stacks)
{
    if (!MatchesWorld(WorldConfig) || !IsValidConstructionId(Id) || Stacks.Num() > MaxStorageSlots) return false;
    TSet<FGuid> SeenItemGuids;
    for (const FKalmalaStorageSaveRecord& Existing : Records)
    {
        if (Existing.ConstructionId == Id) continue;
        for (const FKalmalaInventoryStack& Stack : Existing.Stacks)
            for (const FGuid& ItemGuid : Stack.ItemGuids) SeenItemGuids.Add(ItemGuid);
    }
    TArray<FKalmalaInventoryStack> Candidate = Stacks;
    TSet<FGuid> SeenStackGuids;
    for (FKalmalaInventoryStack& Stack : Candidate)
    {
        if (!UKalmalaItemCatalogue::Get()->IsValidStack(Stack.ItemId, Stack.Quantity)) return false;
        if (!Stack.InstanceGuid.IsValid())
        {
            do { Stack.InstanceGuid = FGuid::NewGuid(); }
            while (!Stack.InstanceGuid.IsValid() || SeenStackGuids.Contains(Stack.InstanceGuid));
        }
        if (SeenStackGuids.Contains(Stack.InstanceGuid) || Stack.ItemGuids.Num() > Stack.Quantity) return false;
        SeenStackGuids.Add(Stack.InstanceGuid);
        for (FGuid& ItemGuid : Stack.ItemGuids)
        {
            if (!ItemGuid.IsValid())
            {
                do { ItemGuid = FGuid::NewGuid(); }
                while (!ItemGuid.IsValid() || SeenItemGuids.Contains(ItemGuid));
            }
            if (SeenItemGuids.Contains(ItemGuid)) return false;
            SeenItemGuids.Add(ItemGuid);
        }
        while (Stack.ItemGuids.Num() < Stack.Quantity)
        {
            FGuid& ItemGuid = Stack.ItemGuids.AddDefaulted_GetRef();
            do { ItemGuid = FGuid::NewGuid(); }
            while (!ItemGuid.IsValid() || SeenItemGuids.Contains(ItemGuid));
            SeenItemGuids.Add(ItemGuid);
        }
    }
    if (!IsValidStacks(Candidate)) return false;
    auto* Record = Records.FindByPredicate([&](const auto& Entry) { return Entry.ConstructionId == Id; });
    if (!Record)
    {
        if (Records.Num() >= MaxRecords) return false;
        Record = &Records.AddDefaulted_GetRef(); Record->ConstructionId = Id;
    }
    Record->Stacks = MoveTemp(Candidate);
    return true;
}

FString UKalmalaStorageSaveGame::MakeSlotName(const FKalmalaWorldGenerationConfig& Config)
{
    return FString::Printf(TEXT("KalmalaStorage_%llu"), Config.WorldSeed);
}
