// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "View/MVVMViewModelContextResolver.h"
#include "WxViewModelResolver_Ability.generated.h"

class UUserWidget;
class UMVVMView;

/**
 * 위젯을 소유한 PlayerController 의 빙의 Pawn 에서 AbilityTags 에 해당하는 어빌리티를 찾아 UWxViewModel_Ability 를 생성/초기화한다.
 * 어빌리티의 최대 충전 수는 UWxViewModel_Ability 가 소속된 WxUI 에서 읽을 수 없으므로, 양쪽에 의존하는 본 리졸버가 주입한다.
 * 해당 어빌리티가 부여되어 있지 않으면 뷰모델을 만들지 않는다.
 */
UCLASS(EditInlineNew, CollapseCategories)
class WXGAME_API UWxViewModelResolver_Ability : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;

	/**
	 * Asset Tags 가 이 태그들을 모두 포함하는(HasAll) 어빌리티를 부여된 스펙에서 찾는다.
	 * 여러 어빌리티가 매칭되면 첫 번째 것을 사용한다.
	 */
	UPROPERTY(EditAnywhere, Category = "Wx")
	FGameplayTagContainer AbilityTags;
};
