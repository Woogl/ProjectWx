// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "View/MVVMViewModelContextResolver.h"
#include "WxViewModelResolver_PlayerCharacter.generated.h"

class UUserWidget;
class UMVVMView;

/**
 * VM_PlayerCharacter 용 View Bindings Resolver.
 *
 * 위젯을 소유한 PlayerController 의 빙의 Pawn 에서 ASC/표시 데이터를 끌어와 위젯별 UWxViewModel_Character 를 생성/초기화한다.
 * UWxViewModel_Character 는 WxUI 플러그인 소속이라 게임 모듈을 참조할 수 없으므로, 데이터 주입은 양쪽에 의존하는 본 리졸버가 수행한다.
 * 생성 시점에 Pawn 을 읽으므로 위젯은 빙의 완료 후에 생성되어야 한다 (HUD 는 OnPossess/OnRep_Pawn 에서 푸시되므로 보장됨).
 * WBP 의 View Bindings 에서 Creation Type = Resolver 로 본 클래스를 선택한다.
 */
UCLASS(EditInlineNew, CollapseCategories)
class WXGAME_API UWxViewModelResolver_PlayerCharacter : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
};
