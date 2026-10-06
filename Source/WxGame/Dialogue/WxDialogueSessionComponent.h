// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "WxDialogueSessionComponent.generated.h"

class ACameraActor;
class APawn;
class APlayerController;
class UAbilitySystemComponent;
class UAnimMontage;
class UWxDialogueComponent;
struct FStreamableHandle;
struct FWxDialogueTableRow;

DECLARE_MULTICAST_DELEGATE_TwoParams(FWxOnDialogueLineChanged, const FText& /*Speaker*/, const FText& /*Line*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FWxOnDialogueEnded, bool /*bCompleted*/);

/**
 * AWxPlayerController 생성자의 기본 서브오브젝트로 붙는다.
 *
 * 대화 대상은 비소유 액터라 Client RPC 를 쏠 수 없어 PC 측 복제 컴포넌트인 여기가 전달을 맡고, 세션은 표시 전용 로컬 상태라 소유 클라가 서버 검증 없이 진행을 소유한다.
 *
 * 대화는 의미(퀘스트 수주 등)를 판정하지 않고 종료를 기다린 쪽에 맡기며, v1 싱글/리슨 호스트 전제라 권위 측 소비자가 이 로컬 상태를 직접 읽는다.
 *
 * 세션 개폐는 폰 ASC 의 State.Dialogue 태그로만 알려 창은 UWxPlayerLayoutComponent 가 여닫으므로, 폰 ASC 가 없으면 세션을 열지 않고 빙의가 바뀌면 세션을 접는다.
 *
 * 게임플레이 카메라는 플레이어 등 뒤에 매여 두 사람 사이를 비껴선 구도를 못 잡으므로, 뷰 타겟에 손이 닿는 여기서 전용 대화 카메라를 세운다.
 *
 * 대상의 포즈도 대사를 넘기는 이 자리에서 갈아끼우되, 카메라와 달리 대화가 끝나도 되돌리지 않는다.
 */
UCLASS()
class WXGAME_API UWxDialogueSessionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWxDialogueSessionComponent(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 서버 권위 진입점. 상호작용 응답이 대상의 대화 정의를 넘겨 호출하면 소유 클라에서 세션이 열린다. */
	void StartDialogue(UWxDialogueComponent* Dialogue);

	/** 대화 정의 컴포넌트 없이 행을 직접 지정하는 진입점이며, Target 은 카메라·포즈용일 뿐이라 비워도 된다(나레이션). */
	void StartDialogueRow(const FDataTableRowHandle& StartRow, AActor* Target);

	/** 뷰의 대사 넘기기 요청. NextRow 를 따라가고, 더 없으면 종료한다. */
	void Advance();

	bool HasActiveDialogue() const;

	FText GetCurrentSpeaker() const;

	FText GetCurrentLine() const;

	FWxOnDialogueLineChanged OnLineChanged;

	/** 대화가 끝나면 한 번 발화하고 스스로 비워지며, bCompleted 는 마지막 행까지 읽었는지라 중단(테이블 갈림·행 해석 실패·빙의 전이·대화 겹침)이면 false 다. */
	FWxOnDialogueEnded OnDialogueEnded;

protected:
	/** 대화 중 시야각(도). 게임플레이(90)보다 좁혀 망원처럼 압축한다 — 광각은 가까운 사람만 크게 부풀리고 얼굴을 왜곡한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Camera")
	float CameraFieldOfView = 55.f;

	/** 두 사람을 잇는 선에서 카메라가 비껴서는 각(도). 작으면 앞사람 어깨 뒤에서 보는 구도, 키우면 둘을 옆에서 보는 구도가 된다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Camera")
	float CameraOffAxisAngle = 45.f;

	/** 두 사람의 중간점에서 카메라까지의 거리(cm). 키우면 전신까지 들어오고(480 부근) 줄이면 상반신으로 조인다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Camera")
	float CameraDistance = 350.f;

	/** 두 사람의 루트(캡슐 중심)에서 카메라와 겨눌 지점까지의 높이(cm)로, 시선은 항상 수평이라 화면이 세로로 어디를 담을지만 정한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Camera")
	float CameraHeightOffset = 40.f;

	/** 대화 카메라로 넘어갈 때와 게임플레이 뷰로 돌아올 때의 블렌드 시간(초). */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Camera")
	float CameraBlendTime = 0.5f;

private:
	UFUNCTION(Client, Reliable)
	void ClientStartDialogue(const FDataTableRowHandle& StartRow, AActor* Target);

	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	/** 행이 없거나 대사가 비어 있으면 실패한다. */
	bool EnterRow(FName RowName);

	/** 재임포트가 행 버퍼를 통째로 갈아끼우면 캐시한 포인터가 해제된 메모리를 가리키므로 매번 테이블에서 되찾는다. */
	const FWxDialogueTableRow* FindCurrentRow() const;

	void PublishCurrentLine();

	/** 호출부가 자기 사유를 알고 있으므로 bCompleted 는 그 자리에서 넘긴다. */
	void EndDialogue(bool bCompleted);

	void BeginDialogueCamera();

	void EndDialogueCamera();

	/** 지목이 없으면 직전 포즈를 두고, 늦게 도착한 포즈가 새 포즈를 덮지 않게 앞 대사의 스트리밍을 접는다. */
	void ApplyCurrentPose();

	void HandlePoseLoaded();

	void PlayPendingPose();

	/** 카메라는 로컬 어포던스라 로컬 컨트롤러가 아니면 null 을 답해 카메라 경로를 통째로 건너뛴다. */
	APlayerController* GetLocalPlayerController() const;

	/** 세션 동안 State.Dialogue 를 발행해 둔 폰 ASC. 빙의 전이로 접힐 때는 컨트롤러가 이미 새 폰을 가리켜 다시 조회할 수 없으므로 기억한다. */
	TWeakObjectPtr<UAbilitySystemComponent> TaggedAbilitySystem;

	TWeakObjectPtr<AActor> CurrentTarget;

	/** 진행 중인 대화를 연 시작 행. 세션 동안 테이블 객체를 붙잡는 강참조이자 진행 중 노드를 찾을 테이블의 출처다. */
	UPROPERTY()
	FDataTableRowHandle CurrentStartRow;

	/** 현재 노드의 행 이름이자 세션 자체의 상태. 비어 있으면 대화 중이 아니다. */
	FName CurrentRowName;

	TWeakObjectPtr<ACameraActor> DialogueCamera;

	/** 완료 콜백이 인자를 받지 않아 요청 시점에 포즈와 대상을 남기며, 세션이 닫힌 뒤 도착해도 제 대상에 얹히도록 CurrentTarget 과 따로 든다. */
	TSoftObjectPtr<UAnimMontage> PendingPose;
	TWeakObjectPtr<AActor> PendingPoseTarget;

	TSharedPtr<FStreamableHandle> PoseLoadHandle;
};
