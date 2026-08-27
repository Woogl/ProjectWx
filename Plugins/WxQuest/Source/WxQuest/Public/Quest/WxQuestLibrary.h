// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WxQuestLibrary.generated.h"

class UWxQuestStateTree;

/**
 * 월드 GameState 의 퀘스트 컴포넌트를 찾아 위임한다(레벨에 배치한 트리거 볼륨 등에서 호출).
 *
 * 러너가 권위에만 존재하므로 서버 권위 호출만 의미가 있고, 컴포넌트가 없거나 비-권위면 내부에서 무시된다.
 */
UCLASS()
class WXQUEST_API UWxQuestLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 진행 중인 퀘스트를 정지하고 지정 퀘스트를 시작한다. */
	UFUNCTION(BlueprintCallable, Category = "Wx|Quest", meta = (WorldContext = "WorldContextObject"))
	static void StartQuest(const UObject* WorldContextObject, UWxQuestStateTree* QuestAsset);
};
