// Copyright Woogle. All Rights Reserved.

#include "WxDialogueSessionComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "WxDialogueComponent.h"
#include "WxDialogueTableRow.h"
#include "WxGameplayTags.h"

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

const FDataTableRowHandle& UWxDialogueSessionComponent::GetCurrentStartRow() const
{
	return CurrentStartRow;
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
		return;
	}

	// 관찰자에게 노출할 대화 대상. 세션이 실제로 열린 뒤에만 기억한다. 대상 없는 대사(나레이션)면 그대로 비어 있다.
	CurrentTarget = Target;

	// 대화 중 상태를 폰 ASC 에 발행한다. 상호작용 어빌리티가 이 태그로 차단되고 스캐너 표시 게이트(프롬프트·하이라이트)도 함께 닫힌다.
	const AController* Controller = Cast<AController>(GetOwner());
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn))
	{
		ASC->AddLooseGameplayTag(WxGameplayTags::State_Dialogue);
		TaggedAbilitySystem = ASC;
	}

	ActivateDialogueCamera(Target);

	// 구독자(PC)가 여기서 대화 위젯을 푸시하고, 위젯의 뷰모델이 현재 대사를 pull 해 시드한다.
	OnDialogueStarted.Broadcast();
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
	CurrentTarget.Reset();

	// 시작 때 발행한 대화 상태 태그를 같은 ASC 에서 되돌려 프롬프트 표시·상호작용을 복귀시킨다.
	if (UAbilitySystemComponent* ASC = TaggedAbilitySystem.Get())
	{
		ASC->RemoveLooseGameplayTag(WxGameplayTags::State_Dialogue);
	}
	TaggedAbilitySystem.Reset();

	RestoreGameplayCamera();

	OnDialogueEnded.Broadcast();
}

void UWxDialogueSessionComponent::ActivateDialogueCamera(AActor* Target)
{
	// 대상 없는 대사(나레이션)면 비출 것이 없다. 뷰는 플레이어 폰에 머문다.
	APlayerController* PlayerController = GetLocalPlayerController();
	if (!PlayerController || !Target)
	{
		return;
	}

	FVector CameraLocation;
	FRotator CameraRotation;
	if (!ComputeDialogueCameraView(PlayerController, Target, CameraLocation, CameraRotation))
	{
		return;
	}

	// 구도가 매 대화마다 달라지므로 그 자리에 세울 액터를 그때 만든다. 뷰 타겟은 액터 단위라 계산 결과를 담을 그릇이 필요하다.
	// 재진입으로 이전 카메라가 남아 있을 수 있어 먼저 보낸다.
	RestoreGameplayCamera();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = PlayerController;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ACameraActor* SpawnedCamera = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), CameraLocation, CameraRotation, SpawnParams);
	if (!SpawnedCamera)
	{
		return;
	}

	if (UCameraComponent* CameraComponent = SpawnedCamera->GetCameraComponent())
	{
		CameraComponent->SetFieldOfView(CameraFov);
		// ACameraActor 기본값(bConstrainAspectRatio=true, 16:9)이 뷰포트를 마스킹해 레터박스를 만든다. 제약을 꺼 뷰포트 전체를 채운다.
		CameraComponent->SetConstraintAspectRatio(false);
	}

	DialogueCamera = SpawnedCamera;
	PlayerController->SetViewTargetWithBlend(SpawnedCamera, CameraBlendTime, EViewTargetBlendFunction::VTBlend_Cubic);
}

void UWxDialogueSessionComponent::RestoreGameplayCamera()
{
	APlayerController* PlayerController = GetLocalPlayerController();

	// 원래 게임플레이 뷰타겟인 플레이어 폰으로 블렌드 복귀한다.
	// bLockOutgoing=true: 블렌드 시작 시점의 출발 POV 를 고정해, 대화가 끝난 NPC 가 움직이기 시작해도 복귀가 튀지 않는다.
	if (PlayerController)
	{
		if (APawn* Pawn = PlayerController->GetPawn())
		{
			PlayerController->SetViewTargetWithBlend(Pawn, CameraBlendTime, EViewTargetBlendFunction::VTBlend_Cubic, 0.f, true);
		}
	}

	// 즉시 파괴하면 복귀 블렌드의 출발 뷰 타겟이 사라져 화면이 튄다. 블렌드가 끝나고도 남을 만큼의 수명만 걸고 손을 뗀다.
	if (ACameraActor* Camera = DialogueCamera.Get())
	{
		Camera->SetLifeSpan(CameraBlendTime + 1.f);
	}
	DialogueCamera.Reset();
}

bool UWxDialogueSessionComponent::ComputeDialogueCameraView(const APlayerController* PlayerController, const AActor* Target, FVector& OutLocation, FRotator& OutRotation) const
{
	const APawn* Pawn = PlayerController->GetPawn();
	if (!Pawn || !Target)
	{
		return false;
	}

	// 두 사람의 눈높이를 기준선으로 삼는다. 같은 오프셋을 카메라 높이와 응시점 양쪽에 쓰므로 평지에선 피치가 0 이고, 지면 높이가 다르면 그만큼만 기운다.
	const FVector EyeOffset(0.f, 0.f, CameraEyeHeight);
	const FVector PlayerEye = Pawn->GetActorLocation() + EyeOffset;
	const FVector TargetEye = Target->GetActorLocation() + EyeOffset;

	FVector Axis = TargetEye - PlayerEye;
	Axis.Z = 0.f;
	if (!Axis.Normalize())
	{
		// 둘이 수평으로 겹쳐 있으면 세울 축이 없다. 뷰를 폰에 남긴다.
		return false;
	}

	const FVector Pivot = FMath::Lerp(PlayerEye, TargetEye, CameraPivotAlpha);
	const FVector Side = FVector::CrossProduct(FVector::UpVector, Axis);

	// 180도 법칙 — 지금 카메라가 있는 쪽 옆으로 나간다. 반대쪽을 고르면 대화가 열리는 순간 화면이 축을 넘어 좌우로 뒤집힌다.
	const APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager;
	const float SideSign = (CameraManager && FVector::DotProduct(CameraManager->GetCameraLocation() - Pivot, Side) < 0.f) ? -1.f : 1.f;
	FVector DesiredLocation = Pivot + Side * (SideSign * CameraLateralOffset);

	// 좁은 실내에선 옆으로 비껴선 자리가 벽 안쪽일 수 있다. 축 위 기준점에서 그 자리까지 훑어 막히면 앞까지 당긴다.
	// 두 당사자는 무시한다 — 카메라가 뚫어야 할 대상은 지형이지 대화하는 사람이 아니다.
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WxDialogueCamera), /*bTraceComplex*/ false, Pawn);
	QueryParams.AddIgnoredActor(Target);

	FHitResult Hit;
	if (GetWorld()->SweepSingleByChannel(Hit, Pivot, DesiredLocation, FQuat::Identity, ECC_Camera, FCollisionShape::MakeSphere(12.f), QueryParams))
	{
		DesiredLocation = Hit.Location;
	}

	OutLocation = DesiredLocation;
	OutRotation = (TargetEye - DesiredLocation).Rotation();
	return true;
}

APlayerController* UWxDialogueSessionComponent::GetLocalPlayerController() const
{
	APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	return (PlayerController && PlayerController->IsLocalController()) ? PlayerController : nullptr;
}
