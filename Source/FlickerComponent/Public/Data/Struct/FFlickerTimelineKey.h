
#pragma once

#include "Data/Enum/EFlickerEasing.h"
#include "FFlickerTimelineKey.generated.h"

USTRUCT(BlueprintType)
struct FFlickerTimelineKey
{
	GENERATED_BODY()
    
	/** Zaman (saniye cinsinden) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Timeline", meta=(ClampMin="0.0"))
	float Time = 0.0f;
    
	/** Bu andaki hedef ışık şiddeti (0-1 arası) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Timeline", meta=(ClampMin="0.0", ClampMax="1.0"))
	float Intensity = 1.0f;
    
	/** Bu andaki hedef renk */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Timeline")
	FLinearColor Color = FLinearColor::White;
    
	/** Bu anda ses çalınsın mı? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Timeline")
	bool bPlaySound = false;
    
	/** Kullanılacak eğri tipi */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Timeline")
	EFlickerEasing EasingFunction = EFlickerEasing::Linear; 
    
	/** Eğri için karışım üssü (SmoothInOut vb. için) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Timeline", meta=(ClampMin="0.1", ClampMax="10.0"))
	float BlendExp = 1.0f;
    
	/** Özel eğri (isteğe bağlı) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Timeline")
	UCurveFloat* CustomCurve = nullptr;
};