// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ControllerComponent.h"
#include "WxPlayerSpawnComponent.generated.h"

class AGameModeBase;
class APawn;
class APlayerController;

/**
 * 저장된 플레이어 상태(재개 지점·스탯)를 새 세션에 세우는 컨트롤러 컴포넌트.
 *
 * GameMode 와 PlayerController 를 건드리지 않고 엔진 기본 스폰 경로에 올라탄다: PostLogin 에서 저장 좌표에 APlayerStart 를 스폰해 AController::StartSpot 에 꽂아두면,
 * 이후 RestartPlayer -> FindPlayerStart 가 ShouldSpawnAtStartSpot 을 통해 ChoosePlayerStart 대신 그 액터를 고르고, 폰 스폰 위치와 컨트롤 로테이션까지 엔진이 처리한다.
 * 로드도 사망 부활도 맵 리로드를 거쳐 이 경로를 다시 타므로 재개 지점 하나로 둘 다 처리된다 — 세이브가 단일 원천이고 마커는 월드 수명의 파생물이다.
 * 월드파티션에서는 UpdatePlayerStartSpot 이 PC 를 StartSpot 자리로 옮겨, 스트리밍 소스인 PC 가 폰보다 먼저 그 셀을 끌어온다.
 *
 * GameMode 의 FrameworkComponents 에 등록해야 부착된다 — 등록하지 않으면 재개 지점과 스탯 복원이 조용히 동작하지 않는다.
 */
UCLASS()
class UWxPlayerSpawnComponent : public UControllerComponent
{
	GENERATED_BODY()

protected:
	//~ Begin UActorComponent
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	//~ End UActorComponent

private:
	/**
	 * GameModePostLoginEvent 핸들러: 오너 PC 의 빙의를 구독하고, 저장된 재개 지점이 있으면 그 좌표에 마커를 스폰해 StartSpot 으로 등록한다.
	 * PostLogin 이 InitNewPlayer(StartSpot 확정)보다 뒤, RestartPlayer 보다 앞이라 이 시점 주입이 성립한다.
	 */
	void HandleGameModePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer);

	/** 빙의 시 저장된 스탯을 새 폰에 복원한다. Possess 가 PossessedBy(ASC 초기화)를 끝낸 뒤 발화하므로 기본값 위에 안전하게 덮어쓴다. */
	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	FDelegateHandle PostLoginHandle;
};
