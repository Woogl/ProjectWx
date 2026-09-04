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
 */
UCLASS()
class WXUI_API UWxViewModel_Character : public UWxViewModel
{
	GENERATED_BODY()

public:
	/**
	 * Source 를 Outer 로 공유되는 인스턴스. 없으면 만든다 — 표시 데이터 주입은 호출자가 한다.
	 * 같은 Outer 를 집는다는 약속이 발행자와 소비자를 잇는 유일한 연결 고리다.
	 */
	static UWxViewModel_Character* GetOrCreate(UObject* Source);

	void Initialize(UAbilitySystemComponent* InASC, const FText& InCharacterName, const TSoftObjectPtr<UObject>& InPortrait);

	/** 표시 필드를 비운다. 파괴 중이 아니면 변경을 통지해, 소스가 빠졌다는 사실이 화면에 반영되게 한다. */
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
