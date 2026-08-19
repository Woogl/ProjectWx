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
 * ASC의 어트리뷰트/어빌리티/이펙트를 자식 ViewModel로 노출하는 Composite 뷰모델.
 *
 * 어트리뷰트/어빌리티 VM 은 바인딩이 요청할 때 GetOrCreate... 로 지연 생성하고, 이펙트 VM 은 활성 GE 추가/제거 이벤트로 관리한다.
 */
UCLASS()
class WXUI_API UWxViewModel_AbilitySystem : public UWxViewModel
{
	GENERATED_BODY()

public:
	/**
	 * ASC 하나당 하나. 없으면 ASC 를 Outer 로 만들어 초기화한다.
	 * 폰이 바뀌면 ASC 도 바뀌므로 새 인스턴스가 생기고, 옛 것은 옛 ASC 와 함께 수거된다.
	 */
	static UWxViewModel_AbilitySystem* GetOrCreate(UAbilitySystemComponent* InASC);

	void Initialize(UAbilitySystemComponent* InASC);
	virtual void Deinitialize() override;

	UWxViewModel_Attribute* FindAttributeViewModel(FGameplayAttribute InAttribute) const;
	UWxViewModel_Ability* FindAbilityViewModel(const FGameplayTagContainer& InAbilityTags) const;
	UWxViewModel_Effect* FindActiveEffectViewModel(FGameplayTag InEffectTag) const;

	/** Max 가 유효하지 않으면 Current 자신을 최대값으로 사용한다. */
	UWxViewModel_Attribute* GetOrCreateAttributeViewModel(FGameplayAttribute Current, FGameplayAttribute Max);

	/**
	 * Asset Tags 가 InAbilityTags 를 모두 포함하는(HasAll) 어빌리티를 부여된 스펙에서 찾는다.
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

	UPROPERTY()
	TArray<TObjectPtr<UWxViewModel_Attribute>> AttributeViewModels;

	UPROPERTY()
	TArray<TObjectPtr<UWxViewModel_Ability>> AbilityViewModels;

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;

private:
	void ClearActiveEffectViewModels();
};
