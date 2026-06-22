// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "WxGameMode.generated.h"

/**
 * 기본 GameMode.
 *
 * 부활 흐름: 클라이언트가 월드 재로드 (ServerTravel) 를 트리거하면 새 월드의 GameMode 가 ChoosePlayerStart 를 호출하면서
 * 슬롯에 기록된 활성 체크포인트 PlayerStartTag 로 배치된 체크포인트(AWxCheckPoint=APlayerStart)를 FindPlayerStart 로 찾아 우선 사용한다.
 * 적/오브젝트 등 다른 savable 액터의 상태 복원은 새 월드 초기화 시 UWxSaveGameSubsystem 의 OnWorldInitializedActors 후크가 자동 처리한다.
 */
UCLASS()
class AWxGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	/** 슬롯에 활성 체크포인트 태그가 있으면 그 PlayerStartTag 의 체크포인트를 찾아 반환, 없으면 기본 흐름. */
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
};
