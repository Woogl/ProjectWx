// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "View/MVVMViewModelContextResolver.h"
#include "WxViewModelResolver_BossCharacter.generated.h"

class UUserWidget;
class UMVVMView;

/**
 * 위젯별 보스 표시 영역 뷰모델을 생성한다. 교전 구독은 반환한 뷰모델이 소유한다.
 * 리졸버 자체는 위젯 클래스가 공유하므로 뷰의 상태나 구독을 보관하지 않는다.
 */
UCLASS(EditInlineNew, CollapseCategories)
class WXGAME_API UWxViewModelResolver_BossCharacter : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;

	virtual void DestroyInstance(UObject* ViewModel, const UMVVMView* View) const override;

};
