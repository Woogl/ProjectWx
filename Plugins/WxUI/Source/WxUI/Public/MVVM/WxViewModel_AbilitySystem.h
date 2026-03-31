// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/WxViewModel.h"
#include "WxViewModel_AbilitySystem.generated.h"

struct FActiveGameplayEffectHandle;
struct FGameplayEffectSpec;
struct FActiveGameplayEffect;
class UAbilitySystemComponent;
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

	/** 어빌리티 목록을 재구축한다. 런타임에 어빌리티가 변경되었을 때 호출 */
	void RebuildAbilityViewModels();
	
	/** 이펙트 목록을 재구축한다. 런타임에 이펙트가 추가/제거되었을 때 호출 */
	void RebuildActiveEffectViewModels();

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|AbilitySystem")
	TArray<TObjectPtr<UWxViewModel_Ability>> AbilityViewModels;
	
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|AbilitySystem")
	TArray<TObjectPtr<UWxViewModel_Effect>> ActiveEffectViewModels;

protected:
	virtual void Deinitialize() override;
	
	void HandleActiveEffectAdded(UAbilitySystemComponent* InASC, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle);
	void HandleActiveEffectRemoved(const FActiveGameplayEffect& ActiveEffect);

private:
	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
};
