// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/WxCharacterBase.h"
#include "WxEnemyCharacter.generated.h"

class UBehaviorTree;
class UWidgetComponent;

/**
 * 에너미 캐릭터.
 * - AWxEnemyController에 의해 제어
 * - BehaviorTree를 BP에서 지정하여 적 종류별 행동 패턴 분리
 */
UCLASS()
class WXGAME_API AWxEnemyCharacter : public AWxCharacterBase
{
	GENERATED_BODY()

public:
	AWxEnemyCharacter();

	UBehaviorTree* GetBehaviorTree() const;

	virtual void HandleDeath() override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx|UI")
	TObjectPtr<UWidgetComponent> HealthBarComponent;
};
