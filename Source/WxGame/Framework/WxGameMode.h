// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Items/WxRewardTableRow.h"
#include "WxGameMode.generated.h"

struct FComponentRequestHandle;
class UGameFrameworkComponent;

/**
 * 기본 GameMode.
 *
 * 플레이어 스폰은 엔진 기본 경로에 전적으로 맡긴다 — 저장된 재개 지점은 UWxPlayerSpawnComponent 가 로그인 시 StartSpot 으로 심고,
 * 신규 세션 시작지점 선택은 엔진 ChoosePlayerStart(PIE + 미점유 APlayerStart)가 처리한다. 저장된 플레이어 스탯 복원도 같은 컴포넌트가 빙의 시 수행한다.
 * 게임플레이 프레임워크 컴포넌트는 FrameworkComponents(에셋 설정)를 InitGame 에서 ModularGameplay 컴포넌트 매니저에 요청 등록해 receiver 액터(GameState 등)에 자동 주입한다 — GameState 가 컴포넌트를 알지 않아도 된다.
 * 새 플레이어가 무엇을 들고 시작하는지도 본 클래스가 정한다 — 접속 시 그 컨트롤러의 인벤토리에 DefaultInventoryItems 를 지급한다(PlayerController 는 인벤토리를 알지 않는다).
 * 적/오브젝트 등 savable 액터의 상태 복원은 새 월드의 UWxSaveWorldSubsystem 이 OnWorldInitializedActors 후크로 자동 처리한다.
 */
UCLASS()
class AWxGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	//~ Begin AGameModeBase
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	//~ End AGameModeBase

protected:
	/** ModularGameplay 프레임워크 컴포넌트 목록. 부착 대상은 컴포넌트 베이스로 자동 추론된다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TArray<TSubclassOf<UGameFrameworkComponent>> FrameworkComponents;

	/**
	 * 접속한 플레이어의 인벤토리에 1회 지급할 시작 아이템 목록.
	 * 빈(아이템 미지정) 항목은 무시되며, 아이템 정의는 지급 시점에 동기 로드된다.
	 * 인벤토리가 FrameworkComponents 에 등록되지 않은 모드에서는 지급 대상이 없어 조용히 건너뛴다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TArray<FWxItemRewardEntry> DefaultInventoryItems;

private:
	/** 주입 요청 핸들. 살아있는 동안 컴포넌트가 유지되므로 GameMode 수명 동안 보유한다. */
	TArray<TSharedPtr<FComponentRequestHandle>> ComponentRequestHandles;
};
