// =====================================================================
// Bu dosya, ışık titreme bileşeninin tam uygulamasını içerir.
// =====================================================================

#include "HCSFlickerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

// =====================================================================
// YAPICI
// =====================================================================

UHCSFlickerComponent::UHCSFlickerComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
    
    SetIsReplicatedByDefault(true);
    SetTickOptimization();
}

// =====================================================================
// SANAL İŞLEVLER
// =====================================================================

void UHCSFlickerComponent::BeginPlay()
{
    Super::BeginPlay();
    
    SetComponentTickEnabled(false);
    SetTickOptimization();
    
    if (GetOwnerRole() == ROLE_Authority)
    {
        UE_LOG(LogTemp, Verbose, TEXT("SUNUCU - Bileşen başlatıldı"));
        
        // Düzenli eşzamanlama
        if (bEnablePeriodicSync)
        {
            GetWorld()->GetTimerManager().SetTimer(StateSyncTimerHandle, this, &UHCSFlickerComponent::ForceStateSync, SyncInterval, true);
        }
    }
    else
    {
        UE_LOG(LogTemp, Verbose, TEXT("İSTEMCİ - Bileşen başlatıldı"));
    }
    
    CurrentColor = BaseColor;
    TargetColor = BaseColor;
    
    CacheLights();
}

void UHCSFlickerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Zamanlayıcılar temizlenir
    if (StateSyncTimerHandle.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(StateSyncTimerHandle);
    }
    
    Super::EndPlay(EndPlayReason);
}

void UHCSFlickerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    // İstemcide ışıklar saklanmamışsa saklanır
    if (GetOwnerRole() < ROLE_Authority && !bClientLightsCached)
    {
        CacheLights();
    }
    
    // Hiçbir şey etkin değilse çıkılır
    if (!bSequenceActive && !bNormalFlickerActive)
    {
        return;
    }
    
    TimeSinceLastTick += DeltaTime;
    
    if (TimeSinceLastTick >= TickInterval)
    {
        if (GetOwnerRole() == ROLE_Authority)
        {
            // ========== SUNUCU: HESAPLAMALAR ==========
            if (bSequenceActive)
            {
                UpdateSequence(TimeSinceLastTick);
            }
            else if (bNormalFlickerActive)
            {
                UpdateNormalFlicker(TimeSinceLastTick);
            }
            
            // Olaylar tetiklenir
            OnIntensityUpdated.Broadcast(CurrentIntensity, CurrentIntensity);
            OnColorUpdated.Broadcast(CurrentColor);
            
            // Işıklar sunucuda güncellenir
            UpdateAllLights(CurrentIntensity, CurrentColor);
            
            // İstemcilere gönderilir
            Multicast_UpdateState(CurrentIntensity, CurrentColor);
        }
        else
        {
            // ========== İSTEMCİ: SADECE GÖRSELLEŞTİRME ==========
            // Zaten çoklu yayın ile gelen değerler uygulanır
            UpdateAllLights(CurrentIntensity, CurrentColor);
        }
        
        TimeSinceLastTick = 0.0f;
    }
    
    // İstemci tahmini
    if (GetOwnerRole() < ROLE_Authority && bEnableClientPrediction)
    {
        TimeSinceLastSync += DeltaTime;
        
        if (TimeSinceLastSync >= SyncInterval * 2.0f)
        {
            ValidatePrediction();
            TimeSinceLastSync = 0.0f;
        }
    }
}

void UHCSFlickerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME_CONDITION(UHCSFlickerComponent, bNormalFlickerActive, COND_None);
    DOREPLIFETIME_CONDITION(UHCSFlickerComponent, bBlackoutActive, COND_None);
    DOREPLIFETIME_CONDITION(UHCSFlickerComponent, bSequenceActive, COND_None);
    DOREPLIFETIME_CONDITION(UHCSFlickerComponent, CurrentSequenceName, COND_None);
    DOREPLIFETIME_CONDITION(UHCSFlickerComponent, CurrentSettings, COND_None);
    DOREPLIFETIME_CONDITION(UHCSFlickerComponent, BaseIntensity, COND_None);
    DOREPLIFETIME_CONDITION(UHCSFlickerComponent, BaseColor, COND_None);
    DOREPLIFETIME_CONDITION(UHCSFlickerComponent, CurrentIntensity, COND_None);
    DOREPLIFETIME_CONDITION(UHCSFlickerComponent, CurrentColor, COND_None);
}

// =====================================================================
// AÇIK İŞLEVLER - TEMEL
// =====================================================================

