// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystem/Effect/WxEffect_Cooldown.h"
#include "AbilitySystem/Effect/WxEffect_Cost.h"
#include "AbilitySystem/Ability/WxAbilityTableRow.h"
#include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameplayEffect.h"
#include "InputAction.h"
#include "WxGameplayTags.h"
#include "Weapon/WxProjectileBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "TimerManager.h"

UWxAbilityBase::UWxAbilityBase()
{
	InstancingPolicy  = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// 기본값을 공용 GE 마커로 둔다. 마커 그대로면 프로젝트 방식(AbilityDataRow 기반), 다른 GE로 바꾸면 커스텀(엔진 순정 경로).
	CooldownGameplayEffectClass = UWxEffect_Cooldown::StaticClass();
	CostGameplayEffectClass = UWxEffect_Cost::StaticClass();
}

bool UWxAbilityBase::IsActivationInput(const UInputAction* Action) const
{
	return Action && Action == ActivationInputAction;
}

void UWxAbilityBase::GetInputActions(TArray<const UInputAction*>& OutActions) const
{
	if (ActivationInputAction)
	{
		OutActions.AddUnique(ActivationInputAction);
	}
}

TSoftObjectPtr<UTexture2D> UWxAbilityBase::GetIcon() const
{
	const FWxAbilityTableRow* Row = GetTableRow();
	return Row ? Row->Icon : nullptr;
}

float UWxAbilityBase::GetMontagePlayRate() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return 1.f;
	}

	const UWxCombatAttributeSet* AttrSet = ASC->GetSet<UWxCombatAttributeSet>();
	if (!AttrSet)
	{
		return 1.f;
	}

	return FMath::Max(AttrSet->GetASPD(), 0.01f);
}

void UWxAbilityBase::StartRecovery()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		// 자기 자신이 건 차단만 정확히 해제한다(전역 스냅샷 아님). ref-count는 0에서 클램프되므로 EndAbility의 중복 해제도 무해.
		ASC->UnBlockAbilitiesWithTags(BlockAbilitiesWithTag);
	}
}

void UWxAbilityBase::SpawnProjectile(TSubclassOf<AWxProjectileBase> ProjectileClass, FName SpawnSocketName) const
{
	if (!ProjectileClass)
	{
		return;
	}

	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	USkeletalMeshComponent* Mesh = ActorInfo ? ActorInfo->SkeletalMeshComponent.Get() : nullptr;

	// 투사체는 서버가 스폰해 복제한다. 클라(예측 인스턴스)에선 authority 게이트로 무동작.
	if (!Avatar || !Mesh || !Avatar->HasAuthority())
	{
		return;
	}

	const FVector SpawnLocation = Mesh->GetSocketLocation(SpawnSocketName);
	const FRotator SpawnRotation = Avatar->GetActorRotation();
	const FTransform SpawnTransform(SpawnRotation, SpawnLocation);

	// 대미지는 투사체가 자기 클래스 데이터로 BeginPlay에서 준비하므로 일반 SpawnActor로 충분하다.
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Avatar;
	SpawnParams.Instigator = Cast<APawn>(Avatar);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	Avatar->GetWorld()->SpawnActor<AWxProjectileBase>(ProjectileClass, SpawnTransform, SpawnParams);
}

#if WITH_EDITOR
bool UWxAbilityBase::CanEditChange(const FProperty* InProperty) const
{
	if (!Super::CanEditChange(InProperty))
	{
		return false;
	}
	
	if (InProperty)
	{
		const FName PropertyName = InProperty->GetFName();

		// 스톡 GE 클래스 경로는 AbilityDataRow와 상호배타다. Row가 설정돼 있으면 스톡 클래스 편집을 막는다.
		static const FName CooldownGEName = TEXT("CooldownGameplayEffectClass");
		static const FName CostGEName = TEXT("CostGameplayEffectClass");
		if ((PropertyName == CooldownGEName || PropertyName == CostGEName) && !AbilityDataRow.IsNull())
		{
			return false;
		}
	}

	return true;
}

void UWxAbilityBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(UWxAbilityBase, AbilityDataRow))
	{
		if (!AbilityDataRow.IsNull())
		{
			// Row = 프로젝트 방식이므로 커스텀 GE를 걷어내고 마커로 되돌린다("Row = 마커 표시" 불변식 유지).
			CooldownGameplayEffectClass = UWxEffect_Cooldown::StaticClass();
			CostGameplayEffectClass = UWxEffect_Cost::StaticClass();
		}
	}
}

