// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "WxMusicSettings.generated.h"

class UChooserTable;

/**
 * BGM 시스템 프로젝트 설정. Project Settings > Wx > "Wx Music Settings".
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Wx Music Settings"))
class WXAUDIO_API UWxMusicSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UWxMusicSettings();

	// 상태→BGM 선택을 정의하는 Chooser 테이블. (Result Class = UWxBGMData, Parameter = FWxBGMChooserContext)
	UPROPERTY(Config, EditAnywhere, Category = "BGM")
	TSoftObjectPtr<UChooserTable> DefaultBGMChooser;

	// BGM 상태를 재평가하는 주기(초). 0 이면 이벤트(지역 변경/폰 교체)에서만 재평가한다.
	UPROPERTY(Config, EditAnywhere, Category = "BGM", meta = (ClampMin = "0.0"))
	float ReevaluateInterval = 0.5f;
};
