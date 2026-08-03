// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTree.h"
#include "WxQuestStateTree.generated.h"

/**
 * 퀘스트 1개를 담는 StateTree 에셋 타입.
 * 지정 필드(ActivateNextQuest 체인)와 컴포넌트 API 가 이 타입만 받아, 기믹 등 일반 ST 오지정을 픽커·컴파일 단계에서 차단한다.
 * 퀘스트 메타데이터(신원 태그 등)가 더 필요해지면 여기에 얹는다.
 * 에셋 신규 생성은 WxEditor 의 팩토리가 담당한다(컴포넌트 러너 전제라 스키마를 StateTreeComponentSchema 로 고정).
 */
UCLASS()
class WXQUEST_API UWxQuestStateTree : public UStateTree
{
	GENERATED_BODY()
};
