
#pragma once

#include "FFlickerSequenceStep.generated.h"

/**
 * Sekansın bir adımı
 * Her adımda kaç titreme yapılacağı ve ne kadar süreceği belirtilir
 */
USTRUCT(BlueprintType)
struct FFlickerSequenceStep
{
	GENERATED_BODY()
    
	/** Bu adımda kaç titreme yapılacak (1-100) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence", meta=(ClampMin="1", ClampMax="100"))
	int32 FlickerCount = 1;
    
	/** Her titremenin süresi (saniye) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence", meta=(ClampMin="0.01", ClampMax="2.0"))
	float FlickerDuration = 0.1f;
    
	/** Titremeler arası bekleme süresi (saniye) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence", meta=(ClampMin="0.0", ClampMax="1.0"))
	float PauseBetweenFlickers = 0.05f;
    
	/** Bu adımda hedef ışık şiddeti (0-1)
	 * 0: Tam sönük
	 * 1: Tam parlak
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence", meta=(ClampMin="0.0", ClampMax="1.0"))
	float TargetIntensity = 0.0f;
    
	/** Bu adımda hedef renk */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence")
	FLinearColor TargetColor = FLinearColor::Black;
};