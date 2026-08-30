// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "UniversalObjectLocator.h"
#include "WxStateTreeTask_WaitForInteraction.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class AActor;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

USTRUCT()
struct FWxStateTreeTask_WaitForInteractionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (AllowedLocators = "Actor"))
	FUniversalObjectLocator Target;

	/** (런타임) 상태를 떠날 때 자기 등록만 골라 걷어내는 데 쓴다. */
	UPROPERTY()
	int32 WaitHandle = INDEX_NONE;
};

/**
 * 지정 대상이 상호작용될 때까지 Running 으로 대기하다, 그 순간 Succeeded 로 완료한다(퀘스트 스텝 게이트).
 * 상호작용 그 자체만 보므로 대상이 NPC 든 장치든 픽업이든 같은 노드로 판정한다 — 대화가 열리는 것은 대상의 반응이지 이 노드의 완료 조건이 아니다.
 * 한 액터에 상호작용 영역이 여럿이면(예: 엘리베이터) 어느 영역이든 성립으로 본다. 주체도 가리지 않는다(상호작용 어빌리티를 가진 것은 플레이어뿐).
 * 진입 이전의 상호작용은 세지 않는다 — 앞 상태를 끝낸 그 상호작용이 다음 상태까지 관통하지 못한다.
 * 통보가 서버 권위 경로(상호작용 어빌리티)에서만 오므로 권위에서 구동되는 ST 전용이다.
 *
 * 폴링하지 않는다 — 진입할 때 자신을 대기 목록에 올려 두고, 상호작용이 성립하는 순간 엔진의 약한 실행 컨텍스트로 완료를 통보받는다(틱 없음).
 * 대상 해석도 그 순간에 한 번만 하므로 진입 시점에 대상이 스트리밍 아웃이어도 상관없다.
 *
 * 대상은 FUniversalObjectLocator 로 배치 액터를 직접 지정한다 — 순수 구조체라 ST 컴파일러의 레벨 액터 참조 검증에 걸리지 않고, 씬 픽커·WP·PIE 해석이 엔진에 내장돼 있어 레벨 밖 호스트(퀘스트 ST)에서도 조립할 수 있다.
 */
USTRUCT(meta = (DisplayName = "상호작용 대기", Category = "Wx"))
struct FWxStateTreeTask_WaitForInteraction : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_WaitForInteractionInstanceData;

	FWxStateTreeTask_WaitForInteraction();

	/**
	 * 상호작용이 성립한 순간 서버 권위 경로(상호작용 어빌리티)가 부른다.
	 * 그 대상을 기다리던 노드가 있으면 완료시키고, 없으면 아무 일도 하지 않는다 — 남는 기록이 없다.
	 */
	static WXWORLD_API void NotifyInteracted(AActor* Target);

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
