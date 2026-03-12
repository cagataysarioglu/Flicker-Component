// HCS FLICKER COMPONENT
// =========================================================================
// Bu bileşen, ışık kaynaklarına gelişmiş titreme efekti ekler.
// Çok oyunculu oyunlar için tam ağ desteği, KYM renk değişimi,
// özel sekans oluşturma, zaman çizelgesi desteği ve detaylı ses sistemi bütünlüğünü içerir.

#pragma once

#include "Components/LightComponent.h"
#include "Data/Enum/EFlickerMode.h"
#include "Data/Struct/FFlickerPredictionState.h"
#include "Data/Struct/FFlickerSequence.h"
#include "Data/Struct/FFlickerSettings.h"
#include "HCSFlickerComponent.generated.h"

struct FFlickerLightSettings;
struct FFlickerTimelineAsset;
struct FFlickerTimelineKey;

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
 * Zaman çizelgesi anahtar olayı - güncel anahtar indeksini bildirir
 * @param KeyIndex - Anahtar sırası
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFlickerTimelineKeyEvent, int32, KeyIndex);

/**
 * Yoğunluk olayı - ışık şiddetini bildirir
 * @param Intensity - Güncel şiddet (0-1 arası)
 * @param Alpha - Ara değer
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFlickerIntensityEvent, float, Intensity, float, Alpha);

// ANA BİLEŞEN SINIFI

/**
 * KULLANIM:
 * 1. Bileşeni herhangi bir aktöre ekleyin
 * 2. Işık bileşenlerini ekleyin (PointLight, SpotLight vb.)
 * 3. StartFlicker() ile başlatın veya PlayFlickerSequence() ile bir sekans oynatın, PlayTimeline() ile bir zaman çizelgesi oynatın
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

    // TEMEL İŞLEVLER
    
    /** Normal titreme efektini başlatır */
    UFUNCTION(BlueprintCallable, Category="HCS Flicker")
    void StartFlicker();
    
    /** Normal titreme efektini durdurur */
    UFUNCTION(BlueprintCallable, Category="HCS Flicker")
    void StopFlicker();
    
    /** Normal titreme etkin mi? */
    UFUNCTION(BlueprintPure, Category="HCS Flicker")
    bool IsFlickerActive() const { return bNormalFlickerActive; }
    
    // SEKANS İŞLEVLERİ
    
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

	// ZAMAN ÇİZELGESİ İŞLEVLERİ
    
	/** Önceden tanımlanmış zaman çizelgesini oynat */
	UFUNCTION(BlueprintCallable, Category="HCS Flicker|Timeline")
	void PlayTimeline(FName TimelineName);
    
	/** Dinamik zaman çizelgesi oynat (anahtar karelerle) */
	UFUNCTION(BlueprintCallable, Category="HCS Flicker|Timeline")
	void PlayCustomTimeline(const TArray<FFlickerTimelineKey>& Keys, bool bLoop = false, bool bRestore = true, bool bApplyFirstKey = true);
    
	/** Zaman çizelgesini durdur */
	UFUNCTION(BlueprintCallable, Category="HCS Flicker|Timeline")
	void StopTimeline();
    
	/** Zaman çizelgesi oynuyor mu? */
	UFUNCTION(BlueprintPure, Category="HCS Flicker|Timeline")
	bool IsTimelinePlaying() const { return bTimelineActive; }
    
	/** Zaman çizelgesini duraklat/sürdür */
	UFUNCTION(BlueprintCallable, Category="HCS Flicker|Timeline")
	void SetTimelinePaused(bool bPaused);
    
	/** Zaman çizelgesinin geçerli zamanını ayarla (sarma) */
	UFUNCTION(BlueprintCallable, Category="HCS Flicker|Timeline")
	void SetTimelineTime(float NewTime);
    
	/** Zaman çizelgesinin toplam süresini döndür */
	UFUNCTION(BlueprintPure, Category="HCS Flicker|Timeline")
	float GetTimelineDuration() const;
    
    // IŞIK İŞLEVLERİ
    
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
    
    // AYAR İŞLEVLERİ
    
    /** Titreme ayarlarını değiştirir */
    UFUNCTION(BlueprintCallable, Category="HCS Flicker")
    void SetSettings(const FFlickerSettings& Settings);
    
    /** Titreme ayarlarını varsayılana sıfırlar */
    UFUNCTION(BlueprintCallable, Category="HCS Flicker")
    void ResetSettings();
    
    /** Mevcut titreme ayarlarını döndürür */
    UFUNCTION(BlueprintPure, Category="HCS Flicker")
    FFlickerSettings GetSettings() const { return CurrentSettings; }
    
    // DURUM SORGULAMA
    
    /** Karartma etkin mi? */
    UFUNCTION(BlueprintPure, Category="HCS Flicker")
    bool IsBlackoutActive() const { return bBlackoutActive; }
    
    /** Karartma etkin mi? (İstemci tahmini dahil) */
    UFUNCTION(BlueprintPure, Category="HCS Flicker|Prediction")
    bool IsBlackoutActivePredicted() const { return bBlackoutActive || PredictionState.bPredictedBlackout; }

    // GÜNCELLEME İŞLEVLERİ (Blueprint'ten çağrılabilir)

	/** Tüm istemcileri elle eşzamanlar (acil durumlar için) */
	UFUNCTION(BlueprintCallable, Category="HCS Flicker|Multiplayer")
	void ForceSyncAllClients();
    
    /** Her frame çağrılır - güncel yoğunluk değerini döndürür */
    UFUNCTION(BlueprintCallable, Category="HCS Flicker")
    float UpdateFlicker() const;
    
    /** Her frame çağrılır - güncel renk değerini döndürür */
    UFUNCTION(BlueprintCallable, Category="HCS Flicker|Color")
    FLinearColor UpdateFlickerColor() const;

    // OLAYLAR (Blueprint'te bağlanabilir)
    
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

	/** Zaman çizelgesi başladığında tetiklenir */
	UPROPERTY(BlueprintAssignable, Category="HCS Flicker|Events|Timeline")
	FFlickerSimpleDelegate OnTimelineStarted;
    
	/** Zaman çizelgesi bittiğinde tetiklenir */
	UPROPERTY(BlueprintAssignable, Category="HCS Flicker|Events|Timeline")
	FFlickerSimpleDelegate OnTimelineFinished;
    
	/** Zaman çizelgesi anahtarı değiştiğinde tetiklenir */
	UPROPERTY(BlueprintAssignable, Category="HCS Flicker|Events|Timeline")
	FFlickerTimelineKeyEvent OnTimelineKeyChanged;

	/** Eşzamanlama yapıldığında tetiklenir */
	UPROPERTY(BlueprintAssignable, Category="HCS Flicker|Events|Multiplayer")
	FFlickerSimpleDelegate OnSyncCompleted;

    // SES AYARLARI
    
    /** Yerleşik ses sistemini kullan (isteğe bağlı) */
    UPROPERTY(EditDefaultsOnly, Category="HCS Flicker|Audio")
    bool bEnableBuiltInAudio = true;
    
    /** Sadece belirli mesafedeki oyunculara ses gitmesi için */
    UPROPERTY(EditDefaultsOnly, Category="HCS Flicker|Audio", meta=(EditCondition="bEnableBuiltInAudio"))
    bool bUseDistanceBasedAudio = true;
    
    /** Ses duyulma mesafesi (birim) */
    UPROPERTY(EditDefaultsOnly, Category="HCS Flicker|Audio", meta=(EditCondition="bUseDistanceBasedAudio", ClampMin="100.0", ClampMax="5000.0"))
    float AudioDistanceRadius = 2000.0f;
    
    /** Ani düşüş (dip) sesi */
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

    // ÇOK OYUNCULU AYARLARI
    
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
	
	/** Otomatik eşzamanlama ayarı */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HCS Flicker|Multiplayer", AdvancedDisplay)
	bool bAutoSyncOnPlayerJoin = true;

    // PERFORMANS AYARLARI
    
    /** Titremenin tick hızı (saniyede kaç kere güncellenecek)
     * 60: Normal (60 FPS)
     * 30: Düşük performanslı cihazlar için
     * 120: Çok yüksek kalite
     */
    UPROPERTY(EditDefaultsOnly, Category="HCS Flicker|Performance", meta=(ClampMin="10", ClampMax="120"))
    int32 TickRate = 60;
    
    /** Yoğunluk eğrisi desteği (isteğe bağlı) */
    UPROPERTY(EditDefaultsOnly, Category="HCS Flicker|Advanced")
    UCurveFloat* FlickerIntensityCurve = nullptr;

    // IŞIK YAPILANDIRMASI
    
    /** Otomatik bulunan tüm ışıklar (salt okunur) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="HCS Flicker|Lights")
    TArray<ULightComponent*> AllLights;
    
    /** Işık temelli özel ayarlar */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HCS Flicker|Lights")
    TArray<FFlickerLightSettings> PerLightSettings;

    // SEKANS YAPILANDIRMASI
    
    /** Önceden tanımlanmış sekanslar */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HCS Flicker|Sequence")
    TArray<FFlickerSequence> Sequences;

    // ZAMAN ÇİZELGESİ YAPILANDIRMASI
	
    /** Önceden tanımlanmış zaman çizelgeleri */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HCS Flicker|Timeline")
	TArray<FFlickerTimelineAsset> Timelines;

