// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPtr.h"

struct FStreamableHandle;

namespace WxViewModel
{
	/**
	 * 표시용 이미지(텍스처 또는 머터리얼)를 비동기 스트리밍하고 OnLoaded 로 넘긴다. 비어 있거나 이미 로드돼 있으면 즉시 넘긴다.
	 * InOutHandle 은 이미지 필드마다 하나 두며, 같은 필드의 이전 요청을 취소하는 데 쓴다. 완료 통지는 Owner 가 살아 있을 때만 온다.
	 */
	WXUI_API void RequestImageAsync(UObject& Owner, TSharedPtr<FStreamableHandle>& InOutHandle, const TSoftObjectPtr<UObject>& Image, TFunction<void(UObject*)> OnLoaded);
}
