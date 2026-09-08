// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "View/MVVMViewModelContextResolver.h"
#include "WxViewModelResolver_Ability.generated.h"

class UUserWidget;
class UMVVMView;

/**
 * 위젯을 소유한 PlayerController 의 빙의 Pawn 에서 ASC 를 끌어와 AbilityTags 가 가리키는 스킬 슬롯의 뷰모델을 얻는다.
 * 후보가 여럿인 슬롯(소환↔명령처럼 상태로 갈리는 쌍)에서는 지금 조건에 맞는 후보의 뷰모델을 주고, 상태가 바뀐 뒤의 교체는 위젯에 붙인 UWxAbilitySlotSwitcher 가 맡는다.
 * 상황별 표시 여부는 위젯의 MVVM 가시성 바인딩에서 정한다.
 */
UCLASS(EditInlineNew, CollapseCategories)
class WXGAME_API UWxViewModelResolver_Ability : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;

	/** 이 슬롯이 지목하는 어빌리티 에셋 태그. 슬롯에 들어올 어빌리티는 이 태그를 달아야 하고, 여럿이면 서로 후보가 된다. */
	UPROPERTY(EditAnywhere, Category = "Wx")
	FGameplayTagContainer AbilityTags;
};
