// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Items/WxRewardTableRow.h"
#include "WxGameMode.generated.h"

class UWxExperienceDefinition;

/**
 * 기본 GameMode.
 *
 * 플레이어 스폰은 엔진 기본 경로에 전적으로 맡긴다 — 저장된 재개 지점은 UWxPlayerSpawnComponent 가 로그인 시 StartSpot 으로 심고,
 * 신규 세션 시작지점 선택은 엔진 ChoosePlayerStart(PIE + 미점유 APlayerStart)가 처리한다. 저장된 플레이어 스탯 복원도 같은 컴포넌트가 빙의 시 수행한다.
 * 게임플레이 프레임워크 컴포넌트 구성은 Experience(에셋 설정)가 정의한다 — InitGameState 에서 GameState 로 넘기면 GameState 가 참조를 복제해 서버·클라 각자 등록한다. GameMode 가 서버에만 있어도 클라 주입이 성립한다.
 * 새 플레이어가 무엇을 들고 시작하는지도 본 클래스가 정한다 — 접속 시 그 컨트롤러의 인벤토리에 DefaultInventoryItems 를 지급한다(PlayerController 는 인벤토리를 알지 않는다).
 * 적/오브젝트 등 savable 액터의 상태 복원은 새 월드의 UWxSaveWorldSubsystem 이 OnWorldInitializedActors 후크로 자동 처리한다.
 */
UCLASS()
class AWxGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	//~ Begin AGameModeBase
	virtual void InitGameState() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	//~ End AGameModeBase

protected:
	/** 이 모드의 프레임워크 컴포넌트 구성. GameState 가 참조를 복제해 서버·클라 각자 적용한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TObjectPtr<const UWxExperienceDefinition> Experience;

	/**
	 * 접속한 플레이어의 인벤토리에 1회 지급할 시작 아이템 목록.
	 * 빈(아이템 미지정) 항목은 무시되며, 아이템 정의는 지급 시점에 동기 로드된다.
	 * 인벤토리가 Experience 에 등록되지 않은 모드에서는 지급 대상이 없어 조용히 건너뛴다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TArray<FWxItemRewardEntry> DefaultInventoryItems;
};
