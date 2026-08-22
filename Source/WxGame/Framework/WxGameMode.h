// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "WxGameMode.generated.h"

class UWxExperienceDefinition;

/**
 * 게임플레이 구성은 Experience 가 정의한다.
 * 본 클래스는 이 판의 Experience 를 확정해 GameState 의 Experience 매니저에 넘기고, 매니저가 참조를 복제해 서버·클라 각자 로드·적용한다.
 * GameMode 가 서버에만 있어도 클라 적용이 성립한다.
 * 로드는 비동기라 접속(PostLogin)보다 늦을 수 있다 — 폰 스폰(HandleStartingNewPlayer)을 로드 완료까지 미루고, 완료 시점에 대기 접속자를 일괄 스폰·지급한다.
 * 플레이어 폰 클래스도 Experience 가 정한다 — 상속받은 DefaultPawnClass 는 읽지 않는다.
 * 플레이어 스폰 위치는 엔진 기본 경로에 전적으로 맡긴다 — 저장된 재개 지점은 UWxPlayerSpawnComponent 가 StartSpot 으로 심고, 신규 세션 시작지점 선택은 엔진 ChoosePlayerStart(PIE + 미점유 APlayerStart)가 처리한다.
 * 저장된 플레이어 스탯 복원도 같은 컴포넌트가 빙의 시 수행한다.
 * 새 플레이어가 무엇을 들고 시작하는지는 Experience 가 정한다 — 본 클래스는 지급 시점만 관장해 그 컨트롤러의 인벤토리에 넣는다(PlayerController 는 인벤토리를 알지 않는다).
 * 적/오브젝트 등 savable 액터의 상태 복원은 새 월드의 UWxSaveWorldSubsystem 이 OnWorldInitializedActors 후크로 자동 처리한다.
 */
UCLASS()
class AWxGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	//~ Begin AGameModeBase
	virtual void InitGameState() override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	//~ End AGameModeBase

private:
	/** 진입 URL 옵션(?Experience=이름) → WorldSettings → DefaultExperience 순으로 이 판의 Experience 를 확정한다. 실패 시 무효 ID. */
	FPrimaryAssetId ResolveExperienceId() const;

	void HandleExperienceLoaded(const UWxExperienceDefinition* Experience);

	void GrantDefaultInventory(APlayerController* PlayerController, const UWxExperienceDefinition* Experience) const;
};
