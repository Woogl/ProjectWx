// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "View/MVVMViewModelContextResolver.h"
#include "WxViewModelResolver_InteractionList.generated.h"

class UUserWidget;
class UMVVMView;

/**
 * VM_InteractionList 용 View Bindings Resolver.
 *
 * 위젯을 소유한 PlayerController 의 LocalPlayer 에서 UWxInteractionRegistrySubsystem 를 찾아 UWxViewModel_InteractionList 을 생성/초기화한다.
 * UWxViewModel_InteractionList(WxUI)은 UWxInteractionRegistrySubsystem(WxWorld)를 참조할 수 없으므로,
 * 양쪽에 의존하는 본 리졸버가 목록 변경 델리게이트를 VM 핸들러에 연결하고 초기 목록을 시드한다.
 * 선택(SelectedIndex)은 UI 가 소유하므로 본 리졸버는 목록만 연결한다.
 * WBP 의 View Bindings 에서 Creation Type = Resolver 로 본 클래스를 선택한다.
 */
UCLASS(EditInlineNew, CollapseCategories)
class WXGAME_API UWxViewModelResolver_InteractionList : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
};
