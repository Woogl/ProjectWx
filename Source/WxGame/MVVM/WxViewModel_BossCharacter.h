// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/WxViewModel_Character.h"
#include "View/MVVMViewModelContextResolver.h"
#include "WxViewModel_BossCharacter.generated.h"

class UUserWidget;
class UMVVMView;
class AWxEnemyCharacter;

/**
 * 보스로 설정된 AI 캐릭터를 관찰해 전투 데이터와 표시 상태를 FieldNotify로 중계한다.
 */
UCLASS()
class WXGAME_API UWxViewModel_BossCharacter : public UWxViewModel_Character
{
	GENERATED_BODY()

public:
	/** 이미 존재하는 보스가 있으면 즉시 연결한다. */
	void StartObserving(UWorld* World);

	virtual void BeginDestroy() override;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Boss")
	bool bBossBattleActive = false;

private:
	void HandleBossReady(AWxEnemyCharacter* BossCharacter);

	void HandleBossEndPlay(AWxEnemyCharacter* BossCharacter);

	void HandleEngagementChanged(bool bEngaged);

	void SetBoss(AWxEnemyCharacter* BossCharacter);

	TWeakObjectPtr<UWorld> ObservedWorld;

	FDelegateHandle BossReadyHandle;

	TWeakObjectPtr<AWxEnemyCharacter> CurrentBossCharacter;
};

UCLASS(EditInlineNew, CollapseCategories)
class WXGAME_API UWxViewModelResolver_BossCharacter : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
};