void UHCSFlickerComponent::StartFlicker()
{
    if (bNormalFlickerActive) return;
    if (GetOwnerRole() == ROLE_Authority)
    {
        bNormalFlickerActive = true;
        CurrentIntensity = 1.0f;
        CurrentColor = BaseColor;
        
        OnRep_NormalFlickerActive();
        SetComponentTickEnabled(true);
        UpdateAllLights(CurrentIntensity, CurrentColor);
        Multicast_UpdateState(CurrentIntensity, CurrentColor);
        
        if (bReplicateFlickerState && GetOwner())
        {
            GetOwner()->ForceNetUpdate();
        }
        
        UE_LOG(LogTemp, Verbose, TEXT("SUNUCU - Normal titreme başlatıldı"));
    }
    else
    {
        ServerStartFlicker();
    }
}

void UHCSFlickerComponent::StopFlicker()
{
    if (!bNormalFlickerActive) return;
    if (GetOwnerRole() == ROLE_Authority)
    {
        bNormalFlickerActive = false;
        OnRep_NormalFlickerActive();
        
        if (!bSequenceActive)
        {
            SetComponentTickEnabled(false);
        }
        
        if (bReplicateFlickerState && GetOwner())
        {
            GetOwner()->ForceNetUpdate();
        }
        
        UE_LOG(LogTemp, Verbose, TEXT("SUNUCU - Normal titreme durduruldu"));
    }
    else
    {
        ServerStopFlicker();
    }
}

// =====================================================================
// AÇIK İŞLEVLER - SEKANS
// =====================================================================

void UHCSFlickerComponent::PlayFlickerSequence(int32 FlickerCount, float FlickerDuration, float Intensity)
{
    if (FlickerCount <= 0)
    {
        return;
    }
    
    // Dinamik sekans oluşturulur
    FFlickerSequence sequence;
    sequence.SequenceName = FName(*FString::Printf(TEXT("Dynamic_%d"), FlickerCount));
    sequence.bRestorePreviousState = true;
    sequence.bLoop = false;
    
    FFlickerSequenceStep step;
    step.FlickerCount = FlickerCount;
    step.FlickerDuration = FlickerDuration;
    step.PauseBetweenFlickers = FlickerDuration * 0.5f;
    step.TargetIntensity = Intensity;
    step.TargetColor = FLinearColor::Black;
    
    sequence.Steps.Add(step);
    
    if (GetOwnerRole() == ROLE_Authority)
    {
        ServerPlaySequence_Implementation(sequence);
    }
    else
    {
        ServerPlaySequence(sequence);
    }
}

void UHCSFlickerComponent::PlayNamedSequence(FName SequenceName)
{
    FFlickerSequence* foundSequence = nullptr;
    
    for (FFlickerSequence& sequence : Sequences)
    {
        if (sequence.SequenceName == SequenceName)
        {
            foundSequence = &sequence;
            break;
        }
    }
    
    if (!foundSequence)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s sekansı bulunamadı!"), *SequenceName.ToString());
        return;
    }
    
    if (GetOwnerRole() == ROLE_Authority)
    {
        ServerPlaySequence_Implementation(*foundSequence);
    }
    else
    {
        ServerPlaySequence(*foundSequence);
    }
}

void UHCSFlickerComponent::StopSequence()
{
    if (!bSequenceActive) return;
    if (GetOwnerRole() == ROLE_Authority)
    {
        ServerStopSequence_Implementation();
    }
    else
    {
        ServerStopSequence();
    }
}

EFlickerMode UHCSFlickerComponent::GetCurrentMode() const
{
    if (bSequenceActive) return EFlickerMode::Sequence;
    if (bNormalFlickerActive) return EFlickerMode::NormalFlicker;
    return EFlickerMode::Inactive;
}

// =====================================================================
// AÇIK İŞLEVLER - IŞIKLAR
// =====================================================================

void UHCSFlickerComponent::SetBaseIntensity(float Intensity)
{
    if (BaseIntensity != Intensity)
    {
        BaseIntensity = Intensity;
        
        if (GetOwnerRole() != ROLE_Authority)
        {
            ServerSetBaseIntensity(Intensity);
        }
    }
}

void UHCSFlickerComponent::SetBaseColor(FLinearColor Color)
{
    if (BaseColor != Color)
    {
        BaseColor = Color;
        CurrentColor = Color;
        TargetColor = Color;
        
        if (GetOwnerRole() != ROLE_Authority)
        {
            ServerSetBaseColor(Color);
        }
    }
}

