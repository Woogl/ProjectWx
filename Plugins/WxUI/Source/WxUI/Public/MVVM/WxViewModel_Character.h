// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "WxViewModel_Character.generated.h"

class UWxViewModel_AbilitySystem;

/**
 * 캐릭터 단위 표시 정보를 묶는 Composite 뷰모델.
 *
 * 리졸버가 넘긴 이름·GAS VM을 표시한다. 구체 캐릭터 타입은 알지 않는다.
 */
UCLASS()
class WXUI_API UWxViewModel_Character : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	/**
	 * AbilitySystem VM을 Outer로 공유하며, 새로 만든 경우에만 표시 데이터를 초기화한다.
	 * 기존 공유본의 표시 데이터는 유지한다.
	 */
	static UWxViewModel_Character* GetOrCreate(UWxViewModel_AbilitySystem* InAbilitySystem, const FText& InCharacterName);

	void Initialize(UWxViewModel_AbilitySystem* InAbilitySystem, FText InCharacterName);

	/** 표시 필드를 비우고 통지해, 소스가 빠졌다는 사실이 화면에 반영되게 한다. */
	void Deinitialize();

	/** ASC 가 소유하는 공유본이다 — 같은 캐릭터를 보는 다른 뷰모델·위젯과 같은 인스턴스를 가리킨다. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Character")
	TObjectPtr<UWxViewModel_AbilitySystem> AbilitySystem;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Character")
	FText CharacterName;
};
