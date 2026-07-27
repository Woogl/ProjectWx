// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/GameStateComponent.h"
#include "GameplayTagContainer.h"
#include "StateTreeExecutionTypes.h"
#include "WxQuestComponent.generated.h"

class UStateTreeComponent;
class UWxQuestStateTree;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWxOnQuestJournalChanged);

/**
 * 저널에 등록된 목표 하나.
 * 목표는 등록한 태스크가 자기 상태를 떠날 때 스스로 걷어가므로, 문구가 아니라 발급 핸들로 지목한다(같은 문구가 둘일 수 있다).
 */
USTRUCT()
struct FWxQuestObjective
{
	GENERATED_BODY()

	/** 등록 시 발급된 핸들. */
	UPROPERTY()
	int32 Handle = INDEX_NONE;

	/** 저널·HUD 에 표시할 목표 문구. */
	UPROPERTY()
	FText Text;
};

/**
 * GameState 에 부착되어 퀘스트 StateTree 실행과 저널(제목·목표)을 서버 권위로 관리하는 컴포넌트.
 *
 * 퀘스트 1개 = UWxQuestStateTree 에셋 1개이며, 활성 퀘스트는 동시 1개다(새 시작은 교체).
 * UStateTreeComponent 와는 다중상속이 불가하므로, 권위 측 BeginPlay 에서 순정 러너를 런타임 생성해 소유하고 실행을 위임한다.
 * 퀘스트 태스크는 컨텍스트 오너(GameState)에서 본 컴포넌트를 찾아 저널 갱신·다음 퀘스트 시작을 요청한다.
 * 러너가 권위에만 존재하므로 월드 부수효과(스폰·보상)는 단일 구동이 보장되고, 저널도 권위(싱글/리슨 호스트)에서만 채워진다.
 * 저널 정리는 태스크가 아니라 러너의 실행 상태 변경 통지로 한다 — 완료·실패·교체 세 종료 경로가 전부 한 곳으로 수렴한다.
 *
 * 본 컴포넌트는 어떤 퀘스트 에셋도 알지 않는다(에셋 불가지). 무엇을 실행할지는 전부 데이터가 지정한다:
 *  - 수주(탑재): bAutoStart 를 켠 퀘스트 에셋을 권위 BeginPlay 가 애셋 레지스트리에서 발견해 탑재 (진행 개시는 퀘스트 자신의 Start 게이트 태스크가 판정)
 *  - 체인: StartNextQuest 태스크의 NextQuest 소프트 참조
 *  - 임의 시작: UWxQuestLibrary::StartQuest (레벨 스크립트 등 외부 호출)
 *
 * 부착은 코드가 아니라 GameMode 가 고른 Experience 에셋의 주입 목록으로 한다(GameState 는 본 클래스를 모른다).
 * 목록에는 사이드 구분이 없어 클라 GameState 에도 사본이 붙으므로, 러너를 권위에서만 띄우는 것은 본 클래스의 책임이다.
 */
UCLASS()
class WXQUEST_API UWxQuestComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
	UWxQuestComponent(const FObjectInitializer& ObjectInitializer);

	/** 진행 중인 퀘스트를 정지하고 지정 퀘스트를 시작한다. 러너가 Running 중엔 에셋 교체가 거부되므로 ST 실행 콜스택 밖에서만 호출한다. */
	void StartQuest(UWxQuestStateTree* QuestAsset);

	/** ST 태스크 등 러너 실행 콜스택 안에서의 시작 요청. 재진입이 막히므로 다음 틱에 로드·시작한다. */
	void RequestStartQuest(TSoftObjectPtr<UWxQuestStateTree> QuestAsset);

	/** 실행 중인 퀘스트 ST 로 이벤트를 보낸다(Quest.Fail 등 전이 트리거). */
	void SendQuestEvent(FGameplayTag EventTag);

	/** SetQuestTitle 태스크 진입점. 저널을 새 퀘스트 제목으로 등록한다(목표는 비움). */
	void SetQuestTitle(const FText& InQuestTitle);

	/** SetQuestObjective 태스크 진입점. 목표를 추가하고 나중에 걷어갈 핸들을 돌려준다. */
	int32 AddObjective(const FText& InObjectiveText);

	/** SetQuestObjective 태스크 이탈점. 지정 핸들의 목표를 걷어간다. 이미 없는 핸들은 무시한다. */
	void RemoveObjective(int32 ObjectiveHandle);

	bool HasActiveQuest() const { return bHasActiveQuest; }
	FText GetQuestTitle() const { return QuestTitle; }

	/** 현재 표시 중인 목표 문구들을 등록 순서대로 돌려준다. */
	TArray<FText> GetObjectiveTexts() const;

	/** 저널 등록·목표 갱신·정리 시 발화. HUD 뷰모델이 구독해 현재 값을 pull 한다. */
	UPROPERTY(BlueprintAssignable, Category = "Wx|Quest")
	FWxOnQuestJournalChanged OnJournalChanged;

protected:
	virtual void BeginPlay() override;

private:
	/** 러너 실행 상태 변경 수신. Running 이탈(완료·실패·정지)이면 탑재 기록·저널만 정리한다 — 이 콜백 안 재시작은 엔진 재진입 가드에 막힌다. */
	UFUNCTION()
	void HandleStateTreeRunStatusChanged(EStateTreeRunStatus StateTreeRunStatus);

	/** RequestStartQuest 의 다음 틱 타이머 콜백. 이 시점엔 러너가 실행 콜스택 밖이라 시작이 안전하다. */
	void HandleDeferredStartQuest(TSoftObjectPtr<UWxQuestStateTree> QuestAsset);

	void ClearJournal();

	/** 퀘스트 ST 를 실행하는 러너. 권위 측 BeginPlay 에서 생성되며 비-권위 머신에선 null 이다. */
	UPROPERTY()
	TObjectPtr<UStateTreeComponent> QuestStateTree;

	FText QuestTitle;

	/** 현재 표시 중인 목표들. 태스크가 상태에 들고 날 때마다 늘고 준다. */
	TArray<FWxQuestObjective> Objectives;

	/** 다음 목표에 발급할 핸들. 재사용하지 않으므로 뒤늦은 제거 요청이 엉뚱한 목표를 걷어가지 않는다. */
	int32 NextObjectiveHandle = 0;

	bool bHasActiveQuest = false;
};
