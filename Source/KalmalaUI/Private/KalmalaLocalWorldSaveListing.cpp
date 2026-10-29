#include "KalmalaLocalWorldSaveListing.h"

#include "PlatformFeatures.h"
#include "SaveGameSystem.h"

namespace
{
    bool TryParseUnsignedDecimal(const FString& Text, uint64& OutValue)
    {
        if (Text.IsEmpty() || (Text.Len() > 1 && Text[0] == TEXT('0'))) return false;

        uint64 Value = 0;
        for (const TCHAR Character : Text)
        {
            if (Character < TEXT('0') || Character > TEXT('9')) return false;
            const uint64 Digit = static_cast<uint64>(Character - TEXT('0'));
            if (Value > (MAX_uint64 - Digit) / 10) return false;
            Value = Value * 10 + Digit;
        }

        OutValue = Value;
        return true;
    }

    void MarkWorldSaveKind(FKalmalaLocalWorldSaveRecord& Record,
        const EKalmalaLocalWorldSaveKind Kind, const bool bHasGeneratorRevision, const int32 GeneratorRevision)
    {
        switch (Kind)
        {
        case EKalmalaLocalWorldSaveKind::Population:
            Record.bHasPopulation = true;
            break;
        case EKalmalaLocalWorldSaveKind::Construction:
            Record.bHasConstruction = true;
            break;
        case EKalmalaLocalWorldSaveKind::Storage:
            Record.bHasStorage = true;
            break;
        case EKalmalaLocalWorldSaveKind::OceanTravel:
            Record.bHasOceanTravel = true;
            if (Record.bHasConflictingGeneratorRevisions) break;
            if (!Record.bHasStoredGeneratorRevision)
            {
                Record.bHasStoredGeneratorRevision = true;
                Record.StoredGeneratorRevision = GeneratorRevision;
            }
            else if (Record.StoredGeneratorRevision != GeneratorRevision)
            {
                Record.bHasStoredGeneratorRevision = false;
                Record.bHasConflictingGeneratorRevisions = true;
            }
            break;
        }
    }
}

FString FKalmalaLocalWorldSaveRecord::GetDisplayLabel() const
{
    return FString::Printf(TEXT("World seed %llu"), Seed);
}

bool FKalmalaLocalWorldSaveListing::ParseSlotName(const FString& SlotName,
    EKalmalaLocalWorldSaveKind& OutKind, uint64& OutSeed,
    bool& bOutHasGeneratorRevision, int32& OutGeneratorRevision)
{
    OutKind = EKalmalaLocalWorldSaveKind::Population;
    OutSeed = 0;
    bOutHasGeneratorRevision = false;
    OutGeneratorRevision = 0;

    struct FSeedSlotPrefix
    {
        const TCHAR* Text;
        EKalmalaLocalWorldSaveKind Kind;
    };
    static constexpr FSeedSlotPrefix SeedPrefixes[] =
    {
        { TEXT("KalmalaPopulationDeltas_"), EKalmalaLocalWorldSaveKind::Population },
        { TEXT("KalmalaConstruction_"), EKalmalaLocalWorldSaveKind::Construction },
        { TEXT("KalmalaStorage_"), EKalmalaLocalWorldSaveKind::Storage }
    };

    for (const FSeedSlotPrefix& Prefix : SeedPrefixes)
    {
        const FString PrefixText(Prefix.Text);
        if (!SlotName.StartsWith(PrefixText, ESearchCase::CaseSensitive)) continue;

        uint64 ParsedSeed = 0;
        if (!TryParseUnsignedDecimal(SlotName.Mid(PrefixText.Len()), ParsedSeed)) return false;
        OutKind = Prefix.Kind;
        OutSeed = ParsedSeed;
        return true;
    }

    const FString OceanPrefix(TEXT("KalmalaOceanTravelWorld_"));
    if (!SlotName.StartsWith(OceanPrefix, ESearchCase::CaseSensitive)) return false;
    const FString Suffix = SlotName.Mid(OceanPrefix.Len());
    int32 SeparatorIndex = INDEX_NONE;
    if (!Suffix.FindChar(TEXT('_'), SeparatorIndex)) return false;

    const FString SeedText = Suffix.Left(SeparatorIndex);
    const FString RevisionText = Suffix.Mid(SeparatorIndex + 1);
    uint64 ParsedSeed = 0;
    uint64 ParsedRevision = 0;
    if (RevisionText.Contains(TEXT("_"))
        || !TryParseUnsignedDecimal(SeedText, ParsedSeed)
        || !TryParseUnsignedDecimal(RevisionText, ParsedRevision)
        || ParsedRevision > static_cast<uint64>(MAX_int32)) return false;

    OutKind = EKalmalaLocalWorldSaveKind::OceanTravel;
    OutSeed = ParsedSeed;
    bOutHasGeneratorRevision = true;
    OutGeneratorRevision = static_cast<int32>(ParsedRevision);
    return true;
}

EKalmalaLocalWorldSaveListingAvailability FKalmalaLocalWorldSaveListing::Enumerate(
    TArray<FKalmalaLocalWorldSaveRecord>& OutRecords)
{
    OutRecords.Reset();

    ISaveGameSystem* SaveSystem = IPlatformFeaturesModule::Get().GetSaveGameSystem();
    if (!SaveSystem) return EKalmalaLocalWorldSaveListingAvailability::Unsupported;

    TArray<FString> SaveNames;
    if (!SaveSystem->GetSaveGameNames(SaveNames, 0))
        return EKalmalaLocalWorldSaveListingAvailability::Unsupported;

    TMap<uint64, FKalmalaLocalWorldSaveRecord> RecordsBySeed;
    for (const FString& SaveName : SaveNames)
    {
        EKalmalaLocalWorldSaveKind Kind;
        uint64 Seed = 0;
        bool bHasGeneratorRevision = false;
        int32 GeneratorRevision = 0;
        if (!ParseSlotName(SaveName, Kind, Seed, bHasGeneratorRevision, GeneratorRevision)) continue;

        FKalmalaLocalWorldSaveRecord& Record = RecordsBySeed.FindOrAdd(Seed);
        Record.Seed = Seed;
        MarkWorldSaveKind(Record, Kind, bHasGeneratorRevision, GeneratorRevision);
    }

    OutRecords.Reserve(RecordsBySeed.Num());
    for (TPair<uint64, FKalmalaLocalWorldSaveRecord>& Pair : RecordsBySeed)
        OutRecords.Add(MoveTemp(Pair.Value));
    OutRecords.Sort([](const FKalmalaLocalWorldSaveRecord& Left, const FKalmalaLocalWorldSaveRecord& Right)
    {
        const int32 LabelOrder = Left.GetDisplayLabel().Compare(Right.GetDisplayLabel(), ESearchCase::CaseSensitive);
        return LabelOrder == 0 ? Left.Seed < Right.Seed : LabelOrder < 0;
    });

    return EKalmalaLocalWorldSaveListingAvailability::Supported;
}
