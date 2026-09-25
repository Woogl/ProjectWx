// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "View/MVVMViewModelContextResolver.h"
#include "WxViewModelResolver_AbilitySystem.generated.h"

class UAbilitySystemComponent;
class UWxViewModel_AbilitySystem;

/** GAS 공유 VM에 게임의 GE 표시 데이터를 연결한다. 캐릭터·슬롯 리졸버도 같은 연결을 사용한다. */
UCLASS(EditInlineNew, CollapseCategories)
class WXGAME_API UWxViewModelResolver_AbilitySystem : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;

	static UWxViewModel_AbilitySystem* GetOrCreate(UAbilitySystemComponent* InASC);
};
