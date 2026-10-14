#include "KalmalaStorageSaveGame.h"
#include "KalmalaItemCatalogue.h"
#include "Kismet/GameplayStatics.h"

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

bool UKalmalaStorageSaveGame::NormalizeLegacyStacks(TArray<FKalmalaInventoryStack>& Stacks)
{
    if (Stacks.Num() > MaxStorageSlots) return false;
    const UKalmalaItemCatalogue* Items = UKalmalaItemCatalogue::Get();
    TArray<FName> LegacyOrder;
    TMap<FName, int64> LegacyQuantities;
    TArray<FKalmalaInventoryStack> Normalized;
    TSet<FGuid> SeenStackGuids;
    TSet<FGuid> SeenItemGuids;
    const auto AddNormalized = [&Items, &Normalized, &SeenStackGuids, &SeenItemGuids](const FName ItemId,
        int64 Quantity, const FGuid PreferredGuid, const TArray<FGuid>& PreferredItemGuids)
    {
        const FKalmalaItemDefinition* Definition = Items->FindItem(ItemId);
        if (!Definition || Definition->MaxStack <= 0 || Quantity <= 0 || PreferredItemGuids.Num() > Quantity) return false;
        bool bUsePreferredGuid = PreferredGuid.IsValid() && !SeenStackGuids.Contains(PreferredGuid);
        int32 PreferredItemIndex = 0;
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
                SeenStackGuids.Add(Stack.InstanceGuid);
            }
            else
            {
                do { Stack.InstanceGuid = FGuid::NewGuid(); }
                while (!Stack.InstanceGuid.IsValid() || SeenStackGuids.Contains(Stack.InstanceGuid));
                SeenStackGuids.Add(Stack.InstanceGuid);
            }
            for (int32 Unit = 0; Unit < Stack.Quantity; ++Unit)
            {
                FGuid& ItemGuid = Stack.ItemGuids.AddDefaulted_GetRef();
                if (PreferredItemGuids.IsValidIndex(PreferredItemIndex))
                {
                    ItemGuid = PreferredItemGuids[PreferredItemIndex++];
                    if (!ItemGuid.IsValid() || SeenItemGuids.Contains(ItemGuid)) return false;
                    SeenItemGuids.Add(ItemGuid);
                }
                else
                {
                    do { ItemGuid = FGuid::NewGuid(); }
                    while (!ItemGuid.IsValid() || SeenItemGuids.Contains(ItemGuid));
                    SeenItemGuids.Add(ItemGuid);
                }
            }
        }
        return true;
    };
    TMap<FName, TArray<FGuid>> LegacyItemGuids;
    const auto AddMaterial = [&LegacyOrder, &LegacyQuantities, &LegacyItemGuids, &AddNormalized](const FName ItemId,
        const int64 Quantity, const FGuid PreferredGuid, const TArray<FGuid>& PreferredItemGuids)
    {
        if (Quantity <= 0) return false;
        if (PreferredGuid.IsValid()) return AddNormalized(ItemId, Quantity, PreferredGuid, PreferredItemGuids);
        if (!LegacyQuantities.Contains(ItemId)) LegacyOrder.Add(ItemId);
        LegacyQuantities.FindOrAdd(ItemId) += Quantity;
        LegacyItemGuids.FindOrAdd(ItemId).Append(PreferredItemGuids);
        return true;
    };
    for (const FKalmalaInventoryStack& Stack : Stacks)
    {
        if (Stack.Quantity <= 0 || Stack.ItemId.IsNone()) return false;
        if (Stack.ItemId == TEXT("Fuel"))
        {
            if (!AddMaterial(TEXT("Wood"), Stack.Quantity, Stack.InstanceGuid, Stack.ItemGuids)) return false;
        }
        else if (Stack.ItemId == TEXT("ConstructionSupply"))
        {
            const int64 SupplyCount = Stack.Quantity;
            if (!AddMaterial(TEXT("Wood"), SupplyCount * 3, Stack.InstanceGuid, {})
                || !AddMaterial(TEXT("Fibre"), SupplyCount * 2, FGuid(), {})) return false;
        }
        else if (Stack.ItemId == TEXT("PeatAmber"))
        {
            if (!AddMaterial(TEXT("Amber"), Stack.Quantity, Stack.InstanceGuid, Stack.ItemGuids)) return false;
        }
        else if (Stack.ItemId == TEXT("MirelingAsh"))
        {
            // Retired loot is discarded while preserving the rest of an existing chest save.
        }
        else if (!Items->FindItem(Stack.ItemId)
            || !AddMaterial(Stack.ItemId, Stack.Quantity, Stack.InstanceGuid, Stack.ItemGuids)) return false;
    }
    for (const FName ItemId : LegacyOrder)
        if (!AddNormalized(ItemId, LegacyQuantities.FindRef(ItemId), FGuid(), LegacyItemGuids.FindRef(ItemId))) return false;
    Stacks = MoveTemp(Normalized);
    return IsValidStacks(Stacks);
}

bool UKalmalaStorageSaveGame::MigrateLegacyItemIds()
{
    if (SchemaVersion != CurrentSchemaVersion && SchemaVersion != 1) return false;
    TArray<FKalmalaStorageSaveRecord> MigratedRecords = Records;
    bool bChanged = SchemaVersion != CurrentSchemaVersion;
    for (FKalmalaStorageSaveRecord& Record : MigratedRecords)
    {
        const TArray<FKalmalaInventoryStack> Before = Record.Stacks;
        if (!NormalizeLegacyStacks(Record.Stacks)) return false;
        if (Before.Num() != Record.Stacks.Num()) bChanged = true;
        else for (int32 Index = 0; Index < Before.Num(); ++Index)
        {
            if (Before[Index].ItemId != Record.Stacks[Index].ItemId
                || Before[Index].Quantity != Record.Stacks[Index].Quantity
                || Before[Index].InstanceGuid != Record.Stacks[Index].InstanceGuid
                || Before[Index].ItemGuids != Record.Stacks[Index].ItemGuids)
            {
                bChanged = true;
                break;
            }
        }
    }
    Records = MoveTemp(MigratedRecords);
    SchemaVersion = CurrentSchemaVersion;
    // Persist generated unit identities immediately; otherwise legacy stacks would receive new IDs on every launch.
    if (!MatchesWorld(WorldConfig)) return false;
    if (bChanged && !UGameplayStatics::SaveGameToSlot(this, MakeSlotName(WorldConfig), 0)) return false;
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
    TSet<FGuid> SeenItemGuids;
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
