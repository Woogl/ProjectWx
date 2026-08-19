// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/WxViewModel.h"
#include "WxViewModel_Character.generated.h"

class UAbilitySystemComponent;
class UTexture2D;
class UWxViewModel_AbilitySystem;

/**
 * 캐릭터 단위 표시 정보를 묶는 Composite 뷰모델.
 *
 * WxUI 는 구체 캐릭터 타입을 알지 못하므로, 표시 데이터는 소비 측(게임 모듈)이 대상 캐릭터에서 읽어 Initialize 로 주입한다.
 * UMG 는 AbilitySystem 자식 VM 에 중첩 바인딩(어트리뷰트는 Get Attribute ViewModel, 어빌리티는 Get Ability ViewModel 컨버전으로 가져옴)하고, 나머지 표시 데이터는 각 프로퍼티에 직접 바인딩한다.
 */
UCLASS()
class WXUI_API UWxViewModel_Character : public UWxViewModel
{
	GENERATED_BODY()

public:
	void Initialize(UAbilitySystemComponent* InASC, const FText& InCharacterName, const TSoftObjectPtr<UObject>& InPortrait);

	virtual void Deinitialize() override;

	/** ASC 가 소유하는 공유본이다 — 같은 캐릭터를 보는 다른 뷰모델·위젯과 같은 인스턴스를 가리킨다. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Character")
	TObjectPtr<UWxViewModel_AbilitySystem> AbilitySystem;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Character")
	FText CharacterName;

	/** Soft 참조를 베이스가 비동기 로드해 세팅한다. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Character")
	TObjectPtr<UObject> Portrait;

protected:
	//~ Begin UWxViewModel
	virtual void ApplyLoadedImage(FName FieldName, UObject* LoadedImage) override;
	//~ End UWxViewModel
};
