// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MVVM/WxViewModel.h"
#include "WxViewModel_AbilitySystem.generated.h"

struct FGameplayAttribute;
struct FActiveGameplayEffectHandle;
struct FGameplayEffectSpec;
struct FActiveGameplayEffect;
class UAbilitySystemComponent;
class UWxViewModel_Attribute;
class UWxViewModel_Ability;
class UWxViewModel_Effect;

/**
 * 어빌리티 시스템 뷰모델 (Composite).
 * ASC의 모든 어빌리티를 자식 UWxViewModel_Ability 배열로 관리한다.
 *
 * 사용 흐름:
 *  1. Initialize(ASC)로 초기화 → 보유 어빌리티마다 자식 ViewModel 생성
 *  2. UI에서 AbilityViewModels 배열을 ListView 등에 바인딩
 *  3. 런타임에 어빌리티가 추가/제거되면 RebuildAbilityViewModels() 호출
 */
UCLASS()
class WXUI_API UWxViewModel_AbilitySystem : public UWxViewModel
{
	GENERATED_BODY()

public:
	void Initialize(UAbilitySystemComponent* InASC);
	virtual void Deinitialize() override;

	UWxViewModel_Attribute* FindAttributeViewModel(FGameplayAttribute InAttribute) const;
	UWxViewModel_Ability* FindAbilityViewModel(FGameplayTag InAbilityTag) const;
	UWxViewModel_Effect* FindActiveEffectViewModel(FGameplayTag InEffectTag) const;

	/**
	 * (현재값, 최대값) 어트리뷰트 쌍에 대응하는 어트리뷰트 VM 을 반환한다. 없으면 생성하여 캐시한다.
	 * UI 바인딩이 실제로 요청한 어트리뷰트에 대해서만 VM 이 지연 생성된다.
	 * Max 가 유효하지 않으면 Current 자신을 최대값으로 사용한다.
	 */
	UWxViewModel_Attribute* GetOrCreateAttributeViewModel(FGameplayAttribute Current, FGameplayAttribute Max);

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|AbilitySystem")
	TArray<TObjectPtr<UWxViewModel_Ability>> AbilityViewModels;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|AbilitySystem")
	TArray<TObjectPtr<UWxViewModel_Effect>> ActiveEffectViewModels;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|AbilitySystem")
	FGameplayTagContainer OwnedTags;

protected:
	void InitializeAbilityViewModels();

	/** 이펙트 목록을 재구축한다. 런타임에 이펙트가 추가/제거되었을 때 호출 */
	void RefreshActiveEffectViewModels();

	/** OwnedTags를 ASC에서 읽어 갱신한다. 태그 변경 시 호출 */
	void RefreshOwnedTags();

	void HandleActiveEffectAdded(UAbilitySystemComponent* InASC, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle);
	void HandleActiveEffectRemoved(const FActiveGameplayEffect& ActiveEffect);
	void HandleTagChanged(const FGameplayTag Tag, int32 NewCount);

	/** 지연 생성된 어트리뷰트 VM 캐시. 바인딩이 요청한 어트리뷰트에 대해서만 채워진다. GetOrCreateAttributeViewModel 참조. */
	UPROPERTY()
	TArray<TObjectPtr<UWxViewModel_Attribute>> AttributeViewModels;

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
};
