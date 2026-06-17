// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WxMusicLibrary.generated.h"

/**
 * BGM 시스템의 Blueprint 진입점. UWxMusicSubsystem 으로 위임하는 thin wrapper.
 */
UCLASS()
class WXAUDIO_API UWxMusicLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** BGM 분류 태그로 BGM 을 시작한다. Chooser 테이블이 이 태그(+ 플레이어 상태)로 고른 곡을 재생한다. */
	UFUNCTION(BlueprintCallable, Category = "Wx|BGM", meta = (WorldContext = "WorldContextObject"))
	static void StartBGM(const UObject* WorldContextObject, FGameplayTag BGMTag);

	/** 재생 중인 BGM 을 페이드아웃하고 보류한다. */
	UFUNCTION(BlueprintCallable, Category = "Wx|BGM", meta = (WorldContext = "WorldContextObject"))
	static void StopBGM(const UObject* WorldContextObject);
};
