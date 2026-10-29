#include "KalmalaServerAddressParser.h"

namespace
{
    constexpr int32 MaximumAddressLength = 253;

    bool IsAsciiDigit(const TCHAR Character)
    {
        return Character >= TEXT('0') && Character <= TEXT('9');
    }

    bool IsAsciiAlphaNumeric(const TCHAR Character)
    {
        return IsAsciiDigit(Character)
            || (Character >= TEXT('a') && Character <= TEXT('z'))
            || (Character >= TEXT('A') && Character <= TEXT('Z'));
    }

    bool ParseIPv4(FString& Host)
    {
        FString CanonicalHost;
        int32 SegmentValue = 0;
        int32 SegmentDigits = 0;
        int32 SegmentCount = 0;

        for (int32 Index = 0; Index <= Host.Len(); ++Index)
        {
            if (Index == Host.Len() || Host[Index] == TEXT('.'))
            {
                if (SegmentDigits == 0 || SegmentDigits > 3 || SegmentValue > 255 || SegmentCount >= 4)
                    return false;
                if (SegmentCount > 0) CanonicalHost.AppendChar(TEXT('.'));
                CanonicalHost += FString::FromInt(SegmentValue);
                ++SegmentCount;
                SegmentValue = 0;
                SegmentDigits = 0;
                continue;
            }

            if (!IsAsciiDigit(Host[Index]) || SegmentDigits >= 3) return false;
            SegmentValue = SegmentValue * 10 + (Host[Index] - TEXT('0'));
            ++SegmentDigits;
            if (SegmentValue > 255) return false;
        }

        if (SegmentCount != 4) return false;
        Host = MoveTemp(CanonicalHost);
        return true;
    }

    bool IsValidDnsHostname(FString& Host)
    {
        if (Host.EndsWith(TEXT("."))) Host.LeftChopInline(1);
        if (Host.IsEmpty() || Host.Len() > MaximumAddressLength) return false;

        int32 LabelStart = 0;
        for (int32 Index = 0; Index <= Host.Len(); ++Index)
        {
            if (Index != Host.Len() && Host[Index] != TEXT('.')) continue;

            const int32 LabelLength = Index - LabelStart;
            if (LabelLength < 1 || LabelLength > 63) return false;
            if (!IsAsciiAlphaNumeric(Host[LabelStart]) || !IsAsciiAlphaNumeric(Host[Index - 1])) return false;
            for (int32 LabelIndex = LabelStart; LabelIndex < Index; ++LabelIndex)
            {
                if (!IsAsciiAlphaNumeric(Host[LabelIndex]) && Host[LabelIndex] != TEXT('-')) return false;
            }
            LabelStart = Index + 1;
        }

        Host.ToLowerInline();
        return true;
    }

    EKalmalaServerAddressParseResult ParsePort(const FString& Text, int32& OutPort)
    {
        if (Text.IsEmpty()) return EKalmalaServerAddressParseResult::InvalidPort;

        uint32 ParsedPort = 0;
        for (const TCHAR Character : Text)
        {
            if (!IsAsciiDigit(Character)) return EKalmalaServerAddressParseResult::InvalidPort;
            const uint32 Digit = static_cast<uint32>(Character - TEXT('0'));
            if (ParsedPort > (65535u - Digit) / 10u)
                return EKalmalaServerAddressParseResult::PortOutOfRange;
            ParsedPort = ParsedPort * 10u + Digit;
        }

        if (ParsedPort == 0 || ParsedPort > 65535u)
            return EKalmalaServerAddressParseResult::PortOutOfRange;
        OutPort = static_cast<int32>(ParsedPort);
        return EKalmalaServerAddressParseResult::Success;
    }
}

FString FKalmalaServerEndpoint::ToTravelAddress() const
{
    return Host.IsEmpty() || Port < 1 || Port > 65535
        ? FString()
        : FString::Printf(TEXT("%s:%d"), *Host, Port);
}

