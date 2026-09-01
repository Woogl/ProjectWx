// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/WxCharacterBase.h"
#include "WxMinion.generated.h"

/**
 * 소환으로 태어나 소환자의 팀으로 싸우는 전투 폰.
 *
 * 적대 여부는 클래스가 아니라 팀이 가르며, 팀은 MinionComponent가 스폰 시점에 소환자에게서 물려준다.
 * 배치가 아니라 스폰으로만 태어나므로 자동 빙의를 스폰까지 열어 두는 것이 에너미와의 유일한 차이다.
 */
UCLASS()
class WXGAME_API AWxMinion : public AWxCharacterBase
{
	GENERATED_BODY()

public:
	AWxMinion(const FObjectInitializer& ObjectInitializer);
};
