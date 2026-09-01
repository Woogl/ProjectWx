// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ControllerComponent.h"
#include "Engine/DataTable.h"
#include "WxDialogueSessionComponent.generated.h"

class ACameraActor;
class APlayerController;
class UAbilitySystemComponent;
class UAnimMontage;
class UWxDialogueComponent;
struct FStreamableHandle;
struct FWxDialogueTableRow;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWxOnDialogueLineChanged, const FText&, Speaker, const FText&, Line);

/**
 * Experience 주입으로 PlayerController 에 붙는다.
 *
 * 대화 대상은 비소유 액터라 Client RPC 를 쏠 수 없으므로, 클라 UI 로 가는 전달은 PC 측인 본 컴포넌트가 소유한다.
 * 그 RPC 때문에 복제 컴포넌트여야 한다.
 * 세션(현재 노드·라인)은 표시 전용 로컬 상태라 소유 클라가 진행을 소유하며 서버 검증은 없다. 대화가 게임 상태를 바꾸게 되면 그때 서버측으로 옮긴다.
 *
 * 대화는 뜻을 해석하지도 기록을 남기지도 않는다 — 진행 중인 대사의 신원(현재 행)과 대상만 노출하고, 그 의미(퀘스트 수주 등)는 소비자가 관찰로 판정한다.
 * v1 싱글/리슨 호스트(소유 클라=권위 동일 머신) 전제라 권위 측 소비자가 이 로컬 상태를 직접 읽는다.
 *
 * UI 는 모른다 — 대사가 바뀌면 델리게이트로 발행해 뷰모델이 받아 가고, 세션이 열리고 닫힌 사실은 폰 ASC 의 State.Dialogue 태그가 알린다.
 * 대화 창을 여닫는 것은 그 태그를 보는 쪽(UI 매니저)의 몫이라 UI 를 위한 시작·종료 델리게이트는 두지 않는다.
 * 그래서 폰 ASC 는 세션의 전제다 — 태그를 올릴 곳이 없으면 창을 띄울 방법도 없으므로 세션을 열지 않는다.
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

	/** 뷰의 대사 넘기기 요청. NextRow 를 따라가고, 더 없으면 종료한다. */
	void Advance();

	bool HasActiveDialogue() const;

	/** 대화 중이 아니거나 대상 없는 대사(나레이션)면 null. */
	AActor* GetCurrentDialogueTarget() const;

	/**
	 * 관찰자가 "지금 어느 대사인가"를 가리는 신원이며, 대사를 넘길 때마다 바뀐다.
	 * 대화 중이 아니면 비어 있다 — 미지정 인자와 같아 보이므로 비교 전에 HasActiveDialogue 로 가린다.
	 */
	FDataTableRowHandle GetCurrentRowHandle() const;

	FText GetCurrentSpeaker() const;

	FText GetCurrentLine() const;

	UPROPERTY(BlueprintAssignable, Category = "Wx")
	FWxOnDialogueLineChanged OnLineChanged;

	/**
	 * 대화가 끝나면 한 번 발화하고 스스로 비워진다. 종료를 기다리는 쪽('Play Dialogue' 태스크)이 대화를 연 직후 붙인다.
	 * 발화와 함께 비워지므로 붙인 쪽이 떼어낼 필요가 없다 — 대화 한 번에 대한 일회성 약속이다.
	 */
	FSimpleMulticastDelegate OnDialogueEnded;

protected:
	/**
	 * 카메라 구도는 대상이 아니라 대화 연출 전반의 정책이라 여기 모아 둔다.
	 * 주입 컴포넌트라 여기 적힌 기본값이 곧 실제 값이다.
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
	/** 대상 액터는 관찰자 노출을 위해 세션 동안 기억한다. */
	UFUNCTION(Client, Reliable)
	void ClientStartDialogue(const FDataTableRowHandle& StartRow, AActor* Target);

	/** 행이 없거나 대사가 비어 있으면 실패한다. */
	bool EnterRow(FName RowName);

	/**
	 * 진행 중인 행을 테이블에서 되찾는다. 세션 중이 아니면 null.
	 * 행 실체를 캐시하지 않는 이유는 그 메모리가 테이블 소유라서다 — 재임포트가 행 버퍼를 통째로 갈아끼우면 붙잡아 둔 포인터는 해제된 메모리를 가리킨다.
	 */
	const FWxDialogueTableRow* FindCurrentRow() const;

	void PublishCurrentLine();

	void EndDialogue();

	/** 두 사람의 배치에서 구도를 잡아 대화 카메라를 세우고 뷰를 넘긴다. 대상 없는 대사(나레이션)면 카메라를 건드리지 않는다. */
	void BeginDialogueCamera();

	void EndDialogueCamera();

	/**
	 * 현재 대사가 지목한 포즈를 스트리밍해 대상 메시에 재생한다. 지목이 없으면 직전 포즈를 그대로 둔다.
	 * 앞 대사가 띄운 스트리밍이 남아 있으면 접는다 — 늦게 도착한 포즈가 새 포즈를 덮어쓰지 않게 한다.
	 */
	void ApplyCurrentPose();

	void HandlePoseLoaded();

	void PlayPendingPose();

	/** 카메라는 로컬 어포던스라 로컬 컨트롤러가 아니면 null 을 답해 카메라 경로를 통째로 건너뛴다. */
	APlayerController* GetLocalPlayerController() const;

	/** 세션 동안 State.Dialogue 를 발행해 둔 폰 ASC. 종료 시 같은 ASC 에서 되돌리기 위해 기억한다(도중 폰 교체 대비). */
	TWeakObjectPtr<UAbilitySystemComponent> TaggedAbilitySystem;

	TWeakObjectPtr<AActor> CurrentTarget;

	/** 진행 중인 대화를 연 시작 행. 세션 동안 테이블 객체를 붙잡는 강참조이자 진행 중 노드를 찾을 테이블의 출처다. */
	UPROPERTY()
	FDataTableRowHandle CurrentStartRow;

	/** 현재 노드의 행 이름이자 세션 자체의 상태. 비어 있으면 대화 중이 아니고, 행 실체는 여기서 매번 되찾는다(FindCurrentRow). */
	FName CurrentRowName;

	/** 세션 동안 세워 둔 대화 카메라. 종료 시 뷰를 되돌리고 만료시킨다. */
	TWeakObjectPtr<ACameraActor> DialogueCamera;

	/**
	 * 스트리밍을 걸어 둔 포즈와 그 대상. 완료 콜백이 인자를 받지 않으므로 "무엇을 누구에게"를 요청 시점에 남긴다.
	 * 세션이 닫힌 뒤 도착해도 제 대상에 얹히도록 CurrentTarget 과 따로 든다 — 포즈는 대화가 끝나도 거두지 않는다.
	 */
	TSoftObjectPtr<UAnimMontage> PendingPose;
	TWeakObjectPtr<AActor> PendingPoseTarget;

	/** 진행 중인 포즈 스트리밍. 다음 대사가 포즈를 새로 지목할 때 취소하는 용도다. */
	TSharedPtr<FStreamableHandle> PoseLoadHandle;
};
