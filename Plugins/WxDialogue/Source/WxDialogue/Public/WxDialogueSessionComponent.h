// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ControllerComponent.h"
#include "Engine/DataTable.h"
#include "WxDialogueSessionComponent.generated.h"

class ACameraActor;
class APlayerController;
class UAbilitySystemComponent;
class UWxDialogueComponent;
struct FWxDialogueTableRow;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWxOnDialogueLineChanged, const FText&, Speaker, const FText&, Line);

/**
 * 플레이어의 대화 세션 컴포넌트. Experience 주입으로 PlayerController 에 붙는다 — 컨트롤러 컴포넌트라는 사실이 곧 주입 대상 선언이다.
 *
 * 서버의 상호작용 응답(예: AWxNpc)이 StartDialogue 로, 퀘스트 ST 처럼 액터가 아닌 쪽은 StartDialogueRow 로 진입하면 소유 클라로 넘겨 세션을 연다.
 * 대화 대상은 비소유 액터라 Client RPC 를 쏠 수 없으므로, 클라 UI 로 가는 전달은 PC 측인 본 컴포넌트가 소유한다.
 * 그 RPC 때문에 복제 컴포넌트여야 한다 — 주입으로 만들어지는 만큼, 클라에 실체가 서 있어야 서버가 쏜 RPC 가 도착할 자리가 생긴다.
 * 세션(현재 노드·라인)은 표시 전용 로컬 상태라 소유 클라가 진행을 소유하며 서버 검증은 없다. 대화가 게임 상태를 바꾸게 되면 그때 서버측으로 옮긴다.
 *
 * 대화는 뜻을 해석하지도 기록을 남기지도 않는다 — 진행 중인 대사의 신원(현재 행)과 대상만 노출하고,
 * 그 의미(퀘스트 수주 등)는 소비자(Wait Dialogue Completed 태스크)가 관찰로 판정한다.
 * 세션 상태는 소유 클라에 있다 — v1 싱글/리슨 호스트(소유 클라=권위 동일 머신) 전제로 권위 측 소비자가 직접 읽는다.
 *
 * UI 는 모른다 — 대사가 바뀌면 델리게이트로 발행해 뷰모델이 받아 가고, 세션이 열리고 닫힌 사실은 폰 ASC 의 State.Dialogue 태그가 알린다.
 * 대화 창을 여닫는 것은 그 태그를 보는 쪽(UI 매니저)의 몫이라 여기엔 시작·종료 델리게이트를 두지 않는다.
 *
 * 반면 대화 카메라는 여기서 직접 든다. 컨트롤러에 붙어 있어 뷰 타겟에 손이 닿고, 구도의 재료인 대상·시작·종료를 이미 다 알기 때문이다.
 * 대화 동안에는 전용 카메라를 세워 뷰 타겟을 그리로 넘긴다 — 게임플레이 카메라는 플레이어 등 뒤에 매여 있어, 두 사람을 잇는 선에서 크게 비껴선 구도를 잡을 수 없다.
 *
 * 대상의 포즈도 같은 이유로 여기서 든다. 어느 대사에 어떤 자세인지는 대화 데이터가 이미 들고 있어, 대사를 넘기는 이 자리가 그것을 갈아끼울 유일한 지점이다.
 * 다만 카메라와 달리 되돌리지 않는다 — 대화가 끝나도 대상은 마지막 자세로 남고, 다음 대사나 다음 대화가 그것을 갈아끼운다. 그래서 세션은 무엇을 재생했는지 기억할 필요가 없다.
 */
UCLASS()
class WXDIALOGUE_API UWxDialogueSessionComponent : public UControllerComponent
{
	GENERATED_BODY()

public:
	UWxDialogueSessionComponent(const FObjectInitializer& ObjectInitializer);

	/** 서버 권위 진입점. 상호작용 응답이 대상의 대화 정의를 넘겨 호출하면 소유 클라에서 세션이 열린다. */
	void StartDialogue(UWxDialogueComponent* Dialogue);

	/**
	 * 대화 정의 컴포넌트 없이 행을 직접 지정하는 진입점. 퀘스트 ST 의 Play Dialogue 태스크처럼 대사를 고르는 쪽이 액터가 아닐 때 쓴다.
	 * Target 은 카메라 전환·관찰자 노출용일 뿐이라 비워도 되며(나레이션), 그때는 뷰 타겟이 플레이어 폰에 머문다.
	 */
	void StartDialogueRow(const FDataTableRowHandle& StartRow, AActor* Target);

	/** 뷰의 대사 넘기기 요청. NextDialogue 를 따라가고, 더 없으면 종료한다. */
	void Advance();

	bool HasActiveDialogue() const;

