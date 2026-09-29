// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "View/MVVMViewModelContextResolver.h"
#include "WxViewModelResolver_Item.generated.h"

class UWxItemDefinition;

/** 고정 아이템의 합계 VM 을 플레이어 Inventory VM 에서 얻는다. 정의로 채우는 일은 그 VM 을 채우는 PC 가 맡는다. */
UCLASS(EditInlineNew, CollapseCategories)
class WXGAME_API UWxViewModelResolver_Item : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Wx|Inventory")
	TObjectPtr<UWxItemDefinition> ItemToDisplay;

	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
};
