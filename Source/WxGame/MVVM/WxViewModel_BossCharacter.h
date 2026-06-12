// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "MVVM/WxViewModel_Character.h"
#include "View/MVVMViewModelContextResolver.h"
#include "WxViewModel_BossCharacter.generated.h"

class AWxBossCharacter;
class UUserWidget;
class UMVVMView;

/**
 * 보스 존재를 스스로 관찰하는 보스 네임플레이트용 뷰모델.
 *
 * 네임플레이트 위젯은 상시 HUD 에 살고 보스는 월드에 늦게 스폰되거나 사라질 수 있어 두 수명이 어긋난다.
 * 리졸버가 돌려준 인스턴스는 뷰가 교체할 수 없으므로, 인스턴스는 고정한 채 월드의 보스 스폰/EndPlay 를 직접 관찰해 내부 상태(Initialize/Deinitialize)만 갈아끼운다.
 * 보스가 미리 배치됐든 중간에 스폰되든 위젯 생성 순서와 무관하게 동작한다.
 */
UCLASS()
class WXGAME_API UWxViewModel_BossCharacter : public UWxViewModel_Character
{
	GENERATED_BODY()

public:
	/** 월드의 보스 스폰/소멸 관찰을 시작하고, 이미 존재하는 보스가 있으면 즉시 연결한다. */
	void StartObserving(UWorld* World);

	virtual void BeginDestroy() override;

private:
	void HandleActorSpawned(AActor* SpawnedActor);

	UFUNCTION()
	void HandleBossEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);

	void SetBoss(AWxBossCharacter* Boss);

	TWeakObjectPtr<UWorld> ObservedWorld;

	FDelegateHandle ActorSpawnedHandle;

	TWeakObjectPtr<AWxBossCharacter> CurrentBoss;
};

/**
 * VM_BossCharacter 용 View Bindings Resolver.
 *
 * 위젯별로 관찰형 보스 뷰모델(UWxViewModel_BossCharacter)을 생성해 돌려준다. 보스 탐색/연결은 뷰모델이 스스로 수행한다.
 * WBP 의 View Bindings 에서 Creation Type = Resolver 로 본 클래스를 선택한다.
 */
UCLASS(EditInlineNew, CollapseCategories)
class WXGAME_API UWxViewModelResolver_BossCharacter : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
};
