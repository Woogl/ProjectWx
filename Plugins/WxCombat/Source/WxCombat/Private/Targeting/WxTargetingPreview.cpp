// Copyright Woogle. All Rights Reserved.

#include "Targeting/WxTargetingPreview.h"

#if WITH_EDITOR

#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimTypes.h"
#include "Components/SkeletalMeshComponent.h"
#include "PrimitiveDrawingUtils.h"
#include "TargetingSystem/TargetingPreset.h"
#include "TargetingSystem/TargetingSubsystem.h"
#include "Tasks/TargetingSelectionTask_AOE.h"
#include "Types/TargetingSystemTypes.h"

void WxTargetingPreview::DrawDebugTargetingPreset(FPrimitiveDrawInterface* PDI, const USkeletalMeshComponent* MeshComp, const FAnimNotifyEvent& NotifyEvent, const UTargetingPreset* TargetingPreset, const FLinearColor& Color)
{
	if (!PDI || !MeshComp || !TargetingPreset)
	{
		return;
	}

	// 뷰포트가 프리뷰 애셋의 모든 노티파이를 매 프레임 그리므로, 표시 구간은 재생 위치로 직접 좁힌다.
	// 몽타주 프리뷰의 현재 시간은 엔진이 몽타주 위치를 그대로 넣은 값이라 노티파이의 절대 시간과 같은 축이다.
	const UAnimSingleNodeInstance* PreviewInstance = MeshComp->GetSingleNodeInstance();
	if (!PreviewInstance)
	{
		return;
	}

	// 단발 노티파이는 구간 길이가 0이라 그냥 두면 한 프레임도 못 보고 지나간다.
	constexpr float MinVisibleDuration = 0.1f;
	const float TriggerTime = NotifyEvent.GetTriggerTime();
	const float EndTime = FMath::Max(NotifyEvent.GetEndTriggerTime(), TriggerTime + MinVisibleDuration);
	const float CurrentTime = PreviewInstance->GetCurrentTime();
	if (CurrentTime < TriggerTime || CurrentTime > EndTime)
	{
		return;
	}

	FTargetingSourceContext SourceContext;
	SourceContext.SourceActor = MeshComp->GetOwner();
	SourceContext.InstigatorActor = SourceContext.SourceActor;
	SourceContext.SourceLocation = MeshComp->GetComponentLocation();

	FTargetingRequestHandle RequestHandle = UTargetingSubsystem::MakeTargetRequestHandle(TargetingPreset, SourceContext);

	// 쿼리는 액터 트랜스폼 기준으로 돈다. ACharacter가 메시를 요 -90도·Z -90으로 앉히므로, 그 보정을 되돌리면 프리뷰 메시에서 액터 프레임이 나온다.
	// 기준을 루트 본으로 잡는다. 프리뷰가 루트 모션을 컴포넌트로 옮기든 본에 남기든, 그래야 볼륨이 몸을 따라간다.
	const FTransform CharacterMeshOffset(FRotator(0.f, -90.f, 0.f), FVector(0.f, 0.f, -90.f));
	const FTransform SourceFrame = CharacterMeshOffset.Inverse() * MeshComp->GetBoneTransform(0);

	constexpr float Thickness = 1.f;
	constexpr int32 NumSides = 24;

	for (const TObjectPtr<UTargetingTask>& Task : TargetingPreset->GetTargetingTaskSet()->Tasks)
	{
		const UTargetingSelectionTask_AOE* AOETask = Cast<UTargetingSelectionTask_AOE>(Task);
		if (!AOETask)
		{
			continue;
		}

		// 게터의 소스 위치·회전은 프리뷰 액터 트랜스폼이라 쓰지 않는다. 저작된 오프셋만 위 프레임에 얹어야 몸 기준 위치가 나온다.
		const FVector Center = SourceFrame.TransformPosition(AOETask->GetSourceOffset(RequestHandle));
		const FQuat Rotation = SourceFrame.GetRotation() * AOETask->GetSourceRotationOffset(RequestHandle).Quaternion();
		const FCollisionShape Shape = AOETask->GetCollisionShape();

		const FVector AxisX = Rotation.GetAxisX();
		const FVector AxisY = Rotation.GetAxisY();
		const FVector AxisZ = Rotation.GetAxisZ();

		switch (AOETask->GetShapeType())
		{
		case ETargetingAOEShape::Box:
			DrawOrientedWireBox(PDI, Center, AxisX, AxisY, AxisZ, Shape.GetExtent(), Color, SDPG_World, Thickness);
			break;

		// 원기둥은 박스 형상으로 저작하되 X가 반지름, Z가 반높이다.
		case ETargetingAOEShape::Cylinder:
			DrawWireCylinder(PDI, Center, AxisX, AxisY, AxisZ, Color, Shape.GetExtent().X, Shape.GetExtent().Z, NumSides, SDPG_World, Thickness);
			break;

		case ETargetingAOEShape::Sphere:
			DrawWireSphereAutoSides(PDI, FTransform(Rotation, Center), Color, Shape.GetSphereRadius(), SDPG_World, Thickness);
			break;

		case ETargetingAOEShape::Capsule:
			DrawWireCapsule(PDI, Center, AxisX, AxisY, AxisZ, Color, Shape.GetCapsuleRadius(), Shape.GetCapsuleHalfHeight(), NumSides, SDPG_World, Thickness);
			break;

		default:
			break;
		}
	}

	UTargetingSubsystem::ReleaseTargetRequestHandle(RequestHandle);
}

#endif