void UHCSFlickerComponent::AddLightSettings(FName LightTag, const FFlickerLightSettings& Settings)
{
    RemoveLightSettings(LightTag);
    PerLightSettings.Add(Settings);
}

void UHCSFlickerComponent::RemoveLightSettings(FName LightTag)
{
    for (int32 i = PerLightSettings.Num() - 1; i >= 0; i--)
    {
        if (PerLightSettings[i].LightTag == LightTag)
        {
            PerLightSettings.RemoveAt(i);
            break;
        }
    }
}

void UHCSFlickerComponent::ClearLightSettings()
{
    PerLightSettings.Empty();
}

void UHCSFlickerComponent::SetLightEnabled(FName LightTag, bool bEnabled)
{
    if (bEnabled)
    {
        DisabledLights.Remove(LightTag);
    }
    else
    {
        DisabledLights.Add(LightTag);
    }
}

// =====================================================================
// AÇIK İŞLEVLER - AYARLAR
// =====================================================================

void UHCSFlickerComponent::SetSettings(const FFlickerSettings& Settings)
{
    if (CurrentSettings != Settings)
    {
        CurrentSettings = Settings;
        
        if (GetOwnerRole() != ROLE_Authority)
        {
            ServerSetSettings(Settings);
        }
    }
}

void UHCSFlickerComponent::ResetSettings()
{
    SetSettings(FFlickerSettings());
}

float UHCSFlickerComponent::UpdateFlicker() const
{
    if (!bSequenceActive && !bNormalFlickerActive)
    {
        return 1.0f;
    }
    
    return CurrentIntensity;
}

FLinearColor UHCSFlickerComponent::UpdateFlickerColor() const
{
    if (!bSequenceActive && !bNormalFlickerActive)
    {
        return BaseColor;
    }
    
    return CurrentColor;
}

// =====================================================================
// ÇOĞALTMA ÇAĞRILARI
// =====================================================================

void UHCSFlickerComponent::OnRep_NormalFlickerActive()
{
    if (bNormalFlickerActive)
    {
        SetComponentTickEnabled(true);
        OnFlickerStarted.Broadcast();
    }
    else
    {
        if (!bSequenceActive)
        {
            SetComponentTickEnabled(false);
        }
        
        OnFlickerStopped.Broadcast();
    }
}

void UHCSFlickerComponent::OnRep_BlackoutActive()
{
    if (bEnableClientPrediction)
    {
        PredictionState.bPredictedBlackout = false;
    }
    
    if (bBlackoutActive)
    {
        OnBlackoutStarted.Broadcast();
    }
    else
    {
        OnBlackoutEnded.Broadcast();
    }
}

void UHCSFlickerComponent::OnRep_SequenceActive()
{
    if (bSequenceActive)
    {
        SetComponentTickEnabled(true);
        OnSequenceStarted.Broadcast();
    }
    else
    {
        if (!bNormalFlickerActive)
        {
            SetComponentTickEnabled(false);
        }
        
        OnSequenceFinished.Broadcast();
    }
}

void UHCSFlickerComponent::OnRep_CurrentSettings()
{
    // Ayarlar değişti - gerekirse ek işlemler yapılır
}

void UHCSFlickerComponent::OnRep_BaseColor()
{
    CurrentColor = BaseColor;
    TargetColor = BaseColor;
    OnColorUpdated.Broadcast(BaseColor);
}

void UHCSFlickerComponent::OnRep_CurrentIntensity()
{
    // İstemcide yoğunluk güncellendi
    if (AllLights.IsEmpty())
    {
        CacheLights();
    }
    
    UpdateAllLights(CurrentIntensity, CurrentColor);
}

void UHCSFlickerComponent::OnRep_CurrentColor()
{
    // İstemcide renk güncellendi
    if (AllLights.IsEmpty())
    {
        CacheLights();
    }
    
    UpdateAllLights(CurrentIntensity, CurrentColor);
    OnColorUpdated.Broadcast(CurrentColor);
}

// =====================================================================
// SUNUCU RPC UYGULAMALARI
// =====================================================================

void UHCSFlickerComponent::ServerStartFlicker_Implementation()
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) 
    {
        return;
    }
    
    StartFlicker();
}

bool UHCSFlickerComponent::ServerStartFlicker_Validate()
{
    return GetOwner() != nullptr;
}

void UHCSFlickerComponent::ServerStopFlicker_Implementation()
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) 
    {
        return;
    }
    
    StopFlicker();
}

