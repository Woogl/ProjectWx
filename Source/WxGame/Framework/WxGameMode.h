// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "WxGameMode.generated.h"

class UWxExperienceDefinition;

/**
 * 게임플레이 구성은 Experience 가 정의한다.
 * 본 클래스는 이 판의 Experience 를 확정해 GameState 의 Experience 매니저에 넘긴다 — 매니저가 참조를 복제하므로 GameMode 가 서버에만 있어도 클라 적용이 성립한다.
 * 로드는 비동기라 접속(PostLogin)보다 늦을 수 있다 — 폰 스폰(HandleStartingNewPlayer)을 로드 완료까지 미루고, 완료 시점에 대기 접속자를 일괄 스폰한다.
 * 플레이어 폰 클래스는 FrontEnd 선택을 우선하고, 선택이 없으면 Experience 기본값을 쓴다.
 * 폰 클래스가 비어 있으면 폰 없는 Experience(프론트엔드)라 엔진 스펙테이터 폰(SpectatorClass)으로 빙의시킨다.
 * 시작 아이템은 본 클래스가 관여하지 않는다 — Experience 의 Add Inventory Items 액션이 지급한다.
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
	//~ End AGameModeBase

private:
	/** 진입 URL 옵션(?Experience=이름) → WorldSettings 순으로 이 판의 Experience 를 확정한다. 그 뒤의 폴백은 없다 — 둘 다 비면 무효 ID 를 돌려주고 매니저가 에러로 드러낸다. */
	FPrimaryAssetId ResolveExperienceId() const;

	void HandleExperienceLoaded(const UWxExperienceDefinition* Experience);
};
