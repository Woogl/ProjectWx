// Copyright Woogle. All Rights Reserved.

#include "WxStateTreeTask_SaveGame.h"

#include "Components/SceneComponent.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Actor.h"
#include "StateTreeExecutionContext.h"
#include "StateTreePropertyBindings.h"
#include "WxSaveGameSubsystem.h"

FWxStateTreeTask_SaveGame::FWxStateTreeTask_SaveGame()
{
	// 디스크 기록이 끝나기를 기다려야 하므로 틱한다(직렬화는 동기지만 쓰기는 비동기다).
}

EStateTreeRunStatus FWxStateTreeTask_SaveGame::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 전이로 들어온 것이 아니면(StateTree 시작·세이브 복원·레이트조인) 저장하지 않는다.
	// 막 로드한 세이브를 로드 직후 되쓰면 그 사이 라이브 상태로 파일이 오염된다.
	const bool bInitialEntry = !Transition.SourceStateID.IsValid();
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

	UGameInstance* GameInstance = Owner->GetGameInstance();
	UWxSaveGameSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<UWxSaveGameSubsystem>() : nullptr;
	if (!SaveSubsystem)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// 재개 지점을 물린 ST 라면 그 컴포넌트 자리로 확정한다 — 플레이어가 어디에 서서 상호작용했든 부활 지점이 흔들리지 않는다(체크포인트).
	// 비었으면 넘기지 않아 세이브 기본대로 저장 시점 플레이어 위치가 재개 지점이 된다.
	const USceneComponent* ResumePoint = Context.GetInstanceData(*this).ResumePoint;
	const FTransform ResumeTransform = ResumePoint ? ResumePoint->GetComponentTransform() : FTransform::Identity;

	// 슬롯을 비워 활성 슬롯에 그대로 기록한다(오토세이브 경로). 실제 디스크 기록은 월드 플러시 완료 후 이어진다.
	SaveSubsystem->SaveToFile(FString(), 0, ResumePoint ? &ResumeTransform : nullptr);

	// 기록이 끝날 때까지 머문다. Tick 이 완료를 폴링해 상태를 완료시킨다(요청 즉시 끝났으면 첫 틱에 바로 빠진다).
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FWxStateTreeTask_SaveGame::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	// 저장을 건 주체가 사라졌으면(레벨 전환 등) 상태가 갇히지 않게 완료로 빠진다.
	const AActor* Owner = Cast<AActor>(Context.GetOwner());
	UGameInstance* GameInstance = Owner ? Owner->GetGameInstance() : nullptr;
	const UWxSaveGameSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<UWxSaveGameSubsystem>() : nullptr;
	if (!SaveSubsystem)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	return SaveSubsystem->IsSaveInProgress() ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

#if WITH_EDITOR
FText FWxStateTreeTask_SaveGame::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType* InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	// 슬롯은 언제나 활성 슬롯이라 갈리는 건 재개 지점뿐이다. 그것을 보여 배선을 빠뜨린 노드가 눈에 띄게 한다.
	// 재개 지점은 보통 바인딩이라 런타임 포인터가 비어 있다. 바인딩 소스명을 우선 보이고, 비우면 플레이어 위치를 쓰므로 player 로 폴백.
	FText ResumeText = BindingLookup.GetBindingSourceDisplayName(FPropertyBindingPath(ID, GET_MEMBER_NAME_CHECKED(FInstanceDataType, ResumePoint)), Formatting);
	if (ResumeText.IsEmpty())
	{
		ResumeText = InstanceData->ResumePoint ? FText::FromString(InstanceData->ResumePoint->GetName()) : INVTEXT("player");
	}

	return FText::Format(INVTEXT("Save Game ({0})"), ResumeText);
}
#endif