bool UHCSFlickerComponent::ServerStopFlicker_Validate()
{
    return GetOwner() != nullptr;
}

void UHCSFlickerComponent::ServerSetBaseIntensity_Implementation(float Intensity)
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) 
    {
        return;
    }
    
    SetBaseIntensity(Intensity);
}

bool UHCSFlickerComponent::ServerSetBaseIntensity_Validate(float Intensity)
{
    return GetOwner() != nullptr && Intensity >= 0.0f;
}

void UHCSFlickerComponent::ServerSetBaseColor_Implementation(const FLinearColor& Color)
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) 
    {
        return;
    }
    
    SetBaseColor(Color);
}

bool UHCSFlickerComponent::ServerSetBaseColor_Validate(const FLinearColor& Color)
{
    return GetOwner() != nullptr;
}

void UHCSFlickerComponent::ServerSetSettings_Implementation(const FFlickerSettings& Settings)
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) 
    {
        return;
    }
    
    SetSettings(Settings);
}

bool UHCSFlickerComponent::ServerSetSettings_Validate(const FFlickerSettings& Settings)
{
    if (!GetOwner())
    {
        return false;
    }
    
    return Settings.MicroJitterStrength >= 0.0f && Settings.MicroJitterStrength <= 2.0f &&
           Settings.DipChanceMultiplier >= 0.0f && Settings.DipChanceMultiplier <= 5.0f &&
           Settings.DipStrength >= 0.1f && Settings.DipStrength <= 5.0f &&
           Settings.BlackoutChanceMultiplier >= 0.0f && Settings.BlackoutChanceMultiplier <= 5.0f &&
           Settings.ColorChangeSpeed >= 0.0f && Settings.ColorChangeSpeed <= 5.0f &&
           Settings.ColorVariationStrength >= 0.0f && Settings.ColorVariationStrength <= 1.0f;
}

void UHCSFlickerComponent::ServerPlaySequence_Implementation(const FFlickerSequence& Sequence)
{
    AActor* owner = GetOwner();
    
    if (!owner || !owner->HasAuthority())
    {
        return;
    }
    
    // Mevcut durum saklanır
    SavePreSequenceState();
    
    // Varsa eski sekans durdurulur
    if (bSequenceActive)
    {
        StopSequence();
    }
    
    // Yeni sekans başlatılır
    ActiveSequence = Sequence;
    CurrentSequenceName = Sequence.SequenceName;
    bCurrentSequenceRestoreState = Sequence.bRestorePreviousState;
    bCurrentSequencePlaySound = Sequence.bPlaySoundOnEachFlicker; 
    bSequenceActive = true;
    CurrentStepIndex = 0;
    CurrentFlickerInStep = 0;
    SequenceStepTimer = 0.0f;
    SequenceFlickerTimer = 0.0f;
    bSequenceFlickerState = true;
    
    // İlk yoğunluk ve renk
    if (Sequence.Steps.Num() > 0)
    {
        CurrentIntensity = Sequence.Steps[0].TargetIntensity;
        CurrentColor = Sequence.Steps[0].TargetColor;
    }
    
    // Olaylar tetiklenir
    OnSequenceStarted.Broadcast();
    Multicast_SequenceStateChanged(true, CurrentSequenceName);
    Multicast_UpdateState(CurrentIntensity, CurrentColor);
    
    // Tick etkin kılınır
    SetComponentTickEnabled(true);
    UpdateAllLights(CurrentIntensity, CurrentColor);
    
    // Ağ güncellemesi
    owner->ForceNetUpdate();
    
    UE_LOG(LogTemp, Verbose, TEXT("SUNUCU - Sekans başlatıldı: %s"), *CurrentSequenceName.ToString());
}

bool UHCSFlickerComponent::ServerPlaySequence_Validate(const FFlickerSequence& Sequence)
{
    if (!GetOwner()) return false;
    if (Sequence.Steps.IsEmpty())
    {
        return false;
    }
    
    for (const FFlickerSequenceStep& step : Sequence.Steps)
    {
        if (step.FlickerCount <= 0 || step.FlickerDuration <= 0.0f)
        {
            return false;
        }
    }
    
    return true;
}

