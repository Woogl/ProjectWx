// Copyright Woogle. All Rights Reserved.

#include "StateTreeTask/WxStateTreeTask_RecordCheckpoint.h"

#include "Components/SceneComponent.h"
#include "Device/WxDevice.h"
#include "Device/WxDeviceExecutionPolicy.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Character.h"
#include "StateTreeExecutionContext.h"
#include "System/WxCheckpointSubsystem.h"
#include "WxWorldModule.h"

FWxStateTreeTask_RecordCheckpoint::FWxStateTreeTask_RecordCheckpoint()
{
	bShouldCallTick = false;
#if WITH_EDITORONLY_DATA
	bConsideredForCompletion = false;
	bCanEditConsideredForCompletion = false;
#endif
}

EStateTreeRunStatus FWxStateTreeTask_RecordCheckpoint::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	if (FWxDeviceExecutionPolicy::IsRestoring(Context, Transition))
	{
		return EStateTreeRunStatus::Succeeded;
	}
	const AWxDevice* Device = Cast<AWxDevice>(Context.GetOwner());
	const ACharacter* Interactor = Device ? Device->GetInteractingCharacter() : nullptr;
	if (!Device || !Device->HasAuthority() || !Interactor || !Interactor->IsPlayerControlled())
	{
		return EStateTreeRunStatus::Succeeded;
	}
	const FInstanceDataType& Instance = Context.GetInstanceData(*this);
	const USceneComponent* Marker = Instance.RespawnPoint.Resolve(Device);
	UGameInstance* GameInstance = Device->GetGameInstance();
	if (!Marker || !GameInstance)
	{
		UE_LOG(LogWxWorld, Warning, TEXT("Checkpoint(%s): 부활 위치 컴포넌트 또는 GameInstance가 없습니다."), *Device->GetName());
		return EStateTreeRunStatus::Failed;
	}
	GameInstance->GetSubsystem<UWxCheckpointSubsystem>()->RecordCheckpoint(Device->GetWorld(), Marker->GetComponentTransform());
	return EStateTreeRunStatus::Succeeded;
}
