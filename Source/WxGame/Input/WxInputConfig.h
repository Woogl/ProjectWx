// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WxInputConfig.generated.h"

class UInputMappingContext;
class UInputAction;

/**
 * 플레이어 입력 설정.
 * IMC와 이동/시선/점프 등 직접 바인딩 입력을 담는다.
 * 어빌리티 입력은 여기 두지 않는다 — 발동 IA는 어빌리티 CDO가 보유하고, 바인딩 목록은 AbilitySet의 부여 대상에서 파생한다.
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
	 * 어빌리티 입력 바인딩이 아니라 직접 바인딩인 이유는, 상호작용이 클라의 로컬 선택 대상을 읽어 전송해야 하기 때문이다.
	 * 캐릭터는 이 입력을 PlayerController 의 UWxInteractionRegistryComponent::TryInteractSelected 로 라우팅한다(컴포넌트가 선택을 읽어 ServerInteract 로 전송).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<UInputAction> InteractAction;
};
