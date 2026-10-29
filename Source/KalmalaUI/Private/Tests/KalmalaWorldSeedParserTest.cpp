#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaWorldSeedParser.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWorldSeedParserTest,
    "Kalmala.UI.WorldSeedParser", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaWorldSeedParserTest::RunTest(const FString& Parameters)
{
    uint64 Seed = 27;
    TestTrue(TEXT("Zero is a supported seed"),
        FKalmalaWorldSeedParser::Parse(TEXT("0"), Seed) == EKalmalaWorldSeedParseResult::Success && Seed == 0);
    TestTrue(TEXT("Leading zeroes remain an exact numeric seed"),
        FKalmalaWorldSeedParser::Parse(TEXT("00042"), Seed) == EKalmalaWorldSeedParseResult::Success && Seed == 42);
    TestTrue(TEXT("The maximum uint64 seed is accepted"),
        FKalmalaWorldSeedParser::Parse(TEXT("18446744073709551615"), Seed) == EKalmalaWorldSeedParseResult::Success
            && Seed == MAX_uint64);

    Seed = 27;
    TestTrue(TEXT("Empty input is reported and clears the output"),
        FKalmalaWorldSeedParser::Parse(TEXT(""), Seed) == EKalmalaWorldSeedParseResult::Empty && Seed == 0);
    Seed = 27;
    TestTrue(TEXT("Overflow is rejected without wrapping"),
        FKalmalaWorldSeedParser::Parse(TEXT("18446744073709551616"), Seed) == EKalmalaWorldSeedParseResult::OutOfRange
            && Seed == 0);
    Seed = 27;
    TestTrue(TEXT("A trailing suffix is rejected"),
        FKalmalaWorldSeedParser::Parse(TEXT("42world"), Seed) == EKalmalaWorldSeedParseResult::InvalidCharacter
            && Seed == 0);
    Seed = 27;
    TestTrue(TEXT("Signs and whitespace are rejected"),
        FKalmalaWorldSeedParser::Parse(TEXT("+42"), Seed) == EKalmalaWorldSeedParseResult::InvalidCharacter
            && Seed == 0
            && FKalmalaWorldSeedParser::Parse(TEXT(" 42"), Seed) == EKalmalaWorldSeedParseResult::InvalidCharacter
            && Seed == 0);
    return true;
}

#endif
