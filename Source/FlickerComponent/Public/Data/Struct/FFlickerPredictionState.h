
#pragma once

#include "FFlickerPredictionState.generated.h"

/** İstemci tarafı tahmini için içsel yapı */
USTRUCT()
struct FFlickerPredictionState
{
	GENERATED_BODY()
    
	UPROPERTY() bool bPredictedBlackout = false;
	UPROPERTY() float PredictedBlackoutTimeRemaining = 0.0f;
	UPROPERTY() float PredictedIntensity = 1.0f;
	UPROPERTY() FLinearColor PredictedColor = FLinearColor::White;
};