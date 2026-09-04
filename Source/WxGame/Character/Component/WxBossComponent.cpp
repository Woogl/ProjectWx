// Copyright Woogle. All Rights Reserved.

#include "Character/Component/WxBossComponent.h"
#include "Character/WxEnemyCharacter.h"
#include "Targeting/WxLockOnComponent.h"
#include "WxGame.h"

FWxOnBossReady UWxBossComponent::OnAnyBossReady;

void UWxBossComponent::BeginPlay()
{
	Super::BeginPlay();

	// 체력바 데이터와 전투 상태를 적 캐릭터에서 읽으므로, 그 타입이 아니면 보스로 발행하지 않는다.
	AWxEnemyCharacter* Boss = GetBossCharacter();
	if (!Boss)
	{
		UE_LOG(LogWxGame, Warning, TEXT("%s: WxBossComponent 는 WxEnemyCharacter 에만 부착할 수 있다."), *GetNameSafe(GetOwner()));
		return;
	}

	Boss->GetLockOnComponent()->OnLockOnTargetChanged.AddDynamic(this, &ThisClass::HandleAITargetChanged);
	Boss->OnDeath.AddDynamic(this, &ThisClass::HandleBossDeath);
	SetEngaged(Boss->HasAITarget());

	OnAnyBossReady.Broadcast(this);
}

void UWxBossComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AWxEnemyCharacter* Boss = GetBossCharacter())
	{
		Boss->GetLockOnComponent()->OnLockOnTargetChanged.RemoveDynamic(this, &ThisClass::HandleAITargetChanged);
		Boss->OnDeath.RemoveDynamic(this, &ThisClass::HandleBossDeath);
	}

	SetEngaged(false);
	OnBossEndPlay.Broadcast(this);

	Super::EndPlay(EndPlayReason);
}

AWxEnemyCharacter* UWxBossComponent::GetBossCharacter() const
{
	return GetOwner<AWxEnemyCharacter>();
}

bool UWxBossComponent::IsEngaged() const
{
	return bEngaged;
}

void UWxBossComponent::HandleAITargetChanged(USceneComponent* NewTarget)
{
	SetEngaged(NewTarget != nullptr);
}

void UWxBossComponent::HandleBossDeath(AWxCharacterBase* DeadCharacter)
{
	SetEngaged(false);
}

void UWxBossComponent::SetEngaged(bool bInEngaged)
{
	if (bEngaged == bInEngaged)
	{
		return;
	}

	bEngaged = bInEngaged;
	OnEngagementChanged.Broadcast(bEngaged);
}
