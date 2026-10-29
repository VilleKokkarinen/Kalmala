#pragma once

#include "CoreMinimal.h"

enum class EKalmalaWorldSeedParseResult : uint8
{
    Success,
    Empty,
    InvalidCharacter,
    OutOfRange
};

/** Parses decimal seed input without accepting signs, suffixes, or integer wraparound. */
class KALMALAUI_API FKalmalaWorldSeedParser
{
public:
    static EKalmalaWorldSeedParseResult Parse(const FString& Input, uint64& OutSeed);
};
