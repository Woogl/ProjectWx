// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/WxCharacterUIData.h"
#include "MVVM/WxViewModel.h"
#include "WxViewModel_Character.generated.h"

class UAbilitySystemComponent;
class UTexture2D;
class UWxViewModel_AbilitySystem;

/**
 * 캐릭터 단위 표시 정보를 묶는 Composite 뷰모델.
 * 자식 UWxViewModel_AbilitySystem 을 생성/소유하여 어트리뷰트/어빌리티/이펙트를 노출하고, 캐릭터 표시 데이터(이름/초상화/설명)를 함께 제공한다.
 *
 * WxUI 는 구체 캐릭터 타입을 알지 못하므로, 표시 데이터는 소비 측(게임 모듈)이 대상 캐릭터의 FWxCharacterUIData 를 Initialize 로 주입한다.
 * UMG 는 AbilitySystem 자식 VM 에 중첩 바인딩(어트리뷰트는 Get Attribute ViewModel 컨버전으로 가져오고, 어빌리티는 AbilitySystem.AbilityViewModels 에 바인딩)하고, 나머지 표시 데이터는 각 프로퍼티에 직접 바인딩한다.
 */
UCLASS()
class WXUI_API UWxViewModel_Character : public UWxViewModel
{
	GENERATED_BODY()

public:
	/** 자식 어빌리티 시스템 VM 을 생성/초기화하고 표시 데이터를 주입한다. */
	void Initialize(UAbilitySystemComponent* InASC, const FWxCharacterUIData& InUIData);

	virtual void Deinitialize() override;

	/** 어빌리티 시스템 자식 VM. UMG 는 본 프로퍼티를 통해 중첩 바인딩한다. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Character")
	TObjectPtr<UWxViewModel_AbilitySystem> AbilitySystem;

	/** 캐릭터 표시 이름. 주입 시점 1회 세팅되며 런타임에 변하지 않는다. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Character")
	FText DisplayName;

	/** 캐릭터 초상화. View 측 UCommonLazyImage 등이 Soft 참조를 비동기 로드한다. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Character")
	TSoftObjectPtr<UTexture2D> Portrait;

	/** 캐릭터 설명 문구. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Character")
	FText Description;
};
