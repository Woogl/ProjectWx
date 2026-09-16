// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.h"
#include "Cutscene/WxSkillCutsceneComponent.h"

UWxAbilityTask_PlaySkillCutscene* UWxAbilityTask_PlaySkillCutscene::CreateTask(UGameplayAbility* OwningAbility)
{
	return NewAbilityTask<UWxAbilityTask_PlaySkillCutscene>(OwningAbility);
}

void UWxAbilityTask_PlaySkillCutscene::Activate()
{
	Super::Activate();

	Coordinator = UWxSkillCutsceneComponent::Get(GetWorld());
	if (!Coordinator.IsValid())
	{
		HandleCutsceneEnded(GetAvatarActor(), true);
		return;
	}

	EndedHandle = Coordinator->OnCutsceneEnded.AddUObject(this, &UWxAbilityTask_PlaySkillCutscene::HandleCutsceneEnded);
}

void UWxAbilityTask_PlaySkillCutscene::HandleCutsceneEnded(AActor* Avatar, bool bCancelled)
{
	// 동시에 도는 컷신은 하나뿐이므로 시전자만 맞으면 내 세션이다.
	if (Avatar != GetAvatarActor())
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
