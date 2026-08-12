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

	// 아래부터 Request 참조를 쓰지 않는다 — ApplyLoadedImage·RequestAsyncLoad 가 재진입해 다른 슬롯을 추가하면 맵이 재해시돼 무효가 된다.

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

	const TSharedPtr<FStreamableHandle> NewHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		InImage.ToSoftObjectPath(),
		FStreamableDelegate::CreateUObject(this, &UWxViewModel::HandleImageLoaded, FieldName));

	// 요청한 에셋이 이미 메모리에 있으면 RequestAsyncLoad 안에서 완료 콜백이 바로 돌 수 있다.
	// 그 사이 슬롯이 다른 대상으로 재요청됐을 수 있으므로, 대상이 그대로일 때만 핸들을 보관한다.
	// (보관은 생략할 수 없다 — 로드 완료와 지연 콜백 사이에 에셋을 붙잡아 두는 유일한 참조다.)
	FWxImageRequest* CurrentRequest = ImageRequests.Find(FieldName);
	if (CurrentRequest && CurrentRequest->Pending == InImage)
	{
		CurrentRequest->Handle = NewHandle;
	}
	else if (NewHandle.IsValid())
	{
		NewHandle->CancelHandle();
	}
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

	// ApplyLoadedImage 재진입이 맵을 재해시하면 Request 가 무효가 되므로 맵 접근을 먼저 끝낸다.
	// 여기서 keep-alive 를 놓아도 파생 VM 이 곧바로 하드 참조로 받으므로 그 사이 GC 가 돌 여지는 없다.
	// 반대로 Pending 해석은 Reset 전이어야 한다.
	UObject* LoadedImage = Request->Pending.Get();
	Request->Handle.Reset();

	ApplyLoadedImage(FieldName, LoadedImage);
}
