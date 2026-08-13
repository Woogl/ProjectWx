// Copyright Woogle. All Rights Reserved.

#include "WxStateTreeTask_SaveGame.h"

#include "Components/SceneComponent.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Actor.h"
#include "StateTreeAsyncExecutionContext.h"
#include "StateTreeExecutionContext.h"
#include "StateTreePropertyBindings.h"
#include "WxSaveGameSubsystem.h"

FWxStateTreeTask_SaveGame::FWxStateTreeTask_SaveGame()
{
	// 기록 완료를 서브시스템의 신호로 받으므로 볼 것이 없다.
	bShouldCallTick = false;
}

EStateTreeRunStatus FWxStateTreeTask_SaveGame::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 막 로드한 세이브를 로드 직후 되쓰면 그 사이 라이브 상태로 파일이 오염된다.
	const bool bInitialEntry = !Transition.SourceStateID.IsValid();
	if (bInitialEntry)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	const AActor* Owner = Cast<AActor>(Context.GetOwner());
	if (!Owner || !Owner->HasAuthority())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	UGameInstance* GameInstance = Owner->GetGameInstance();
	UWxSaveGameSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<UWxSaveGameSubsystem>() : nullptr;
	if (!SaveSubsystem)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	const USceneComponent* ResumePoint = Context.GetInstanceData(*this).ResumePoint;
	const FTransform ResumeTransform = ResumePoint ? ResumePoint->GetComponentTransform() : FTransform::Identity;

	// 실제 디스크 기록은 월드 플러시 완료 후 이어진다.
	SaveSubsystem->SaveToFile(FString(), 0, ResumePoint ? &ResumeTransform : nullptr);

	// 요청 안에서 이미 끝났으면 기다릴 신호가 없다.
	if (!SaveSubsystem->IsSaveInProgress())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 약한 실행 컨텍스트를 넘기는 것이 엔진이 제시하는 방식이라 여기선 람다를 쓴다.
	// 신호는 발화와 함께 비워지므로 상태를 먼저 떠난 노드의 등록도 남지 않는다(그 경우 이 컨텍스트가 무효라 무시된다).
	SaveSubsystem->OnSaveCompleted.AddLambda([WeakContext = Context.MakeWeakExecutionContext()]()
	{
		WeakContext.FinishTask(EStateTreeFinishTaskType::Succeeded);
	});

	return EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FWxStateTreeTask_SaveGame::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	// 슬롯은 언제나 활성 슬롯이라 갈리는 건 재개 지점뿐이다.
	// 그것을 보여 배선을 빠뜨린 노드가 눈에 띄게 한다.
	// 재개 지점은 보통 바인딩이라 런타임 포인터가 비어 있다.
	FText ResumeText = BindingLookup.GetBindingSourceDisplayName(FPropertyBindingPath(ID, GET_MEMBER_NAME_CHECKED(FInstanceDataType, ResumePoint)), Formatting);
	if (ResumeText.IsEmpty())
	{
		ResumeText = InstanceData->ResumePoint ? FText::FromString(InstanceData->ResumePoint->GetName()) : INVTEXT("player");
	}

	return FText::Format(INVTEXT("게임 저장 ({0})"), ResumeText);
}
#endif
