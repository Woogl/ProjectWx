// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "View/MVVMViewModelContextResolver.h"
#include "WxViewModelResolver_Player.generated.h"

/**
 * 로컬 PC 가 Global Collection 에 등록한 플레이어 공유 VM 을 위젯이 기대하는 클래스로 찾아 돌려준다.
 * WBP 가 이름 문자열 없이 받게 하려는 것이라, 플레이어 공유 VM 은 클래스마다 하나만 등록한다.
 */
UCLASS(EditInlineNew, CollapseCategories)
class WXGAME_API UWxViewModelResolver_Player : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
};
