// Copyright Woogle. All Rights Reserved.

#include "WxDialogueSessionComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "WxDialogueActor.h"
#include "WxDialogueComponent.h"
#include "WxDialogueModule.h"
#include "WxDialogueTableRow.h"
#include "WxGameplayTags.h"

UWxDialogueSessionComponent::UWxDialogueSessionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 주입으로 붙는 동적 컴포넌트라 기본 서브오브젝트의 안정된 이름이 없다 — 원격에서 이 객체를 해소하는 수단이 복제뿐이다.
	SetIsReplicatedByDefault(true);
}

void UWxDialogueSessionComponent::StartDialogue(UWxDialogueComponent* Dialogue)
{
	if (!Dialogue)
	{
		UE_LOG(LogWxDialogue, Warning, TEXT("StartDialogue: 대화 정의 컴포넌트가 없다."));
		return;
	}

	StartDialogueRow(Dialogue->GetStartRow(), Dialogue->GetOwner());
}

void UWxDialogueSessionComponent::StartDialogueRow(const FDataTableRowHandle& StartRow, AActor* Target)
{
	if (!StartRow.DataTable || StartRow.RowName.IsNone())
	{
		// 이 갈래가 조용하면 "F 를 눌러도 아무 일이 없다"만 남는다.
		UE_LOG(LogWxDialogue, Warning, TEXT("StartDialogueRow: 시작 행이 지정되지 않음(테이블 %s / 행 %s, 대상 %s)."),
			*GetNameSafe(StartRow.DataTable), *StartRow.RowName.ToString(), *GetNameSafe(Target));
		return;
	}

	ClientStartDialogue(StartRow, Target);
}

void UWxDialogueSessionComponent::Advance()
{
	if (!HasActiveDialogue())
	{
		return;
	}

	const FWxDialogueTableRow* Row = FindCurrentRow();
	if (!Row)
	{
		// 세션 도중 테이블이 갈린 경우다(에디터 재임포트). 이어갈 곳이 없으니 접는다 — 남겨 두면 진행도 종료도 없는 세션이 굳는다.
		EndDialogue();
		return;
	}

	const FName NextRowName = Row->NextRow;
	if (NextRowName.IsNone())
	{
		EndDialogue();
		return;
	}

	if (!EnterRow(NextRowName))
	{
		// 다음 행이 지정돼 있는데 해석에 실패한 것은 정상 종료가 아니다. 구분해 찍지 않으면 오타가 "대화가 이유 없이 끊김"으로만 보인다.
		UE_LOG(LogWxDialogue, Warning, TEXT("Advance: 다음 행을 해석하지 못해 대화를 종료한다(테이블 %s / 행 %s → %s)."),
			*GetNameSafe(CurrentStartRow.DataTable), *CurrentRowName.ToString(), *NextRowName.ToString());
		EndDialogue();
		return;
	}

	PublishCurrentLine();
	ApplyCurrentPose();
}

bool UWxDialogueSessionComponent::HasActiveDialogue() const
{
	return !CurrentRowName.IsNone();
}

AActor* UWxDialogueSessionComponent::GetCurrentDialogueTarget() const
{
	return CurrentTarget.Get();
}

FDataTableRowHandle UWxDialogueSessionComponent::GetCurrentRowHandle() const
{
	FDataTableRowHandle Handle;
	Handle.DataTable = CurrentStartRow.DataTable;
	Handle.RowName = CurrentRowName;

	return Handle;
}

FText UWxDialogueSessionComponent::GetCurrentSpeaker() const
{
	const FWxDialogueTableRow* Row = FindCurrentRow();
	return Row ? Row->Speaker : FText::GetEmpty();
}

FText UWxDialogueSessionComponent::GetCurrentLine() const
{
	const FWxDialogueTableRow* Row = FindCurrentRow();
	return Row ? Row->Line : FText::GetEmpty();
}

