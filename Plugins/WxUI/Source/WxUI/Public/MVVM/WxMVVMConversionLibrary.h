// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/SlateWrapperTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WxMVVMConversionLibrary.generated.h"

struct FGameplayAttribute;
class UWxViewModel_AbilitySystem;
class UWxViewModel_Attribute;
class UWxViewModel_Ability;
class UWxViewModel_Effect;

/**
 * WxUI 전용 Blueprint Function Library.
 * MVVM 컨버전 함수 등 UI 바인딩에 필요한 유틸리티를 제공한다.
 */
UCLASS()
class WXUI_API UWxMVVMConversionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	/** 특정 태그 보유 여부에 따라 Visibility를 반환한다. */
	UFUNCTION(BlueprintPure, Category = "Wx", meta = (DisplayName = "To Visibility (GameplayTag)"))
	static ESlateVisibility Conv_GameplayTagToSlateVisibility(const FGameplayTagContainer& TagContainer, FGameplayTag Tag, ESlateVisibility TrueVisibility = ESlateVisibility::SelfHitTestInvisible, ESlateVisibility FalseVisibility = ESlateVisibility::Collapsed);

	/**
	 * AbilitySystem VM 에서 (현재값, 최대값) 어트리뷰트 쌍에 대응하는 어트리뷰트 VM 을 가져온다. 없으면 생성한다.
	 * 바인딩이 요청한 어트리뷰트에 대해서만 VM 이 지연 생성된다. MaxAttribute 미지정 시 CurrentAttribute 를 최대값으로 사용한다.
	 */
	UFUNCTION(BlueprintPure, Category = "Wx", meta = (DisplayName = "Get Attribute ViewModel"))
	static UWxViewModel_Attribute* GetAttributeViewModel(UWxViewModel_AbilitySystem* AbilitySystem, FGameplayAttribute CurrentAttribute, FGameplayAttribute MaxAttribute);

	/** AbilitySystem VM 에서 AbilityTag 가 일치하는 어빌리티 VM 을 반환한다. */
	UFUNCTION(BlueprintPure, Category = "Wx", meta = (DisplayName = "Find Ability ViewModel By Tag"))
	static UWxViewModel_Ability* FindAbilityViewModelByTag(UWxViewModel_AbilitySystem* AbilitySystem, FGameplayTag AbilityTag);

	/** AbilitySystem VM 에서 EffectTag 가 일치하는 이펙트 VM 을 반환한다. */
	UFUNCTION(BlueprintPure, Category = "Wx", meta = (DisplayName = "Find Effect ViewModel By Tag"))
	static UWxViewModel_Effect* FindActiveEffectViewModelByTag(UWxViewModel_AbilitySystem* AbilitySystem, FGameplayTag EffectTag);
};
