// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "MVVM/WxViewModel_Character.h"
#include "View/MVVMViewModelContextResolver.h"
#include "WxViewModel_BossCharacter.generated.h"

class AWxEnemyCharacter;
class UUserWidget;
class UMVVMView;
class UWxBossComponent;

/**
 * 보스 존재를 스스로 관찰하는 보스 네임플레이트용 뷰모델.
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
	bool bHasAITarget = false;

private:
	void HandleBossReady(UWxBossComponent* BossComponent);

	UFUNCTION()
	void HandleBossEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);

	void HandleAITargetChanged(bool bNewHasAITarget);

	void SetBoss(AWxEnemyCharacter* Boss);

	TWeakObjectPtr<UWorld> ObservedWorld;

	FDelegateHandle BossReadyHandle;

	TWeakObjectPtr<AWxEnemyCharacter> CurrentBoss;
};

UCLASS(EditInlineNew, CollapseCategories)
class WXGAME_API UWxViewModelResolver_BossCharacter : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
};
