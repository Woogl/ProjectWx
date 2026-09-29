// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "Types/MVVMViewModelContext.h"
#include "WxViewModel_Character.generated.h"

class UAbilitySystemComponent;
class UWxViewModel_AbilitySystem;

/**
 * 캐릭터 단위 표시 정보를 묶는 Composite 뷰모델.
 *
 * 조립 층이 넘긴 ASC·이름을 표시한다. 구체 캐릭터 타입은 알지 않는다.
 * 플레이어 것은 로컬 PC 가 Global Collection 에 VM_PlayerCharacter 로 등록하고, 적·보스 것은 그 뷰를 만드는 쪽이 만든다.
 */
UCLASS()
class WXUI_API UWxViewModel_Character : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	/** WBP 의 Global Viewmodel Collection 소스가 쓰는 이름과 같아야 한다. */
	static FMVVMViewModelContext GetPlayerContext();

	/** 로컬 PC 가 등록한 플레이어 Character VM. 등록 전이거나 원격 PC 뿐인 월드면 nullptr. */
	static UWxViewModel_Character* FindPlayer(const UObject* WorldContextObject);

	/** ASC 가 바뀌면 자기 AbilitySystem VM 을 새로 만들어 소유하고 이전 것은 놓는다. 같은 ASC 면 이름만 갱신한다. */
	void Initialize(UAbilitySystemComponent* InASC, FText InCharacterName);

	/** 표시 필드를 비우고 통지해, 소스가 빠졌다는 사실이 화면에 반영되게 한다. */
	void Deinitialize();

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Character")
	TObjectPtr<UWxViewModel_AbilitySystem> AbilitySystem;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Character")
	FText CharacterName;
};
