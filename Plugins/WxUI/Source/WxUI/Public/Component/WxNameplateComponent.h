// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "Components/WidgetComponent.h"
#include "MVVM/WxCharacterUIData.h"
#include "WxNameplateComponent.generated.h"

class UAbilitySystemComponent;

/**
 * 네임플레이트 위젯 컴포넌트.
 * WidgetComponent를 확장하여 ASC 기반 MVVM ViewModel 초기화를 캡슐화한다.
 * 카메라 거리에 따라 위젯 스케일을 자동 조절하여 원근 효과를 적용한다.
 *
 * 사용 흐름:
 *  1. 오너 액터의 생성자에서 서브오브젝트로 생성
 *  2. BP에서 Widget Class에 네임플레이트 위젯을 지정
 *  3. BeginPlay 이후 InitializeViewModels()로 ViewModel 바인딩
 */
UCLASS(ClassGroup = (Wx), meta = (BlueprintSpawnableComponent))
class WXUI_API UWxNameplateComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	UWxNameplateComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	 * 주입받은 표시 데이터(InUIData)와 ASC 를 묶은 UWxViewModel_Character 를 생성해 MVVM View 에 바인딩한다.
	 * (자식 AbilitySystem VM 이 어트리뷰트/이펙트를, 본체가 이름/초상화/설명을 노출한다.)
	 * WxUI 는 구체 캐릭터 타입을 알지 못하므로 표시 데이터는 소비 측이 주입한다.
	 * Widget 이 유효하고 UMVVMView Extension 이 존재해야 동작한다.
	 */
	void InitializeViewModels(UAbilitySystemComponent* InASC, const FWxCharacterUIData& InUIData);

protected:
	/**
	 * 네임플레이트 표시 조건.
	 * ASC 보유 태그로 평가한다: Must Have(HasAll)·Must Not Have(HasAny면 숨김)·Query Must Match(복합).
	 * 기본값은 생성자에서 저작한다(Dead 면 숨김, InCombat 또는 LockedOn 이면 표시).
	 * 엔티티별로 BP 에서 오버라이드한다.
	 */
	UPROPERTY(EditAnywhere, Category = "Wx")
	FGameplayTagRequirements VisibilityRequirements;

	/** 이 거리에서 위젯 스케일이 1.0이 된다. */
	UPROPERTY(EditAnywhere, Category = "Wx")
	float ReferenceDistance = 1000.f;

	/** 위젯 스케일의 최솟값. */
	UPROPERTY(EditAnywhere, Category = "Wx")
	float MinScale = 0.5f;

	/** 위젯 스케일의 최댓값. */
	UPROPERTY(EditAnywhere, Category = "Wx")
	float MaxScale = 1.f;

private:
	/**
	 * 표시 조건을 매 틱 진실로부터 재계산한다.
	 * 표시 = VisibilityRequirements.RequirementsMet(ASC 보유 태그).
	 * 어떤 게임플레이 상태가 조건인지는 VisibilityRequirements 가 정하며, 본 함수는 구체 태그를 알지 않는다.
	 */
	void RefreshVisibility();

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
};
