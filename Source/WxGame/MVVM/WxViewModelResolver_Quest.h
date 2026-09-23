// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "View/MVVMViewModelContextResolver.h"
#include "WxViewModelResolver_Quest.generated.h"

class UUserWidget;
class UMVVMView;

/**
 * GameState의 퀘스트 저널을 위젯별 Quest VM에 싣는다.
 * 리졸버는 위젯 클래스가 공유하므로 구독은 VM을 소유자로 걸고, 해제도 그 VM의 것만 끊는다.
 */
UCLASS(EditInlineNew, CollapseCategories)
class WXGAME_API UWxViewModelResolver_Quest : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;

	virtual void DestroyInstance(UObject* ViewModel, const UMVVMView* View) const override;
};
