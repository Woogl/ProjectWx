// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystem/Effect/WxEffect_Cooldown.h"
#include "AbilitySystem/Effect/WxEffect_Cost.h"
#include "AbilitySystem/Ability/WxAbilityTableRow.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "Weapon/WxProjectileBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "WxGameplayTags.h"

UWxAbilityBase::UWxAbilityBase()
{
	InstancingPolicy  = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	CooldownGameplayEffectClass = UWxEffect_Cooldown::StaticClass();
	CostGameplayEffectClass = UWxEffect_Cost::StaticClass();
}

TSoftObjectPtr<UObject> UWxAbilityBase::GetIcon() const
{
	const FWxAbilityTableRow* Row = GetTableRow();
	return Row ? Row->Icon : nullptr;
}

float UWxAbilityBase::GetMontagePlayRate() const
{
	const UWxAbilitySystemComponent* ASC = Cast<UWxAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	return ASC ? ASC->GetMontagePlayRate() : 1.f;
}

void UWxAbilityBase::OpenComboWindow()
{
	// 본동작에서만 연다 — 콤보가 없는 어빌리티의 몽타주에 노티파이가 섞여도 Independent를 점유자로 승격시키지 않는다.
	if (ActivationGroup == EWxAbilityActivationGroup::Exclusive_Blocking)
	{
		ActivationGroup = EWxAbilityActivationGroup::Exclusive_ComboWindow;
	}
}

void UWxAbilityBase::CloseComboWindow()
{
	if (ActivationGroup == EWxAbilityActivationGroup::Exclusive_ComboWindow)
	{
		ActivationGroup = EWxAbilityActivationGroup::Exclusive_Blocking;
	}
}

void UWxAbilityBase::StartRecovery()
{
	// 배타 본동작·콤보 창에서만 후딜로 — 엉뚱한 노티파이가 Independent를 점유자로 승격시키거나 Reaction의 캔슬 면역을 벗기지 않게 한다.
	if (ActivationGroup == EWxAbilityActivationGroup::Exclusive_Blocking || ActivationGroup == EWxAbilityActivationGroup::Exclusive_ComboWindow)
	{
		ActivationGroup = EWxAbilityActivationGroup::Exclusive_Recovery;
	}
}

bool UWxAbilityBase::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	/** 순정 검사에 배타 그룹 판정을 더한다. 점유자가 자기 자신이면 콤보 창으로, 남이면 CancelAbilitiesWithTag 지목으로 가른다. */
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	if (ActivationGroup == EWxAbilityActivationGroup::Independent || ActivationGroup == EWxAbilityActivationGroup::Reaction)
	{
		return true;
	}

	const UWxAbilitySystemComponent* WxASC = ActorInfo ? Cast<UWxAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get()) : nullptr;
	if (!WxASC)
	{
		return true;
	}

	// 반응형은 배타를 뚫고 공존할 수 있으므로 점유자 전원을 각각 통과해야 한다.
	for (const UWxAbilityBase* Blocker : WxASC->FindActivationGroupBlockers())
	{
		// 점유자가 나 자신이면 엔진 재발동으로 들어온 콤보 진행이다. 콤보 창은 후딜보다 이르므로 여기서 갈라야 두 창이 분리된다.
		if (Blocker == this)
		{
			if (ActivationGroup != EWxAbilityActivationGroup::Exclusive_ComboWindow)
			{
				return false;
			}
		}
		// 끊겠다고 지목한 어빌리티는 나를 막지 못한다. 취소 자체는 발동 직후 순정 PreActivate가 수행한다.
		else if (!Blocker->GetAssetTags().HasAny(CancelAbilitiesWithTag))
		{
			return false;
		}
	}

	return true;
}

bool UWxAbilityBase::CanBeCanceled() const
{
	return ActivationGroup != EWxAbilityActivationGroup::Reaction && Super::CanBeCanceled();
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

	if (!Avatar || !Mesh || !Avatar->HasAuthority())
	{
		return;
	}

	const FVector SpawnLocation = Mesh->GetSocketLocation(SpawnSocketName);
	const FRotator SpawnRotation = Avatar->GetActorRotation();
	const FTransform SpawnTransform(SpawnRotation, SpawnLocation);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Avatar;
	SpawnParams.Instigator = Cast<APawn>(Avatar);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	Avatar->GetWorld()->SpawnActor<AWxProjectileBase>(ProjectileClass, SpawnTransform, SpawnParams);
}

void UWxAbilityBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// 직전 활성화의 후딜 전이가 재사용 인스턴스에 남긴 그룹을 선언값으로 되돌린다.
	ActivationGroup = GetClass()->GetDefaultObject<UWxAbilityBase>()->ActivationGroup;

	if (ActivationGroup != EWxAbilityActivationGroup::Independent)
	{
		if (UWxAbilitySystemComponent* WxASC = Cast<UWxAbilitySystemComponent>(ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr))
		{
			// 후딜에 든 앞 액션은 들어온 배타 발동이 끊는다 — 후딜은 지목할 태그가 없어 여기서만 그룹으로 가른다.
			// 본동작 점유를 무엇까지 끊을지는 CancelAbilitiesWithTag 선언이 정하고 순정 PreActivate가 수행한다.
			WxASC->CancelActivationGroupAbilities(EWxAbilityActivationGroup::Exclusive_Recovery, this);
		}
	}

	// 구체 어빌리티가 Super를 먼저 부르므로, 커밋 실패로 곧장 종료하는 경우엔 EndAbility가 같은 프레임에 다시 걷는다.
	for (const TSubclassOf<UGameplayEffect>& EffectClass : ActivationOwnedEffects)
	{
		if (EffectClass)
		{
			FActiveGameplayEffectHandle EffectHandle = ApplyGameplayEffectToOwner(Handle, ActorInfo, ActivationInfo, EffectClass.GetDefaultObject(), GetAbilityLevel());
			ActivationOwnedEffectHandles.Add(EffectHandle);
		}
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UWxAbilityBase::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 태스크를 여기서 끝내면 안 된다 — 엔진 EndAbility가 소유자 종료로 끝내는 경로만 재생 중인 몽타주를 멈추므로, 미리 끊으면 루핑 가드 몽타주처럼 스스로 끝나지 않는 것이 종료 후에도 계속 돈다.
	MontageTask = nullptr;
	ActiveMontage = nullptr;

	// 캔슬·중단도 이 경로를 지나므로 효과가 새지 않는다. 활성 중에 이미 걷힌 것은 조회에 걸리지 않아 무해하다.
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		for (FActiveGameplayEffectHandle EffectHandle : ActivationOwnedEffectHandles)
		{
			ASC->RemoveActiveGameplayEffect(EffectHandle);
		}
	}
	ActivationOwnedEffectHandles.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UWxAbilityBase::PlayMontage(UAnimMontage* Montage, FName StartSection)
{
	if (!Montage)
	{
		return false;
	}

	// EndTask가 AnimInstance 바인딩을 해제하므로 구 태스크의 후속 이벤트는 발송되지 않는다.
	if (MontageTask)
	{
		MontageTask->EndTask();
		MontageTask = nullptr;
	}

	UAbilityTask_PlayMontageAndWait* NewMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, Montage, GetMontagePlayRate(), StartSection, true, 1.f, 0.f, true);
	if (!NewMontageTask)
	{
		return false;
	}

	MontageTask = NewMontageTask;
	ActiveMontage = Montage;

	NewMontageTask->OnCompleted.AddDynamic(this, &UWxAbilityBase::HandleMontageCompleted);
	NewMontageTask->OnBlendOut.AddDynamic(this, &UWxAbilityBase::HandleMontageBlendOut);
	NewMontageTask->OnInterrupted.AddDynamic(this, &UWxAbilityBase::HandleMontageInterrupted);
	NewMontageTask->OnCancelled.AddDynamic(this, &UWxAbilityBase::HandleMontageCancelled);
	NewMontageTask->ReadyForActivation();
	return true;
}

UAnimMontage* UWxAbilityBase::GetActiveMontage() const
{
	return ActiveMontage;
}

void UWxAbilityBase::HandleMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxAbilityBase::HandleMontageBlendOut()
{
}

void UWxAbilityBase::HandleMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxAbilityBase::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxAbilityBase::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	if (ActivationPolicy == EWxAbilityActivationPolicy::OnGiven)
	{
		if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
		{
			ASC->TryActivateAbility(Spec.Handle);
		}
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

	// 단일 충전은 공유 CDO로 충분하다.
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

void UWxAbilityBase::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (CooldownGameplayEffectClass && CooldownGameplayEffectClass != UWxEffect_Cooldown::StaticClass())
	{
		// 커스텀 쿨다운 클래스를 적용하는 경우
		Super::ApplyCooldown(Handle, ActorInfo, ActivationInfo);
		return;
	}

	const UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();
	const FWxAbilityTableRow* Row = GetTableRow();
	if (!CooldownGE || !Row)
	{
		return;
	}

	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, CooldownGE->GetClass(), GetAbilityLevel(Handle, ActorInfo));
	if (!SpecHandle.IsValid())
	{
		return;
	}

	// 충전이 직렬로 회복되도록 이미 도는 쿨다운의 최장 잔여시간을 더한 값을 SetByCaller로 실어 적용한다.
	// 이 조회는 GE 적용 전이라 방금 거는 쿨다운이 섞이지 않는다.
	float LongestRemaining = 0.f;
	float LongestDuration = 0.f;
	if (const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		QueryActiveCooldowns(*ASC, LongestRemaining, LongestDuration);
	}

	SpecHandle.Data->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Duration, LongestRemaining + Row->CooldownTime);
	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
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

	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC)
	{
		return true;
	}

	float LongestRemaining = 0.f;
	float LongestDuration = 0.f;
	if (QueryActiveCooldowns(*ASC, LongestRemaining, LongestDuration) >= FMath::Max(1, Row->MaxRecharges))
	{
		// 순정 CheckCooldown처럼 실패 사유 태그를 채워 OnAbilityFailed 파이프라인에 전달한다
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
	/**
	 * 엔진 순정 구현은 쿨다운 GE의 GrantedTags 쿼리 기반이라, 태그를 부여하지 않는 공용 쿨다운 GE에서는 항상 0을 반환한다.
	 * CDO 기반 쿼리로 대체해 순정 API(BP 노드 포함) 호출자가 올바른 값을 받게 한다.
	 */
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

const FWxAbilityTableRow* UWxAbilityBase::GetTableRow() const
{
	if (AbilityDataRow.IsNull())
	{
		return nullptr;
	}
	return AbilityDataRow.GetRow<FWxAbilityTableRow>(TEXT("WxAbilityBase::GetTableRow"));
}

int32 UWxAbilityBase::QueryActiveCooldowns(const UAbilitySystemComponent& ASC, float& OutLongestRemaining, float& OutLongestDuration) const
{
	OutLongestRemaining = 0.f;
	OutLongestDuration = 0.f;

	const UWorld* World = ASC.GetWorld();
	if (!World)
	{
		return 0;
	}

	const UGameplayAbility* AbilityCDO = GetClass()->GetDefaultObject<UGameplayAbility>();
	const float WorldTime = World->GetTimeSeconds();

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

		// 만료됐지만 아직 제거되지 않은 GE(클라는 제거 복제가 늦게 도착)는 회복된 충전으로 친다
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
