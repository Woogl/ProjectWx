// Copyright Woogle. All Rights Reserved.

#include "UI/MVVM/WxViewModelUtils.h"

#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "MVVMGameSubsystem.h"

void WxViewModel::RequestImageAsync(UObject& Owner, TSharedPtr<FStreamableHandle>& InOutHandle, const TSoftObjectPtr<UObject>& Image, TFunction<void(UObject*)> OnLoaded)
{
	// 같은 필드의 재요청이 흔하다(슬롯이 다른 어빌리티·아이템으로 갈릴 때 등).
	// CancelHandle 은 지연 콜백 큐에 들어간 완료 델리게이트까지 취소하므로, 취소된 요청이 뒤늦게 발화해 새 값을 덮어쓰지 않는다.
	if (InOutHandle.IsValid())
	{
		InOutHandle->CancelHandle();
		InOutHandle.Reset();
	}

	if (Image.IsNull() || Image.Get())
	{
		OnLoaded(Image.Get());
		return;
	}

	const TSharedPtr<FStreamableHandle> Handle = UAssetManager::GetStreamableManager().RequestAsyncLoad(Image.ToSoftObjectPath(),
		FStreamableDelegate::CreateWeakLambda(&Owner, [Image, OnLoaded]()
		{
			OnLoaded(Image.Get());
		}));

	// 완료 콜백이 위 호출 안에서 돌며 같은 필드에 새 요청을 걸었다면 그 핸들을 덮어쓰지 않는다.
	if (!InOutHandle.IsValid())
	{
		InOutHandle = Handle;
	}
}

UMVVMViewModelCollectionObject* WxViewModel::GetGlobalCollection(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const UMVVMGameSubsystem* ViewModelSubsystem = GameInstance ? GameInstance->GetSubsystem<UMVVMGameSubsystem>() : nullptr;
	return ViewModelSubsystem ? ViewModelSubsystem->GetViewModelCollection() : nullptr;
}
