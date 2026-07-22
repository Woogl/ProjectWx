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
 * 상호작용 입력도 여기 두지 않는다 — HUD 리스트 위젯이 Enhanced Input으로 직접 받아 뷰모델 요청으로 넘긴다.
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
};
