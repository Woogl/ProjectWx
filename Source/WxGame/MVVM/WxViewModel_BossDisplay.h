// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/WxViewModel.h"
#include "WxViewModel_BossDisplay.generated.h"

class AWxEnemyCharacter;
class UWxViewModel_Character;
class UWorld;

/** 위젯별 보스 표시 영역. 교전 목록을 관찰하고 현재 표시할 캐릭터 데이터를 제공한다. */
UCLASS()
class WXGAME_API UWxViewModel_BossDisplay : public UWxViewModel
{
	GENERATED_BODY()

public:
	void StartObserving(UWorld* World);
	virtual void Deinitialize() override;

	/** 빈 상태에도 같은 자식을 유지하여 중첩 MVVM 바인딩이 계속 연결될 수 있게 한다. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Boss")
	TObjectPtr<UWxViewModel_Character> Character;

private:
	void HandleBossEngagementChanged(AWxEnemyCharacter* BossCharacter, bool bEngaged);
	void RefreshDisplayedBoss();

	TWeakObjectPtr<UWorld> ObservedWorld;
	TArray<TWeakObjectPtr<AWxEnemyCharacter>> EngagedBosses;
	TWeakObjectPtr<AWxEnemyCharacter> DisplayedBoss;
	FDelegateHandle BossEngagementHandle;
};
