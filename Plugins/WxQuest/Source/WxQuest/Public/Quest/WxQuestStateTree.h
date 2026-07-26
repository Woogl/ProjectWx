// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTree.h"
#include "WxQuestStateTree.generated.h"

/**
 * 퀘스트 1개를 담는 StateTree 에셋 타입.
 * 지정 필드(StartNextQuest 체인)와 컴포넌트 API 가 이 타입만 받아, 기믹 등 일반 ST 오지정을 픽커·컴파일 단계에서 차단한다.
 * 퀘스트 메타데이터(신원 태그 등)가 더 필요해지면 여기에 얹는다.
 * 에셋 신규 생성은 WxEditor 의 팩토리가 담당한다(컴포넌트 러너 전제라 스키마를 StateTreeComponentSchema 로 고정).
 */
UCLASS()
class WXQUEST_API UWxQuestStateTree : public UStateTree
{
	GENERATED_BODY()

public:
	/**
	 * 레벨 시작 시 자동 탑재(수주 대기) 여부. 퀘스트 컴포넌트가 애셋 레지스트리에서 발견해 탑재하며, 진행 개시는 이 에셋의 Start 게이트 태스크가 판정한다.
	 * 체인(StartNextQuest)으로 시작되는 후속 퀘스트는 끈 채로 둔다. 활성 1개 원칙이라 켜진 에셋은 프로젝트에 1개여야 한다.
	 * 레지스트리 태그로 로드 없이 판정되므로, 값 변경은 에셋 재저장 후에야 발견에 반영된다.
	 */
	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = "Wx")
	bool bAutoStart = false;
};
