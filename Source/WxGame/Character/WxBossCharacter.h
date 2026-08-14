// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/WxEnemyCharacter.h"
#include "WxBossCharacter.generated.h"

/**
 * 보스 체력바는 UWxViewModel_BossCharacter 가 본 타입의 스폰/EndPlay 를 관찰해 표시하므로, 이 클래스에는 UI 연동 코드가 없다.
 */
UCLASS(Abstract)
class WXGAME_API AWxBossCharacter : public AWxEnemyCharacter
{
	GENERATED_BODY()
};
