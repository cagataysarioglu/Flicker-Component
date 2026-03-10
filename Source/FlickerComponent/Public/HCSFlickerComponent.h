// =====================================================================
// Bu bileşen, ışık kaynaklarına titreme efekti ekler.
// Çok oyunculu oyunlar için tam destek, KYM renk değişimi, özel sekanslar ve ses sistemi içerir.
// =====================================================================

#pragma once

#include "Components/LightComponent.h"
#include "HCSFlickerComponent.generated.h"

// =====================================================================
// DELEGELER (OLAYLAR)
// =====================================================================

/**
 * Basit olay - parametre yok
 * Örnek: OnFlickerStarted, OnFlickerStopped
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFlickerSimpleDelegate);

/**
 * Ses olayı - hangi sesin çalınacağını bildirir
 * @param Sound - Çalınan ses
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFlickerSoundEvent, USoundBase*, Sound);

/**
 * Renk olayı - güncel rengi bildirir
 * @param Color - Güncel renk
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFlickerColorEvent, const FLinearColor&, Color);

/**
 * Yoğunluk olayı - ışık şiddetini bildirir
 * @param Intensity - Güncel şiddet (0-1 arası)
 * @param Alpha - Ara değer
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFlickerIntensityEvent, float, Intensity, float, Alpha);

// =====================================================================
// TEMEL AYARLAR YAPISI
// =====================================================================

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

// =====================================================================
// IŞIK AYARLARI YAPISI
// =====================================================================

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

// =====================================================================
// SEKANS ADIMI YAPISI
// =====================================================================

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

// =====================================================================
// SEKANS YAPISI
// =====================================================================

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

// =====================================================================
// ÇALIŞMA MODLARI
// =====================================================================

/** Bileşenin şu an hangi modda çalıştığını belirtir */
UENUM(BlueprintType)
enum class EFlickerMode : uint8
{
    Inactive UMETA(DisplayName="Inactive"), // Hiçbir şey çalışmıyor
    NormalFlicker UMETA(DisplayName="Normal Flicker"), // Normal titreme çalışıyor
    Sequence UMETA(DisplayName="Sequence") // Sekans çalışıyor
};

// =====================================================================
// ÇOĞALTMA DURUMU (İÇSEL)
// =====================================================================

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

// =====================================================================
// ANA BİLEŞEN SINIFI
// =====================================================================

