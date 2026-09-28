#pragma once

#include "CoreMinimal.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"

/** Server-owned selection and transaction cost for burning unprocessed materials. */
struct KALMALAGAMEPLAY_API FKalmalaRawFuelContract
{
    static const TArray<FName>& GetFuelItemIds()
    {
        static const TArray<FName> FuelItemIds = {
            TEXT("Wood"), TEXT("Lightwood"), TEXT("Densewood"), TEXT("Coal")
        };
        return FuelItemIds;
    }

    /**
     * Append the requested raw fuel units to an exchange cost. A single batch may
     * consume a mix of fuels; the server always chooses from its authoritative pack.
     */
    static bool AddCosts(const TArray<FKalmalaInventoryStack>& Available,
        const int32 Units, TArray<FKalmalaInventoryStack>& Costs, FString& Reason)
    {
        Reason = TEXT("Need raw fuel: Wood, Lightwood, Densewood, or Coal");
        if (Units < 1 || Units > 10) return false;

        int32 Remaining = Units;
        TArray<FKalmalaInventoryStack> CandidateCosts = Costs;
        for (const FName FuelItemId : GetFuelItemIds())
        {
            const FKalmalaInventoryStack* Stack = Available.FindByPredicate(
                [FuelItemId](const FKalmalaInventoryStack& Candidate) { return Candidate.ItemId == FuelItemId; });
            FKalmalaInventoryStack* ExistingCost = CandidateCosts.FindByPredicate(
                [FuelItemId](const FKalmalaInventoryStack& Candidate) { return Candidate.ItemId == FuelItemId; });
            const int32 AlreadyCosted = ExistingCost ? ExistingCost->Quantity : 0;
            const int32 AvailableQuantity = Stack ? Stack->Quantity - AlreadyCosted : 0;
            if (AvailableQuantity <= 0) continue;

            const int32 Taken = FMath::Min(Remaining, AvailableQuantity);
            ExistingCost = CandidateCosts.FindByPredicate(
                [FuelItemId](const FKalmalaInventoryStack& Candidate) { return Candidate.ItemId == FuelItemId; });
            if (ExistingCost)
            {
                if (ExistingCost->Quantity < 1
                    || Taken > UKalmalaItemCatalogue::AbsoluteMaxStack - ExistingCost->Quantity)
                {
                    Reason = TEXT("Raw fuel cost exceeds its bound");
                    return false;
                }
                ExistingCost->Quantity += Taken;
            }
            else
            {
                if (CandidateCosts.Num() >= UKalmalaInventoryComponent::MaxSlots)
                {
                    Reason = TEXT("Raw fuel cost exceeds its bound");
                    return false;
                }
                FKalmalaInventoryStack& FuelCost = CandidateCosts.AddDefaulted_GetRef();
                FuelCost.ItemId = FuelItemId;
                FuelCost.Quantity = Taken;
            }

            Remaining -= Taken;
            if (Remaining == 0)
            {
                Costs = MoveTemp(CandidateCosts);
                Reason = TEXT("Ready");
                return true;
            }
        }
        return false;
    }
};
