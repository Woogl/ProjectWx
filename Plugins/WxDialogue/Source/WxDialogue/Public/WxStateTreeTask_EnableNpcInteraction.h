// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "UniversalObjectLocator.h"
#include "WxStateTreeTask_EnableNpcInteraction.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class UWxDialogueComponent;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

USTRUCT()
struct FWxStateTreeTask_EnableNpcInteractionInstanceData
{
	GENERATED_BODY()

	/** 상호작용을 토글할 NPC 배치 액터 지정. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (AllowedLocators = "Actor"))
	TArray<FUniversalObjectLocator> Targets;

	/** 진입 시 그 NPC 들의 상호작용 활성 여부. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bEnable = true;

	/**
	 * (런타임) 이 노드가 실제로 토글을 걸어 준 대화 컴포넌트.
	 * Targets 와 같은 인덱스로 짝을 이루며, 그 자리를 다시 해석한 컴포넌트가 기록과 다르면 그때 다시 적용한다.
	 */
	UPROPERTY()
	TArray<TWeakObjectPtr<UWxDialogueComponent>> AppliedTargets;
};

/**
 * 지정 NPC 들의 상호작용을 bEnable 로 토글하고 그 상태에 머물며 유지한다 — 잠긴 NPC 는 스캔 후보에서 빠져 프롬프트조차 뜨지 않고, 서버 활성 검증에도 걸려 대화가 열리지 않는다.
 * 대화가 아니라 화자를 다루는 노드다.
 * 상태를 떠나도 되돌리지 않는다. 잠금은 그 상태에 딸린 임시 연출이 아니라 월드에 남는 변경이며, 다시 열 시점은 퀘스트마다 다르므로 여는 상태에 이 태스크를 bEnable=true 로 한 번 더 두어 에셋이 정한다.
 * 상태 완료 판정에서는 빠진다(bConsideredForCompletion=false) — 완료를 내지 않는 태스크가 판정에 끼면 대기 태스크와 같은 상태(TasksCompletion=All)가 영영 완료되지 않는다.
 *
 * 토글은 대상 액터의 대화 컴포넌트에 건다 — NPC 액터 타입을 알지 않아도 되므로, 호스트가 어느 모듈에 있든 이 노드는 대화 모듈에 남는다.
 * 대상 해석은 매 틱 대상마다 수행하고, 해석된 컴포넌트가 그 자리의 기록과 다를 때만(첫 해석 성공·스트리밍 재로드로 액터가 새로 만들어진 경우) 토글을 적용한다.
 * 대상마다 독립적으로 적용되므로 하나가 스트리밍 아웃돼도 나머지 토글은 그대로 유지된다.
 * 토글은 대상 액터의 메시 콜리전이라 재로드 때 레벨에 저장된 값으로 되돌아가므로, 진입 1회 적용으로는 대상이 그 순간 언로드면 영영 적용되지 않고 상태 유지 중 스트리밍되면 적용분이 사라진다.
 * 미해석은 스트리밍 아웃의 정상 상황이라 조용히 대기하고, 빈 지정은 기능 미사용(재사용 스텝의 옵션 파라미터)이라 무동작이다. 잘못된 조립(빈 로케이터 항목·대화 상대 아님)만 진입 시 1회 경고한다.
 * 값을 복제하지 않으므로 서버가 곧 클라인 싱글/리슨 호스트가 전제다(다른 크로스모듈 노드와 같은 전제).
 *
 * NPC 지목은 FUniversalObjectLocator 로 배치 액터를 직접 가리킨다 — 퀘스트·스포너·인디케이터 노드와 같은 방식이라 씬 픽커·WP·PIE 해석이 엔진에 내장돼 있다.
 * 하나만 쓸 자리라도 배열로 받는다 — 인스턴스 데이터의 직속 UOL 멤버는 ST 에디터가 값 위젯을 만들지 못하는 엔진(5.8) 제한에 걸리고, 배열 원소는 그렇지 않다.
 */
USTRUCT(meta = (DisplayName = "Enable Npc Interaction", Category = "Wx"))
struct FWxStateTreeTask_EnableNpcInteraction : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_EnableNpcInteractionInstanceData;

	FWxStateTreeTask_EnableNpcInteraction();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif

private:
	/**
	 * 재로드로 액터가 새로 만들어지면 콜리전이 레벨 값으로 돌아가 있으므로 다시 걸고, 같은 액터면 이미 적용돼 있어 건드리지 않는다.
	 * bWarnNotDialogueTarget 은 진입 1회 경고용이다 — 매 틱 도는 자리라 켜 두면 로그가 폭주한다.
	 */
	void RefreshNpcInteraction(const FStateTreeExecutionContext& Context, FInstanceDataType& Instance, bool bWarnNotDialogueTarget) const;

#if WITH_EDITOR
	/** 로케이터의 표시명. 에디터에서 해석되면 액터 라벨(아웃라이너와 동일), 미해석이면 경로 끝 오브젝트 이름, 빈 로케이터는 unset. */
	FString GetTargetDisplayName(const FUniversalObjectLocator& Locator) const;

	/** 지정 목록의 표시 텍스트. 라벨 3개까지 나열하고 초과분은 +N 로 줄인다. 빈 배열은 none. */
	FText GetTargetsText(const TArray<FUniversalObjectLocator>& Targets) const;
#endif
};
