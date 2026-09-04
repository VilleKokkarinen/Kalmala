#pragma once

#include "CoreMinimal.h"
#include "KalmalaWeatherState.generated.h"

/**
 * The active server-owned weather interval replicated to all session peers.
 * Values are display inputs only on clients; GameMode is the sole writer.
 */
USTRUCT(BlueprintType)
struct KALMALAWORLD_API FKalmalaWeatherState
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, Category = "Weather")
    int32 WeatherCycleIndex = 0;

    UPROPERTY(VisibleAnywhere, Category = "Weather")
    float ServerStartTimeSeconds = 0.0f;

    UPROPERTY(VisibleAnywhere, Category = "Weather")
    float DurationSeconds = 120.0f;

    UPROPERTY(VisibleAnywhere, Category = "Weather", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float PrecipitationIntensity = 0.0f;

    UPROPERTY(VisibleAnywhere, Category = "Weather", meta = (ClampMin = "0", ClampMax = "315"))
    int32 WindDirectionDegrees = 0;

    UPROPERTY(VisibleAnywhere, Category = "Weather", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float WindStrength = 0.0f;

    bool IsValid() const
    {
        return WeatherCycleIndex >= 0
            && DurationSeconds >= 120.0f && DurationSeconds <= 240.0f
            && FMath::IsWithinInclusive(PrecipitationIntensity, 0.0f, 1.0f)
            && WindDirectionDegrees >= 0 && WindDirectionDegrees < 360 && WindDirectionDegrees % 45 == 0
            && FMath::IsWithinInclusive(WindStrength, 0.0f, 1.0f);
    }
};
