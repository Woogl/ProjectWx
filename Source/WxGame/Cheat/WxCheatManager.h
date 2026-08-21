// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "WxCheatManager.generated.h"

/**
 * AWxPlayerController 가 CheatClass 로 지정하며, 엔진은 AGameModeBase::AllowCheats(Standalone·에디터)일 때만 이 객체를 만든다.
 * 따라서 배포 빌드에는 존재하지 않고, 존재하는 시점은 곧 권위 측이므로 각 치트는 권위 가드 없이 곧바로 적용한다.
 */
UCLASS()
class WXGAME_API UWxCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	/** 사망 연출·사망 화면까지 정상 경로로 확인하기 위한 치트다. */
	UFUNCTION(Exec)
	void WxKillPlayer();

	/**
	 * 대미지 파이프라인(HP 차감·DP 누적, 피격 리액션, 대미지 플로터)을 콘솔에서 재현하기 위한 치트다.
	 * 대미지 계산은 정상 경로를 그대로 타므로 무적·가드·퍼펙트 가드 상태에서는 그 판정이 적용된다.
	 */
	UFUNCTION(Exec)
	void WxDamagePlayer(float Amount = 30.f);

	/**
	 * 플레이어 캐릭터 주변 반경(미터) 안의 액터를 전부 즉사시킨다.
	 * 시전자 자신은 제외한다.
	 * 구역의 적을 한 번에 치워 「전원 처치」 집계·기믹 진행·전투 종료 흐름을 확인하기 위한 치트다.
	 */
	UFUNCTION(Exec)
	void WxKillEnemies(float RadiusMeters = 100.f);
};
