
#pragma once

#include "FFlickerSequenceStep.h"
#include "FFlickerSequence.generated.h"

/**
 * Birden çok adımdan oluşan titreme sekansı
 * Önceden tanımlayıp istediğiniz zaman oynatabilirsiniz
 */
USTRUCT(BlueprintType)
struct FFlickerSequence
{
	GENERATED_BODY()
    
	/** Sekans adı (PlayNamedSequence() ile çağırmak için) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence")
	FName SequenceName = NAME_None;
    
	/** Sekans adımları */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence")
	TArray<FFlickerSequenceStep> Steps;
    
	/** Sekans sonunda başa dön (sonsuz döngü) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence")
	bool bLoop = false;
    
	/** Sekans bitince önceki duruma dön (yoğunluk, renk, normal titreme) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence")
	bool bRestorePreviousState = true;

	/** Her titremede ani düşüş sesi çal (normal titremedeki gibi) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequence")
	bool bPlaySoundOnEachFlicker = true; 
};