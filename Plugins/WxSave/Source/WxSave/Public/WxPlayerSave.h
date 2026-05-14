// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "WxPlayerSave.generated.h"

/**
 * 플레이어 세션 SaveGame.
 * UWxSaveSubsystem 이 슬롯 IO 시 직접 직렬화한다.
 *
 * 단순 위치 복원 용도의 경량 데이터. 슬롯명/슬롯 인덱스는 서브시스템이 결정한다.
 */
UCLASS()
class WXSAVE_API UWxPlayerSave : public USaveGame
{
	GENERATED_BODY()

public:
	/** 마지막으로 저장된 플레이어 캐릭터의 월드 Transform. */
	UPROPERTY()
	FTransform PlayerTransform = FTransform::Identity;

	/** 유효한 위치가 저장되어 있는지 여부. */
	UPROPERTY()
	bool bHasSavedLocation = false;
};
