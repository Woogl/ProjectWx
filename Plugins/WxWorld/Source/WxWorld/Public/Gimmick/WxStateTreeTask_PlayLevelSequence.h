// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "WxStateTreeTask_PlayLevelSequence.generated.h"

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;
class ALevelSequenceActor;
class ULevelSequence;
class ULevelSequencePlayer;

// GetInstanceDataType() 의 헤더 정의는 코딩 규칙 6 의 예외다 — using FInstanceDataType 을 그대로 되돌려주는 타입 표기라 옮길 본문이 없고, 엔진 StateTree 도 전부 이 모양이다.

USTRUCT()
struct FWxStateTreeTask_PlayLevelSequenceInstanceData
{
	GENERATED_BODY()

	/** 재생할 Level Sequence. ST 에셋에서 Context 액터의 프로퍼티(예: LevelSequence)로 바인딩한다. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	TObjectPtr<ULevelSequence> LevelSequence;

	/** (런타임) 생성한 시퀀스 플레이어. 정리 시 정지·해제한다. */
	UPROPERTY()
	TObjectPtr<ULevelSequencePlayer> Player;

	/** (런타임) CreateLevelSequencePlayer 가 만든 액터. 정리 시 Destroy 한다. */
	UPROPERTY()
	TObjectPtr<ALevelSequenceActor> SequenceActor;
};

/**
 * 라이브 전이로 진입할 때 Level Sequence 를 재생하고, 재생이 끝나면 Succeeded 를 반환한다. 상태를 읽지 않아 어떤 기믹이든 재사용한다.
 * 초기 진입(StateTree 시작/복원/레이트조인: SourceStateID 무효)이면 재생 없이 곧바로 완료한다 — 컷신은 발동 순간에만 재생하고 복원 시엔 침묵한다. 라이브 진입인데 재생할 게 없으면(시퀀스/월드 부재·플레이어 생성 실패) 상태가 갇히지 않게 곧장 완료한다.
 * 입력 차단은 직교 태스크(EnablePlayerInput)가 맡고, 이 노드는 재생만 다룬다.
 * Tick 이 ULevelSequencePlayer::IsPlaying 로 종료를 폴링하다, 종료되면 시퀀스를 정리하고 완료한다 — 다음 상태로 넘기는 것은 이 상태의 완료 전이(On State Completed)다. OnFinished 콜백 중 시퀀스 액터 파괴를 피하려고 폴링→다음 틱 정리를 쓴다.
 * 중도 이탈·액터 파괴 시엔 ExitState 가 시퀀스를 정지·정리한다(멱등). 모든 피어가 각자 진입 시 로컬 재생하므로 별도 멀티캐스트가 필요 없다.
 */
USTRUCT(meta = (DisplayName = "레벨 시퀀스 재생", Category = "Wx"))
struct FWxStateTreeTask_PlayLevelSequence : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FWxStateTreeTask_PlayLevelSequenceInstanceData;

	FWxStateTreeTask_PlayLevelSequence();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif

private:
	/** 재생을 정지·정리한다(플레이어 정지 → 시퀀스 액터 파괴 → 런타임 핸들 비움). 핸들이 비어 있으면 멱등하게 노옵이라 종료·이탈 양쪽에서 호출한다. */
	void FinishSequencePlayback(FInstanceDataType& Instance) const;
};
