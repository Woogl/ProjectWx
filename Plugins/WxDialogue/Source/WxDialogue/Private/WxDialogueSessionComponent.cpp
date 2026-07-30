// Copyright Woogle. All Rights Reserved.

#include "WxDialogueSessionComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "WxDialogueComponent.h"
#include "WxDialogueTableRow.h"
#include "WxGameplayTags.h"

UWxDialogueSessionComponent::UWxDialogueSessionComponent()
{
	// 카메라를 조정하는 동안만 돈다. 시작 때 켜고, 대화가 끝나 원래 값으로 돌아오면 틱이 스스로 꺼진다.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UWxDialogueSessionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	USpringArmComponent* CameraBoom = FindCameraBoom();
	if (!CameraBoom)
	{
		// 폰이 사라졌으면 되돌릴 카메라도 없다. 다음 대화가 다시 켠다.
		SetComponentTickEnabled(false);
		return;
	}

	// 목표는 대화가 살아 있는지에서 그대로 나온다 — 끝나면 같은 보간이 원래 값으로 되돌리므로 진행 상태를 따로 들지 않는다.
	const AActor* Target = CurrentTarget.Get();
	const float DesiredArmLength = Target ? RestoreArmLength - CameraZoomDistance : RestoreArmLength;
	const float DesiredShoulderOffset = Target ? CameraShoulderOffset : RestoreShoulderOffset;

	CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, DesiredArmLength, DeltaTime, CameraZoomSpeed);
	CameraBoom->SocketOffset.Y = FMath::FInterpTo(CameraBoom->SocketOffset.Y, DesiredShoulderOffset, DeltaTime, CameraZoomSpeed);

	if (!Target)
	{
		// 되돌리기를 마쳤으면 더 돌 이유가 없다. FInterpTo 는 목표에 닿으면 목표값을 그대로 돌려주므로 여기서 정확히 걸린다.
		if (FMath::IsNearlyEqual(CameraBoom->TargetArmLength, RestoreArmLength) && FMath::IsNearlyEqual(CameraBoom->SocketOffset.Y, RestoreShoulderOffset))
		{
			SetComponentTickEnabled(false);
		}
		return;
	}

	// 대상을 향해 카메라를 돌린다. 대화 중에도 시선 입력이 살아 있어 한 번 맞춘 각은 곧 풀리므로, 락온과 같이 매 틱 붙잡는다.
	APlayerController* PlayerController = GetLocalPlayerController();
	if (!PlayerController)
	{
		return;
	}

	// 겨누는 출발점은 폰이 아니라 카메라가 실제로 서 있는 자리다. 어깨 쪽으로 비껴선 만큼 각이 저절로 보정돼 대상이 화면 중앙에 온다.
	// 돌린 결과가 다시 카메라 자리를 옮기므로 한 번에 맞아떨어지지는 않지만, 매 틱 다시 겨누며 한 자리로 수렴한다.
	// 겨누는 지점은 대상의 루트에서 얼굴 높이로 올린 자리다. 루트는 캡슐 중심(허리)이라 그대로 겨누면 카메라가 아래를 내려본다.
	const FVector CameraLocation = CameraBoom->GetSocketLocation(USpringArmComponent::SocketName);
	const FVector LookAtLocation = Target->GetActorLocation() + FVector(0.f, 0.f, CameraTargetHeightOffset);
	const FRotator LookAtRotation = (LookAtLocation - CameraLocation).Rotation();
	PlayerController->SetControlRotation(FMath::RInterpTo(PlayerController->GetControlRotation(), LookAtRotation, DeltaTime, CameraTurnSpeed));
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

	// 대화 중 상태를 폰 ASC 에 발행한다. 상호작용 어빌리티가 이 태그로 차단되고 스캐너 표시 게이트(프롬프트·하이라이트)도 함께 닫힌다.
	const AController* Controller = Cast<AController>(GetOwner());
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn))
	{
		ASC->AddLooseGameplayTag(WxGameplayTags::State_Dialogue);
		TaggedAbilitySystem = ASC;
	}

	BeginDialogueCamera();

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

	// 시작 때 발행한 대화 상태 태그를 같은 ASC 에서 되돌려 프롬프트 표시·상호작용을 복귀시킨다.
	if (UAbilitySystemComponent* ASC = TaggedAbilitySystem.Get())
	{
		ASC->RemoveLooseGameplayTag(WxGameplayTags::State_Dialogue);
	}
	TaggedAbilitySystem.Reset();

	// 카메라 복귀는 따로 부르지 않는다 — 세션이 비었으므로 돌고 있던 틱이 다음 프레임부터 원래 값을 목표로 삼는다.

	OnDialogueEnded.Broadcast();
}

void UWxDialogueSessionComponent::BeginDialogueCamera()
{
	// 대상 없는 대사(나레이션)면 쳐다볼 것이 없다. 카메라를 평소 그대로 둔다.
	USpringArmComponent* CameraBoom = FindCameraBoom();
	if (!CameraBoom || !CurrentTarget.IsValid())
	{
		return;
	}

	// 되돌릴 기준은 대화가 손대기 전의 값이다. 앞선 대화의 복귀가 아직 진행 중이면 지금 값은 당겨진 상태라, 그때는 처음 잰 값을 그대로 쓴다.
	if (!IsComponentTickEnabled())
	{
		RestoreArmLength = CameraBoom->TargetArmLength;
		RestoreShoulderOffset = CameraBoom->SocketOffset.Y;
	}

	SetComponentTickEnabled(true);
}

APlayerController* UWxDialogueSessionComponent::GetLocalPlayerController() const
{
	APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	return (PlayerController && PlayerController->IsLocalController()) ? PlayerController : nullptr;
}

USpringArmComponent* UWxDialogueSessionComponent::FindCameraBoom() const
{
	// 스프링암은 엔진 타입이라, 플레이어 캐릭터 클래스를 알지 못해도(WxDialogue 는 게임 모듈을 참조하지 않는다) 폰에서 바로 집을 수 있다.
	const APlayerController* PlayerController = GetLocalPlayerController();
	const APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;

	return Pawn ? Pawn->FindComponentByClass<USpringArmComponent>() : nullptr;
}
