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
#include "Engine/World.h"
#include "WxGameplayTags.h"

UWxAbilityBase::UWxAbilityBase()
{
	InstancingPolicy  = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	CooldownGameplayEffectClass = UWxEffect_Cooldown::StaticClass();
	CostGameplayEffectClass = UWxEffect_Cost::StaticClass();
}

FText UWxAbilityBase::GetTitle() const
{
	const FWxAbilityTableRow* Row = GetTableRow();
	return Row ? Row->Title : FText::GetEmpty();
}

FText UWxAbilityBase::GetDescription() const
{
	const FWxAbilityTableRow* Row = GetTableRow();
	return Row ? Row->Description : FText::GetEmpty();
}

TSoftObjectPtr<UObject> UWxAbilityBase::GetIcon() const
{
	const FWxAbilityTableRow* Row = GetTableRow();
	return Row ? Row->Icon : nullptr;
}

int32 UWxAbilityBase::GetMaxRecharges() const
{
	// 커스텀 쿨다운 GE는 엔진 순정 판정(태그 기반 단일 쿨다운)으로 도니 충전을 여러 개 쌓을 수 없다.
	if (CooldownGameplayEffectClass && CooldownGameplayEffectClass != UWxEffect_Cooldown::StaticClass())
	{
		return 1;
	}

	const FWxAbilityTableRow* Row = GetTableRow();
	return Row ? FMath::Max(1, Row->MaxRecharges) : 1;
}

float UWxAbilityBase::GetMontagePlayRate() const
{
	const UWxAbilitySystemComponent* ASC = Cast<UWxAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	return ASC ? ASC->GetMontagePlayRate() : 1.f;
}

void UWxAbilityBase::OpenComboWindow()
{
	// 배타 본동작에서만 연다 — 콤보가 없는 어빌리티의 몽타주에 노티파이가 섞여도 Independent를 점유자로 승격시키지 않는다.
	if (ActivationGroup == EWxAbilityActivationGroup::Exclusive && ActionPhase == EWxAbilityActionPhase::Blocking)
	{
		ActionPhase = EWxAbilityActionPhase::ComboWindow;

		// 창이 열린 순간 쌓인 입력을 재생한다. 재발동이 이 인스턴스를 그대로 되살리므로 전이 뒤에는 아무것도 쓰지 않는다.
		if (UWxAbilitySystemComponent* WxASC = Cast<UWxAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
		{
			WxASC->FlushBufferedInputs();
		}
	}
}

void UWxAbilityBase::CloseComboWindow()
{
	// 창이 아직 열려 있을 때만 되돌린다 — 창이 후딜보다 늦게 닫히는 배치가 정상이라 무조건 되돌리면 후딜을 도로 닫는다.
	if (ActionPhase == EWxAbilityActionPhase::ComboWindow)
	{
		ActionPhase = EWxAbilityActionPhase::Blocking;
	}
}

void UWxAbilityBase::StartRecovery()
{
	// 배타 어빌리티만 후딜로 — 엉뚱한 노티파이가 Independent를 점유자로 승격시키거나 Override의 캔슬 면역을 벗기지 않게 한다.
	if (ActivationGroup == EWxAbilityActivationGroup::Exclusive && ActionPhase != EWxAbilityActionPhase::Recovery)
	{
		ActionPhase = EWxAbilityActionPhase::Recovery;

		// 후딜이 열린 순간 쌓인 입력을 재생한다. 성립한 어빌리티가 이 인스턴스를 끊으므로 전이 뒤에는 아무것도 쓰지 않는다.
		if (UWxAbilitySystemComponent* WxASC = Cast<UWxAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
		{
			WxASC->FlushBufferedInputs();
		}
	}
}

const UWxAbilityBase* UWxAbilityBase::FindActivationGroupBlocker(const UAbilitySystemComponent& ASC, const UWxAbilityBase* Candidate)
{
	for (const FGameplayAbilitySpec& Spec : ASC.GetActivatableAbilities())
	{
		if (!Spec.IsActive())
		{
			continue;
		}

		// 모든 Wx 어빌리티는 기반 생성자가 InstancedPerActor를 강제하므로 스펙당 인스턴스는 하나뿐이다.
		const UWxAbilityBase* Occupant = Cast<UWxAbilityBase>(Spec.GetPrimaryInstance());
		if (!Occupant || !Occupant->IsActive())
		{
			continue;
		}

		const bool bOccupying = Occupant->ActivationGroup == EWxAbilityActivationGroup::Override
			|| (Occupant->ActivationGroup == EWxAbilityActivationGroup::Exclusive && Occupant->ActionPhase != EWxAbilityActionPhase::Recovery);
		if (!bOccupying)
		{
			continue;
		}

		if (!Candidate)
		{
			return Occupant;
		}

		// 점유자가 후보 자신이면 엔진 재발동으로 들어온 콤보 진행이다. 콤보 창은 후딜보다 이르므로 여기서 갈라야 두 창이 분리된다.
		if (Occupant == Candidate)
		{
			if (Candidate->ActionPhase != EWxAbilityActionPhase::ComboWindow)
			{
				return Occupant;
			}
		}
		// 끊겠다고 지목한 점유자는 후보를 막지 못한다. 취소 자체는 발동 직후 순정 PreActivate가 수행한다.
		else if (!Occupant->GetAssetTags().HasAny(Candidate->CancelAbilitiesWithTag))
		{
			return Occupant;
		}
	}

	return nullptr;
}

bool UWxAbilityBase::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	if (ActivationGroup == EWxAbilityActivationGroup::Independent || ActivationGroup == EWxAbilityActivationGroup::Override)
	{
		return true;
	}

	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC)
	{
		return true;
	}

	return FindActivationGroupBlocker(*ASC, this) == nullptr;
}