void UHCSFlickerComponent::ServerStopSequence_Implementation()
{
    AActor* owner = GetOwner();
    
    if (!owner || !owner->HasAuthority()) return;
    if (bCurrentSequenceRestoreState)
    {
        RestorePreSequenceState();
    }
    else
    {
        // Varsayılan değerlere dönülür
        CurrentIntensity = 1.0f;
        CurrentColor = BaseColor;
        bNormalFlickerActive = false;
    }
    
    // Sekans durdurulur
    bSequenceActive = false;
    CurrentSequenceName = NAME_None;
    
    // Olaylar tetiklenir
    OnSequenceFinished.Broadcast();
    Multicast_SequenceStateChanged(false, NAME_None);
    Multicast_UpdateState(CurrentIntensity, CurrentColor);
    
    // Işıklar güncellenir
    UpdateAllLights(CurrentIntensity, CurrentColor);
    
    // Tick'i kapat (eğer normal titreme de yoksa)
    if (!bNormalFlickerActive)
    {
        SetComponentTickEnabled(false);
    }
    
    // Ağ güncellemesi
    owner->ForceNetUpdate();

    UE_LOG(LogTemp, Verbose, TEXT("SUNUCU - Sekans durduruldu"));
}

bool UHCSFlickerComponent::ServerStopSequence_Validate()
{
    return GetOwner() != nullptr;
}

// =====================================================================
// ÇOKLU YAYIN RPC UYGULAMALARI
// =====================================================================

void UHCSFlickerComponent::Multicast_PlaySound_Implementation(USoundBase* Sound)
{
    if (!bEnableBuiltInAudio || !Sound) 
    {
        return;
    }
    
    AActor* owner = GetOwner();
    
    if (!owner || !owner->GetRootComponent()) 
    {
        return;
    }
    
    UGameplayStatics::SpawnSoundAttached(Sound, owner->GetRootComponent(), NAME_None, FVector::ZeroVector,
        EAttachLocation::SnapToTarget, true, SoundVolumeMultiplier, SoundPitchMultiplier,
        0.0f, SoundAttenuation, nullptr, true);
}

void UHCSFlickerComponent::Multicast_PlaySoundNearby_Implementation(USoundBase* Sound, float Radius)
{
    if (!bEnableBuiltInAudio || !Sound || !IsClientWithinDistance(Radius)) 
    {
        return;
    }
    
    AActor* owner = GetOwner();
    
    if (!owner || !owner->GetRootComponent()) 
    {
        return;
    }
    
    UGameplayStatics::SpawnSoundAttached(Sound, owner->GetRootComponent(), NAME_None, FVector::ZeroVector,
        EAttachLocation::SnapToTarget, true, SoundVolumeMultiplier, SoundPitchMultiplier,
        0.0f, SoundAttenuation, nullptr, true);
}

void UHCSFlickerComponent::Multicast_BlackoutStateChanged_Implementation(bool bNewBlackoutActive)
{
    if (bEnableClientPrediction && GetOwnerRole() < ROLE_Authority)
    {
        PredictionState.bPredictedBlackout = false;
    }
    
    SetBlackoutActive(bNewBlackoutActive, true);
}

void UHCSFlickerComponent::Multicast_SequenceStateChanged_Implementation(bool bNewSequenceActive, FName SequenceName)
{
    bSequenceActive = bNewSequenceActive;
    CurrentSequenceName = SequenceName;
    
    if (bNewSequenceActive)
    {
        OnSequenceStarted.Broadcast();
    }
    else
    {
        OnSequenceFinished.Broadcast();
    }
}

void UHCSFlickerComponent::Multicast_UpdateState_Implementation(float Intensity, const FLinearColor& Color)
{
    // İstemcide durum güncellenir
    CurrentIntensity = Intensity;
    CurrentColor = Color;
    
    // Işıklar hemen güncellenir
    if (AllLights.IsEmpty())
    {
        CacheLights();
    }
    
    UpdateAllLights(Intensity, Color);
}

// =====================================================================
// İSTEMCİ RPC UYGULAMALARI
// =====================================================================

void UHCSFlickerComponent::Client_PredictionCorrection_Implementation(bool bCorrectedBlackout, float CorrectedIntensity)
{
    if (bEnableClientPrediction)
    {
        if (bCorrectedBlackout != PredictionState.bPredictedBlackout)
        {
            PredictionState.bPredictedBlackout = bCorrectedBlackout;
            CurrentIntensity = CorrectedIntensity;
        }
    }
}

// =====================================================================
// İÇSEL İŞLEVLER - DURUM YÖNETİMİ
// =====================================================================