#endif

void UWxAbilityBase::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	if (ActivationPolicy == EWxAbilityActivationPolicy::OnGranted)
	{
		ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle);
	}
}

UGameplayEffect* UWxAbilityBase::GetCooldownGameplayEffect() const
{
	if (CooldownGameplayEffectClass && CooldownGameplayEffectClass != UWxEffect_Cooldown::StaticClass())
	{
		return Super::GetCooldownGameplayEffect();
	}

	const FWxAbilityTableRow* Row = GetTableRow();
	if (!Row || Row->CooldownTime <= 0.f)
	{
		return nullptr;
	}

	// 단일 충전이면 공유 CDO로 충분하다(ViewModel이 Max(1, StackLimitCount)=1로 읽는다). per-ability 인스턴스는 다중 충전에서만 StackLimitCount 전달용으로 만든다.
	const int32 MaxRecharges = FMath::Max(1, Row->MaxRecharges);
	if (MaxRecharges <= 1)
	{
		return Super::GetCooldownGameplayEffect();
	}

	if (!CooldownEffect)
	{
		CooldownEffect = NewObject<UWxEffect_Cooldown>(const_cast<UWxAbilityBase*>(this), TEXT("CooldownEffect"));
	}

	CooldownEffect->StackLimitCount = MaxRecharges;
	return CooldownEffect;
}

bool UWxAbilityBase::CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (CooldownGameplayEffectClass && CooldownGameplayEffectClass != UWxEffect_Cooldown::StaticClass())
	{
		return Super::CheckCooldown(Handle, ActorInfo, OptionalRelevantTags);
	}

	const FWxAbilityTableRow* Row = GetTableRow();
	if (!Row || Row->CooldownTime <= 0.f)
	{
		return true;
	}

	const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		return true;
	}

	float LongestRemaining = 0.f;
	float LongestDuration = 0.f;
	if (QueryActiveCooldowns(*ASC, LongestRemaining, LongestDuration) >= FMath::Max(1, Row->MaxRecharges))
	{
		// 엔진 순정 CheckCooldown과 동일하게 실패 사유 태그를 채워 OnAbilityFailed 파이프라인(실패 피드백 UI 등)에 전달한다
		if (OptionalRelevantTags)
		{
			const FGameplayTag& FailCooldownTag = UAbilitySystemGlobals::Get().ActivateFailCooldownTag;
			if (FailCooldownTag.IsValid())
			{
				OptionalRelevantTags->AddTag(FailCooldownTag);
			}
		}
		return false;
	}

	return true;
}

float UWxAbilityBase::GetCooldownTimeRemaining(const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (CooldownGameplayEffectClass && CooldownGameplayEffectClass != UWxEffect_Cooldown::StaticClass())
	{
		return Super::GetCooldownTimeRemaining(ActorInfo);
	}

	float TimeRemaining = 0.f;
	float Duration = 0.f;
	if (const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		QueryActiveCooldowns(*ASC, TimeRemaining, Duration);
	}
	return TimeRemaining;
}

void UWxAbilityBase::GetCooldownTimeRemainingAndDuration(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, float& TimeRemaining, float& CooldownDuration) const
{
	if (CooldownGameplayEffectClass && CooldownGameplayEffectClass != UWxEffect_Cooldown::StaticClass())
	{
		Super::GetCooldownTimeRemainingAndDuration(Handle, ActorInfo, TimeRemaining, CooldownDuration);
		return;
	}

	TimeRemaining = 0.f;
	CooldownDuration = 0.f;
	if (const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		QueryActiveCooldowns(*ASC, TimeRemaining, CooldownDuration);
	}
}

void UWxAbilityBase::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 어빌리티가 끝나면 히트스톱 복원 타이머를 정리한다. 취소·블렌드아웃된 몽타주에 뒤늦은 복원이 닿지 않게 한다.
	if (AActor* Avatar = GetAvatarActorFromActorInfo())
	{
		Avatar->GetWorldTimerManager().ClearTimer(HitStopResumeTimer);
	}
	HitStopListenerTask = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWxAbilityBase::StartHitStopListener()
{
	// 콤보 재발동 등으로 이전 활성화의 리스너가 남아 있으면 교체한다.
	if (HitStopListenerTask)
	{
		HitStopListenerTask->EndTask();
		HitStopListenerTask = nullptr;
	}

	// OnlyTriggerOnce=false: 한 몽타주 안 여러 적중마다 히트스톱을 받는다.
	HitStopListenerTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, WxGameplayTags::Event_HitStop, nullptr, false);
	HitStopListenerTask->EventReceived.AddDynamic(this, &UWxAbilityBase::HandleHitStopEvent);
	HitStopListenerTask->ReadyForActivation();
}