protected:
    // YERLEŞİK SANAL İŞLEVLER
    
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // ÇOĞALTMA ÇAĞRILARI
    
    UFUNCTION()
    void OnRep_NormalFlickerActive();
    
    UFUNCTION()
    void OnRep_BlackoutActive();

	UFUNCTION()
	void OnRep_TimelineActive();
    
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

    // SUNUCU RPC
    
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

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerPlayTimeline(const TArray<FFlickerTimelineKey>& Keys, bool bLoop, bool bRestore, bool bApplyFirstKey);
	void ServerPlayTimeline_Implementation(const TArray<FFlickerTimelineKey>& Keys, bool bLoop, bool bRestore, bool bApplyFirstKey);
	bool ServerPlayTimeline_Validate(const TArray<FFlickerTimelineKey>& Keys, bool bLoop, bool bRestore, bool bApplyFirstKey);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetTimelineTime(float NewTime);
	void ServerSetTimelineTime_Implementation(float NewTime);
	bool ServerSetTimelineTime_Validate(float NewTime);

    // ÇOKLU YAYIN RPC
    
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
    void Multicast_TimelineStateChanged(bool bNewTimelineActive, FName TimelineName);
    void Multicast_TimelineStateChanged_Implementation(bool bNewTimelineActive, FName TimelineName);
    
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_UpdateState(float Intensity, const FLinearColor& Color);
    void Multicast_UpdateState_Implementation(float Intensity, const FLinearColor& Color);

    // İSTEMCİ RPC
    
    UFUNCTION(Client, Unreliable)
    void Client_PredictionCorrection(bool bCorrectedBlackout, float CorrectedIntensity);
    void Client_PredictionCorrection_Implementation(bool bCorrectedBlackout, float CorrectedIntensity);

    // İÇSEL İŞLEVLER
    
    void SetBlackoutActive(bool bNewActive, bool bFromReplication = false);
    void PlayFlickerSound(const TSoftObjectPtr<USoundBase>& Sound, const FFlickerSoundEvent& SoundEvent);
    void UpdateNormalFlicker(float DeltaSeconds);
    void UpdateSequence(float DeltaSeconds);
    void UpdateTimeline(float DeltaSeconds);
    void UpdateColor(float DeltaSeconds);
	void EvaluateTimelineAtTime(float Time, int32 PrevIndex, int32 NextIndex);
    void SetTickOptimization();
    void ForceStateSync();
    void ValidatePrediction();
    FFlickerLightSettings* GetLightSettingsForTag(FName LightTag);
    bool IsLightEnabled(FName LightTag) const;
    void SavePreSequenceState();
    void RestorePreSequenceState();
    void SavePreTimelineState();
    void RestorePreTimelineState();
    void CacheLights();
    void UpdateAllLights(float Intensity, const FLinearColor& Color);

