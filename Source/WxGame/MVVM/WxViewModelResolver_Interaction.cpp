// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModelResolver_Interaction.h"
#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/WxInteractionRegistrySubsystem.h"
#include "MVVM/WxViewModel_Interaction.h"

UObject* UWxViewModelResolver_Interaction::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	const APlayerController* PC = UserWidget ? UserWidget->GetOwningPlayer() : nullptr;
	ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	UWxInteractionRegistrySubsystem* Registry = LocalPlayer ? LocalPlayer->GetSubsystem<UWxInteractionRegistrySubsystem>() : nullptr;
	if (!Registry)
	{
		return nullptr;
	}

	// 데이터 소스(LocalPlayer)를 Outer 로 생성한다. 폰 리스폰에도 생존하며, 수명은 뷰의 강참조와 BeginDestroy 의 Deinitialize 가 관리한다.
	UWxViewModel_Interaction* ViewModel = NewObject<UWxViewModel_Interaction>(LocalPlayer);

	// WxUI VM 은 WxWorld 레지스트리를 못 보므로, 목록 변경 델리게이트를 VM 핸들러(엔진 타입 인자)에 본 리졸버가 연결한다.
	// 동적 멀티캐스트는 broadcast 시 무효 바인딩을 자동 정리하므로 VM 파괴 시 별도 해제가 필요 없다.
	Registry->OnListChanged.AddDynamic(ViewModel, &UWxViewModel_Interaction::HandleListChanged);

	// 선택(SelectedIndex)은 UI 가 소유한다. 초기엔 첫 항목을 기본 선택으로 시드한다(목록이 비면 VM 이 INDEX_NONE 로 클램프).
	ViewModel->Initialize(Registry->GetPrompts(), 0);

	return ViewModel;
}
