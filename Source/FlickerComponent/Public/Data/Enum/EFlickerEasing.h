
#pragma once

/** Yumuşatma işlevleri için eğri tipini belirtir */
UENUM(BlueprintType)
enum class EFlickerEasing : uint8
{
	Linear UMETA(DisplayName="Linear"),
	SinusoidalIn UMETA(DisplayName="Sinusoidal In"),
	SinusoidalOut UMETA(DisplayName="Sinusoidal Out"),
	SinusoidalInOut UMETA(DisplayName="Sinusoidal In/Out"),
	EaseIn UMETA(DisplayName="Ease In"),
	EaseOut UMETA(DisplayName="Ease Out"),
	EaseInOut UMETA(DisplayName="Ease In/Out")
};