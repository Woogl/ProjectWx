// Copyright Woogle. All Rights Reserved.

#include "Interaction/WxStateTreeTask_EnableInteraction.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "Gimmick/WxGimmickStateTreeComponent.h"
#include "StateTreeExecutionContext.h"
#include "StateTreePropertyBindings.h"
#include "UniversalObjectLocators/ActorLocatorFragment.h"
#include "WxInteractable.h"
#include "WxWorldModule.h"

FWxStateTreeTask_EnableInteraction::FWxStateTreeTask_EnableInteraction()
{
	// 그 상태의 상호작용 가용성·문구를 선언하는 상태형 태스크라, 같은 상태가 재선택돼도 이미 적용된 값이라 다시 쓸 것이 없다.
	bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FWxStateTreeTask_EnableInteraction::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	if (Instance.TargetMesh)
	{
		// 프롬프트·발행 자리까지 담아야 해서 계약이 아니라 기믹 컴포넌트로 직접 부른다 — 그러지 않으면 공용 계약이 StateTree 를 떠안는다.
		// 꺼진 영역은 기믹의 활성 목록에서 빠져 다음 스캔에서 탈락한다. 콜리전은 건드리지 않으므로 대상 메시의 설정은 보존된다.
		const AActor* Owner = Cast<AActor>(Context.GetOwner());
		if (UWxGimmickStateTreeComponent* Gimmick = Owner ? Owner->FindComponentByClass<UWxGimmickStateTreeComponent>() : nullptr)
		{
			// 발행은 상호작용을 받은 그 순간, 즉 트리 틱 밖에서 일어난다. 그래서 발행자만이 아니라 지금의 실행 컨텍스트를 약참조로 함께 남긴다.
			FWxGimmickInteractionRegion Region;
			Region.Prompt = Instance.Prompt;
			Region.Dispatcher = Instance.OnInteracted;
			Region.Context = Context.MakeWeakExecutionContext();

			Gimmick->SetInteractionRegionEnabled(Instance.TargetMesh, Instance.bEnable, Region);
		}

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
	// 빈 지정은 기능 미사용(재사용 스텝의 옵션 파라미터)이라 걸 것이 없다.
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

	// 켜고 끄는 수단은 대상이 정한다(대화 상대는 영역 메시의 쿼리 콜리전).
	IWxInteractable* Interactable = IWxInteractable::Find(Target);
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

	// 대상 메시는 보통 바인딩이라 런타임 포인터가 비어 있다. 바인딩 소스명을 우선 보이고, 직접 지정 시 그 이름으로 폴백.
	FText TargetText = BindingLookup.GetBindingSourceDisplayName(FPropertyBindingPath(ID, GET_MEMBER_NAME_CHECKED(FInstanceDataType, TargetMesh)), Formatting);
	if (TargetText.IsEmpty() && InstanceData->TargetMesh)
	{
		TargetText = FText::FromString(InstanceData->TargetMesh->GetName());
	}

	// 메시를 지목하지 않았으면 액터 갈래다. 로케이터도 보통 바인딩이라 같은 순서로 본다.
	if (TargetText.IsEmpty())
	{
		TargetText = BindingLookup.GetBindingSourceDisplayName(FPropertyBindingPath(ID, GET_MEMBER_NAME_CHECKED(FInstanceDataType, Target)), Formatting);
	}
	if (TargetText.IsEmpty())
	{
		TargetText = FText::FromString(GetTargetDisplayName(InstanceData->Target));
	}

	// 상호작용을 켜고 프롬프트가 있으면 함께 보여, 상태별 프롬프트를 노드 설명에서 바로 확인할 수 있게 한다.
	if (InstanceData->bEnable && !InstanceData->Prompt.IsEmpty())
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

	// 미해석(언로드 등)이면 액터 프래그먼트의 소프트 경로 끝 이름이라도 보여준다.
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
