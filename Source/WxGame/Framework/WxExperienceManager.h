// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "WxExperienceManager.generated.h"

/**
 * PIE 다중 세션에서 GameFeature 플러그인 활성 요청을 URL 별로 카운팅하는 엔진 서브시스템 (Lyra ExperienceManager 이식).
 * 없으면 한 PIE 세션의 종료가 다른 세션이 아직 쓰는 플러그인까지 비활성화한다. 비에디터에선 카운팅 없이 항상 통과한다.
 */
UCLASS()
class WXGAME_API UWxExperienceManager : public UEngineSubsystem
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	/** PIE 시작 시 카운터를 리셋한다. WxEditor 모듈이 BeginPIE 델리게이트에서 호출한다. */
	void OnPlayInEditorBegun();
#endif

	/** 활성 요청 카운트를 올린다. LoadAndActivate 요청 직전에 호출한다. */
	static void NotifyOfPluginActivation(const FString& PluginURL);

	/** 카운트를 내리고 마지막 요청이 풀렸으면 true — 그때만 실제 Deactivate 를 호출한다. */
	static bool RequestToDeactivatePlugin(const FString& PluginURL);

#if WITH_EDITOR
private:
	TMap<FString, int32> GameFeaturePluginRequestCountMap;
#endif
};
