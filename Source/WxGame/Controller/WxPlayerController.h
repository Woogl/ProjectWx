// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "WxPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UWxInputConfig;

/**
 * 플레이어 컨트롤러.
 * 입력 관련 데이터(MappingContext, InputAction, InputConfig)를 소유.
 */
UCLASS()
class WXGAME_API AWxPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UInputMappingContext* GetDefaultMappingContext() const;
	const UInputAction* GetMoveAction() const;
	const UInputAction* GetLookAction() const;
	const UWxInputConfig* GetInputConfig() const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<UInputAction> LookAction;

	/** 어빌리티 입력 바인딩 설정. InputAction → InputTag 매핑 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<const UWxInputConfig> InputConfig;
};
