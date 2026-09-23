#pragma once

#include "CoreMinimal.h"
#include "KalmalaWeatherState.generated.h"

UENUM(BlueprintType)
enum class EKalmalaWeatherActivityLevel : uint8
{
    Calm UMETA(DisplayName = "Calm"),
    Active UMETA(DisplayName = "Active"),
    HighlyActive UMETA(DisplayName = "Highly Active")
};

/**
 * The active server-owned weather interval replicated to all session peers.
 * Values are display inputs only on clients; GameMode is the sole writer.
 */
USTRUCT(BlueprintType)
struct KALMALAWORLD_API FKalmalaWeatherState
{
    GENERATED_BODY()

    static constexpr float HighlyActiveStormThreshold = 0.65f;

    UPROPERTY(VisibleAnywhere, Category = "Weather")
    int32 WeatherCycleIndex = 0;

    UPROPERTY(VisibleAnywhere, Category = "Weather")
    float ServerStartTimeSeconds = 0.0f;

    UPROPERTY(VisibleAnywhere, Category = "Weather")
    float DurationSeconds = 120.0f;

    UPROPERTY(VisibleAnywhere, Category = "Weather", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float PrecipitationIntensity = 0.0f;

    UPROPERTY(VisibleAnywhere, Category = "Weather", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float FogIntensity = 0.0f;

    UPROPERTY(VisibleAnywhere, Category = "Weather", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float StormIntensity = 0.0f;

    UPROPERTY(VisibleAnywhere, Category = "Weather", meta = (ClampMin = "0", ClampMax = "315"))
    int32 WindDirectionDegrees = 0;

    UPROPERTY(VisibleAnywhere, Category = "Weather", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float WindStrength = 0.0f;

    UPROPERTY(VisibleAnywhere, Category = "Weather")
    EKalmalaWeatherActivityLevel ActivityLevel = EKalmalaWeatherActivityLevel::Calm;

    float GetStormIntensity() const
    {
        return StormIntensity;
    }

    static float DeriveStormIntensity(const float InPrecipitationIntensity, const float InWindStrength)
    {
        return FMath::Clamp(InPrecipitationIntensity * InWindStrength, 0.0f, 1.0f);
    }

    static EKalmalaWeatherActivityLevel DeriveActivityLevel(const float InFogIntensity, const float InPrecipitationIntensity, const float InWindStrength)
    {
        const float Fog = FMath::Clamp(InFogIntensity, 0.0f, 1.0f);
        const float Precipitation = FMath::Clamp(InPrecipitationIntensity, 0.0f, 1.0f);
        const float Wind = FMath::Clamp(InWindStrength, 0.0f, 1.0f);
        if (Fog >= 0.80f || Precipitation * Wind >= HighlyActiveStormThreshold)
        {
            return EKalmalaWeatherActivityLevel::HighlyActive;
        }
        if (Fog >= 0.25f || Precipitation >= 0.15f || Wind >= 0.55f)
        {
            return EKalmalaWeatherActivityLevel::Active;
        }
        return EKalmalaWeatherActivityLevel::Calm;
    }

    void RefreshActivityLevel()
    {
        StormIntensity = DeriveStormIntensity(PrecipitationIntensity, WindStrength);
        ActivityLevel = DeriveActivityLevel(FogIntensity, PrecipitationIntensity, WindStrength);
    }

    bool IsValid() const
    {
        return WeatherCycleIndex >= 0
            && FMath::IsFinite(ServerStartTimeSeconds) && ServerStartTimeSeconds >= 0.0f
            && DurationSeconds >= 120.0f && DurationSeconds <= 240.0f
            && FMath::IsWithinInclusive(PrecipitationIntensity, 0.0f, 1.0f)
            && FMath::IsWithinInclusive(FogIntensity, 0.0f, 1.0f)
            && FMath::IsWithinInclusive(StormIntensity, 0.0f, 1.0f)
            && WindDirectionDegrees >= 0 && WindDirectionDegrees < 360 && WindDirectionDegrees % 45 == 0
            && FMath::IsWithinInclusive(WindStrength, 0.0f, 1.0f)
            && FMath::IsNearlyEqual(StormIntensity, DeriveStormIntensity(PrecipitationIntensity, WindStrength))
            && ActivityLevel == DeriveActivityLevel(FogIntensity, PrecipitationIntensity, WindStrength);
    }
};
