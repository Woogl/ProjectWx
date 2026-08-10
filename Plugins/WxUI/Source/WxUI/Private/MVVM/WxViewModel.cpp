// Copyright Woogle. All Rights Reserved.

#include "MVVM/WxViewModel.h"

#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"

void UWxViewModel::BeginDestroy()
{
	Deinitialize();
	Super::BeginDestroy();
}

void UWxViewModel::Deinitialize()
{
	for (TPair<FName, FWxImageRequest>& Request : ImageRequests)
	{
		if (Request.Value.Handle.IsValid())
		{
			Request.Value.Handle->CancelHandle();
		}
	}
	ImageRequests.Reset();
}

void UWxViewModel::RequestImageAsync(FName FieldName, const TSoftObjectPtr<UObject>& InImage)
{
	FWxImageRequest& Request = ImageRequests.FindOrAdd(FieldName);

	// 같은 슬롯의 재요청이 흔하다(충전량 변화로 아이콘 교체 등).
	// CancelHandle 은 지연 콜백 큐에 들어간 완료 델리게이트까지 취소하므로, 취소된 요청이 뒤늦게 발화해 새 값을 덮어쓰지 않는다.
	if (Request.Handle.IsValid())
	{
		Request.Handle->CancelHandle();
		Request.Handle.Reset();
	}

	Request.Pending = InImage;

	if (InImage.IsNull())
	{
		ApplyLoadedImage(FieldName, nullptr);
		return;
	}

	if (UObject* Loaded = InImage.Get())
	{
		ApplyLoadedImage(FieldName, Loaded);
		return;
	}

	Request.Handle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		InImage.ToSoftObjectPath(),
		FStreamableDelegate::CreateUObject(this, &UWxViewModel::HandleImageLoaded, FieldName));
}

void UWxViewModel::ApplyLoadedImage(FName FieldName, UObject* LoadedImage)
{
}

void UWxViewModel::HandleImageLoaded(FName FieldName)
{
	FWxImageRequest* Request = ImageRequests.Find(FieldName);
	if (!Request)
	{
		return;
	}

	ApplyLoadedImage(FieldName, Request->Pending.Get());
	Request->Handle.Reset();
}
