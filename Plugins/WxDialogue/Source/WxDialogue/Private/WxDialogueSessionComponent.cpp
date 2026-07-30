// Copyright Woogle. All Rights Reserved.

#include "WxDialogueSessionComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "WxDialogueComponent.h"
#include "WxDialogueTableRow.h"
#include "WxGameplayTags.h"

UWxDialogueSessionComponent::UWxDialogueSessionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 진행 상태는 복제하지 않지만, 서버가 쏘는 ClientStartDialogue 가 클라의 이 컴포넌트에 도착하려면 복제 대상이어야 한다.
	// 주입으로 붙는 동적 컴포넌트라 기본 서브오브젝트의 안정된 이름이 없다 — 원격에서 이 객체를 해소하는 수단이 복제뿐이다.
	SetIsReplicatedByDefault(true);
}

void UWxDialogueSessionComponent::StartDialogue(UWxDialogueComponent* Dialogue)
{
	if (!Dialogue)
	{
		return;
	}

	// 정의 컴포넌트는 행과 대상을 꺼내기 위한 껍데기다 — 세션은 행만 안다.
	StartDialogueRow(Dialogue->GetStartRow(), Dialogue->GetOwner());
}

void UWxDialogueSessionComponent::StartDialogueRow(const FDataTableRowHandle& StartRow, AActor* Target)
{
	if (!StartRow.DataTable || StartRow.RowName.IsNone())
	{
		return;
	}

	ClientStartDialogue(StartRow, Target);
}

void UWxDialogueSessionComponent::Advance()
{
	if (!CurrentRow)
	{
		return;
	}

	if (CurrentRow->NextDialogue.IsNone() || !EnterRow(CurrentRow->NextDialogue))
	{
		EndDialogue();
		return;
	}

	PublishCurrentLine();
}

AActor* UWxDialogueSessionComponent::GetCurrentDialogueTarget() const
{
	return CurrentTarget.Get();
}

FDataTableRowHandle UWxDialogueSessionComponent::GetCurrentRowHandle() const
{
	// 테이블은 세션이 붙잡고 있는 그것이고, 행 이름만 진행에 따라 갈아끼운다.
	FDataTableRowHandle Handle;
	Handle.DataTable = CurrentStartRow.DataTable;
	Handle.RowName = CurrentRowName;

	return Handle;
}

FText UWxDialogueSessionComponent::GetCurrentSpeaker() const
{
	return CurrentRow ? CurrentRow->Speaker : FText::GetEmpty();
}

FText UWxDialogueSessionComponent::GetCurrentLine() const
{
	return CurrentRow ? CurrentRow->Line : FText::GetEmpty();
}

void UWxDialogueSessionComponent::ClientStartDialogue_Implementation(const FDataTableRowHandle& StartRow, AActor* Target)
{
	CurrentStartRow = StartRow;
	if (!EnterRow(StartRow.RowName))
	{
		CurrentStartRow = FDataTableRowHandle();
		CurrentRowName = NAME_None;
		return;
	}

	// 관찰자에게 노출할 대화 대상. 세션이 실제로 열린 뒤에만 기억한다. 대상 없는 대사(나레이션)면 그대로 비어 있다.
	CurrentTarget = Target;

	// 대화 중 상태를 폰 ASC 에 발행한다. 상호작용 어빌리티가 이 태그로 차단되고, 스캐너 표시 게이트(프롬프트·하이라이트)와 대화 창이 함께 이 태그를 따른다.
	// 대화 창을 여는 관찰자는 여기서 현재 대사를 pull 해 시드하므로, 세션이 다 채워진 뒤인 이 자리에서 올린다.
	const AController* Controller = GetController<AController>();
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn))
	{
		ASC->AddLooseGameplayTag(WxGameplayTags::State_Dialogue);
		TaggedAbilitySystem = ASC;
	}

	BeginDialogueCamera();
}

bool UWxDialogueSessionComponent::EnterRow(FName RowName)
{
	const UDataTable* Table = CurrentStartRow.DataTable;
	const FWxDialogueTableRow* Row = Table ? Table->FindRow<FWxDialogueTableRow>(RowName, TEXT("WxDialogueSession")) : nullptr;
	if (!Row || Row->Line.IsEmpty())
	{
		return false;
	}

	CurrentRow = Row;
	CurrentRowName = RowName;

	return true;
}

void UWxDialogueSessionComponent::PublishCurrentLine()
{
	OnLineChanged.Broadcast(GetCurrentSpeaker(), GetCurrentLine());
}

void UWxDialogueSessionComponent::EndDialogue()
{
	CurrentStartRow = FDataTableRowHandle();
	CurrentRow = nullptr;
	CurrentRowName = NAME_None;
	CurrentTarget.Reset();

	// 시작 때 발행한 대화 상태 태그를 같은 ASC 에서 되돌려 대화 창·프롬프트 표시·상호작용을 복귀시킨다.
	if (UAbilitySystemComponent* ASC = TaggedAbilitySystem.Get())
	{
		ASC->RemoveLooseGameplayTag(WxGameplayTags::State_Dialogue);
	}
	TaggedAbilitySystem.Reset();

	EndDialogueCamera();
}

void UWxDialogueSessionComponent::BeginDialogueCamera()
{
	// 대상 없는 대사(나레이션)면 잡을 구도가 없다. 카메라를 평소 그대로 둔다.
	APlayerController* PlayerController = GetLocalPlayerController();
	const APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!Pawn || !CurrentTarget.IsValid())
	{
		return;
	}

	// 겨눌 지점은 두 사람의 중간이다. 한쪽만 겨누면 그쪽이 화면 정중앙을 차지하고 다른 쪽이 밀려나 구도가 쏠린다.
	// 높이 오프셋을 더하는 이유는 루트가 각자 캡슐 중심(허리)이기 때문이다. 카메라도 같은 높이에 서므로 시선은 수평이 된다.
	const FVector PawnLocation = Pawn->GetActorLocation();
	const FVector TargetLocation = CurrentTarget->GetActorLocation();
	const FVector AimLocation = (PawnLocation + TargetLocation) * 0.5f + FVector(0.f, 0.f, CameraHeightOffset);

	// 두 사람을 잇는 선에서 비껴선 자리에서 본다. 선 위에 서면 앞사람이 뒷사람을 가리고 뒤통수만 보인다.
	const FVector TalkAxis = (TargetLocation - PawnLocation).GetSafeNormal2D();

	// 어느 쪽으로 비껴설지는 지금 게임플레이 카메라가 서 있는 쪽을 따른다. 반대편으로 넘어가면 좌우가 뒤집혀 컷이 튀고, 그 쪽은 스프링암이 이미 시야를 확보해 둔 방향이기도 하다.
	const FVector AxisRight = FVector::CrossProduct(FVector::UpVector, TalkAxis);
	const FVector ViewLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
	const float Side = (FVector::DotProduct(AxisRight, ViewLocation - AimLocation) >= 0.f) ? 1.f : -1.f;

	// 시선은 대화 축을 그만큼 돌린 방향이고, 카메라는 그 반대로 물러선 자리에 선다. 수평 방향이라 회전이 그대로 카메라 회전이 된다.
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
	// ACameraActor 기본값(bConstrainAspectRatio=true, 16:9)이 뷰포트를 마스킹해 레터박스를 만든다. 제약을 꺼 뷰포트 전체를 채운다.
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

APlayerController* UWxDialogueSessionComponent::GetLocalPlayerController() const
{
	APlayerController* PlayerController = GetController<APlayerController>();
	return (PlayerController && PlayerController->IsLocalController()) ? PlayerController : nullptr;
}
