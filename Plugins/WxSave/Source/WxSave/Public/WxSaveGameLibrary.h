// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WxSaveGameLibrary.generated.h"

/**
 * WxSave 의 BP 진입점. UWxSaveGameSubsystem 의 공개 API 를 정적 래퍼로 노출한다.
 */
UCLASS()
class WXSAVE_API UWxSaveGameLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 현재 월드의 IWxSavable 액터 + 첫 플레이어 Pawn Transform 을 캡처하고 SlotName 파일에 비동기 기록한다. */
	UFUNCTION(BlueprintCallable, Category = "Wx|Save", meta = (WorldContext = "WorldContextObject"))
	static void SaveSlot(const UObject* WorldContextObject, const FString& SlotName);

	/** SlotName 파일을 메모리로 로드한 뒤 현재 맵을 리로드(ServerTravel)하여 액터를 복원한다(authority 전제). 반환값은 슬롯 파일 존재 여부. */
	UFUNCTION(BlueprintCallable, Category = "Wx|Save", meta = (WorldContext = "WorldContextObject"))
	static bool LoadSlot(const UObject* WorldContextObject, const FString& SlotName);
};