/**
 * KULLANIM:
 * 1. Bileşeni herhangi bir aktöre ekleyin
 * 2. Işık bileşenlerini ekleyin (PointLight, SpotLight vb.)
 * 3. StartFlicker() ile başlatın veya PlayFlickerSequence() ile sekans oynatın
 * 
 * ÇOK OYUNCULU:
 * - Aktörün "Replicates" özelliği TRUE olmalı
 * - Tüm istemciler aynı efekti görür
 * - Sunucuda hesaplanır, istemcilere otomatik çoğaltılır
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent, DisplayName="HCS Flicker Component"))
class FLICKERCOMPONENT_API UHCSFlickerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UHCSFlickerComponent();

    // =================================================================
    // TEMEL İŞLEVLER
    // =================================================================
    
    /** Normal titreme efektini başlatır */
    UFUNCTION(BlueprintCallable, Category="HCS Flicker")
    void StartFlicker();
    
    /** Normal titreme efektini durdurur */
    UFUNCTION(BlueprintCallable, Category="HCS Flicker")
    void StopFlicker();
    
    /** Normal titreme etkin mi? */
    UFUNCTION(BlueprintPure, Category="HCS Flicker")
    bool IsFlickerActive() const { return bNormalFlickerActive; }
    
    // =================================================================
    // SEKANS İŞLEVLERİ
    // =================================================================
    
    /**
     * Dinamik sekans oluşturup oynatır
     * @param FlickerCount - Kaç kez titreme yapılacak (örn: 7)
     * @param FlickerDuration - Her titremenin süresi (saniye)
     * @param Intensity - Titreme anındaki ışık şiddeti (0-1)
     */
    UFUNCTION(BlueprintCallable, Category="HCS Flicker|Sequence")
    void PlayFlickerSequence(int32 FlickerCount, float FlickerDuration = 0.1f, float Intensity = 0.0f);
    
    /**
     * Önceden tanımlanmış sekansı oynatır
     * @param SequenceName - Sekans dizisindeki sekans adı
     */
    UFUNCTION(BlueprintCallable, Category="HCS Flicker|Sequence")
    void PlayNamedSequence(FName SequenceName);
    
    /** Oynayan sekansı durdurur */
    UFUNCTION(BlueprintCallable, Category="HCS Flicker|Sequence")
    void StopSequence();
    
    /** Sekans oynuyor mu? */
    UFUNCTION(BlueprintPure, Category="HCS Flicker|Sequence")
    bool IsSequencePlaying() const { return bSequenceActive; }
    
    /** Şu anki çalışma modunu döndürür */
    UFUNCTION(BlueprintPure, Category="HCS Flicker")
    EFlickerMode GetCurrentMode() const;
    
    // =================================================================
    // IŞIK İŞLEVLERİ
    // =================================================================
    
    /** Temel ışık şiddetini ayarlar (lümen) */
    UFUNCTION(BlueprintCallable, Category="HCS Flicker|Lights")
    void SetBaseIntensity(float Intensity);
    
    /** Temel ışık şiddetini döndürür */
    UFUNCTION(BlueprintPure, Category="HCS Flicker|Lights")
    float GetBaseIntensity() const { return BaseIntensity; }
    
    /** Temel ışık rengini ayarlar */
    UFUNCTION(BlueprintCallable, Category="HCS Flicker|Lights")
    void SetBaseColor(FLinearColor Color);
    
    /** Temel ışık rengini döndürür */
    UFUNCTION(BlueprintPure, Category="HCS Flicker|Lights")
    FLinearColor GetBaseColor() const { return BaseColor; }
    
    /**
     * Belirli bir ışığa özel ayar ekler
     * @param LightTag - Işık bileşeninin etiket değeri
     * @param Settings - Özel ayarlar
     */
    UFUNCTION(BlueprintCallable, Category="HCS Flicker|Lights")
    void AddLightSettings(FName LightTag, const FFlickerLightSettings& Settings);
    
    /** Belirli bir ışığın özel ayarlarını kaldırır */
    UFUNCTION(BlueprintCallable, Category="HCS Flicker|Lights")
    void RemoveLightSettings(FName LightTag);
    
    /** Tüm özel ışık ayarlarını temizler */
    UFUNCTION(BlueprintCallable, Category="HCS Flicker|Lights")
    void ClearLightSettings();
    
    /**
     * Belirli bir ışığı geçici olarak devre dışı bırakır
     * @param LightTag - Işık bileşeninin etiket değeri
     * @param bEnabled - true: etkin, false: devre dışı
     */
    UFUNCTION(BlueprintCallable, Category="HCS Flicker|Lights")
    void SetLightEnabled(FName LightTag, bool bEnabled);
    
    // =================================================================
    // AYAR İŞLEVLERİ
    // =================================================================
    
    /** Titreme ayarlarını değiştirir */
    UFUNCTION(BlueprintCallable, Category="HCS Flicker")
    void SetSettings(const FFlickerSettings& Settings);
    
    /** Titreme ayarlarını varsayılana sıfırlar */
    UFUNCTION(BlueprintCallable, Category="HCS Flicker")
    void ResetSettings();
    
    /** Mevcut titreme ayarlarını döndürür */
    UFUNCTION(BlueprintPure, Category="HCS Flicker")
    FFlickerSettings GetSettings() const { return CurrentSettings; }
    
    // =================================================================
    // DURUM SORGULAMA
    // =================================================================
    
    /** Karartma etkin mi? */
    UFUNCTION(BlueprintPure, Category="HCS Flicker")
    bool IsBlackoutActive() const { return bBlackoutActive; }
    
    /** Karartma etkin mi? (İstemci tahmini dahil) */
    UFUNCTION(BlueprintPure, Category="HCS Flicker|Prediction")
    bool IsBlackoutActivePredicted() const { return bBlackoutActive || PredictionState.bPredictedBlackout; }

    // =================================================================
    // GÜNCELLEME İŞLEVLERİ (Blueprint'ten çağrılabilir)
    // =================================================================
    
    /** Her frame çağrılır - güncel yoğunluk değerini döndürür */
    UFUNCTION(BlueprintCallable, Category="HCS Flicker")
    float UpdateFlicker() const;
    
    /** Her frame çağrılır - güncel renk değerini döndürür */
    UFUNCTION(BlueprintCallable, Category="HCS Flicker|Color")
    FLinearColor UpdateFlickerColor() const;

    // =================================================================
    // OLAYLAR (Blueprint'te bağlanabilir)
    // =================================================================
    
    /** Normal titreme başladığında tetiklenir */
    UPROPERTY(BlueprintAssignable, Category="HCS Flicker|Events")
    FFlickerSimpleDelegate OnFlickerStarted;
    
    /** Normal titreme durduğunda tetiklenir */
    UPROPERTY(BlueprintAssignable, Category="HCS Flicker|Events")
    FFlickerSimpleDelegate OnFlickerStopped;
    
    /** Karartma başladığında tetiklenir */
    UPROPERTY(BlueprintAssignable, Category="HCS Flicker|Events")
    FFlickerSimpleDelegate OnBlackoutStarted;
    
    /** Karartma bittiğinde tetiklenir */
    UPROPERTY(BlueprintAssignable, Category="HCS Flicker|Events")
    FFlickerSimpleDelegate OnBlackoutEnded;
    
    /** Dip (ani düşüş) sesi çalındığında tetiklenir */
    UPROPERTY(BlueprintAssignable, Category="HCS Flicker|Events")
    FFlickerSoundEvent OnDipSound;
    
    /** Karartma başlangıç sesi çalındığında tetiklenir */
    UPROPERTY(BlueprintAssignable, Category="HCS Flicker|Events")
    FFlickerSoundEvent OnBlackoutStartSound;
    
    /** Karartma bitiş sesi çalındığında tetiklenir */
    UPROPERTY(BlueprintAssignable, Category="HCS Flicker|Events")
    FFlickerSoundEvent OnBlackoutEndSound;
    
    /** Yoğunluk (intensity) güncellendiğinde tetiklenir */
    UPROPERTY(BlueprintAssignable, Category="HCS Flicker|Events")
    FFlickerIntensityEvent OnIntensityUpdated;
    
    /** Renk güncellendiğinde tetiklenir */
    UPROPERTY(BlueprintAssignable, Category="HCS Flicker|Events")
    FFlickerColorEvent OnColorUpdated;
    
    /** Sekans başladığında tetiklenir */
    UPROPERTY(BlueprintAssignable, Category="HCS Flicker|Events|Sequence")
    FFlickerSimpleDelegate OnSequenceStarted;
    
    /** Sekans bittiğinde tetiklenir */
    UPROPERTY(BlueprintAssignable, Category="HCS Flicker|Events|Sequence")
    FFlickerSimpleDelegate OnSequenceFinished;
    
    /** Sekans adımı değiştiğinde tetiklenir */
    UPROPERTY(BlueprintAssignable, Category="HCS Flicker|Events|Sequence")
    FFlickerSimpleDelegate OnSequenceStepChanged;

    // =================================================================
    // SES AYARLARI
    // =================================================================
    
    /** Yerleşik ses sistemini kullan (isteğe bağlı) */
    UPROPERTY(EditDefaultsOnly, Category="HCS Flicker|Audio")
    bool bEnableBuiltInAudio = true;
    
    /** Sadece belirli mesafedeki oyunculara ses gitmesi için */
    UPROPERTY(EditDefaultsOnly, Category="HCS Flicker|Audio", meta=(EditCondition="bEnableBuiltInAudio"))
    bool bUseDistanceBasedAudio = true;
    
    /** Ses duyulma mesafesi (birim) */
    UPROPERTY(EditDefaultsOnly, Category="HCS Flicker|Audio", meta=(EditCondition="bUseDistanceBasedAudio", ClampMin="100.0", ClampMax="5000.0"))
    float AudioDistanceRadius = 2000.0f;
    
    /** Dip (ani düşüş) sesi */
    UPROPERTY(EditDefaultsOnly, Category="HCS Flicker|Audio", meta=(EditCondition="bEnableBuiltInAudio"))
    TSoftObjectPtr<USoundBase> FlickerDipSound;
    
    /** Karartma başlangıç sesi */
    UPROPERTY(EditDefaultsOnly, Category="HCS Flicker|Audio", meta=(EditCondition="bEnableBuiltInAudio"))
    TSoftObjectPtr<USoundBase> FlickerBlackoutStartSound;
    
    /** Karartma bitiş sesi */
    UPROPERTY(EditDefaultsOnly, Category="HCS Flicker|Audio", meta=(EditCondition="bEnableBuiltInAudio"))
    TSoftObjectPtr<USoundBase> FlickerBlackoutEndSound;
    
    /** Ses volume çarpanı */
    UPROPERTY(EditDefaultsOnly, Category="HCS Flicker|Audio", meta=(EditCondition="bEnableBuiltInAudio", ClampMin="0.0", ClampMax="2.0"))
    float SoundVolumeMultiplier = 1.0f;
    
    /** Ses pitch çarpanı */
    UPROPERTY(EditDefaultsOnly, Category="HCS Flicker|Audio", meta=(EditCondition="bEnableBuiltInAudio", ClampMin="0.5", ClampMax="2.0"))
    float SoundPitchMultiplier = 1.0f;
    
    /** Ses uzaklık ayarları (attenuation) */
    UPROPERTY(EditDefaultsOnly, Category="HCS Flicker|Audio", meta=(EditCondition="bEnableBuiltInAudio"))
    USoundAttenuation* SoundAttenuation = nullptr;

    // =================================================================
    // ÇOK OYUNCULU AYARLARI
    // =================================================================
    
    /** Titreme durumunu çok oyunculuda çoğalt */
    UPROPERTY(EditDefaultsOnly, Category="HCS Flicker|Multiplayer")
    bool bReplicateFlickerState = true;
    
    /** Karartma durumunu çok oyunculuda çoğalt */
    UPROPERTY(EditDefaultsOnly, Category="HCS Flicker|Multiplayer")
    bool bReplicateBlackoutState = true;
    
    /** İstemci tarafı tahmini kullan (gecikmeyi azaltır) */
    UPROPERTY(EditDefaultsOnly, Category="HCS Flicker|Multiplayer")
    bool bEnableClientPrediction = true;
    
    /** Düzenli durum eşzamanlaması */
    UPROPERTY(EditDefaultsOnly, Category="HCS Flicker|Multiplayer")
    bool bEnablePeriodicSync = true;
    
    /** Eşzamanlama aralığı (saniye) */
    UPROPERTY(EditDefaultsOnly, Category="HCS Flicker|Multiplayer", meta=(EditCondition="bEnablePeriodicSync", ClampMin="0.5", ClampMax="5.0"))
    float SyncInterval = 1.0f;

    // =================================================================
    // PERFORMANS AYARLARI
    // =================================================================
    
    /** Titremenin tick hızı (saniyede kaç kere güncellenecek)
     * 60: Normal (60 FPS)
     * 30: Düşük performanslı cihazlar için
     * 120: Çok yüksek kalite
     */
    UPROPERTY(EditDefaultsOnly, Category="HCS Flicker|Performance", meta=(ClampMin="10", ClampMax="120"))
    int32 TickRate = 60;
    
    /** Yoğunluk eğri (curve) desteği (isteğe bağlı) */
    UPROPERTY(EditDefaultsOnly, Category="HCS Flicker|Advanced")
    UCurveFloat* FlickerIntensityCurve = nullptr;

    // =================================================================
    // IŞIK YAPILANDIRMASI
    // =================================================================
    
    /** Otomatik bulunan tüm ışıklar (salt okunur) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="HCS Flicker|Lights")
    TArray<ULightComponent*> AllLights;
    
    /** Işık temelli özel ayarlar */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HCS Flicker|Lights")
    TArray<FFlickerLightSettings> PerLightSettings;

    // =================================================================
    // SEKANS YAPILANDIRMASI
    // =================================================================
    
    /** Önceden tanımlanmış sekanslar */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HCS Flicker|Sequence")
    TArray<FFlickerSequence> Sequences;

