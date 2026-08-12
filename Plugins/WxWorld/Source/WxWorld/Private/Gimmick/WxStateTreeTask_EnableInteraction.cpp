// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxStateTreeTask_EnableInteraction.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "Gimmick/WxGimmickStateTreeComponent.h"
#include "StateTreeExecutionContext.h"
#include "StateTreePropertyBindings.h"

FWxStateTreeTask_EnableInteraction::FWxStateTreeTask_EnableInteraction()
{
	// 인터랙션을 진입 시 1회 토글만 하므로 틱이 불필요하다.
	bShouldCallTick = false;

	// 그 상태의 상호작용 가용성·문구를 선언하는 상태형 태스크라, 같은 상태가 재선택돼도 이미 적용된 값이라 다시 쓸 것이 없다.
	bShouldStateChangeOnReselect = false;
}

EStateTreeRunStatus FWxStateTreeTask_EnableInteraction::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);

	UPrimitiveComponent* TargetMesh = Instance.TargetMesh;
	if (!TargetMesh)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 활성 여부와 프롬프트를 함께 오너 기믹에 세팅한다 — 콜리전은 관여하지 않으므로 대상 메시의 콜리전 설정은 그대로 보존된다.
	// 꺼진 영역은 기믹의 활성 목록에서 빠져 스캐너의 다음 스캔에서 자연 탈락하고, 외곽선도 그때 스캐너가 끈다.
	// 프롬프트는 영역별로 담기므로 한 상태가 여러 영역을 켜도 서로 덮어쓰지 않는다.
	// 오너에 기믹 컴포넌트가 없으면(비기믹 ST) 세팅할 대상이 없으므로 노옵이다.
	const AActor* Owner = Cast<AActor>(Context.GetOwner());
	if (UWxGimmickStateTreeComponent* Gimmick = Owner ? Owner->FindComponentByClass<UWxGimmickStateTreeComponent>() : nullptr)
	{
		Gimmick->SetInteractionEnabled(TargetMesh, Instance.bEnable, Instance.Prompt);
	}

	// 토글은 즉시 끝나므로 곧바로 완료한다.
	return EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_EnableInteraction::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	// 대상 메시는 보통 바인딩이라 런타임 포인터가 비어 있다. 바인딩 소스명을 우선 보이고, 직접 지정 시 그 이름으로 폴백.
	FText TargetText = BindingLookup.GetBindingSourceDisplayName(FPropertyBindingPath(ID, GET_MEMBER_NAME_CHECKED(FInstanceDataType, TargetMesh)), Formatting);
	if (TargetText.IsEmpty())
	{
		TargetText = InstanceData->TargetMesh ? FText::FromString(InstanceData->TargetMesh->GetName()) : INVTEXT("none");
	}

	// 상호작용을 켜고 프롬프트가 있으면 함께 보여, 상태별 프롬프트를 노드 설명에서 바로 확인할 수 있게 한다.
	if (InstanceData->bEnable && !InstanceData->Prompt.IsEmpty())
	{
		return FText::Format(INVTEXT("Enable \"{1}\" Interaction ({0})"), TargetText, InstanceData->Prompt);
	}

	return FText::Format(INVTEXT("{0} ({1})"), InstanceData->bEnable ? INVTEXT("Enable Interaction") : INVTEXT("Disable Interaction"), TargetText);
}
#endif
