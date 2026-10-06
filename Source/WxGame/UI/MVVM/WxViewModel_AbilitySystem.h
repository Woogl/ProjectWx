// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "GameplayTagContainer.h"
#include "MVVMViewModelBase.h"
#include "WxViewModel_AbilitySystem.generated.h"

struct FGameplayAttribute;
struct FActiveGameplayEffectHandle;
struct FGameplayEffectSpec;
struct FActiveGameplayEffect;
class UAbilitySystemComponent;
class UWxViewModel_Ability;
class UWxViewModel_Attribute;
class UWxViewModel_Effect;

/**
 * ASC의 어트리뷰트/어빌리티/이펙트를 자식 ViewModel로 노출하는 Composite 뷰모델.
 *
 * 어트리뷰트·스킬 슬롯 VM 은 조회할 때 지연 생성한다. 이펙트 목록은 초기화 때 구성하고 활성 GE 추가/제거 이벤트로 관리한다.
 */
UCLASS()
class WXGAME_API UWxViewModel_AbilitySystem : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	/** Character VM 이 자기 것을 만들 때 한 번 부른다. ASC 가 바뀌면 Character VM 이 새 인스턴스를 만든다. */
	void Initialize(UAbilitySystemComponent* InASC);

	/** 소유 Character 가 ASC 를 놓을 때, 외부 위젯이 보유한 자식 VM 도 함께 비활성화한다. */
	void Deinitialize();

	UAbilitySystemComponent* GetBoundASC() const;

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
	FGameplayTagContainer OwnedTags;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|AbilitySystem")
	TArray<TObjectPtr<UWxViewModel_Effect>> ActiveEffectViewModels;

private:
	void RefreshOwnedTags();

	void HandleActiveEffectAdded(UAbilitySystemComponent* InASC, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle);
	void HandleActiveEffectRemoved(const FActiveGameplayEffect& ActiveEffect);
	void HandleTagChanged(const FGameplayTag Tag, int32 NewCount);

	void FlushOwnedTagsRefresh();

	/** 최초 목록 구성에서도 사용하므로 FieldNotify 없이 추가 성공 여부만 반환한다. */
	bool AddActiveEffectViewModel(UAbilitySystemComponent* InASC, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle);

	UPROPERTY()
	TArray<TObjectPtr<UWxViewModel_Attribute>> AttributeViewModels;

	UPROPERTY()
	TArray<TObjectPtr<UWxViewModel_Ability>> AbilityViewModels;

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;

	/** 타이머가 활성이면 갱신이 이미 예약돼 있다. 실행 중에도 활성으로 잡히므로 플러시가 먼저 놓는다. */
	FTimerHandle OwnedTagsRefreshHandle;
};
