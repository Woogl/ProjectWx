// Copyright Woogle. All Rights Reserved.

#include "StateTreeTask/WxStateTreeTask_SaveCheckpoint.h"

#include "Components/SceneComponent.h"
#include "Device/WxDevice.h"
#include "Device/WxDeviceStateTreeComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "StateTreeExecutionContext.h"
#include "System/WxCheckpointSaveGame.h"
#include "WxWorldModule.h"

FWxStateTreeTask_SaveCheckpoint::FWxStateTreeTask_SaveCheckpoint()
{
	bShouldCallTick = false;
#if WITH_EDITORONLY_DATA
	bConsideredForCompletion = false;
	bCanEditConsideredForCompletion = false;
#endif
}

EStateTreeRunStatus FWxStateTreeTask_SaveCheckpoint::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	if (UWxDeviceStateTreeComponent::IsRestoring(Context, Transition))
	{
		return EStateTreeRunStatus::Succeeded;
	}
	const AWxDevice* Device = Cast<AWxDevice>(Context.GetOwner());
	const ACharacter* Interactor = Device ? Device->GetInteractingCharacter() : nullptr;
	if (!Device || !Device->HasAuthority() || !Interactor || !Interactor->IsPlayerControlled()
		|| !Device->GetWorld()->IsNetMode(NM_Standalone))
	{
		return EStateTreeRunStatus::Succeeded;
	}
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);
	const USceneComponent* Marker = Instance.RespawnPoint.Resolve(Device);
	if (!Marker)
	{
		UE_LOG(LogWxWorld, Warning, TEXT("Checkpoint(%s): 부활 위치 컴포넌트가 없습니다."), *Device->GetName());
		return EStateTreeRunStatus::Failed;
	}
	if (!UWxCheckpointSaveGame::SaveCheckpoint(Device->GetWorld(), Marker->GetComponentTransform()))
	{
		UE_LOG(LogWxWorld, Warning, TEXT("Checkpoint(%s): 체크포인트 저장에 실패했습니다."), *Device->GetName());
		return EStateTreeRunStatus::Failed;
	}
	return EStateTreeRunStatus::Succeeded;
}
