// Copyright Woogle. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "UITag.h"

/**
 * CommonUI 액션 태그(UI.Action.*) 네이티브 선언부.
 * FUIActionTag는 일반 게임플레이 태그와 등록 방식이 달라 별도 Adder로 선언한다.
 * 각 태그의 키 매핑은 Project Settings > Common UI Input Settings에서 지정한다.
 */
struct FWxUITags : public FGameplayTagNativeAdder
{
	/** 인벤토리 토글. HUD가 RegisterUIActionBinding으로 수신 */
	FUIActionTag UIAction_Inventory;

	/** 일시정지 메뉴 토글. HUD가 RegisterUIActionBinding으로 수신 */
	FUIActionTag UIAction_PauseMenu;

	virtual void AddTags() override
	{
		UIAction_Inventory = FUIActionTag::AddNativeTag(TEXT("Inventory"));
		UIAction_PauseMenu = FUIActionTag::AddNativeTag(TEXT("PauseMenu"));
	}

	static const FWxUITags& Get() { return GWxUITags; }

private:
	static FWxUITags GWxUITags;
};
