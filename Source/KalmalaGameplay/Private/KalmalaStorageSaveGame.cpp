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
    TSet<FGuid> SeenGuids;
    for (const auto& Stack : Stacks)
    {
        if (!UKalmalaItemCatalogue::Get()->IsValidStack(Stack.ItemId, Stack.Quantity)
            || !Stack.InstanceGuid.IsValid() || SeenGuids.Contains(Stack.InstanceGuid)) return false;
        SeenGuids.Add(Stack.InstanceGuid);
    }
    return true;
}

bool UKalmalaStorageSaveGame::NormalizeLegacyStacks(TArray<FKalmalaInventoryStack>& Stacks)
{
    if (Stacks.Num() > MaxStorageSlots) return false;
    const UKalmalaItemCatalogue* Items = UKalmalaItemCatalogue::Get();
    TArray<FName> LegacyOrder;
    TMap<FName, int64> LegacyQuantities;
    TArray<FKalmalaInventoryStack> Normalized;
    TSet<FGuid> SeenGuids;
    const auto AddNormalized = [&Items, &Normalized, &SeenGuids](const FName ItemId, int64 Quantity, const FGuid PreferredGuid)
    {
        const FKalmalaItemDefinition* Definition = Items->FindItem(ItemId);
        if (!Definition || Definition->MaxStack <= 0 || Quantity <= 0) return false;
        bool bUsePreferredGuid = PreferredGuid.IsValid() && !SeenGuids.Contains(PreferredGuid);
        while (Quantity > 0)
        {
            if (Normalized.Num() >= MaxStorageSlots) return false;
            FKalmalaInventoryStack& Stack = Normalized.AddDefaulted_GetRef();
            Stack.ItemId = ItemId;
            Stack.Quantity = int32(FMath::Min<int64>(Quantity, Definition->MaxStack));
            Quantity -= Stack.Quantity;
            if (bUsePreferredGuid)
            {
                Stack.InstanceGuid = PreferredGuid;
                bUsePreferredGuid = false;
            }
            else
            {
                do { Stack.InstanceGuid = FGuid::NewGuid(); }
                while (!Stack.InstanceGuid.IsValid() || SeenGuids.Contains(Stack.InstanceGuid));
            }
            SeenGuids.Add(Stack.InstanceGuid);
        }
        return true;
    };
    const auto AddMaterial = [&LegacyOrder, &LegacyQuantities, &AddNormalized](const FName ItemId,
        const int64 Quantity, const FGuid PreferredGuid)
    {
        if (Quantity <= 0) return false;
        if (PreferredGuid.IsValid()) return AddNormalized(ItemId, Quantity, PreferredGuid);
        if (!LegacyQuantities.Contains(ItemId)) LegacyOrder.Add(ItemId);
        LegacyQuantities.FindOrAdd(ItemId) += Quantity;
        return true;
    };
    for (const FKalmalaInventoryStack& Stack : Stacks)
    {
        if (Stack.Quantity <= 0 || Stack.ItemId.IsNone()) return false;
        if (Stack.ItemId == TEXT("Fuel"))
        {
            if (!AddMaterial(TEXT("Wood"), Stack.Quantity, Stack.InstanceGuid)) return false;
        }
        else if (Stack.ItemId == TEXT("ConstructionSupply"))
        {
            const int64 SupplyCount = Stack.Quantity;
            if (!AddMaterial(TEXT("Wood"), SupplyCount * 3, Stack.InstanceGuid)
                || !AddMaterial(TEXT("Fibre"), SupplyCount * 2, FGuid())) return false;
        }
        else if (Stack.ItemId == TEXT("PeatAmber"))
        {
            if (!AddMaterial(TEXT("Amber"), Stack.Quantity, Stack.InstanceGuid)) return false;
        }
        else if (Stack.ItemId == TEXT("MirelingAsh"))
        {
            // Retired loot is discarded while preserving the rest of an existing chest save.
        }
        else if (!Items->FindItem(Stack.ItemId)
            || !AddMaterial(Stack.ItemId, Stack.Quantity, Stack.InstanceGuid)) return false;
    }
    for (const FName ItemId : LegacyOrder)
        if (!AddNormalized(ItemId, LegacyQuantities.FindRef(ItemId), FGuid())) return false;
    Stacks = MoveTemp(Normalized);
    return IsValidStacks(Stacks);
}

bool UKalmalaStorageSaveGame::MigrateLegacyItemIds()
{
    TArray<FKalmalaStorageSaveRecord> MigratedRecords = Records;
    for (FKalmalaStorageSaveRecord& Record : MigratedRecords)
        if (!NormalizeLegacyStacks(Record.Stacks)) return false;
    Records = MoveTemp(MigratedRecords);
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
    if (!MatchesWorld(WorldConfig) || !IsValidConstructionId(Id) || Stacks.Num() > MaxStorageSlots) return false;
    TArray<FKalmalaInventoryStack> Candidate = Stacks;
    TSet<FGuid> SeenGuids;
    for (FKalmalaInventoryStack& Stack : Candidate)
    {
        if (!UKalmalaItemCatalogue::Get()->IsValidStack(Stack.ItemId, Stack.Quantity)) return false;
        if (!Stack.InstanceGuid.IsValid())
        {
            do { Stack.InstanceGuid = FGuid::NewGuid(); }
            while (!Stack.InstanceGuid.IsValid() || SeenGuids.Contains(Stack.InstanceGuid));
        }
        if (SeenGuids.Contains(Stack.InstanceGuid)) return false;
        SeenGuids.Add(Stack.InstanceGuid);
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
