// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WxInputConfig.generated.h"

class UInputMappingContext;
class UInputAction;

/**
 * 플레이어 입력 설정.
 * 이동/시선/점프 등 직접 바인딩 입력과 어빌리티 입력 매핑을 포함한다.
 * 메뉴/UI 입력은 CommonUI 액션(WxHUDLayout)으로 처리하므로 여기 포함하지 않는다.
 */
UCLASS()
class WXGAME_API UWxInputConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 캐릭터 IMC. Possess 시 Subsystem에 추가됨 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<UInputAction> CrouchAction;

	/**
	 * 상호작용 입력.
	 * 어빌리티 입력 바인딩이 아니라 직접 바인딩인 이유는, 상호작용이 클라의 로컬 선택 대상을 페이로드로 함께 실어 보내야 하기 때문이다.
	 * ASC 의 입력 태그 라우팅에는 페이로드 통로가 없다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<UInputAction> InteractAction;

	/** 어빌리티 입력으로 바인딩할 InputAction 목록. 각 액션의 press/release가 ASC 입력 라우팅으로 전달된다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TArray<TObjectPtr<const UInputAction>> AbilityInputActions;
};
