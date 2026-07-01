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
class WXSOUND_API UWxMusicSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UWxMusicSettings();

	// 상태→BGM 선택을 정의하는 Chooser 테이블. (Result Class = UWxBGMData, Parameter = FWxBGMChooserContext)
	UPROPERTY(Config, EditAnywhere, Category = "BGM")
	TSoftObjectPtr<UChooserTable> DefaultBGMChooser;
};
