// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "View/MVVMViewModelContextResolver.h"
#include "WxViewModelResolver_InteractionList.generated.h"

class UUserWidget;
class UMVVMView;

/**
 * 소유 PC의 상호작용 스캐너를 위젯별 InteractionList VM에 싣고, VM의 명령을 스캐너에 잇는다.
 * 리졸버는 위젯 클래스가 공유하므로 구독은 VM을 소유자로 걸고, 해제도 그 VM의 것만 끊는다.
 * 스캐너는 AWxPlayerController 생성자 컴포넌트라 위젯보다 먼저 있다. 나중에 주입하는 구조로 바꾸면 늦은 도착 처리가 다시 필요하다.
 */
UCLASS(EditInlineNew, CollapseCategories)
class WXGAME_API UWxViewModelResolver_InteractionList : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;

	virtual void DestroyInstance(UObject* ViewModel, const UMVVMView* View) const override;
};
