#pragma once

#include "CoreMinimal.h"

enum class EKalmalaServerAddressParseResult : uint8
{
    Success,
    Empty,
    TooLong,
    UnsupportedAddress,
    InvalidCharacter,
    InvalidHost,
    InvalidPort,
    PortOutOfRange,
    DefaultPortUnavailable
};

struct FKalmalaServerEndpoint
{
    FString Host;
    int32 Port = 0;

    FString ToTravelAddress() const;
};

/** Parses only a bare IPv4 address or DNS hostname with an optional decimal port. */
class KALMALAUI_API FKalmalaServerAddressParser
{
public:
    static EKalmalaServerAddressParseResult Parse(const FString& Input, int32 DefaultPort,
        FKalmalaServerEndpoint& OutEndpoint);
    static const TCHAR* ErrorMessage(EKalmalaServerAddressParseResult Result);
};
