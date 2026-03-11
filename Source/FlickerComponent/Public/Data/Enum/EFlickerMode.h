
#pragma once

/** Bileşenin şu an hangi modda çalıştığını belirtir */
UENUM(BlueprintType)
enum class EFlickerMode : uint8
{
	Inactive UMETA(DisplayName="Inactive"), // Hiçbir şey çalışmıyor
	NormalFlicker UMETA(DisplayName="Normal Flicker"), // Normal titreme çalışıyor
	Sequence UMETA(DisplayName="Sequence"), // Sekans çalışıyor
	Timeline UMETA(DisplayName="Timeline") // Zaman çizelgesi çalışıyor
};