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
 * 신규 세션 시작지점 선택은 엔진 기본 ChoosePlayerStart(PIE + 미점유 APlayerStart)에 맡긴다.
 * 게임플레이 프레임워크 컴포넌트는 FrameworkComponents(에셋 설정)를 InitGame 에서 ModularGameplay 컴포넌트 매니저에 요청 등록해 receiver 액터(GameState 등)에 자동 주입한다 — GameState 가 컴포넌트를 알지 않아도 된다.
 * 적/오브젝트 등 savable 액터의 상태 복원은 새 월드의 UWxPersistenceWorldSubsystem 이 OnWorldInitializedActors 후크로 자동 처리한다.
 */
UCLASS()
class AWxGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	//~ Begin AGameModeBase
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	/** 저장된 부활 지점이 있으면(TryGetSavedRespawnTransform) 그 트랜스폼으로, 아니면 인자(PlayerStart)로 Super 스폰한다. */
	virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) override;

	/** 부활 스폰(Super) 이후 저장된 플레이어 스탯을 복원하고, 저장 지점으로 복원된 경우 카메라(컨트롤 로테이션)를 캐릭터가 바라보는 방향으로 맞춘다. */
	virtual void FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation) override;
	//~ End AGameModeBase

protected:
	/** ModularGameplay 프레임워크 컴포넌트 목록. 부착 대상은 컴포넌트 베이스로 자동 추론된다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TArray<TSubclassOf<UGameFrameworkComponent>> FrameworkComponents;

private:
	/**
	 * 활성 세이브에 저장된 부활 지점 트랜스폼이 있고 저장 맵이 현재 월드와 일치하면 OutTransform 에 채우고 true.
	 * 없으면(신규 세션/체크포인트 미접촉/맵 불일치) false.
	 * 유효성은 Identity(미설정) sentinel 로 판정한다(별도 bool 플래그 없음).
	 * PIE "여기서 플레이"(APlayerStartPIE)가 있으면 개발 중 지정 위치 우선이라 false 를 반환한다.
	 * 스폰 위치(SpawnDefaultPawnAtTransform)와 로드 직후 카메라 시선(FinishRestartPlayer)이 공유한다.
	 */
	bool TryGetSavedRespawnTransform(FTransform& OutTransform) const;

	/** 주입 요청 핸들. 살아있는 동안 컴포넌트가 유지되므로 GameMode 수명 동안 보유한다. */
	TArray<TSharedPtr<FComponentRequestHandle>> ComponentRequestHandles;
};
