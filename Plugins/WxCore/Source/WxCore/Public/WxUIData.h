// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WxUIData.generated.h"

/**
 * 아이콘·이름·설명·충전 칸 수처럼 UI 가 그대로 표시하는 데이터의 공용 계약.
 *
 * 계약이 WxCore 에 있으므로 WxUI 가 도메인 플러그인에 의존하지 않고도 이 값들을 읽는다.
 * 구현체는 어빌리티·GE 컴포넌트처럼 저작 데이터를 쥔 쪽이며, 대개 데이터테이블 행을 그대로 흘려보낸다.
 */
UINTERFACE(MinimalAPI, NotBlueprintable, meta = (CannotImplementInterfaceInBlueprint))
class UWxUIData : public UInterface
{
	GENERATED_BODY()
};

class WXCORE_API IWxUIData
{
	GENERATED_BODY()

public:
	virtual FText GetTitle() const = 0;

	virtual FText GetDescription() const = 0;

	/** 텍스처와 머티리얼 양쪽이 올 수 있다. 스트리밍은 소비처가 맡도록 소프트 참조로 넘긴다. */
	virtual TSoftObjectPtr<UObject> GetIcon() const = 0;

	/** 충전 칸 수. 충전 개념이 없는 구현체는 기본값 1 을 그대로 둔다. */
	virtual int32 GetMaxRecharges() const;
};
