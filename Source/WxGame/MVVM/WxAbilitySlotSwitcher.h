// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Extensions/UserWidgetExtension.h"
#include "GameplayTagContainer.h"
#include "WxAbilitySlotSwitcher.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;
class UMVVMView;
class UUserWidget;
class UWxViewModel_Ability;
struct FGameplayAbilitySpec;

/**
 * 슬롯 하나가 지금 어느 어빌리티를 보여야 하는지 감시하고, 바뀌면 슬롯의 뷰모델을 갈아 끼운다.
 *
 * 어빌리티 뷰모델은 어빌리티 하나만 담당하는 부품이라 상황 판단을 넣지 않는다 — 같은 슬롯 태그를 단 후보(소환↔명령처럼 상태로 갈리는 쌍) 중 무엇을 그릴지는 여기가 정한다.
 * 리졸버가 뷰모델을 만들며 위젯에 붙인다. 후보가 하나뿐인 슬롯에서는 매번 같은 것을 골라 교체가 일어나지 않는다.
 *
 * 한 번 갈아 끼운 소스는 엔진이 수동 지정으로 표시해 두어(FMVVMView_Source::bSetManually) 리졸버를 다시 부르지 않는다.
 * 그래서 위젯이 다시 구성될 때의 재무장과 빙의 폰이 바뀌었을 때의 갈아타기도 여기가 맡는다.
 */
UCLASS()
class WXGAME_API UWxAbilitySlotSwitcher : public UUserWidgetExtension
{
	GENERATED_BODY()

public:
	/**
	 * 슬롯 태그를 단 후보 중 지금 표시할 어빌리티. 요건을 만족하는 첫 후보 → 직전 후보 유지 → 첫 후보 순.
	 * 요건은 어빌리티의 발동 태그 조건과 다른 어빌리티가 태그로 건 차단까지다. 쿨다운·비용은 보지 않아 표시가 그것들로 흔들리지 않는다.
	 */
	static const UGameplayAbility* PickAbility(const UAbilitySystemComponent& ASC, const FGameplayTagContainer& SlotTags, const UGameplayAbility* PreviousAbility);

	/** 위젯을 소유한 PlayerController 의 빙의 Pawn 에서 ASC 를 끌어온다. 리졸버와 재무장이 같은 곳을 본다. */
	static UAbilitySystemComponent* FindAbilitySystem(const UUserWidget* Widget);

	/** 리졸버가 뷰모델을 만들어 넘긴 직후에 부른다. 넘긴 뷰모델은 나중에 자기 소스를 찾는 표식이 된다. */
	void WatchSlot(UAbilitySystemComponent* InASC, const FGameplayTagContainer& InSlotTags, const UGameplayAbility* InAbility, UWxViewModel_Ability* InViewModel);

	/** 한 위젯에 슬롯이 여럿이므로 확장도 여럿 붙는다. 재초기화가 확장을 늘리지 않도록 같은 슬롯의 것을 찾는 데 쓴다. */
	bool HandlesSlot(const FGameplayTagContainer& InSlotTags) const;

	//~ Begin UUserWidgetExtension
	virtual void Construct() override;
	virtual void Destruct() override;
	//~ End UUserWidgetExtension

	//~ Begin UObject
	virtual void BeginDestroy() override;
	//~ End UObject

private:
	void HandleTagChanged(const FGameplayTag Tag, int32 NewCount);

	void HandleAbilitySpecDirtied(const FGameplayAbilitySpec& Spec);

	/** 재구성마다 불리므로 겹쳐 걸리지 않게 먼저 걷고 건다. */
	void Subscribe();

	/** 태그·부여 통지가 한 프레임에 여러 번 몰리므로 다음 틱에 한 번만 판정한다. */
	void RequestRefresh();

	bool FlushRefresh(float DeltaTime);

	/** 리졸버가 뷰모델을 돌려준 시점에는 아직 소스에 꽂히기 전이라, 첫 교체 때 자기 뷰모델을 들고 있는 소스를 찾아 확정한다. */
	FName FindSourceName(const UMVVMView& View) const;

	/** 구독과 예약만 걷는다. 어느 슬롯을 보고 있었는지는 남겨 재구성 때 그대로 재무장한다. */
	void Shutdown();

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;

	TWeakObjectPtr<const UGameplayAbility> CurrentAbility;

	TWeakObjectPtr<UWxViewModel_Ability> CurrentViewModel;

	/** 후보를 고르는 키. 리졸버가 조회에 쓴 태그와 같다. */
	FGameplayTagContainer SlotTags;

	FName SourceName;

	FTSTicker::FDelegateHandle RefreshHandle;
};