void UHCSFlickerComponent::SetBlackoutActive(bool bNewActive, bool bFromReplication)
{
    if (bBlackoutActive != bNewActive)
    {
        bBlackoutActive = bNewActive;
        
        if (!bFromReplication)
        {
            OnRep_BlackoutActive();
        }
        
        if (GetOwnerRole() == ROLE_Authority && bReplicateBlackoutState && GetOwner())
        {
            Multicast_BlackoutStateChanged(bNewActive);
            GetOwner()->ForceNetUpdate();
        }
    }
}

void UHCSFlickerComponent::SavePreSequenceState()
{
    PreSequenceIntensity = CurrentIntensity;
    PreSequenceColor = CurrentColor;
    bPreSequenceNormalFlickerActive = bNormalFlickerActive;
    bPreSequenceBlackoutActive = bBlackoutActive;
}

void UHCSFlickerComponent::RestorePreSequenceState()
{
    CurrentIntensity = PreSequenceIntensity;
    CurrentColor = PreSequenceColor;
    bNormalFlickerActive = bPreSequenceNormalFlickerActive;
    bBlackoutActive = bPreSequenceBlackoutActive;
}

// =====================================================================
// İÇSEL İŞLEVLER - TİTREME GÜNCELLEMELERİ
// =====================================================================

void UHCSFlickerComponent::UpdateNormalFlicker(float DeltaSeconds)
{
    DeltaSeconds = FMath::Min(DeltaSeconds, 0.1f);
    
    // ========== MİKRO TİTREME ==========
    MicroJitterTimer += DeltaSeconds;
    
    if (MicroJitterTimer >= 0.08f)
    {
        MicroJitterTimer = 0.0f;
        float jitterRange = 0.3f * CurrentSettings.MicroJitterStrength;
        MicroJitterTarget = FMath::FRandRange(1.0f - jitterRange, 1.0f + jitterRange);
    }
    
    MicroJitterAlpha = FMath::FInterpTo(MicroJitterAlpha, MicroJitterTarget, DeltaSeconds, 10.0f);
    CurrentIntensity = MicroJitterAlpha;
    
    // Yoğunluk eğrisi
    if (FlickerIntensityCurve)
    {
        float curveValue = FlickerIntensityCurve->GetFloatValue(GetWorld()->GetTimeSeconds());
        CurrentIntensity *= curveValue;
    }
    
    // ========== ANİ DÜŞÜŞ ==========
    DipCooldown -= DeltaSeconds;
    
    if (DipCooldown <= 0.0f)
    {
        float dipChance = 0.05f * CurrentSettings.DipChanceMultiplier * DeltaSeconds * 60.0f;
        dipChance = FMath::Clamp(dipChance, 0.0f, 1.0f);
        
        if (FMath::FRand() < dipChance)
        {
            DipTimer = 0.12f;
            DipCooldown = 1.0f;
            
            if (GetOwnerRole() == ROLE_Authority)
            {
                PlayFlickerSound(FlickerDipSound, OnDipSound);
            }
        }
    }
    
    if (DipTimer > 0.0f)
    {
        DipTimer -= DeltaSeconds;
        
        float dipMin = 0.25f / CurrentSettings.DipStrength;
        float dipMax = 0.45f / CurrentSettings.DipStrength;
        
        dipMin = FMath::Clamp(dipMin, 0.0f, 1.0f);
        dipMax = FMath::Clamp(dipMax, 0.0f, 1.0f);
        
        CurrentIntensity *= FMath::FRandRange(dipMin, dipMax);
    }
    
    // ========== KARARTMA ==========
    if (GetOwnerRole() == ROLE_Authority && !bSequenceActive)
    {
        float blackoutChance = 0.01f * CurrentSettings.BlackoutChanceMultiplier * DeltaSeconds * 60.0f;
        blackoutChance = FMath::Clamp(blackoutChance, 0.0f, 1.0f);
        
        if (!bBlackoutActive && FMath::FRand() < blackoutChance)
        {
            SetBlackoutActive(true);
            NormalBlackoutTimer = FMath::FRandRange(0.03f, 0.07f);
            PlayFlickerSound(FlickerBlackoutStartSound, OnBlackoutStartSound);
        }
    }
    
    if (bBlackoutActive)
    {
        NormalBlackoutTimer -= DeltaSeconds;
        CurrentIntensity = 0.0f;
        
        if (GetOwnerRole() == ROLE_Authority && NormalBlackoutTimer <= 0.0f)
        {
            SetBlackoutActive(false);
            PlayFlickerSound(FlickerBlackoutEndSound, OnBlackoutEndSound);
        }
    }
    
    CurrentIntensity = FMath::Clamp(CurrentIntensity, 0.0f, 1.0f);
    
    // ========== RENK ==========
    UpdateColor(DeltaSeconds);
}

