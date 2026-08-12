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
 * ASC의 어트리뷰트/어빌리티/이펙트를 자식 ViewModel로 노출한다.
 *
 * 어트리뷰트/어빌리티 VM 은 바인딩이 요청할 때 GetOrCreate... 로 지연 생성하고, 이펙트 VM 은 활성 GE 추가/제거 이벤트로 관리한다.
 */
UCLASS()
class WXUI_API UWxViewModel_AbilitySystem : public UWxViewModel
{
	GENERATED_BODY()

public:
	void Initialize(UAbilitySystemComponent* InASC);
	virtual void Deinitialize() override;

	UWxViewModel_Attribute* FindAttributeViewModel(FGameplayAttribute InAttribute) const;
	UWxViewModel_Ability* FindAbilityViewModel(const FGameplayTagContainer& InAbilityTags) const;
	UWxViewModel_Effect* FindActiveEffectViewModel(FGameplayTag InEffectTag) const;

	/**
	 * (현재값, 최대값) 어트리뷰트 쌍에 대응하는 VM 을 반환하고, 없으면 생성해 캐시한다.
	 * UI 바인딩이 실제로 요청한 어트리뷰트에 대해서만 VM 이 지연 생성된다.
	 * Max 가 유효하지 않으면 Current 자신을 최대값으로 사용한다.
	 */
	UWxViewModel_Attribute* GetOrCreateAttributeViewModel(FGameplayAttribute Current, FGameplayAttribute Max);

	/**
	 * Asset Tags 가 InAbilityTags 를 모두 포함하는(HasAll) 어빌리티의 VM 을 반환하고, 없으면 부여된 스펙에서 찾아 생성해 캐시한다.
	 * 매칭 시멘틱은 엔진의 GetActivatableGameplayAbilitySpecsByAllMatchingTags 와 동일하며, 여러 어빌리티가 매칭되면 첫 번째 것을 사용한다.
	 * 매칭되는 어빌리티가 부여되지 않았으면 nullptr 를 반환한다.
	 */
	UWxViewModel_Ability* GetOrCreateAbilityViewModel(const FGameplayTagContainer& InAbilityTags);

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|AbilitySystem")
	TArray<TObjectPtr<UWxViewModel_Effect>> ActiveEffectViewModels;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|AbilitySystem")
	FGameplayTagContainer OwnedTags;

protected:
	/**
	 * 초기화 시점에 이미 활성인 GE 로 목록을 구축한다.
	 * 이후 추가/제거는 HandleActiveEffectAdded/Removed 가 증분 처리한다.
	 */
	void RefreshActiveEffectViewModels();

	void RefreshOwnedTags();

	void HandleActiveEffectAdded(UAbilitySystemComponent* InASC, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle);
	void HandleActiveEffectRemoved(const FActiveGameplayEffect& ActiveEffect);
	void HandleTagChanged(const FGameplayTag Tag, int32 NewCount);

	/** 지연 생성된 어트리뷰트 VM 캐시. GetOrCreateAttributeViewModel 참조. */
	UPROPERTY()
	TArray<TObjectPtr<UWxViewModel_Attribute>> AttributeViewModels;

	/** 지연 생성된 어빌리티 VM 캐시. GetOrCreateAbilityViewModel 참조. */
	UPROPERTY()
	TArray<TObjectPtr<UWxViewModel_Ability>> AbilityViewModels;

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;

private:
	/** 각 자식의 Deinitialize 후 배열을 비운다. 통째로 비우는 두 지점(Deinitialize·RefreshActiveEffectViewModels)이 공유한다. */
	void ClearActiveEffectViewModels();
};