bool UWxAbilityBase::CanBeCanceled() const
{
	return ActivationGroup != EWxAbilityActivationGroup::Override && Super::CanBeCanceled();
}

void UWxAbilityBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// 직전 활성화가 재사용 인스턴스에 남긴 캔슬 창을 닫는다.
	ActionPhase = EWxAbilityActionPhase::Blocking;

	if (ActivationGroup != EWxAbilityActivationGroup::Independent)
	{
		if (UWxAbilitySystemComponent* WxASC = Cast<UWxAbilitySystemComponent>(ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr))
		{
			// 후딜에 든 앞 액션은 들어온 배타 발동이 끊는다 — 후딜은 지목할 태그가 없어 여기서만 창으로 가른다.
			// 본동작 점유를 무엇까지 끊을지는 CancelAbilitiesWithTag 선언이 정하고 순정 PreActivate가 수행한다.
			WxASC->CancelRecoveringAbilities(this);
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

	// EndTask가 구 태스크를 가비지로 표시하므로, 바인딩은 남아도 약참조가 끊겨 후속 이벤트는 발송되지 않는다.
	if (MontageTask)
	{
		MontageTask->EndTask();
		MontageTask = nullptr;
	}

	UAbilityTask_PlayMontageAndWait* NewMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, Montage, GetMontagePlayRate(), StartSection, true, 1.f, 0.f, true);
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
		// 호출자들이 이 nullptr을 "쿨다운 없음" 게이트로 쓴다.
		return nullptr;
	}

	return Super::GetCooldownGameplayEffect();
}

void UWxAbilityBase::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (CooldownGameplayEffectClass && CooldownGameplayEffectClass != UWxEffect_Cooldown::StaticClass())
	{
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
	if (QueryActiveCooldowns(*ASC, LongestRemaining, LongestDuration) >= GetMaxRecharges())
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
	const UGameplayEffect* CooldownDef = GetDefault<UWxEffect_Cooldown>();
	const float WorldTime = World->GetTimeSeconds();

	// 홀드 입력이면 매 프레임 도는 경로다. GetActiveEffects는 핸들 배열을 새로 할당하고 핸들마다 컨테이너를 다시 찾게 만들어 직접 순회한다.
	int32 ActiveCount = 0;
	for (const FActiveGameplayEffect& ActiveGE : &ASC.GetActiveGameplayEffects())
	{
		// 엔진 쿼리의 정의 비교와 같은 규칙 — 하위 클래스가 아니라 CDO 일치다.
		if (ActiveGE.Spec.Def != CooldownDef || ActiveGE.Spec.GetEffectContext().GetAbility() != AbilityCDO)
		{
			continue;
		}

		// 만료됐지만 아직 제거되지 않은 GE(클라는 제거 복제가 늦게 도착)는 회복된 충전으로 친다
		const float Remaining = (ActiveGE.StartWorldTime + ActiveGE.Spec.GetDuration()) - WorldTime;
		if (Remaining <= 0.f)
		{
			continue;
		}

		++ActiveCount;
		if (Remaining > OutLongestRemaining)
		{
			OutLongestRemaining = Remaining;
			OutLongestDuration = ActiveGE.Spec.GetDuration();
		}
	}

	return ActiveCount;
}
