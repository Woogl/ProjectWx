// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/WxCharacterBase.h"
#include "WxEnemyCharacter.generated.h"

/**
 * 에너미 캐릭터.
 * - AI Controller에 의해 제어
 * - PossessedBy에서 ASC InitAbilityActorInfo 호출
 * - 사망 시 일정 시간 뒤 자동 제거
 */
UCLASS()
class WXCOMBAT_API AWxEnemyCharacter : public AWxCharacterBase
{
	GENERATED_BODY()

public:
	AWxEnemyCharacter();

	virtual void PossessedBy(AController* NewController) override;

	/** 사망 후 액터 제거까지의 대기 시간 (초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Enemy")
	float DeathLifeSpan = 5.f;

protected:
	virtual void InitAbilityActorInfo() override;
	virtual void HandleDeath() override;
};
