
#pragma once

#include "FFlickerTimelineKey.h"
#include "FFlickerTimelineAsset.generated.h"

USTRUCT(BlueprintType)
struct FFlickerTimelineAsset
{
	GENERATED_BODY()
    
	/** Zaman çizelgesinin adı */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Timeline")
	FName TimelineName = NAME_None;
    
	/** Anahtar kareler (zaman sıralı olmalı) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Timeline")
	TArray<FFlickerTimelineKey> Keys;
    
	/** Zaman çizelgesi sonunda başa dön (döngü) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Timeline")
	bool bLoop = false;
    
	/** Zaman çizelgesi bittiğinde eski duruma dön */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Timeline")
	bool bRestorePreviousState = true;
    
	/** Otomatik sırala (zaman temelli) */
	void SortKeys()
	{
		Keys.Sort([](const FFlickerTimelineKey& A, const FFlickerTimelineKey& B)
		{
			return A.Time < B.Time;
		});
	}
};