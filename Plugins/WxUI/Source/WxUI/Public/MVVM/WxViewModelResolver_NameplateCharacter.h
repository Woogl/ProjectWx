// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "View/MVVMViewModelContextResolver.h"
#include "WxViewModelResolver_NameplateCharacter.generated.h"

/** 네임플레이트 Owner의 공유 Character VM을 반환한다. ASC는 위젯 구성 전에 생성되어 있어야 한다. */
UCLASS(EditInlineNew, CollapseCategories)
class WXUI_API UWxViewModelResolver_NameplateCharacter : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
};