protected:
    // =================================================================
    // GÖMÜLÜ SANAL İŞLEVLER
    // =================================================================
    
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // =================================================================
    // ÇOĞALTMA ÇAĞRILARI
    // =================================================================
    
    UFUNCTION()
    void OnRep_NormalFlickerActive();
    
    UFUNCTION()
    void OnRep_BlackoutActive();
    
    UFUNCTION()
    void OnRep_SequenceActive();
    
    UFUNCTION()
    void OnRep_CurrentSettings();
    
    UFUNCTION()
    void OnRep_BaseColor();
    
    UFUNCTION()
    void OnRep_CurrentIntensity();
    
    UFUNCTION()
    void OnRep_CurrentColor();

    // =================================================================
    // SUNUCU RPC
    // =================================================================
    
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerStartFlicker();
    void ServerStartFlicker_Implementation();
    bool ServerStartFlicker_Validate();
    
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerStopFlicker();
    void ServerStopFlicker_Implementation();
    bool ServerStopFlicker_Validate();
    
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSetBaseIntensity(float Intensity);
    void ServerSetBaseIntensity_Implementation(float Intensity);
    bool ServerSetBaseIntensity_Validate(float Intensity);
    
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSetBaseColor(const FLinearColor& Color);
    void ServerSetBaseColor_Implementation(const FLinearColor& Color);
    bool ServerSetBaseColor_Validate(const FLinearColor& Color);
    
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSetSettings(const FFlickerSettings& Settings);
    void ServerSetSettings_Implementation(const FFlickerSettings& Settings);
    bool ServerSetSettings_Validate(const FFlickerSettings& Settings);
    
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerPlaySequence(const FFlickerSequence& Sequence);
    void ServerPlaySequence_Implementation(const FFlickerSequence& Sequence);
    bool ServerPlaySequence_Validate(const FFlickerSequence& Sequence);
    
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerStopSequence();
    void ServerStopSequence_Implementation();
    bool ServerStopSequence_Validate();

    // =================================================================
    // ÇOKLU YAYIN RPC
    // =================================================================
    
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_PlaySound(USoundBase* Sound);
    void Multicast_PlaySound_Implementation(USoundBase* Sound);
    
    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_PlaySoundNearby(USoundBase* Sound, float Radius);
    void Multicast_PlaySoundNearby_Implementation(USoundBase* Sound, float Radius);
    
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_BlackoutStateChanged(bool bNewBlackoutActive);
    void Multicast_BlackoutStateChanged_Implementation(bool bNewBlackoutActive);
    
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_SequenceStateChanged(bool bNewSequenceActive, FName SequenceName);
    void Multicast_SequenceStateChanged_Implementation(bool bNewSequenceActive, FName SequenceName);
    
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_UpdateState(float Intensity, const FLinearColor& Color);
    void Multicast_UpdateState_Implementation(float Intensity, const FLinearColor& Color);

    // =================================================================
    // İSTEMCİ RPC
    // =================================================================
    
    UFUNCTION(Client, Unreliable)
    void Client_PredictionCorrection(bool bCorrectedBlackout, float CorrectedIntensity);
    void Client_PredictionCorrection_Implementation(bool bCorrectedBlackout, float CorrectedIntensity);

    // =================================================================
    // İÇSEL İŞLEVLER
    // =================================================================
    
    void SetBlackoutActive(bool bNewActive, bool bFromReplication = false);
    void PlayFlickerSound(const TSoftObjectPtr<USoundBase>& Sound, const FFlickerSoundEvent& SoundEvent);
    void UpdateNormalFlicker(float DeltaSeconds);
    void UpdateSequence(float DeltaSeconds);
    void UpdateColor(float DeltaSeconds);
    FLinearColor CalculateSequenceColor() const;
    void SetTickOptimization();
    void ForceStateSync();
    void ValidatePrediction();
    bool IsClientWithinDistance(float Radius) const;
    void UpdateAllLights(float Intensity, const FLinearColor& Color);
    FFlickerLightSettings* GetLightSettingsForTag(FName LightTag);
    bool IsLightEnabled(FName LightTag) const;
    void SavePreSequenceState();
    void RestorePreSequenceState();
    void CacheLights();