void UWxDialogueSessionComponent::ClientStartDialogue_Implementation(const FDataTableRowHandle& StartRow, AActor* Target)
{
	// 겹쳐 열리는 경로가 실재한다(퀘스트 트리의 Play Dialogue 는 상호작용 차단 태그의 게이트를 거치지 않는다).
	// 앞 세션을 그대로 덮으면 그쪽 태그·카메라를 되돌릴 주체가 사라지므로, 새 세션을 열기 전에 접는다.
	if (HasActiveDialogue())
	{
		EndDialogue();
	}

	CurrentStartRow = StartRow;
	if (!EnterRow(StartRow.RowName))
	{
		// 앞서 대입한 테이블도 되돌린다 — 행 없이 테이블만 남으면 GetCurrentRowHandle() 이 반쪽짜리 핸들을 답한다.
		CurrentStartRow = FDataTableRowHandle();
		CurrentRowName = NAME_None;
		return;
	}

	// 세션이 실제로 열린 뒤에만 기억한다.
	CurrentTarget = Target;

	// 대화 중 상태를 폰 ASC 에 발행한다. 상호작용 어빌리티가 이 태그로 차단되고, 스캐너 표시 게이트(프롬프트·하이라이트)와 대화 창이 함께 이 태그를 따른다.
	// 대화 창을 여는 관찰자는 여기서 현재 대사를 pull 해 시드하므로, 세션이 다 채워진 뒤인 이 자리에서 올린다.
	const AController* Controller = GetController<AController>();
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn))
	{
		// 이 태그를 loose 로 쓰는 곳은 이 컴포넌트뿐이고, 소비자(UI 매니저)는 0↔비0 전이만 듣기 때문에 카운트가 1 이라도 남으면 대화 창이 영영 닫히지 않는다.
		ASC->SetLooseGameplayTagCount(WxGameplayTags::State_Dialogue, 1);
		TaggedAbilitySystem = ASC;
	}

	BeginDialogueCamera();
	ApplyCurrentPose();
}

bool UWxDialogueSessionComponent::EnterRow(FName RowName)
{
	const UDataTable* Table = CurrentStartRow.DataTable;
	const FWxDialogueTableRow* Row = Table ? Table->FindRow<FWxDialogueTableRow>(RowName, TEXT("WxDialogueSession")) : nullptr;
	if (!Row)
	{
		UE_LOG(LogWxDialogue, Warning, TEXT("EnterRow: 행을 찾지 못했다(테이블 %s / 행 %s). 가리키는 이름이 틀렸거나 행이 지워졌다."),
			*GetNameSafe(Table), *RowName.ToString());
		return false;
	}

	if (Row->Line.IsEmpty())
	{
		// FindRow 의 ContextString 경고는 행이 아예 없을 때만 뜬다 — 대사가 빈 행은 여기서만 드러난다.
		UE_LOG(LogWxDialogue, Warning, TEXT("EnterRow: 대사가 비어 있다(테이블 %s / 행 %s). 종료는 NextRow=None 으로 표시한다."),
			*GetNameSafe(Table), *RowName.ToString());
		return false;
	}

	CurrentRowName = RowName;

	return true;
}

const FWxDialogueTableRow* UWxDialogueSessionComponent::FindCurrentRow() const
{
	// 이름이 비어 있는 것은 세션이 닫혔다는 뜻이라 오류가 아니다 — FindRow 는 NAME_None 에도 경고를 찍으므로 그 앞에서 가른다.
	const UDataTable* Table = CurrentStartRow.DataTable;
	if (!Table || CurrentRowName.IsNone())
	{
		return nullptr;
	}

	return Table->FindRow<FWxDialogueTableRow>(CurrentRowName, TEXT("WxDialogueSession"));
}

void UWxDialogueSessionComponent::PublishCurrentLine()
{
	OnLineChanged.Broadcast(GetCurrentSpeaker(), GetCurrentLine());
}

