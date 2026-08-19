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

UCLASS()
class WXUI_API UWxMVVMConversionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintPure, Category = "Wx", meta = (DisplayName = "To Visibility (GameplayTag)"))
	static ESlateVisibility Conv_GameplayTagToSlateVisibility(const FGameplayTagContainer& TagContainer, FGameplayTag Tag, ESlateVisibility TrueVisibility = ESlateVisibility::SelfHitTestInvisible, ESlateVisibility FalseVisibility = ESlateVisibility::Collapsed);

	/**
	 * 없으면 생성한다 — 바인딩이 요청한 어트리뷰트에 대해서만 VM 이 지연 생성된다.
	 * MaxAttribute 미지정 시 CurrentAttribute 를 최대값으로 사용한다.
	 */
	UFUNCTION(BlueprintPure, Category = "Wx", meta = (DisplayName = "Get Attribute ViewModel"))
	static UWxViewModel_Attribute* GetAttributeViewModel(UWxViewModel_AbilitySystem* AbilitySystem, FGameplayAttribute CurrentAttribute, FGameplayAttribute MaxAttribute);

	/**
	 * Asset Tags 가 AbilityTags 를 모두 포함하는(HasAll) 어빌리티 VM 을 가져오고, 없으면 생성한다.
	 * 바인딩이 요청한 어빌리티에 대해서만 VM 이 지연 생성된다.
	 * 매칭되는 어빌리티가 부여되지 않았으면 nullptr 를 반환한다.
	 */
	UFUNCTION(BlueprintPure, Category = "Wx", meta = (DisplayName = "Get Ability ViewModel"))
	static UWxViewModel_Ability* GetAbilityViewModel(UWxViewModel_AbilitySystem* AbilitySystem, FGameplayTagContainer AbilityTags);

	/**
	 * AbilityTags 로 찾은 어빌리티가 소모하는 자원의 VM 을 가져오고, 없으면 생성한다.
	 * 어빌리티 매칭은 Get Ability ViewModel 과 같다.
	 * 매칭되는 어빌리티가 없거나 코스트가 없으면 nullptr 를 반환한다.
	 */
	UFUNCTION(BlueprintPure, Category = "Wx", meta = (DisplayName = "Get Cost ViewModel"))
	static UWxViewModel_Attribute* GetCostViewModel(UWxViewModel_AbilitySystem* AbilitySystem, FGameplayTagContainer AbilityTags);

	UFUNCTION(BlueprintPure, Category = "Wx", meta = (DisplayName = "Find Effect ViewModel By Tag"))
	static UWxViewModel_Effect* FindActiveEffectViewModelByTag(UWxViewModel_AbilitySystem* AbilitySystem, FGameplayTag EffectTag);
};
