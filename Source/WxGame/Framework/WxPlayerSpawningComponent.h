// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/GameStateComponent.h"
#include "WxPlayerSpawningComponent.generated.h"

class APlayerStart;
class AController;

/**
 * 플레이어 스폰 지점 선택을 담당하는 GameState 컴포넌트 (Lyra PlayerSpawningManager 패턴).
 *
 * ChoosePlayerStart 는 GameMode virtual 이라 컴포넌트로 완전히 옮길 수 없으므로, AWxGameMode 는 얇은 위임만 남기고 실제 선택 로직(PIE 시작 → 저장 PlayerStartTag → 최초 접속 bIsDefaultStart 체크포인트)을 이 컴포넌트가 소유한다.
 * Lyra 와 동일하게 ModularGameplay 의 UGameStateComponent 를 상속한다.
 * AWxGameState 가 생성자에서 부착한다.
 */
UCLASS()
class WXGAME_API UWxPlayerSpawningComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
	UWxPlayerSpawningComponent(const FObjectInitializer& ObjectInitializer);

	/**
	 * 부활/시작 PlayerStart 를 선택한다: ⓪ PIE 시작 지점 → ① 저장된 PlayerStartTag(체크포인트 등) → ② bIsDefaultStart 체크포인트(최초 접속).
	 * 모두 못 찾으면 nullptr 을 반환한다(호출 측 GameMode 가 엔진 기본 선택으로 폴백).
	 * 체크포인트는 있는데 최초 시작지점 미지정이면 에러 로그를 남긴다.
	 */
	AActor* ChoosePlayerStart(AController* Player);

private:
	/**
	 * 월드에서 PlayerStartTag 가 일치하는 첫 PlayerStart 를 직접 탐색(미일치 시 nullptr).
	 * 엔진 FindPlayerStart 의 ChoosePlayerStart 재귀 폴백을 피한다.
	 */
	APlayerStart* FindPlayerStartByTag(FName Tag) const;
};