void UWxDialogueSessionComponent::EndDialogue()
{
	CurrentStartRow = FDataTableRowHandle();
	CurrentRowName = NAME_None;
	CurrentTarget.Reset();

	// 발행과 대칭으로 절대값 0 을 지정한다 — 감산이면 겹침 이력에 따라 잔량이 남을 수 있다.
	if (UAbilitySystemComponent* ASC = TaggedAbilitySystem.Get())
	{
		ASC->SetLooseGameplayTagCount(WxGameplayTags::State_Dialogue, 0);
	}
	TaggedAbilitySystem.Reset();

	// 포즈는 거두지 않고, 진행 중인 스트리밍도 접지 않는다 — 마지막 대사의 자세가 늦게 도착했을 뿐이고, 요청이 대상을 따로 들고 있어 세션 없이도 얹힌다.
	EndDialogueCamera();

	OnDialogueEnded.Broadcast();
	OnDialogueEnded.Clear();
}

void UWxDialogueSessionComponent::BeginDialogueCamera()
{
	APlayerController* PlayerController = GetLocalPlayerController();
	const APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!Pawn || !CurrentTarget.IsValid())
	{
		return;
	}

	// 한쪽만 겨누면 그쪽이 화면 정중앙을 차지하고 다른 쪽이 밀려나 구도가 쏠린다.
	const FVector PawnLocation = Pawn->GetActorLocation();
	const FVector TargetLocation = CurrentTarget->GetActorLocation();
	const FVector AimLocation = (PawnLocation + TargetLocation) * 0.5f + FVector(0.f, 0.f, CameraHeightOffset);

	// 두 사람을 잇는 선 위에 서면 앞사람이 뒷사람을 가리고 뒤통수만 보인다.
	const FVector TalkAxis = (TargetLocation - PawnLocation).GetSafeNormal2D();

	// 어느 쪽으로 비껴설지는 지금 게임플레이 카메라가 서 있는 쪽을 따른다. 반대편으로 넘어가면 좌우가 뒤집혀 컷이 튀고, 그 쪽은 스프링암이 이미 시야를 확보해 둔 방향이기도 하다.
	const FVector AxisRight = FVector::CrossProduct(FVector::UpVector, TalkAxis);
	const FVector ViewLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
	const float Side = (FVector::DotProduct(AxisRight, ViewLocation - AimLocation) >= 0.f) ? 1.f : -1.f;

	// 시선이 수평 방향이라 회전이 그대로 카메라 회전이 된다.
	const FVector ViewDirection = TalkAxis.RotateAngleAxis(-Side * CameraOffAxisAngle, FVector::UpVector);
	const FTransform CameraTransform(ViewDirection.Rotation(), AimLocation - ViewDirection * CameraDistance);

	FActorSpawnParameters SpawnParams;
	// 컨트롤러가 사라지면 카메라도 함께 정리된다.
	SpawnParams.Owner = PlayerController;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ACameraActor* CameraActor = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), CameraTransform, SpawnParams);
	if (!CameraActor)
	{
		return;
	}

	UCameraComponent* CameraComponent = CameraActor->GetCameraComponent();
	CameraComponent->SetFieldOfView(CameraFieldOfView);
	// ACameraActor 기본값(bConstrainAspectRatio=true, 16:9)이 뷰포트를 마스킹해 레터박스를 만든다.
	CameraComponent->SetConstraintAspectRatio(false);

	DialogueCamera = CameraActor;

	// FOV 는 뷰 타겟 블렌드가 함께 보간하므로 따로 손대지 않는다.
	PlayerController->SetViewTargetWithBlend(CameraActor, CameraBlendTime, EViewTargetBlendFunction::VTBlend_Cubic);
}

