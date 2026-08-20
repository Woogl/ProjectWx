// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotifyState_CameraMove.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"

#if WITH_EDITOR
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimTypes.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#endif

void UWxAnimNotifyState_CameraMove::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner)
	{
		return;
	}

	// 몽타주가 모든 클라에 복제돼 이 노티가 남의 클라에서도 실행되므로, 로컬이 아닌 플레이어의 몽타주는 여기서 거른다.
	// 적·AI 몽타주는 걸러지지 않고 각 클라의 로컬 플레이어 뷰에 적용된다.
	const APawn* OwnerPawn = Cast<APawn>(Owner);
	if (OwnerPawn && OwnerPawn->IsPlayerControlled() && !OwnerPawn->IsLocallyControlled())
	{
		return;
	}

	APlayerController* PC = GEngine->GetFirstLocalPlayerController(Owner->GetWorld());
	if (!PC)
	{
		return;
	}

	// 배치 기준은 액터가 아니라 몽타주가 재생되는 스켈레탈 메시다.
	// 액터 프레임을 쓰면 ACharacter의 메시 -90도 보정 때문에 보정이 없는 애님 프리뷰와 방향이 어긋난다.
	const FTransform CameraTransform = FTransform(CameraRelativeRotation, CameraRelativeLocation) * MeshComp->GetComponentTransform();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Owner;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ACameraActor* CameraActor = Owner->GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), CameraTransform, SpawnParams);
	if (!CameraActor)
	{
		return;
	}

	if (UCameraComponent* CameraComponent = CameraActor->GetCameraComponent())
	{
		CameraComponent->SetFieldOfView(FieldOfView);
		// ACameraActor 기본값(bConstrainAspectRatio=true, 16:9)이 뷰포트를 마스킹해 레터박스를 만든다.
		CameraComponent->SetConstraintAspectRatio(false);
	}

	if (bAttachToOwner)
	{
		CameraActor->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
	}

	// NotifyEnd가 누락되는 예외 상황(몽타주 급종료 등)에서도 임시 카메라가 남지 않도록 안전망을 건다.
	CameraActor->SetLifeSpan(TotalDuration + BlendOutTime + 1.0f);

	PC->SetViewTargetWithBlend(CameraActor, BlendInTime, EViewTargetBlendFunction::VTBlend_Cubic);
}

void UWxAnimNotifyState_CameraMove::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

#if WITH_EDITOR
	// 애님 에디터 프리뷰에는 PlayerController·뷰타겟이 없어 실제 뷰 전환을 재현할 수 없다.
	if (!MeshComp)
	{
		return;
	}

	UWorld* World = MeshComp->GetWorld();
	if (!World || World->WorldType != EWorldType::EditorPreview)
	{
		return;
	}

	if (!bDrawEditorPreview)
	{
		if (PreviewCameraMeshComponent)
		{
			PreviewCameraMeshComponent->SetVisibility(false);
		}
		return;
	}

	// 이전 에디터 세션 등 다른 월드에 묶인 낡은 컴포넌트는 버리고 현재 프리뷰 월드에 새로 만든다.
	if (PreviewCameraMeshComponent && PreviewCameraMeshComponent->GetWorld() != World)
	{
		PreviewCameraMeshComponent = nullptr;
	}

	// 에디터 프리뷰 전용이라 로드 비용은 신경 쓰지 않고 생성 시점에 동기 로드한다.
	if (!PreviewCameraMeshComponent)
	{
		UStaticMesh* CameraMesh = PreviewCameraMesh.LoadSynchronous();
		if (!CameraMesh)
		{
			return;
		}

		UStaticMeshComponent* CameraMeshComponent = NewObject<UStaticMeshComponent>(World);
		CameraMeshComponent->SetStaticMesh(CameraMesh);
		CameraMeshComponent->SetMobility(EComponentMobility::Movable);
		CameraMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CameraMeshComponent->SetCastShadow(false);
		CameraMeshComponent->RegisterComponentWithWorld(World);
		PreviewCameraMeshComponent = CameraMeshComponent;
	}

	PreviewCameraMeshComponent->SetVisibility(true);

	// 이 기준 트랜스폼은 정지 중 프로퍼티 편집을 즉시 반영할 때도 재사용한다.
	if (bAttachToOwner || !PreviewCameraTransform.IsSet())
	{
		PreviewCameraTransform = MeshComp->GetComponentTransform();
	}
	const FTransform CameraTransform = FTransform(CameraRelativeRotation, CameraRelativeLocation) * PreviewCameraTransform.GetValue();
	PreviewCameraMeshComponent->SetWorldTransform(CameraTransform);
#endif
}

void UWxAnimNotifyState_CameraMove::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

#if WITH_EDITOR
	// 구간 안에서 일시정지해도 이 프리뷰 경로는 NotifyEnd를 호출한다.
	// 그래서 무조건 숨기지 않고, 플레이헤드가 실제로 구간 밖일 때만 숨겨 멈춘 상태에서도 계속 보이게 한다.
	const FAnimNotifyEvent* NotifyEvent = EventReference.GetNotify();
	const UAnimSingleNodeInstance* PreviewInstance = MeshComp ? Cast<UAnimSingleNodeInstance>(MeshComp->GetAnimInstance()) : nullptr;
	const bool bStillInsideRegion = NotifyEvent && PreviewInstance
		&& (PreviewInstance->GetCurrentTime() > NotifyEvent->GetTriggerTime())
		&& (PreviewInstance->GetCurrentTime() <= NotifyEvent->GetEndTriggerTime());

	if (!bStillInsideRegion)
	{
		if (PreviewCameraMeshComponent)
		{
			PreviewCameraMeshComponent->SetVisibility(false);
		}
		PreviewCameraTransform.Reset();
	}
#endif

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner)
	{
		return;
	}

	// NotifyBegin과 같은 게이트다.
	const APawn* OwnerPawn = Cast<APawn>(Owner);
	if (OwnerPawn && OwnerPawn->IsPlayerControlled() && !OwnerPawn->IsLocallyControlled())
	{
		return;
	}

	APlayerController* PC = GEngine->GetFirstLocalPlayerController(Owner->GetWorld());
	if (!PC)
	{
		return;
	}

	// 임시 카메라 액터는 lifespan으로 스스로 정리된다.
	// bLockOutgoing은 블렌드 시작 시점의 출발 POV를 고정한다 — 부착 카메라가 캐릭터를 따라 움직이거나 파괴돼도 출발점이 튀지 않는다.
	PC->SetViewTargetWithBlend(PC->GetPawn(), BlendOutTime, EViewTargetBlendFunction::VTBlend_Cubic, 0.0f, true);
}

#if WITH_EDITOR
void UWxAnimNotifyState_CameraMove::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PreviewCameraMeshComponent && PreviewCameraTransform.IsSet())
	{
		const FTransform CameraTransform = FTransform(CameraRelativeRotation, CameraRelativeLocation) * PreviewCameraTransform.GetValue();
		PreviewCameraMeshComponent->SetWorldTransform(CameraTransform);
	}
}
#endif
