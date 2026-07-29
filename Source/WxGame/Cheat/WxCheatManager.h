// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "WxCheatManager.generated.h"

/**
 * 개발용 콘솔 치트 모음.
 * AWxPlayerController 가 CheatClass 로 지정하며, 엔진은 AGameModeBase::AllowCheats(Standalone·에디터)일 때만 이 객체를 만든다.
 * 따라서 배포 빌드에는 존재하지 않고, 존재하는 시점은 곧 권위 측이므로 각 치트는 권위 가드 없이 곧바로 적용한다.
 */
UCLASS()
class WXGAME_API UWxCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	/** 플레이어 캐릭터를 즉사시킨다. 사망 연출·사망 화면까지 정상 경로로 확인하기 위한 치트다. */
	UFUNCTION(Exec)
	void WxKillPlayer();
};