void UWxDialogueSessionComponent::EndDialogueCamera()
{
	AActor* CameraActor = DialogueCamera.Get();
	if (!CameraActor)
	{
		return;
	}
	DialogueCamera.Reset();

	// bLockOutgoing=true: 블렌드 시작 시점의 출발 POV 를 고정한다. 복귀 도중 대화 카메라가 정리돼도 화면이 튀지 않는다.
	if (APlayerController* PlayerController = GetLocalPlayerController())
	{
		PlayerController->SetViewTargetWithBlend(PlayerController->GetPawn(), CameraBlendTime, EViewTargetBlendFunction::VTBlend_Cubic, 0.f, true);
	}

	// 블렌드가 끝날 때까지는 남아 있어야 하므로 즉시 파괴하지 않고 수명만 준다.
	CameraActor->SetLifeSpan(CameraBlendTime + 1.f);
}

void UWxDialogueSessionComponent::ApplyCurrentPose()
{
	// 한 자세로 여러 대사를 이어가는 것이 기본값이라 매 행에 같은 몽타주를 반복 기입시키지 않는다.
	// 앞 대사가 띄운 스트리밍도 그대로 둔다. 그것이 곧 이어갈 "직전 포즈"다.
	const FWxDialogueTableRow* Row = FindCurrentRow();
	if (!Row || Row->TargetPose.IsNull())
	{
		return;
	}

	// CancelHandle 은 지연 콜백 큐에 들어간 완료 델리게이트까지 취소한다.
	if (PoseLoadHandle.IsValid())
	{
		PoseLoadHandle->CancelHandle();
		PoseLoadHandle.Reset();
	}

	PendingPose = Row->TargetPose;
	PendingPoseTarget = CurrentTarget;

	// 이미 로드돼 있으면(직전 대사와 같은 포즈를 다시 지목하는 등) 스트리밍을 거치지 않는다.
	if (PendingPose.Get())
	{
		PlayPendingPose();
		return;
	}

	PoseLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		PendingPose.ToSoftObjectPath(),
		FStreamableDelegate::CreateUObject(this, &UWxDialogueSessionComponent::HandlePoseLoaded));
}

void UWxDialogueSessionComponent::HandlePoseLoaded()
{
	PlayPendingPose();

	// 재생을 시작한 뒤에 놓는다 — 그 전까지 몽타주를 붙잡는 것은 이 핸들뿐이고, 재생이 걸려야 애님 인스턴스가 수명을 넘겨받는다.
	PoseLoadHandle.Reset();
}

void UWxDialogueSessionComponent::PlayPendingPose()
{
	UAnimMontage* Pose = PendingPose.Get();
	if (!Pose)
	{
		UE_LOG(LogWxDialogue, Warning, TEXT("PlayPendingPose: 포즈 몽타주를 로드하지 못했다(포즈 %s)."), *PendingPose.ToString());
		return;
	}

	// 포즈 대상은 대화 액터뿐이다 — 상호작용은 대화 정의의 오너로, 퀘스트 트리는 대상 없이 들어온다.
	const AWxDialogueActor* Target = Cast<AWxDialogueActor>(PendingPoseTarget.Get());
	const USkeletalMeshComponent* Mesh = Target ? Target->GetPoseMesh() : nullptr;
	UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		// 대상이 애님 BP 없이(단일 노드 모드 등) 도는 경우다. 조용히 넘기면 "포즈를 지정했는데 아무 일도 없다"만 남는다.
		UE_LOG(LogWxDialogue, Warning, TEXT("PlayPendingPose: 대상에 애님 인스턴스가 없어 포즈를 얹을 수 없다(대상 %s / 포즈 %s)."),
			*GetNameSafe(Target), *GetNameSafe(Pose));
		return;
	}

	// 같은 슬롯이라 직전 포즈는 엔진이 알아서 블렌드 아웃시킨다. 블렌드 시간도 루프 여부도 전부 몽타주 애셋의 값이다.
	AnimInstance->Montage_Play(Pose);
}

APlayerController* UWxDialogueSessionComponent::GetLocalPlayerController() const
{
	APlayerController* PlayerController = GetController<APlayerController>();
	return (PlayerController && PlayerController->IsLocalController()) ? PlayerController : nullptr;
}
