// Copyright Woogle. All Rights Reserved.

#include "Character/Component/WxBossComponent.h"
#include "Character/WxEnemyCharacter.h"
#include "WxGame.h"

FWxOnBossReady UWxBossComponent::OnAnyBossReady;

void UWxBossComponent::BeginPlay()
{
	Super::BeginPlay();

	// 체력바가 ASC·이름·초상화·타겟 유무를 적 캐릭터에서 읽으므로, 그 타입이 아니면 보스로 발행하지 않는다.
	if (!GetBossCharacter())
	{
		UE_LOG(LogWxGame, Warning, TEXT("%s: WxBossComponent 는 WxEnemyCharacter 에만 부착할 수 있다."), *GetNameSafe(GetOwner()));
		return;
	}

	OnAnyBossReady.Broadcast(this);
}

AWxEnemyCharacter* UWxBossComponent::GetBossCharacter() const
{
	return GetOwner<AWxEnemyCharacter>();
}
