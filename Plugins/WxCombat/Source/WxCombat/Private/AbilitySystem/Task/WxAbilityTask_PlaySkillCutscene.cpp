// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.h"
#include "AbilitySystemComponent.h"
#include "Cutscene/WxSkillCutsceneComponent.h"

UWxAbilityTask_PlaySkillCutscene* UWxAbilityTask_PlaySkillCutscene::CreateTask(UGameplayAbility* OwningAbility, ULevelSequence* InLevelSequence, float InGlobalTimeDilation)
{
	UWxAbilityTask_PlaySkillCutscene* Task = NewAbilityTask<UWxAbilityTask_PlaySkillCutscene>(OwningAbility);
	Task->LevelSequence = InLevelSequence;
	Task->GlobalTimeDilation = InGlobalTimeDilation;
	return Task;
}

void UWxAbilityTask_PlaySkillCutscene::Activate()
{
	Super::Activate();
	Coordinator = UWxSkillCutsceneComponent::Get(GetWorld());
	if (Coordinator.IsValid())
	{
		if (AbilitySystemComponent.IsValid() && !AbilitySystemComponent->IsOwnerActorAuthoritative())
		{
			// 로컬 발동은 유지하되 독립적인 시퀀스를 만들지 않고 서버의 해당 시전자 세션을 기다린다.
			SessionId = Coordinator->GetSessionId() + 1;
			EndedHandle = Coordinator->OnCutsceneEnded.AddUObject(this, &UWxAbilityTask_PlaySkillCutscene::HandleCutsceneEnded);
			return;
		}
		SessionId = Coordinator->GetSessionId();
		EndedHandle = Coordinator->OnCutsceneEnded.AddUObject(this, &UWxAbilityTask_PlaySkillCutscene::HandleCutsceneEnded);
		if (Coordinator->Start(Ability, LevelSequence, GlobalTimeDilation))
		{
			return;
		}
	}
	HandleCutsceneEnded(GetAvatarActor(), SessionId, true);
}

void UWxAbilityTask_PlaySkillCutscene::HandleCutsceneEnded(AActor* Avatar, uint32 FinishedId, bool bCancelled)
{
	if (Avatar != GetAvatarActor() || FinishedId < SessionId)
	{
		return;
	}
	if (Coordinator.IsValid())
	{
		Coordinator->OnCutsceneEnded.Remove(EndedHandle);
	}
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		if (bCancelled)
		{
			OnCancelled.Broadcast();
		}
		else
		{
			OnCompleted.Broadcast();
		}
	}
	EndTask();
}

void UWxAbilityTask_PlaySkillCutscene::OnDestroy(bool bInOwnerFinished)
{
	if (Coordinator.IsValid())
	{
		Coordinator->OnCutsceneEnded.Remove(EndedHandle);
		Coordinator->Cancel(Ability);
	}
	Super::OnDestroy(bInOwnerFinished);
}