private:
    // =================================================================
    // ÇOĞALTILMIŞ ÖZELLİKLER (Tüm istemciler görür)
    // =================================================================
    
    /** Normal titreme etkin mi? */
    UPROPERTY(ReplicatedUsing=OnRep_NormalFlickerActive)
    bool bNormalFlickerActive = false;
    
    /** Karartma etkin mi? */
    UPROPERTY(ReplicatedUsing=OnRep_BlackoutActive)
    bool bBlackoutActive = false;
    
    /** Sekans etkin mi? */
    UPROPERTY(ReplicatedUsing=OnRep_SequenceActive)
    bool bSequenceActive = false;
    
    /** Etkin sekans adı */
    UPROPERTY(Replicated)
    FName CurrentSequenceName = NAME_None;
    
    /** Güncel titreme ayarları */
    UPROPERTY(ReplicatedUsing=OnRep_CurrentSettings)
    FFlickerSettings CurrentSettings;
    
    /** Temel ışık şiddeti */
    UPROPERTY(Replicated)
    float BaseIntensity = 0.0f;
    
    /** Temel ışık rengi */
    UPROPERTY(ReplicatedUsing=OnRep_BaseColor)
    FLinearColor BaseColor = FLinearColor::White;
    
    /** Güncel ışık şiddeti (0-1 arası çarpan) */
    UPROPERTY(ReplicatedUsing=OnRep_CurrentIntensity)
    float CurrentIntensity = 1.0f;
    
    /** Güncel ışık rengi */
    UPROPERTY(ReplicatedUsing=OnRep_CurrentColor)
    FLinearColor CurrentColor = FLinearColor::White;

    // =================================================================
    // SALT SUNUCU ÖZELLİKLERİ (Çoğaltılmaz)
    // =================================================================
    
    /** Etkin sekans */
    FFlickerSequence ActiveSequence;
    
    /** Sekans durumu */
    int32 CurrentStepIndex = 0;
    int32 CurrentFlickerInStep = 0;
    float SequenceStepTimer = 0.0f;
    float SequenceFlickerTimer = 0.0f;
    bool bSequenceFlickerState = true;
    bool bCurrentSequenceRestoreState = true;
    bool bCurrentSequencePlaySound = true;
    
    /** Normal titreme durumu */
    float MicroJitterTimer = 0.0f;
    float MicroJitterTarget = 1.0f;
    float MicroJitterAlpha = 1.0f;
    float DipCooldown = 0.0f;
    float DipTimer = 0.0f;
    float NormalBlackoutTimer = 0.0f;
    float ColorTimer = 0.0f;
    FLinearColor TargetColor = FLinearColor::White;
    
    /** Önceki sekans durumu (restore için) */
    float PreSequenceIntensity = 1.0f;
    FLinearColor PreSequenceColor = FLinearColor::White;
    bool bPreSequenceNormalFlickerActive = false;
    bool bPreSequenceBlackoutActive = false;
    
    // =================================================================
    // PERFORMANS
    // =================================================================
    
    float TickInterval = 0.0f;
    float TimeSinceLastTick = 0.0f;
    
    // =================================================================
    // TAHMİN
    // =================================================================
    
    FFlickerPredictionState PredictionState;
    float TimeSinceLastSync = 0.0f;
    
    // =================================================================
    // EŞZAMANLAMA
    // =================================================================
    
    FTimerHandle StateSyncTimerHandle;
    bool bOldBlackoutState = false;
    
    // =================================================================
    // IŞIKLAR
    // =================================================================
    
    bool bClientLightsCached = false;
    TSet<FName> DisabledLights;
    TMap<FName, FLinearColor> OriginalLightColors;
};