
#pragma once

#include "FFlickerLightSettings.generated.h"

/**
* Her bir ışık için özel ayarlar
 * Aynı aktördeki farklı ışıkları farklı şekilde titretebilirsiniz
 */
USTRUCT(BlueprintType)
struct FFlickerLightSettings
{
	GENERATED_BODY()
    
	/** Bu ayarların uygulanacağı ışığın etiketi
	 * Işık bileşeninin etiketine ekleyerek hangi ışığın
	 * bu ayarları kullanacağını belirleyin
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light")
	FName LightTag = NAME_None;
    
	/** Temel ayarları kullan (true) veya özel ayar kullan (false) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light")
	bool bUseMasterSettings = true;
    
	/** Özel mikro titreme şiddeti (bUseMasterSettings=false ise) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light", meta=(EditCondition="!bUseMasterSettings", ClampMin="0.0", ClampMax="2.0"))
	float MicroJitterStrength = 1.0f;
    
	/** Özel dip şansı (bUseMasterSettings=false ise) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light", meta=(EditCondition="!bUseMasterSettings", ClampMin="0.0", ClampMax="5.0"))
	float DipChanceMultiplier = 1.0f;
    
	/** Özel renk kullan (true) veya temel rengi kullan (false) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light", meta=(EditCondition="!bUseMasterSettings"))
	bool bOverrideColor = false;
    
	/** Özel renk (bOverrideColor=true ise) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light", meta=(EditCondition="bOverrideColor"))
	FLinearColor CustomColor = FLinearColor::White;
};