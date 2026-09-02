// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "GameplayTagContainer.h"
#include "MVVM/WxViewModel.h"
#include "WxViewModel_AbilitySystem.generated.h"

struct FGameplayAttribute;
struct FActiveGameplayEffectHandle;
struct FGameplayAbilitySpec;
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
 * 어빌리티 부여가 바뀌면 만들어 둔 슬롯 VM 전부에 재매칭을 지시해, 스킬이 교체돼도 슬롯이 따라간다.
 */
UCLASS()
class WXUI_API UWxViewModel_AbilitySystem : public UWxViewModel
{
	GENERATED_BODY()

public:
	/**
	 * ASC 하나당 하나. 없으면 ASC 를 Outer 로 만들어 초기화한다.
	 * 폰이 바뀌면 ASC 도 바뀌므로 새 인스턴스가 생긴다.
	 * GC 는 객체 → Outer 방향만 수집하므로 Outer 인 ASC 는 이 VM 을 살려 두지 않는다 — 수명은 이 VM 을 Outer 로 삼는 자식 VM 이 쥔다.
	 */
	static UWxViewModel_AbilitySystem* GetOrCreate(UAbilitySystemComponent* InASC);

	void Initialize(UAbilitySystemComponent* InASC);
	virtual void Deinitialize() override;

	/**
	 * 현재값과 최대치 쌍이 같아야 같은 뷰모델이다 — 최대치가 비율과 가득참 여부를 결정한다.
	 * Max 가 유효하지 않으면 Current 자신을 최대값으로 사용한다.
	 */
	UWxViewModel_Attribute* GetOrCreateAttributeViewModel(FGameplayAttribute Current, FGameplayAttribute Max);

	/**
	 * InAbilityTags 가 가리키는 스킬 슬롯의 뷰모델. 그 태그가 곧 공유 키이며, 어빌리티 매칭은 뷰모델이 스스로 한다.
	 * 조회는 컨테이너 정확 일치다 — 포함 관계로 찾으면 넓은 질의가 먼저 만들어진 것을 주워 생성 순서에 따라 결과가 갈린다.
	 * 맞는 어빌리티가 아직 부여되지 않았어도 뷰모델은 만들어진다. 부여되면 그때 물고, 교체되면 갈아탄다.
	 */
	UWxViewModel_Ability* GetOrCreateAbilityViewModel(const FGameplayTagContainer& InAbilityTags);

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|AbilitySystem")
	TArray<TObjectPtr<UWxViewModel_Effect>> ActiveEffectViewModels;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|AbilitySystem")
	FGameplayTagContainer OwnedTags;

protected:
	void BuildActiveEffectViewModels();

	void RefreshOwnedTags();

	void HandleActiveEffectAdded(UAbilitySystemComponent* InASC, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle);
	void HandleActiveEffectRemoved(const FActiveGameplayEffect& ActiveEffect);
	void HandleTagChanged(const FGameplayTag Tag, int32 NewCount);
	void HandleAbilitySpecDirtied(const FGameplayAbilitySpec& Spec);

	bool FlushOwnedTagsRefresh(float DeltaTime);
	bool FlushAbilityRebind(float DeltaTime);

	UPROPERTY()
	TArray<TObjectPtr<UWxViewModel_Attribute>> AttributeViewModels;

	UPROPERTY()
	TArray<TObjectPtr<UWxViewModel_Ability>> AbilityViewModels;

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;

	/** 유효하면 이번 프레임의 갱신이 이미 예약돼 있다는 뜻이다. */
	FTSTicker::FDelegateHandle OwnedTagsRefreshHandle;

	/** 유효하면 이번 프레임의 슬롯 재매칭이 이미 예약돼 있다는 뜻이다. */
	FTSTicker::FDelegateHandle AbilityRebindHandle;
};