	/** 진행 중인 대화의 대상 액터. 대화 중이 아니거나 대상 없는 대사(나레이션)면 null. */
	AActor* GetCurrentDialogueTarget() const;

	/**
	 * 진행 중인 대사의 행 핸들. 관찰자가 "지금 어느 대사인가"를 가리는 신원이며, 대사를 넘길 때마다 바뀐다.
	 * 대화 중이 아니면 비어 있다 — 미지정 인자와 같아 보이므로 비교 전에 HasActiveDialogue 로 가린다.
	 */
	FDataTableRowHandle GetCurrentRowHandle() const;

	FText GetCurrentSpeaker() const;

	FText GetCurrentLine() const;

	/** 표시할 대사가 바뀌었다. */
	UPROPERTY(BlueprintAssignable, Category = "Wx")
	FWxOnDialogueLineChanged OnLineChanged;

protected:
	/**
	 * 대화 카메라 구도 파라미터.
	 * 대상이 아니라 대화 연출 전반의 정책이라 여기 모아 둔다.
	 * 주입 컴포넌트라 여기 적힌 기본값이 곧 실제 값이다 — 에셋으로 조정하려면 이 클래스의 BP 서브클래스를 만들어 주입 액션에 등록한다.
	 */

	/** 대화 중 시야각(도). 게임플레이(90)보다 좁혀 망원처럼 압축한다 — 광각은 가까운 사람만 크게 부풀리고 얼굴을 왜곡한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Camera")
	float CameraFieldOfView = 55.f;

	/** 두 사람을 잇는 선에서 카메라가 비껴서는 각(도). 작으면 앞사람 어깨 뒤에서 보는 구도, 키우면 둘을 옆에서 보는 구도가 된다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Camera")
	float CameraOffAxisAngle = 45.f;

	/** 두 사람의 중간점에서 카메라까지의 거리(cm). 키우면 전신까지 들어오고(480 부근) 줄이면 상반신으로 조인다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Camera")
	float CameraDistance = 350.f;

	/**
	 * 카메라와 겨눌 지점의 높이(cm). 두 사람의 루트(캡슐 중심)에서 이만큼 위다.
	 * 둘이 같은 높이라 시선은 항상 수평이며, 이 값은 화면이 세로로 어디를 담을지를 정한다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Camera")
	float CameraHeightOffset = 40.f;

	/** 대화 카메라로 넘어갈 때와 게임플레이 뷰로 돌아올 때의 블렌드 시간(초). */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Camera")
	float CameraBlendTime = 0.5f;

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

	/** 두 사람의 배치에서 구도를 잡아 대화 카메라를 세우고 뷰를 넘긴다. 대상 없는 대사(나레이션)면 카메라를 건드리지 않는다. */
	void BeginDialogueCamera();

	/** 게임플레이 뷰로 되돌리고 대화 카메라를 정리한다. */
	void EndDialogueCamera();

	/** 현재 대사가 지목한 포즈를 대상 메시에 재생한다. 지목이 없으면 직전 포즈를 그대로 둔다. */
	void ApplyCurrentPose();

	/** 오너 컨트롤러를 로컬 플레이어 컨트롤러로 얻는다. 카메라는 로컬 어포던스라 그 밖에선 null 을 답해 카메라 경로를 통째로 건너뛴다. */
	APlayerController* GetLocalPlayerController() const;

	/** 세션 동안 State.Dialogue 를 발행해 둔 폰 ASC. 종료 시 같은 ASC 에서 되돌리기 위해 기억한다(도중 폰 교체 대비). */
	TWeakObjectPtr<UAbilitySystemComponent> TaggedAbilitySystem;

	/** 진행 중인 대화의 대상 액터. 관찰자(GetCurrentDialogueTarget)에게 노출되며 세션 중에만 유효하다. 대상 없는 대사(나레이션)에선 비어 있다. */
	TWeakObjectPtr<AActor> CurrentTarget;

	/** 진행 중인 대화를 연 시작 행. 세션 동안 행 메모리를 붙잡는 강참조이자 진행 중 노드를 찾을 테이블의 출처다. */
	UPROPERTY()
	FDataTableRowHandle CurrentStartRow;

	/** 현재 노드. 시작 행이 붙잡고 있는 테이블의 행 메모리를 가리키며 세션 중에만 유효하다. */
	const FWxDialogueTableRow* CurrentRow = nullptr;

	/** 현재 노드의 행 이름. 행 구조체가 자기 이름을 모르므로 관찰자에게 노출할 신원을 세션이 따로 기억한다. */
	FName CurrentRowName;

	/** 세션 동안 세워 둔 대화 카메라. 종료 시 뷰를 되돌리고 만료시킨다. */
	TWeakObjectPtr<ACameraActor> DialogueCamera;
};