EKalmalaServerAddressParseResult FKalmalaServerAddressParser::Parse(const FString& Input,
    const int32 DefaultPort, FKalmalaServerEndpoint& OutEndpoint)
{
    OutEndpoint = FKalmalaServerEndpoint();
    FString Address = Input;
    Address.TrimStartAndEndInline();
    if (Address.IsEmpty()) return EKalmalaServerAddressParseResult::Empty;
    if (Address.Len() > MaximumAddressLength) return EKalmalaServerAddressParseResult::TooLong;

    int32 ColonIndex = INDEX_NONE;
    for (int32 Index = 0; Index < Address.Len(); ++Index)
    {
        const TCHAR Character = Address[Index];
        if (Character <= TEXT(' ') || Character == 127 || Character > 127)
            return EKalmalaServerAddressParseResult::InvalidCharacter;
        if (Character == TEXT('[') || Character == TEXT(']'))
            return EKalmalaServerAddressParseResult::UnsupportedAddress;
        if (Character == TEXT('/') || Character == TEXT('\\') || Character == TEXT('?')
            || Character == TEXT('#') || Character == TEXT('&') || Character == TEXT('='))
            return EKalmalaServerAddressParseResult::InvalidCharacter;
        if (Character == TEXT(':'))
        {
            if (ColonIndex != INDEX_NONE) return EKalmalaServerAddressParseResult::UnsupportedAddress;
            ColonIndex = Index;
        }
    }

    FString Host = ColonIndex == INDEX_NONE ? Address : Address.Left(ColonIndex);
    if (Host.IsEmpty()) return EKalmalaServerAddressParseResult::InvalidHost;

    int32 Port = DefaultPort;
    if (ColonIndex != INDEX_NONE)
    {
        const FString PortText = Address.Mid(ColonIndex + 1);
        const EKalmalaServerAddressParseResult PortResult = ParsePort(PortText, Port);
        if (PortResult != EKalmalaServerAddressParseResult::Success) return PortResult;
    }
    else if (DefaultPort < 1 || DefaultPort > 65535)
    {
        return EKalmalaServerAddressParseResult::DefaultPortUnavailable;
    }

    bool bNumericAddress = true;
    bool bHasDot = false;
    for (const TCHAR Character : Host)
    {
        bHasDot |= Character == TEXT('.');
        if (!IsAsciiDigit(Character) && Character != TEXT('.')) bNumericAddress = false;
    }

    if (bNumericAddress)
    {
        if (!bHasDot || !ParseIPv4(Host)) return EKalmalaServerAddressParseResult::InvalidHost;
    }
    else if (!IsValidDnsHostname(Host))
    {
        return EKalmalaServerAddressParseResult::InvalidHost;
    }

    OutEndpoint.Host = MoveTemp(Host);
    OutEndpoint.Port = Port;
    return EKalmalaServerAddressParseResult::Success;
}

const TCHAR* FKalmalaServerAddressParser::ErrorMessage(const EKalmalaServerAddressParseResult Result)
{
    switch (Result)
    {
    case EKalmalaServerAddressParseResult::Empty:
        return TEXT("Enter an IPv4 address or DNS hostname.");
    case EKalmalaServerAddressParseResult::TooLong:
        return TEXT("The address is too long. Use at most 253 characters, including a port.");
    case EKalmalaServerAddressParseResult::UnsupportedAddress:
        return TEXT("IPv6 addresses and join codes are not supported. Enter an IPv4 address or DNS hostname.");
    case EKalmalaServerAddressParseResult::InvalidCharacter:
        return TEXT("Enter a hostname or IPv4 address with an optional :port. URLs, spaces, paths, and travel options are not accepted.");
    case EKalmalaServerAddressParseResult::InvalidHost:
        return TEXT("Enter a valid IPv4 address or DNS hostname.");
    case EKalmalaServerAddressParseResult::InvalidPort:
        return TEXT("The port after ':' must contain decimal digits only.");
    case EKalmalaServerAddressParseResult::PortOutOfRange:
        return TEXT("The port must be a number from 1 to 65535.");
    case EKalmalaServerAddressParseResult::DefaultPortUnavailable:
        return TEXT("The configured gameplay port is unavailable.");
    case EKalmalaServerAddressParseResult::Success:
    default:
        return TEXT("");
    }
}
