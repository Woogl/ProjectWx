// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/WxEnemyCharacter.h"
#include "GameplayTagContainer.h"
#include "WxBossCharacter.generated.h"

class UWxViewModel_Character;

/**
 * 보스 캐릭터.
 * 인식 상태(State.Recognized)를 관찰하여:
 *  - 인식되면 글로벌 Shell 뷰모델에 자신의 ASC를 세팅(보스 체력바 표시)
 *  - 인식이 해제되거나 EndPlay 시 Shell 뷰모델 내부 상태를 해제
 *
 * 시야를 잠시 잃어도(보스 등 뒤로 이동 등) 인식이 유지되는 한 체력바도 유지된다.
 */
UCLASS(Abstract)
class WXGAME_API AWxBossCharacter : public AWxEnemyCharacter
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleRecognizedTagChanged(const FGameplayTag Tag, int32 NewCount);

	void ActivateBossCharacterViewModel();
	void DeactivateBossCharacterViewModel();

	UWxViewModel_Character* GetBossCharacterViewModel() const;
};
