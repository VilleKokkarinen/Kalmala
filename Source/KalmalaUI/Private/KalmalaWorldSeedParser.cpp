#include "KalmalaWorldSeedParser.h"

EKalmalaWorldSeedParseResult FKalmalaWorldSeedParser::Parse(const FString& Input, uint64& OutSeed)
{
    OutSeed = 0;
    if (Input.IsEmpty()) return EKalmalaWorldSeedParseResult::Empty;

    uint64 ParsedSeed = 0;
    for (int32 Index = 0; Index < Input.Len(); ++Index)
    {
        const TCHAR Character = Input[Index];
        if (Character < TEXT('0') || Character > TEXT('9'))
            return EKalmalaWorldSeedParseResult::InvalidCharacter;

        const uint64 Digit = static_cast<uint64>(Character - TEXT('0'));
        if (ParsedSeed > (MAX_uint64 - Digit) / 10ull)
            return EKalmalaWorldSeedParseResult::OutOfRange;

        ParsedSeed = ParsedSeed * 10ull + Digit;
    }

    OutSeed = ParsedSeed;
    return EKalmalaWorldSeedParseResult::Success;
}