void UHCSFlickerComponent::UpdateColor(float DeltaSeconds)
{
    ColorTimer += DeltaSeconds * CurrentSettings.ColorChangeSpeed;
    
    if (ColorTimer >= 1.0f)
    {
        ColorTimer = 0.0f;
        
        float variation = CurrentSettings.ColorVariationStrength;
        TargetColor.R = FMath::Clamp(BaseColor.R + FMath::FRandRange(-variation, variation), 0.0f, 1.0f);
        TargetColor.G = FMath::Clamp(BaseColor.G + FMath::FRandRange(-variation, variation), 0.0f, 1.0f);
        TargetColor.B = FMath::Clamp(BaseColor.B + FMath::FRandRange(-variation, variation), 0.0f, 1.0f);
        TargetColor.A = BaseColor.A;
    }
    
    CurrentColor = FLinearColor::LerpUsingHSV(CurrentColor, TargetColor, DeltaSeconds * 5.0f);
    
    if (bBlackoutActive || (bEnableClientPrediction && PredictionState.bPredictedBlackout))
    {
        CurrentColor = FLinearColor::Black;
    }
    else
    {
        CurrentColor *= CurrentIntensity;
    }
}

void UHCSFlickerComponent::UpdateSequence(float DeltaSeconds)
{
    if (!bSequenceActive || !ActiveSequence.Steps.IsValidIndex(CurrentStepIndex))
    {
        return;
    }
    
    FFlickerSequenceStep& currentStep = ActiveSequence.Steps[CurrentStepIndex];
    
    if (bSequenceFlickerState)
    {
        // TİTREME MODU
        SequenceFlickerTimer += DeltaSeconds;
        CurrentIntensity = currentStep.TargetIntensity;
        CurrentColor = currentStep.TargetColor;

        if (GetOwnerRole() == ROLE_Authority)
        {
            PlayFlickerSound(FlickerDipSound, OnDipSound);
        }
        
        if (SequenceFlickerTimer >= currentStep.FlickerDuration)
        {
            bSequenceFlickerState = false;
            SequenceFlickerTimer = 0.0f;
            CurrentFlickerInStep++;
        }
    }
    else
    {
        // DURAKLAMA MODU
        SequenceStepTimer += DeltaSeconds;
        CurrentIntensity = 1.0f;
        CurrentColor = BaseColor;
        
        if (SequenceStepTimer >= currentStep.PauseBetweenFlickers)
        {
            bSequenceFlickerState = true;
            SequenceStepTimer = 0.0f;
            
            if (CurrentFlickerInStep >= currentStep.FlickerCount)
            {
                CurrentStepIndex++;
                CurrentFlickerInStep = 0;
                
                if (CurrentStepIndex >= ActiveSequence.Steps.Num())
                {
                    if (ActiveSequence.bLoop)
                    {
                        CurrentStepIndex = 0;
                    }
                    else
                    {
                        StopSequence();
                        return;
                    }
                }
                
                OnSequenceStepChanged.Broadcast();
            }
        }
    }
}

FLinearColor UHCSFlickerComponent::CalculateSequenceColor() const
{
    if (bSequenceActive && ActiveSequence.Steps.IsValidIndex(CurrentStepIndex))
    {
        return ActiveSequence.Steps[CurrentStepIndex].TargetColor;
    }
    
    return CurrentColor;
}

// =====================================================================
// İÇSEL İŞLEVLER - IŞIK YÖNETİMİ
// =====================================================================

void UHCSFlickerComponent::CacheLights()
{
    AllLights.Empty();
    
    if (AActor* owner = GetOwner())
    {
        owner->GetComponents<ULightComponent>(AllLights);
        
        for (ULightComponent* light : AllLights)
        {
            if (light && light->ComponentTags.Num() > 0)
            {
                OriginalLightColors.Add(light->ComponentTags[0], light->GetLightColor());
            }
        }
        
        bClientLightsCached = true;
    }
}

