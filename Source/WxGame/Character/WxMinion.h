// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/WxCharacterBase.h"
#include "WxMinion.generated.h"

/**
 * 소환으로 태어나 소환자의 팀으로 싸우는 전투 폰.
 *
 * 적대 여부는 클래스가 아니라 팀이 가르며, 팀은 MinionComponent가 스폰 시점에 소환자에게서 물려준다.
 * 적이 아니라서 WxEnemyCharacter 계열이 아니다 — 네임플레이트·피니셔·보상 같은 적 전용 장치를 받지 않는다.
 */
UCLASS()
class WXGAME_API AWxMinion : public AWxCharacterBase
{
	GENERATED_BODY()

public:
	AWxMinion(const FObjectInitializer& ObjectInitializer);
};
