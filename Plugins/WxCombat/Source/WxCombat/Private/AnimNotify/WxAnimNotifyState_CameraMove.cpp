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

	// 플레이어가 재생한 몽타주는 그 플레이어 본인의 화면에만 적용한다.
	// 몽타주는 모든 클라에 리플리케이트돼 이 노티가 남의 클라에서도 실행되므로, 로컬이 아닌 플레이어의 몽타주는 여기서 걸러 그 사람 피니셔가 남의 카메라를 흔들지 않게 한다.
	// 적/AI(비플레이어) 몽타주는 걸러지지 않고 각 클라의 로컬 플레이어 뷰에 적용된다.
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

	// 배치 기준은 몽타주가 재생되는 스켈레탈 메시(실제 렌더되는 캐릭터 몸체).
	// 애님 프리뷰 기즈모도 같은 기준을 써야 프리뷰와 인게임 배치가 일치한다.
	// 액터 프레임을 쓰면 ACharacter의 메시 -90도 보정 때문에 프리뷰(보정 없음)와 방향이 어긋난다.
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
		// 제약을 꺼 뷰포트 전체를 채운다.
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
	// 애님 에디터 프리뷰에는 PlayerController/뷰타겟이 없어 실제 뷰 전환을 재현할 수 없다.
	// 대신 프리뷰 월드에 엔진 카메라 모델 메시를 놓아, 카메라가 놓일 위치·각도를 실제 형상으로 보여준다.
	if (!MeshComp)
	{
		return;
	}

	UWorld* World = MeshComp->GetWorld();
	if (!World || World->WorldType != EWorldType::EditorPreview)
	{
		return;
	}

	// 프리뷰 토글이 꺼지면 카메라 모델을 숨긴다(파괴하지 않고 비저빌리티만 끈다).
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

	// 프리뷰 카메라 모델 메시(PreviewCameraMesh, 기본 MatineeCam_SM)를 프리뷰 월드에 한 번만 스폰해 그대로 렌더한다(디버그 드로잉 아님).
	// 컴포넌트는 파괴하지 않고 재사용하며, 구간 진입/이탈은 비저빌리티로만 토글한다(생성·파괴 반복 회피).
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

	// 구간 안(NotifyTick 실행 중)에는 보이게 한다.
	PreviewCameraMeshComponent->SetVisibility(true);

	// 부착 모드(bAttachToOwner=true)는 실시간 몸체 트랜스폼을 따라가고(매 틱 갱신), 고정 모드는 구간 첫 진입 시점의 몸체 트랜스폼에 고정한다(최초 1회만 기록).
	// 이 기준 트랜스폼은 정지 중 프로퍼티 편집 즉시 반영(PostEditChangeProperty)에도 재사용한다.
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
	// 그래서 무조건 숨기지 않고, 현재 플레이헤드가 실제로 구간 밖일 때만 숨긴다(구간 안이면 유지 → 멈춰도 계속 보인다).
	// 런타임 PIE에선 컴포넌트가 없어 no-op.
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

	// NotifyBegin과 같은 게이트: 남의 플레이어 몽타주는 여기서도 걸러 내 카메라를 건드리지 않는다.
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

	// 로컬 플레이어가 조종하는 폰(원래 게임플레이 뷰타겟)으로 블렌드 복귀한다.
	// 적 패턴이 끝나도 적이 아니라 내 캐릭터 시점으로 돌아온다.
	// 임시 카메라 액터는 lifespan으로 스스로 정리된다.
	// bLockOutgoing=true: 블렌드 시작 시점의 출발 POV를 고정한다.
	// 부착(bAttachToOwner) 카메라가 피니셔 종료 후 캐릭터를 따라 움직이거나 lifespan으로 파괴돼도 출발점이 튀지 않고 매끄럽게 복귀한다.
	PC->SetViewTargetWithBlend(PC->GetPawn(), BlendOutTime, EViewTargetBlendFunction::VTBlend_Cubic, 0.0f, true);
}

#if WITH_EDITOR
void UWxAnimNotifyState_CameraMove::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// 프리뷰가 떠 있는 동안(구간 안에서 정지 등) 오프셋·회전을 편집하면, 다음 NotifyTick을 기다리지 않고 마지막 틱의 기준 트랜스폼으로 즉시 재배치해 정지 상태에서도 편집이 바로 보이게 한다.
	if (PreviewCameraMeshComponent && PreviewCameraTransform.IsSet())
	{
		const FTransform CameraTransform = FTransform(CameraRelativeRotation, CameraRelativeLocation) * PreviewCameraTransform.GetValue();
		PreviewCameraMeshComponent->SetWorldTransform(CameraTransform);
	}
}
#endif