void UHCSFlickerComponent::UpdateAllLights(float Intensity, const FLinearColor& Color)
{
    // Işık listesi boşsa saklanır
    if (AllLights.IsEmpty())
    {
        CacheLights();
    }
    
    // Temel yoğunluk güvenliği
    float safeBaseIntensity = BaseIntensity;
    
    if (safeBaseIntensity <= 0.0f)
    {
        safeBaseIntensity = 3000.0f; // Varsayılan değer
    }
    
    // Tüm ışıklar güncellenir
    for (ULightComponent* light : AllLights)
    {
        if (!light)
        {
            continue;
        }
        
        FName lightTag = light->ComponentTags.Num() > 0 ? light->ComponentTags[0] : NAME_None;
        
        // Işık devre dışı mı?
        if (!IsLightEnabled(lightTag))
        {
            continue;
        }
        
        // Özel ayar var mı?
        FFlickerLightSettings* lightSettings = GetLightSettingsForTag(lightTag);
        
        float FinalIntensity = safeBaseIntensity * Intensity;
        FLinearColor FinalColor = Color;
        
        if (lightSettings)
        {
            if (!lightSettings->bUseMasterSettings)
            {
                float lightJitter = FMath::FRandRange(1.0f - 0.3f * lightSettings->MicroJitterStrength, 1.0f + 0.3f * lightSettings->MicroJitterStrength);
                FinalIntensity *= lightJitter;
            }
            
            if (lightSettings->bOverrideColor)
            {
                FinalColor = lightSettings->CustomColor;
            }
        }
        
        // Işık güncellenir
        light->SetIntensity(FinalIntensity);
        light->SetLightColor(FinalColor);
    }
}

FFlickerLightSettings* UHCSFlickerComponent::GetLightSettingsForTag(FName LightTag)
{
    for (FFlickerLightSettings& settings : PerLightSettings)
    {
        if (settings.LightTag == LightTag)
        {
            return &settings;
        }
    }
    
    return nullptr;
}

bool UHCSFlickerComponent::IsLightEnabled(FName LightTag) const
{
    return !DisabledLights.Contains(LightTag);
}

// =====================================================================
// İÇSEL İŞLEVLER - SES YÖNETİMİ
// =====================================================================

void UHCSFlickerComponent::PlayFlickerSound(const TSoftObjectPtr<USoundBase>& Sound, const FFlickerSoundEvent& SoundEvent)
{
    USoundBase* soundPtr = Sound.Get();
    
    if (!soundPtr && Sound.IsPending())
    {
        soundPtr = Sound.LoadSynchronous();
    }
    
    if (!soundPtr) return;
    if (SoundEvent.IsBound())
    {
        SoundEvent.Broadcast(soundPtr);
    }
    
    // Yerleşik ses sistemini kullan
    if (bEnableBuiltInAudio && GetOwnerRole() == ROLE_Authority)
    {
        if (bUseDistanceBasedAudio && AudioDistanceRadius > 0.0f)
        {
            Multicast_PlaySoundNearby(soundPtr, AudioDistanceRadius);
        }
        else
        {
            Multicast_PlaySound(soundPtr);
        }
    }
}

// =====================================================================
// İÇSEL İŞLEVLER - PERFORMANS & EŞZAMANLAMA
// =====================================================================

void UHCSFlickerComponent::SetTickOptimization()
{
    if (TickRate > 0)
    {
        TickInterval = 1.0f / static_cast<float>(TickRate);
    }
    else
    {
        TickInterval = 0.0f;
    }
}

void UHCSFlickerComponent::ForceStateSync()
{
    if (GetOwnerRole() != ROLE_Authority) return;
    if (bOldBlackoutState != bBlackoutActive)
    {
        Multicast_BlackoutStateChanged(bBlackoutActive);
        bOldBlackoutState = bBlackoutActive;
        
        if (AActor* owner = GetOwner())
        {
            owner->ForceNetUpdate();
        }
    }
}

void UHCSFlickerComponent::ValidatePrediction()
{
    if (GetOwnerRole() >= ROLE_Authority || !bEnableClientPrediction) return;
    if (PredictionState.bPredictedBlackout != bBlackoutActive)
    {
        if (!bBlackoutActive)
        {
            PredictionState.bPredictedBlackout = false;
        }
    }
}

bool UHCSFlickerComponent::IsClientWithinDistance(float Radius) const
{
    AActor* owner = GetOwner();
    
    if (!owner)
    {
        return false;
    }
    
    UWorld* world = GetWorld();
    
    if (!world) return false;
    if (!GEngine)
    {
        return false;
    }
    
    APlayerController* playerController = GEngine->GetFirstLocalPlayerController(world);
    
    if (!playerController || !playerController->GetPawn())
    {
        return true;
    }
    
    float distance = FVector::Dist(owner->GetActorLocation(), playerController->GetPawn()->GetActorLocation());
    return distance <= Radius;
}