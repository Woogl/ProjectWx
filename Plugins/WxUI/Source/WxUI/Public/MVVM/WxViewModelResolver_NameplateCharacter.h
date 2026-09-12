// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "View/MVVMViewModelContextResolver.h"
#include "WxViewModelResolver_NameplateCharacter.generated.h"

/** Owner의 ASC별 Character VM은 공유하고, 거리·가시성 VM은 위젯별로 생성·해제한다. */
UCLASS(EditInlineNew, CollapseCategories)
class WXUI_API UWxViewModelResolver_NameplateCharacter : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
	virtual void DestroyInstance(UObject* ViewModel, const UMVVMView* View) const override;
};
