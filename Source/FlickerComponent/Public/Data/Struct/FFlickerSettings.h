
#pragma once

#include "FFlickerSettings.generated.h"

/**
 * Titreme efekti için temel ayarlar
 * Bu yapıyla tüm titreme parametrelerini kontrol edebilirsiniz
 */
USTRUCT(BlueprintType)
struct FFlickerSettings
{
    GENERATED_BODY()
    
    /** Mikro titreme şiddeti (0-2 arası)
     * 0: Titreme yok
     * 1: Normal titreme
     * 2: Çok şiddetli titreme
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flicker", meta=(ClampMin="0.0", ClampMax="2.0"))
    float MicroJitterStrength = 1.0f;
    
    /** Ani düşüş (dip) şansı çarpanı (0-5 arası)
     * 0: Ani düşüş olmaz
     * 1: Normal ani düşüş sıklığı
     * 5: Çok sık ani düşüş
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flicker", meta=(ClampMin="0.0", ClampMax="5.0"))
    float DipChanceMultiplier = 1.0f;
    
    /** Ani düşüş (dip) şiddeti (0.1-5 arası)
     * Düşük değer = daha karanlık ani düşüş
     * Yüksek değer = daha az karanlık
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flicker", meta=(ClampMin="0.1", ClampMax="5.0"))
    float DipStrength = 1.0f;
    
    /** Karartma şansı çarpanı (0-5 arası)
     * 0: Karartma olmaz
     * 1: Normal karartma sıklığı
     * 5: Çok sık karartma
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flicker", meta=(ClampMin="0.0", ClampMax="5.0"))
    float BlackoutChanceMultiplier = 1.0f;
    
    /** Renk değişim hızı (0-5 arası)
     * 0: Renk değişmez
     * 1: Normal hız
     * 5: Çok hızlı renk değişimi
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flicker|Color", meta=(ClampMin="0.0", ClampMax="5.0"))
    float ColorChangeSpeed = 1.0f;
    
    /** Renk değişim şiddeti (0-1 arası)
     * 0: Temel renkten sapma yok
     * 1: Temel renkten çok sapma
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flicker|Color", meta=(ClampMin="0.0", ClampMax="1.0"))
    float ColorVariationStrength = 0.2f;

    FFlickerSettings() {}
    
    bool operator==(const FFlickerSettings& Other) const
    {
        return MicroJitterStrength == Other.MicroJitterStrength && 
               DipChanceMultiplier == Other.DipChanceMultiplier &&
               DipStrength == Other.DipStrength && 
               BlackoutChanceMultiplier == Other.BlackoutChanceMultiplier &&
               ColorChangeSpeed == Other.ColorChangeSpeed &&
               ColorVariationStrength == Other.ColorVariationStrength;
    }
    
    bool operator!=(const FFlickerSettings& Other) const
    {
        return !(*this == Other);
    }
};