// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/WxCharacterBase.h"
#include "WxEnemyCharacter.generated.h"

/**
 * 에너미 캐릭터.
 * - AI Controller에 의해 제어
 * - PossessedBy에서 ASC InitAbilityActorInfo 호출
 */
UCLASS()
class WX_API AWxEnemyCharacter : public AWxCharacterBase
{
	GENERATED_BODY()

public:
	AWxEnemyCharacter();

	virtual void PossessedBy(AController* NewController) override;

protected:
	virtual void InitAbilityActorInfo() override;
};
