// Copyright Woogle. All Rights Reserved.

#include "Interaction/WxStateTreeTask_EnableInteraction.h"

#include "Device/WxDevice.h"
#include "GameFramework/Actor.h"
#include "StateTreeExecutionContext.h"
#include "StateTreePropertyBindings.h"
#include "UniversalObjectLocators/ActorLocatorFragment.h"
#include "WxInteractable.h"
#include "WxWorldModule.h"

FWxStateTreeTask_EnableInteraction::FWxStateTreeTask_EnableInteraction()
{
	// 같은 상태가 재선택돼도 이미 적용된 값이라 다시 쓸 것이 없다.
	bShouldStateChangeOnReselect = false;

#if WITH_EDITORONLY_DATA
	bConsideredForCompletion = false;
	bCanEditConsideredForCompletion = false;
#endif
}

EStateTreeRunStatus FWxStateTreeTask_EnableInteraction::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	if (Instance.TargetKind == EWxInteractionToggleTarget::Self)
	{
		// 프롬프트·발행 자리까지 담아야 해서 계약이 아니라 장치로 직접 부른다 — 그러지 않으면 공용 계약이 StateTree 를 떠안는다.
		AWxDevice* Device = Cast<AWxDevice>(Context.GetOwner());
		if (!Device)
		{
			UE_LOG(LogWxWorld, Warning, TEXT("Enable Interaction: 오너 %s 가 장치가 아니라 자기 상호작용을 여닫을 수 없음."), *GetNameSafe(Cast<AActor>(Context.GetOwner())));

			return EStateTreeRunStatus::Succeeded;
		}

		// 발행은 상호작용을 받은 그 순간, 즉 트리 틱 밖에서 일어난다. 그래서 발행자만이 아니라 지금의 실행 컨텍스트를 약참조로 함께 남긴다.
		FWxDeviceInteractionBinding Binding;
		Binding.Prompt = Instance.Prompt;
		Binding.Dispatcher = Instance.OnInteracted;
		Binding.Context = Context.MakeWeakExecutionContext();

		// 꺼지면 장치가 활성 판정에 false 를 답해 다음 스캔에서 탈락한다. 콜리전은 건드리지 않으므로 대상 메시의 설정은 보존된다.
		Device->SetInteractionBinding(Instance.bEnable, Binding);

		return EStateTreeRunStatus::Succeeded;
	}

	return ApplyTargetInteraction(Context, Instance);
}

EStateTreeRunStatus FWxStateTreeTask_EnableInteraction::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	return ApplyTargetInteraction(Context, Instance);
}

EStateTreeRunStatus FWxStateTreeTask_EnableInteraction::ApplyTargetInteraction(const FStateTreeExecutionContext& Context, const FInstanceDataType& Instance) const
{
	// 지목을 비운 것은 옵션 파라미터를 쓰지 않는 재사용 스텝이라 여닫을 대상이 없다.
	if (Instance.Target.IsEmpty())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 미해석은 스트리밍 아웃의 정상 상황이라 들어올 때까지 기다린다.
	AActor* Owner = Cast<AActor>(Context.GetOwner());
	AActor* Target = Cast<AActor>(Instance.Target.SyncFind(Owner));
	if (!Target)
	{
		return EStateTreeRunStatus::Running;
	}

	IWxInteractable* Interactable = Cast<IWxInteractable>(Target);
	if (!Interactable)
	{
		UE_LOG(LogWxWorld, Warning, TEXT("Enable Interaction: 대상 %s 는 상호작용 대상이 아니라 여닫을 것이 없음."), *GetNameSafe(Target));

		return EStateTreeRunStatus::Succeeded;
	}

	Interactable->SetInteractionEnabled(Instance.bEnable);

	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_EnableInteraction::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	FText TargetText = INVTEXT("self");
	if (InstanceData->TargetKind == EWxInteractionToggleTarget::Actor)
	{
		// 로케이터는 보통 바인딩이라 런타임 값이 비어 있다.
		TargetText = BindingLookup.GetBindingSourceDisplayName(FPropertyBindingPath(ID, GET_MEMBER_NAME_CHECKED(FInstanceDataType, Target)), Formatting);
		if (TargetText.IsEmpty())
		{
			TargetText = FText::FromString(GetTargetDisplayName(InstanceData->Target));
		}
	}

	if (InstanceData->TargetKind == EWxInteractionToggleTarget::Self && InstanceData->bEnable && !InstanceData->Prompt.IsEmpty())
	{
		return FText::Format(INVTEXT("\"{1}\" 상호작용 켜기 ({0})"), TargetText, InstanceData->Prompt);
	}

	return FText::Format(INVTEXT("{0} ({1})"), InstanceData->bEnable ? INVTEXT("상호작용 켜기") : INVTEXT("상호작용 끄기"), TargetText);
}

FString FWxStateTreeTask_EnableInteraction::GetTargetDisplayName(const FUniversalObjectLocator& Locator) const
{
	if (Locator.IsEmpty())
	{
		return TEXT("unset");
	}

	if (const AActor* Actor = Cast<AActor>(Locator.SyncFind()))
	{
		return Actor->GetActorLabel();
	}

	const FUniversalObjectLocatorFragment* Fragment = Locator.GetLastFragment();
	const FActorLocatorFragment* Payload = nullptr;
	if (Fragment && Fragment->TryGetPayloadAs(FActorLocatorFragment::FragmentType, Payload) && Payload)
	{
		const FString SubPath = Payload->Path.GetSubPathString();
		int32 DotIndex = INDEX_NONE;
		return SubPath.FindLastChar(TEXT('.'), DotIndex) ? SubPath.Mid(DotIndex + 1) : SubPath;
	}

	return TEXT("unresolved");
}
#endif
