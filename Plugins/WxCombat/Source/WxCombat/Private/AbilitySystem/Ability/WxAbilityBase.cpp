// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystem/Effect/WxEffect_Cooldown.h"
#include "AbilitySystem/Effect/WxEffect_Cost.h"
#include "AbilitySystem/Ability/WxAbilityTableRow.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystem/WxInputBufferComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "WxCombatModule.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

UWxAbilityBase::UWxAbilityBase()
{
	InstancingPolicy  = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// 쿨다운 GE는 각 어빌리티가 지정한다 — 엔진이 스택을 GE 클래스 단위로 병합해서, 여기에 공용 기본값을 두면 어빌리티끼리 쿨다운이 섞인다.
	// 코스트는 Instant라 병합될 것이 없어 공용 GE 하나로 충분하다.
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
	const FWxAbilityTableRow* Row = GetTableRow();
	return Row ? FMath::Max(1, Row->MaxRecharges) : 1;
}

float UWxAbilityBase::GetCooldownTime() const
{
	const FWxAbilityTableRow* Row = GetTableRow();
	return Row ? FMath::Max(0.f, Row->CooldownTime) : 0.f;
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

		// 재발동이 이 인스턴스를 그대로 되살리므로 전이 뒤에는 아무것도 쓰지 않는다.
		const AActor* Avatar = GetAvatarActorFromActorInfo();
		if (UWxInputBufferComponent* InputBuffer = Avatar ? Avatar->FindComponentByClass<UWxInputBufferComponent>() : nullptr)
		{
			InputBuffer->FlushBufferedInputs();
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

		// 성립한 어빌리티가 이 인스턴스를 끊으므로 전이 뒤에는 아무것도 쓰지 않는다.
		const AActor* Avatar = GetAvatarActorFromActorInfo();
		if (UWxInputBufferComponent* InputBuffer = Avatar ? Avatar->FindComponentByClass<UWxInputBufferComponent>() : nullptr)
		{
			InputBuffer->FlushBufferedInputs();
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

	// 쿨다운 태그가 없으면 순정 판정이 통과시켜 쿨다운이 조용히 사라진다 — GE를 못 찾은 경우와 태그를 빠뜨린 GE를 함께 잡는다.
	const FGameplayTagContainer* CooldownTags = GetCooldownTags();
	if (GetCooldownTime() > 0.f && (!CooldownTags || CooldownTags->IsEmpty()))
	{
		UE_LOG(LogWxCombat, Error, TEXT("%s: 테이블에 쿨다운 수치가 있는데 쿨다운 태그를 부여하는 GE가 없다. CooldownGameplayEffectClass에 전용 UWxEffect_Cooldown 파생 GE를 지정했는지 확인하라."), *GetName());
	}

	if (ActivationPolicy == EWxAbilityActivationPolicy::OnGiven)
	{
		if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
		{
			ASC->TryActivateAbility(Spec.Handle);
		}
	}
}

#if WITH_EDITOR
EDataValidationResult UWxAbilityBase::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	const FGameplayTagContainer* CooldownTags = GetCooldownTags();
	if (GetCooldownTime() > 0.f && (!CooldownTags || CooldownTags->IsEmpty()))
	{
		Result = EDataValidationResult::Invalid;
		Context.AddError(FText::FromString(TEXT("테이블에 쿨다운 수치가 있으나 쿨다운 태그를 부여하는 GE가 없습니다. CooldownGameplayEffectClass에 전용 UWxEffect_Cooldown 파생 GE를 지정했는지 확인하세요.")));
	}

	return Result;
}
#endif

UGameplayEffect* UWxAbilityBase::GetCooldownGameplayEffect() const
{
	UGameplayEffect* CooldownGE = Super::GetCooldownGameplayEffect();

	// 테이블 기반 쿨다운은 수치가 없으면 걸지 않는다 — 지속시간이 0인 GE는 만료 타이머가 걸리지 않는다.
	if (CooldownGE && CooldownGE->IsA<UWxEffect_Cooldown>() && GetCooldownTime() <= 0.f)
	{
		return nullptr;
	}

	return CooldownGE;
}

bool UWxAbilityBase::CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	// 순정 판정은 쿨다운 태그가 붙어 있기만 하면 막는다.
	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	const FGameplayTagContainer* CooldownTags = GetCooldownTags();
	if (ASC && CooldownTags && !CooldownTags->IsEmpty())
	{
		const FGameplayEffectQuery CooldownQuery = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(*CooldownTags);
		if (ASC->GetAggregatedStackCount(CooldownQuery) < GetMaxRecharges())
		{
			return true;
		}
	}

	// 실패 사유 태그를 채워 OnAbilityFailed 파이프라인에 전달하는 것까지 순정에 맡긴다.
	return Super::CheckCooldown(Handle, ActorInfo, OptionalRelevantTags);
}

const FWxAbilityTableRow* UWxAbilityBase::GetTableRow() const
{
	if (AbilityDataRow.IsNull())
	{
		return nullptr;
	}
	return AbilityDataRow.GetRow<FWxAbilityTableRow>(TEXT("WxAbilityBase::GetTableRow"));
}
