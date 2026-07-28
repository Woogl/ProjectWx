// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "WxDialogueSessionComponent.generated.h"

class UAbilitySystemComponent;
class UWxDialogueComponent;
struct FWxDialogueTableRow;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWxOnDialogueStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWxOnDialogueLineChanged, const FText&, Speaker, const FText&, Line);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWxOnDialogueEnded);

/**
 * 플레이어의 대화 세션 컴포넌트. PlayerController 에 붙는다.
 *
 * 서버의 상호작용 응답(예: AWxNpc)이 StartDialogue 로, 퀘스트 ST 처럼 액터가 아닌 쪽은 StartDialogueRow 로 진입하면 소유 클라로 넘겨 세션을 연다.
 * 대화 대상은 비소유 액터라 Client RPC 를 쏠 수 없으므로, 클라 UI 로 가는 전달은 PC 측인 본 컴포넌트가 소유한다.
 * 세션(현재 노드·라인)은 표시 전용 로컬 상태라 소유 클라가 진행을 소유하며 서버 검증은 없다. 대화가 게임 상태를 바꾸게 되면 그때 서버측으로 옮긴다.
 *
 * 대화는 뜻을 해석하지도 기록을 남기지도 않는다 — 진행 중인 대화의 신원(시작 행)과 대상만 노출하고,
 * 그 의미(퀘스트 수주 등)는 소비자(Wait Dialogue Completed 태스크)가 관찰로 판정한다.
 * 세션 상태는 소유 클라에 있다 — v1 싱글/리슨 호스트(소유 클라=권위 동일 머신) 전제로 권위 측 소비자가 직접 읽는다.
 *
 * UI 는 모른다 — 시작·대사·종료를 델리게이트로 발행하고, 구독자(PC·뷰모델)가 위젯을 잇는다.
 */
UCLASS()
class WXDIALOGUE_API UWxDialogueSessionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** 서버 권위 진입점. 상호작용 응답이 대상의 대화 정의를 넘겨 호출하면 소유 클라에서 세션이 열린다. */
	void StartDialogue(UWxDialogueComponent* Dialogue);

	/**
	 * 대화 정의 컴포넌트 없이 행을 직접 지정하는 진입점. 퀘스트 ST 의 Play Dialogue 태스크처럼 대사를 고르는 쪽이 액터가 아닐 때 쓴다.
	 * Target 은 카메라 전환·관찰자 노출용일 뿐이라 비워도 되며(나레이션), 그때는 뷰 타겟이 플레이어 폰에 머문다.
	 */
	void StartDialogueRow(const FDataTableRowHandle& StartRow, AActor* Target);

	/** 뷰의 대사 넘기기 요청. NextDialogue 를 따라가고, 더 없으면 종료한다. 선택지 노드에선 Choose 를 기다린다. */
	void Advance();

	/** 뷰의 선택지 선택 요청. 현재 노드의 Choices[ChoiceIndex] 가 가리키는 노드로 점프한다. */
	void Choose(int32 ChoiceIndex);

	bool HasActiveDialogue() const { return CurrentRow != nullptr; }

	/** 진행 중인 대화의 대상 액터. 대화 중이 아니거나 대상 없는 대사(나레이션)면 null. */
	AActor* GetCurrentDialogueTarget() const;

	/** 진행 중인 대화를 연 시작 행. 관찰자가 "어느 대화인가"를 가리는 신원이며, 세션이 열려 있는 동안 변하지 않는다. 대화 중이 아니면 비어 있다. */
	const FDataTableRowHandle& GetCurrentStartRow() const;

	FText GetCurrentSpeaker() const;

	FText GetCurrentLine() const;

	/** 세션이 열렸다. 소유 클라에서만 발행된다. 구독자(PC)가 대화 위젯을 띄운다. */
	UPROPERTY(BlueprintAssignable, Category = "Wx")
	FWxOnDialogueStarted OnDialogueStarted;

	/** 표시할 대사가 바뀌었다. */
	UPROPERTY(BlueprintAssignable, Category = "Wx")
	FWxOnDialogueLineChanged OnLineChanged;

	/** 세션이 끝났다. 구독자(PC)가 대화 위젯을 닫는다. */
	UPROPERTY(BlueprintAssignable, Category = "Wx")
	FWxOnDialogueEnded OnDialogueEnded;

private:
	/** 시작 행을 소유 클라로 넘겨 세션을 시드하고 시작을 발행한다. 대상 액터는 관찰자 노출을 위해 세션 동안 기억한다. */
	UFUNCTION(Client, Reliable)
	void ClientStartDialogue(const FDataTableRowHandle& StartRow, AActor* Target);

	/** 이름으로 노드를 찾아 현재 노드로 전환한다. 행이 없거나 대사가 비어 있으면 실패한다. */
	bool EnterRow(FName RowName);

	/** 현재 대사를 OnLineChanged 로 발행한다. */
	void PublishCurrentLine();

	/** 세션을 비우고 종료를 발행한다. */
	void EndDialogue();

	/** 세션 동안 State.Dialogue 를 발행해 둔 폰 ASC. 종료 시 같은 ASC 에서 되돌리기 위해 기억한다(도중 폰 교체 대비). */
	TWeakObjectPtr<UAbilitySystemComponent> TaggedAbilitySystem;

	/** 진행 중인 대화의 대상 액터. 관찰자(GetCurrentDialogueTarget)에게 노출되며 세션 중에만 유효하다. 대상 없는 대사(나레이션)에선 비어 있다. */
	TWeakObjectPtr<AActor> CurrentTarget;

	/** 진행 중인 대화를 연 시작 행. 세션 동안 행 메모리를 붙잡는 강참조이자 관찰자에게 노출하는 대화의 신원이다. */
	UPROPERTY()
	FDataTableRowHandle CurrentStartRow;

	/** 현재 노드. 시작 행이 붙잡고 있는 테이블의 행 메모리를 가리키며 세션 중에만 유효하다. */
	const FWxDialogueTableRow* CurrentRow = nullptr;
};
