// Copyright Woogle. All Rights Reserved.

#include "WxStateTreeTask_SaveGame.h"

#include "GameFramework/Actor.h"
#include "StateTreeExecutionContext.h"
#include "WxGameplayTags.h"
#include "WxSaveLibrary.h"

FWxStateTreeTask_SaveGame::FWxStateTreeTask_SaveGame()
{
	// 진입 시 1회 저장만 하므로 틱이 불필요하다.
	bShouldCallTick = false;
#if WITH_EDITORONLY_DATA
	// 무틱 즉시완료 태스크라 상태 완료를 구동하지 않는다(정지 leaf 에 놓여도 즉시 완료→재선택 루프에 빠지지 않게).
	bConsideredForCompletion = false;
#endif
}

EStateTreeRunStatus FWxStateTreeTask_SaveGame::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 초기 진입(StateTree 시작/복원/레이트조인) 또는 세이브 복원(호스트가 상태 태그와 함께 보내는 Gimmick.Restore 마커)이면 저장하지 않는다.
	// 막 로드한 세이브를 로드 직후 되쓰면 그 사이 라이브 상태로 파일이 오염된다.
	const bool bInitialEntry = !Transition.SourceStateID.IsValid() || Context.HasEventToProcess(WxGameplayTags::Gimmick_Restore);
	if (bInitialEntry)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 저장 파일은 서버가 소유하므로 클라 진입은 노옵이다.
	const AActor* Owner = Cast<AActor>(Context.GetOwner());
	if (!Owner || !Owner->HasAuthority())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 슬롯을 비워 활성 슬롯에 그대로 기록한다(오토세이브 경로). 실제 디스크 기록은 월드 플러시 완료 후 이어진다.
	UWxSaveLibrary::SaveToFile(Owner, FString(), 0);

	// 저장 요청은 즉시 끝나므로 곧바로 완료한다.
	return EStateTreeRunStatus::Succeeded;
}
