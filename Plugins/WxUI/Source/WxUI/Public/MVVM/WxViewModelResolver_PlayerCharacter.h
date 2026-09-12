// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "View/MVVMViewModelContextResolver.h"
#include "WxViewModelResolver_PlayerCharacter.generated.h"

class UUserWidget;
class UMVVMView;

/**
 * 위젯을 소유한 PlayerController의 빙의 Pawn과 ASC로 UWxViewModel_Character를 생성/초기화한다.
 * 표시 데이터 조회는 ViewModel이 대상의 IWxUIData를 통해 수행한다.
 * 생성 시점에 Pawn 을 읽으므로 위젯은 빙의 완료 후에 생성되어야 한다 (HUD 는 OnPossessedPawnChanged 에서 푸시되므로 보장됨).
 */
UCLASS(EditInlineNew, CollapseCategories)
class WXUI_API UWxViewModelResolver_PlayerCharacter : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
};
