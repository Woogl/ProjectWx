// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "WxDialogueSessionComponent.generated.h"

class APlayerController;
class UAbilitySystemComponent;
class USpringArmComponent;
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
 * 대화는 뜻을 해석하지도 기록을 남기지도 않는다 — 진행 중인 대사의 신원(현재 행)과 대상만 노출하고,
 * 그 의미(퀘스트 수주 등)는 소비자(Wait Dialogue Completed 태스크)가 관찰로 판정한다.
 * 세션 상태는 소유 클라에 있다 — v1 싱글/리슨 호스트(소유 클라=권위 동일 머신) 전제로 권위 측 소비자가 직접 읽는다.
 *
 * UI 는 모른다 — 시작·대사·종료를 델리게이트로 발행하고, 구독자(PC·뷰모델)가 위젯을 잇는다.
 *
 * 반면 대화 카메라는 여기서 직접 든다. 컨트롤러에 붙어 있어 컨트롤 회전에 손이 닿고, 구도의 재료인 대상·시작·종료를 이미 다 알기 때문이다.
 * 카메라를 갈아끼우지는 않는다 — 뷰 타겟은 플레이어 폰에 그대로 두고, 평소 쓰는 3인칭 팔로우 카메라를 대화 동안만 대상 쪽으로 돌리고 앞으로 당긴다.
 */
UCLASS()
class WXDIALOGUE_API UWxDialogueSessionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWxDialogueSessionComponent();

	/** 대화 카메라 조정. 세션이 열려 있는 동안만 돌고, 복귀를 마치면 스스로 멈춘다. */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 서버 권위 진입점. 상호작용 응답이 대상의 대화 정의를 넘겨 호출하면 소유 클라에서 세션이 열린다. */
	void StartDialogue(UWxDialogueComponent* Dialogue);

	/**
	 * 대화 정의 컴포넌트 없이 행을 직접 지정하는 진입점. 퀘스트 ST 의 Play Dialogue 태스크처럼 대사를 고르는 쪽이 액터가 아닐 때 쓴다.
	 * Target 은 카메라 전환·관찰자 노출용일 뿐이라 비워도 되며(나레이션), 그때는 뷰 타겟이 플레이어 폰에 머문다.
	 */
	void StartDialogueRow(const FDataTableRowHandle& StartRow, AActor* Target);

	/** 뷰의 대사 넘기기 요청. NextDialogue 를 따라가고, 더 없으면 종료한다. */
	void Advance();

	bool HasActiveDialogue() const { return CurrentRow != nullptr; }

	/** 진행 중인 대화의 대상 액터. 대화 중이 아니거나 대상 없는 대사(나레이션)면 null. */
	AActor* GetCurrentDialogueTarget() const;

	/**
	 * 진행 중인 대사의 행 핸들. 관찰자가 "지금 어느 대사인가"를 가리는 신원이며, 대사를 넘길 때마다 바뀐다.
	 * 대화 중이 아니면 비어 있다 — 미지정 인자와 같아 보이므로 비교 전에 HasActiveDialogue 로 가린다.
	 */
	FDataTableRowHandle GetCurrentRowHandle() const;

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

protected:
	/**
	 * 대화 카메라 조정 파라미터.
	 * 대상이 아니라 대화 연출 전반의 정책이라 여기 모아 둔다.
	 * 본 컴포넌트는 PlayerController 의 기본 서브오브젝트라 컨트롤러 BP 의 디테일 패널에서 조정한다.
	 */

	/** 대화 중 당길 스프링암 길이(cm). 평소 팔 길이에서 이만큼 줄인다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Camera")
	float CameraZoomDistance = 200.f;

	/** 대화 중 카메라가 어깨 쪽으로 비껴서는 거리(cm). 0 이면 플레이어 등 뒤 정중앙이라 대상이 등에 가린다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Camera")
	float CameraShoulderOffset = 60.f;

	/**
	 * 겨눌 지점을 대상의 루트에서 위로 올리는 높이(cm). 루트는 캡슐 중심(허리)이라 0 이면 얼굴이 아니라 허리를 내려본다.
	 * 기본값은 눈높이 수준이다. 카메라보다 조금 높은 지점이라 살짝 올려보게 되며, 수평에 가깝게 두려면 가슴 높이로 낮춘다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Camera")
	float CameraTargetHeightOffset = 40.f;

	/** 줌·어깨 오프셋 보간 속도. 대화가 열릴 때와 끝나고 되돌아올 때 양쪽에 쓴다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Camera")
	float CameraZoomSpeed = 12.f;

	/** 대상을 향해 도는 속도. 겨누는 자리가 회전을 따라 함께 움직이므로 실제로 도는 속도는 이 값보다 느리다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Camera")
	float CameraTurnSpeed = 15.f;

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

	/** 평소 팔 길이를 재 두고 카메라 조정을 시작한다. 대상 없는 대사(나레이션)면 카메라를 건드리지 않는다. */
	void BeginDialogueCamera();

	/** 오너 컨트롤러를 로컬 플레이어 컨트롤러로 얻는다. 카메라는 로컬 어포던스라 그 밖에선 null 을 답해 카메라 경로를 통째로 건너뛴다. */
	APlayerController* GetLocalPlayerController() const;

	/** 조정 대상인 팔로우 카메라의 스프링암. 폰의 것이므로 폰이 없으면 null. */
	USpringArmComponent* FindCameraBoom() const;

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

	/** 대화가 카메라를 건드리기 전의 팔 길이·어깨 오프셋. 종료 후 여기로 되돌아간다. */
	float RestoreArmLength = 0.f;

	float RestoreShoulderOffset = 0.f;
};
