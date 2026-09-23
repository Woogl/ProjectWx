// Copyright Woogle. All Rights Reserved.

#include "Battle/WxBattleSubsystem.h"
#include "AbilitySystemComponent.h"
#include "Character/WxCharacterBase.h"
#include "WxGameplayTags.h"

void UWxBattleSubsystem::NotifyEngagementChanged(AWxCharacterBase* Character, bool bEngaged)
{
	const AWxCharacterBase* PreviousBoss = GetCurrentBoss();
	if (bEngaged)
	{
		// 교전 중에 태그가 빠져도 목록에서 내려갈 수 있게 보스 판정은 추가할 때만 한다.
		const UAbilitySystemComponent* ASC = Character ? Character->GetAbilitySystemComponent() : nullptr;
		if (!ASC || !ASC->HasMatchingGameplayTag(WxGameplayTags::Character_Boss))
		{
			return;
		}
		EngagedBosses.AddUnique(Character);
	}
	else
	{
		EngagedBosses.Remove(Character);
	}

	AWxCharacterBase* CurrentBoss = GetCurrentBoss();
	if (CurrentBoss != PreviousBoss)
	{
		OnCurrentBossChanged.Broadcast(CurrentBoss);
	}
}

AWxCharacterBase* UWxBattleSubsystem::GetCurrentBoss() const
{
	return EngagedBosses.IsEmpty() ? nullptr : EngagedBosses[0].Get();
}