const FWxAbilityTableRow* UWxAbilityBase::GetTableRow() const
{
	if (AbilityDataRow.IsNull())
	{
		return nullptr;
	}
	return AbilityDataRow.GetRow<FWxAbilityTableRow>(TEXT("WxAbilityBase::GetDataRow"));
}

void UWxAbilityBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	for (const FWxAbilityEffect& Effect : OnActivateEffects)
	{
		if (Effect.EffectClass)
		{
			FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(Effect.EffectClass, GetAbilityLevel());
			if (SpecHandle.IsValid())
			{
				for (const auto& [Tag, Value] : Effect.SetByCallers)
				{
					SpecHandle.Data->SetSetByCallerMagnitude(Tag, Value);
				}
				ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
			}
		}
	}
}

int32 UWxAbilityBase::QueryActiveCooldowns(const UAbilitySystemComponent& ASC, float& OutLongestRemaining, float& OutLongestDuration) const
{
	OutLongestRemaining = 0.f;
	OutLongestDuration = 0.f;

	const UGameplayAbility* AbilityCDO = GetClass()->GetDefaultObject<UGameplayAbility>();
	const float WorldTime = ASC.GetWorld()->GetTimeSeconds();

	FGameplayEffectQuery Query;
	Query.EffectDefinition = UWxEffect_Cooldown::StaticClass();

	int32 ActiveCount = 0;
	for (const FActiveGameplayEffectHandle& ActiveHandle : ASC.GetActiveEffects(Query))
	{
		const FActiveGameplayEffect* ActiveGE = ASC.GetActiveGameplayEffect(ActiveHandle);
		if (!ActiveGE || ActiveGE->Spec.GetEffectContext().GetAbility() != AbilityCDO)
		{
			continue;
		}

		// 만료됐지만 아직 제거되지 않은 GE(클라이언트는 제거가 리플리케이션으로 도착할 때까지 지연됨)는 회복된 충전으로 취급한다
		const float Remaining = (ActiveGE->StartWorldTime + ActiveGE->Spec.GetDuration()) - WorldTime;
		if (Remaining <= 0.f)
		{
			continue;
		}

		++ActiveCount;
		if (Remaining > OutLongestRemaining)
		{
			OutLongestRemaining = Remaining;
			OutLongestDuration = ActiveGE->Spec.GetDuration();
		}
	}

	return ActiveCount;
}

void UWxAbilityBase::HandleHitStopEvent(FGameplayEventData Payload)
{
	const float Duration = Payload.EventMagnitude;
	if (Duration <= 0.f)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!ASC || !Avatar)
	{
		return;
	}

	// 이 어빌리티가 더 이상 재생 중인 몽타주의 주인이 아니면(예: 패리 반응이 몽타주를 가로챈 경우) 히트스톱을 적용하지 않는다.
	// CurrentMontageSetPlayRate는 ASC의 현재 몽타주를 건드리므로, 남의 몽타주를 0.001로 얼려 영구 정지시키는 것을 막는다.
	if (ASC->GetAnimatingAbility() != this)
	{
		return;
	}

	// 재생 중인 자기 몽타주를 거의 정지시킨다. 완전한 0이 아닌 미세 값으로 두어 몽타주 진행 판정 이슈를 피한다.
	ASC->CurrentMontageSetPlayRate(0.001f);

	// 연속 적중이면 타이머를 재설정해 조기 복원을 막는다.
	Avatar->GetWorldTimerManager().SetTimer(HitStopResumeTimer, this, &UWxAbilityBase::ResumeFromHitStop, Duration, false);
}

void UWxAbilityBase::ResumeFromHitStop()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		// 하드코딩 1.0이 아니라 ASPD가 반영된 재생률로 복원한다.
		ASC->CurrentMontageSetPlayRate(GetMontagePlayRate());
	}
}
