// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "View/MVVMViewModelContextResolver.h"
#include "WxViewModelResolver_Dialogue.generated.h"

class UUserWidget;
class UMVVMView;

/**
 * 소유 컨트롤러의 대화 세션을 위젯별 Dialogue VM에 연결한다. 대사는 세션에서 VM으로, 진행 입력은 VM에서 세션으로 간다.
 * 리졸버는 위젯 클래스가 공유하므로 구독은 VM을 소유자로 걸고, 해제도 그 VM의 것만 끊는다.
 */
UCLASS(EditInlineNew, CollapseCategories)
class WXGAME_API UWxViewModelResolver_Dialogue : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;

	virtual void DestroyInstance(UObject* ViewModel, const UMVVMView* View) const override;
};