private:
    // ÇOĞALTILMIŞ ÖZELLİKLER (Tüm istemciler görür)
    
    /** Normal titreme etkin mi? */
    UPROPERTY(ReplicatedUsing=OnRep_NormalFlickerActive)
    bool bNormalFlickerActive = false;
    
    /** Karartma etkin mi? */
    UPROPERTY(ReplicatedUsing=OnRep_BlackoutActive)
    bool bBlackoutActive = false;

	/** Zaman çizelgesi etkin mi? */
	UPROPERTY(ReplicatedUsing=OnRep_TimelineActive)
	bool bTimelineActive = false;
    
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

    // SALT SUNUCU ÖZELLİKLERİ (Çoğaltılmaz)
    
    /** Normal titreme durumu */
    float MicroJitterTimer = 0.0f;
    float MicroJitterTarget = 1.0f;
    float MicroJitterAlpha = 1.0f;
    float DipCooldown = 0.0f;
    float DipTimer = 0.0f;
    float NormalBlackoutTimer = 0.0f;
    float ColorTimer = 0.0f;
    FLinearColor TargetColor = FLinearColor::White;

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
    
    /** Sekans öncesi durumu (geri yükleme için) */
    float PreSequenceIntensity = 1.0f;
    FLinearColor PreSequenceColor = FLinearColor::White;
    bool bPreSequenceNormalFlickerActive = false;
    bool bPreSequenceBlackoutActive = false;

    /** Zaman çizelgesi durumu */
	bool bTimelinePaused = false;
	FName CurrentTimelineName = NAME_None;
	TArray<FFlickerTimelineKey> ActiveTimelineKeys;
	int32 CurrentKeyIndex = 0;
	float CurrentTimelineTime = 0.0f;
	float TimelineDuration = 0.0f;
	bool bTimelineLoop = false;
	bool bTimelineRestore = true;
	
    /** Zaman çizelgesi öncesi durumu (geri yükleme için) */
	float PreTimelineIntensity = 1.0f;
	FLinearColor PreTimelineColor = FLinearColor::White;
	bool bPreTimelineNormalFlickerActive = false;
	bool bPreTimelineBlackoutActive = false;
    
    // PERFORMANS
    
    float TickInterval = 0.0f;
    float TimeSinceLastTick = 0.0f;
    
    // TAHMİN
    
    FFlickerPredictionState PredictionState;
    float TimeSinceLastSync = 0.0f;
    
    // EŞZAMANLAMA
    
    FTimerHandle StateSyncTimerHandle;
    bool bOldBlackoutState = false;
	bool bOldNormalFlickerActive = false;
	bool bOldSequenceActive = false;
	bool bOldTimelineActive = false;
	float bOldIntensity = 1.0f;
	FLinearColor bOldColor = FLinearColor::White;
    
    // IŞIKLAR
    
    bool bClientLightsCached = false;
    TSet<FName> DisabledLights;

	// YARDIMCI İŞLEVLER

	bool IsClientWithinDistance(float Radius) const;
	void AutoConfigureBaseIntensity();
};