// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WxSavePreset.generated.h"

/**
 * 저장 정책 데이터 에셋. "무엇을, 어떻게" 저장할지 게임 측에서 에디터로 정의한다.
 *
 * 예정 항목: 사용할 SlotInfo/SlotData 클래스, 오토세이브 주기, 슬롯 최대 개수,
 * 비동기/동기 정책, 압축 여부 등.
 */
UCLASS()
class WXSAVE_API UWxSavePreset : public UDataAsset
{
	GENERATED_BODY()
};
