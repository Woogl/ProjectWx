// Copyright Woogle. All Rights Reserved.

#include "StateTreeTask/WxStateTreeTask_PlayMontageOnce.h"

#include "AbilitySystem/Ability/WxAbility_PlayMontageOnce.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Actor.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FWxStateTreeTask_PlayMontageOnce::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Instance = Context.GetInstanceData(*this);
	Instance.AbilitySystem = nullptr;
	Instance.AbilityHandle = FGameplayAbilitySpecHandle();

	const bool bInitialEntry = !Transition.SourceStateID.IsValid();
	if (bInitialEntry)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	const AActor* Owner = Cast<AActor>(Context.GetOwner());
	if (!Owner)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 어빌리티를 부여하고 그 끝을 아는 것은 권위뿐이다. 권위가 아닌 피어는 서버가 발행하는 다음 상태를 기다린다.
	if (!Owner->HasAuthority())
	{
		return EStateTreeRunStatus::Running;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Instance.Target);
	if (!ASC || !Instance.Montage)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	FGameplayEventData Payload;
	Payload.Instigator = Instance.Instigator;
	Payload.Target = Instance.Target;
	Payload.OptionalObject = Instance.Montage;

	// 발동에 실패하면 엔진이 스펙을 걷고 빈 핸들을 돌려준다.
	FGameplayAbilitySpec Spec(UWxAbility_PlayMontageOnce::StaticClass(), 1);
	const FGameplayAbilitySpecHandle Handle = ASC->GiveAbilityAndActivateOnce(Spec, &Payload);
	if (!Handle.IsValid())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	Instance.AbilitySystem = ASC;
	Instance.AbilityHandle = Handle;

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_PlayMontageOnce::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	// 부여하지 않은 피어(권위가 아닌 쪽)는 계속 기다린다.
	if (!Instance.AbilityHandle.IsValid())
	{
		return EStateTreeRunStatus::Running;
	}

	// 1회 부여한 스펙은 어빌리티가 끝나면 걷힌다. Target 이 사라져 ASC 가 없어져도 끝난 것으로 본다.
	const UAbilitySystemComponent* ASC = Instance.AbilitySystem.Get();
	const FGameplayAbilitySpec* Spec = ASC ? ASC->FindAbilitySpecFromHandle(Instance.AbilityHandle) : nullptr;

	return Spec && Spec->IsActive() ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_PlayMontageOnce::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	return FText::Format(INVTEXT("몽타주 1회 재생 ({0})"),
		InstanceData->Montage ? FText::FromString(InstanceData->Montage->GetName()) : INVTEXT("none"));
}
#endif
