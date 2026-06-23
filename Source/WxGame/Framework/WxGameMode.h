// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "WxGameMode.generated.h"

struct FComponentRequestHandle;
class UGameFrameworkComponent;

/**
 * 기본 GameMode.
 *
 * 스폰 지점 선택은 GameState 의 UWxPlayerSpawningComponent 에 위임한다(Lyra 패턴). ChoosePlayerStart 가 컴포넌트를
 * 찾아 위임하고, 컴포넌트가 nullptr 을 주면(저장/"Default" 태그 미발견) 엔진 기본(미점유 PlayerStart 랜덤)으로 폴백한다.
 * 게임플레이 프레임워크 컴포넌트는 InjectedFrameworkComponents(에셋 설정)를 InitGame 에서 ModularGameplay 컴포넌트
 * 매니저에 요청 등록해 receiver 액터(GameState 등)에 자동 주입한다 — GameState 가 컴포넌트를 알지 않아도 된다.
 * 적/오브젝트 등 savable 액터의 상태 복원은 새 월드 초기화 시 UWxSaveGameSubsystem 의 OnWorldInitializedActors 후크가 자동 처리한다.
 */
UCLASS()
class AWxGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	/** 스폰 지점 선택을 UWxPlayerSpawningComponent 에 위임하고, 없거나 못 찾으면 엔진 기본으로 폴백한다. */
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	//~ Begin AGameModeBase
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	//~ End AGameModeBase

protected:
	/** receiver 액터에 자동 주입할 프레임워크 컴포넌트 목록. 부착 대상은 컴포넌트 베이스로 자동 추론된다. 디자이너가 GameMode 에셋에서 설정(ModularGameplay). */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Framework Components")
	TArray<TSubclassOf<UGameFrameworkComponent>> InjectedFrameworkComponents;

private:
	/** 주입 요청 핸들. 살아있는 동안 컴포넌트가 유지되므로 GameMode 수명 동안 보유한다. */
	TArray<TSharedPtr<FComponentRequestHandle>> ComponentRequestHandles;
};
