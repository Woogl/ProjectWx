// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel_BossDisplay.h"

#include "AbilitySystemComponent.h"
#include "Character/WxEnemyCharacter.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "MVVM/WxViewModel_Character.h"
#include "WxGameplayTags.h"

void UWxViewModel_BossDisplay::StartObserving(UWorld* World)
{
	Deinitialize();
	if (!World)
	{
		return;
	}

	if (!Character)
	{
		UE_MVVM_SET_PROPERTY_VALUE(Character, NewObject<UWxViewModel_Character>(this));
	}
	ObservedWorld = World;
	BossEngagementHandle = AWxEnemyCharacter::OnAnyBossEngagementChanged.AddUObject(
		this, &ThisClass::HandleBossEngagementChanged);

	// 위젯보다 먼저 교전한 보스도 모두 수집하여 현재 보스 이탈 후의 후보를 남긴다.
	for (TActorIterator<AWxEnemyCharacter> It(World); It; ++It)
	{
		const UAbilitySystemComponent* ASC = It->GetAbilitySystemComponent();
		if (It->IsBoss() && ASC && ASC->HasMatchingGameplayTag(WxGameplayTags::State_Engaged))
		{
			EngagedBosses.Add(*It);
		}
	}
	RefreshDisplayedBoss();
}

void UWxViewModel_BossDisplay::Deinitialize()
{
	AWxEnemyCharacter::OnAnyBossEngagementChanged.Remove(BossEngagementHandle);
	BossEngagementHandle.Reset();
	ObservedWorld.Reset();
	EngagedBosses.Reset();
	DisplayedBoss.Reset();

	// GC 중에는 자식의 FieldNotify를 발행하지 않는다. 자식의 구독은 자신의 소멸 경로가 정리한다.
	if (Character && !HasAnyFlags(RF_BeginDestroyed))
	{
		Character->Deinitialize();
	}
	Super::Deinitialize();
}

void UWxViewModel_BossDisplay::HandleBossEngagementChanged(AWxEnemyCharacter* BossCharacter, bool bEngaged)
{
	if (!ObservedWorld.IsValid() || !BossCharacter || BossCharacter->GetWorld() != ObservedWorld.Get())
	{
		return;
	}

	if (bEngaged)
	{
		EngagedBosses.AddUnique(BossCharacter);
	}
	else
	{
		EngagedBosses.Remove(BossCharacter);
	}
	RefreshDisplayedBoss();
}

void UWxViewModel_BossDisplay::RefreshDisplayedBoss()
{
	for (int32 Index = EngagedBosses.Num() - 1; Index >= 0; --Index)
	{
		if (!EngagedBosses[Index].IsValid())
		{
			EngagedBosses.RemoveAt(Index);
		}
	}

	// 새 보스가 합류하거나 같은 교전 통지가 반복돼도 현재 표시를 갈아끼우지 않는다.
	AWxEnemyCharacter* NextBoss = EngagedBosses.IsEmpty() ? nullptr : EngagedBosses[0].Get();
	if (NextBoss == DisplayedBoss.Get() && (NextBoss || !Character->AbilitySystem))
	{
		return;
	}

	DisplayedBoss = NextBoss;
	if (NextBoss)
	{
		Character->Initialize(NextBoss->GetAbilitySystemComponent(), NextBoss->GetCharacterName(), NextBoss->GetPortrait());
	}
	else
	{
		Character->Deinitialize();
	}
}
