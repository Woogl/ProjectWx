// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModelResolver_InteractionList.h"
#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/WxInteractionRegistrySubsystem.h"
#include "MVVM/WxViewModel_InteractionList.h"

UObject* UWxViewModelResolver_InteractionList::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	const APlayerController* PC = UserWidget ? UserWidget->GetOwningPlayer() : nullptr;
	ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	UWxInteractionRegistrySubsystem* Registry = LocalPlayer ? LocalPlayer->GetSubsystem<UWxInteractionRegistrySubsystem>() : nullptr;
	if (!Registry)
	{
		return nullptr;
	}

	// 데이터 소스(LocalPlayer)를 Outer 로 생성한다. 폰 리스폰에도 생존하며, 수명은 뷰의 강참조와 BeginDestroy 의 Deinitialize 가 관리한다.
	UWxViewModel_InteractionList* ViewModel = NewObject<UWxViewModel_InteractionList>(LocalPlayer);

	// WxUI VM 은 WxWorld 레지스트리를 못 보므로, 목록·선택 변경 델리게이트를 VM 핸들러(엔진 타입 인자)에 본 리졸버가 연결한다.
	// 동적 멀티캐스트는 broadcast 시 무효 바인딩을 자동 정리하므로 VM 파괴 시 별도 해제가 필요 없다.
	Registry->OnListChanged.AddDynamic(ViewModel, &UWxViewModel_InteractionList::HandleListChanged);
	Registry->OnSelectionChanged.AddDynamic(ViewModel, &UWxViewModel_InteractionList::HandleSelectionChanged);

	// 선택은 레지스트리가 소유한다. 현재 목록/선택을 그대로 시드한다(목록이 비면 INDEX_NONE).
	ViewModel->Initialize(Registry->GetPrompts(), Registry->GetSelectedIndex());

	return ViewModel;
}
