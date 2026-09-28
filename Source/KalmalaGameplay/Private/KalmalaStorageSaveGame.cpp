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
        if (Seen.Contains(Stack.ItemId) || !UKalmalaItemCatalogue::Get()->IsValidStack(Stack.ItemId, Stack.Quantity)) return false;
        Seen.Add(Stack.ItemId);
    }
    return true;
}

bool UKalmalaStorageSaveGame::NormalizeLegacyStacks(TArray<FKalmalaInventoryStack>& Stacks)
{
    if (Stacks.Num() > UKalmalaInventoryComponent::MaxSlots) return false;
    const UKalmalaItemCatalogue* Items = UKalmalaItemCatalogue::Get();
    TArray<FName> ItemOrder;
    TMap<FName, int64> Quantities;
    const auto AddQuantity = [&ItemOrder, &Quantities](const FName ItemId, const int64 Quantity)
    {
        if (Quantity <= 0) return false;
        if (!Quantities.Contains(ItemId)) ItemOrder.Add(ItemId);
        Quantities.FindOrAdd(ItemId) += Quantity;
        return true;
    };
    for (const FKalmalaInventoryStack& Stack : Stacks)
    {
        if (Stack.Quantity <= 0 || Stack.ItemId.IsNone()) return false;
        if (Stack.ItemId == TEXT("Fuel"))
        {
            if (!AddQuantity(TEXT("Wood"), Stack.Quantity)) return false;
        }
        else if (Stack.ItemId == TEXT("ConstructionSupply"))
        {
            const int64 SupplyCount = Stack.Quantity;
            if (!AddQuantity(TEXT("Wood"), SupplyCount * 3) || !AddQuantity(TEXT("Fibre"), SupplyCount * 2)) return false;
        }
        else if (!Items->FindItem(Stack.ItemId) || !AddQuantity(Stack.ItemId, Stack.Quantity)) return false;
    }

    TArray<FKalmalaInventoryStack> Normalized;
    for (const FName ItemId : ItemOrder)
    {
        const FKalmalaItemDefinition* Definition = Items->FindItem(ItemId);
        if (!Definition) return false;
        int64 Remaining = Quantities.FindRef(ItemId);
        while (Remaining > 0)
        {
            if (Normalized.Num() >= UKalmalaInventoryComponent::MaxSlots) return false;
            FKalmalaInventoryStack& Stack = Normalized.AddDefaulted_GetRef();
            Stack.ItemId = ItemId;
            Stack.Quantity = int32(FMath::Min<int64>(Remaining, Definition->MaxStack));
            Remaining -= Stack.Quantity;
        }
    }
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
    return FString::Printf(TEXT("KalmalaStorage_%llu"), Config.WorldSeed);
}
