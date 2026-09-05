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
 * 슬롯에 어떤 어빌리티가 물리는지는 뷰모델이 스스로 정하고 부여가 바뀌면 갈아타므로, 여기서는 슬롯을 지목만 한다.
 */
UCLASS(EditInlineNew, CollapseCategories)
class WXGAME_API UWxViewModelResolver_Ability : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;

	/** 이 슬롯이 지목하는 어빌리티 에셋 태그. 슬롯에 들어올 어빌리티는 이 태그를 달아야 한다. */
	UPROPERTY(EditAnywhere, Category = "Wx")
	FGameplayTagContainer AbilityTags;
};
