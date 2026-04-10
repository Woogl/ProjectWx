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

	/**
	 * ASC의 전투 어트리뷰트를 기반으로 네임플레이트에 필요한 ViewModel을 생성하고 MVVM View에 바인딩한다.
	 * Widget이 유효하고 UMVVMView Extension이 존재해야 동작한다.
	 */
	void InitializeViewModels(UAbilitySystemComponent* InASC);

private:
	void HandleDeadTagChanged(const FGameplayTag Tag, int32 NewCount);
};
