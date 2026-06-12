// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/WidgetComponent.h"
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
class WXGAME_API UWxNameplateComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	UWxNameplateComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	 * 오너 캐릭터의 표시 데이터와 ASC 를 묶은 UWxViewModel_Character 를 생성해 MVVM View 에 바인딩한다.
	 * (자식 AbilitySystem VM 이 어트리뷰트/이펙트를, 본체가 이름/초상화/설명을 노출한다.)
	 * Widget 이 유효하고 UMVVMView Extension 이 존재해야 동작한다.
	 */
	void InitializeViewModels(UAbilitySystemComponent* InASC);

protected:
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
	 * 표시 = 사망 아님 && (적이 플레이어를 인식 || 로컬 플레이어가 이 적을 락온).
	 * 인식은 복제된 State.InCombat 태그, 락온은 로컬 플레이어의 LockOn 컴포넌트에서 파생한다.
	 */
	void RefreshVisibility();

	/** 로컬 플레이어가 오너(이 적)를 락온 중인지 반환. 락온 표시는 시각적·개인 UI라 로컬에서만 판정한다. */
	bool IsLockedOnByLocalPlayer() const;

	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
};
