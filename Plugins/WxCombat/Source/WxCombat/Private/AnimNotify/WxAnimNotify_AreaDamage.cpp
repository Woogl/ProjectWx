// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotify_AreaDamage.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "PrimitiveDrawingUtils.h"
#include "TargetingSystem/TargetingPreset.h"
#include "TargetingSystem/TargetingSubsystem.h"
#include "Tasks/TargetingSelectionTask_AOE.h"
#include "Types/TargetingSystemTypes.h"
#include "WxCombatLibrary.h"

void UWxAnimNotify_AreaDamage::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	UTargetingSubsystem* TargetingSubsystem = Owner ? UTargetingSubsystem::Get(Owner->GetWorld()) : nullptr;
	if (!TargetingPreset || !TargetingSubsystem)
	{
		return;
	}

	FTargetingSourceContext SourceContext;
	SourceContext.SourceActor = Owner;
	SourceContext.InstigatorActor = Owner;

	FTargetingRequestHandle RequestHandle = UTargetingSubsystem::MakeTargetRequestHandle(TargetingPreset, SourceContext);
	TargetingSubsystem->ExecuteTargetingRequestWithHandle(RequestHandle);

	// 결과를 복사해 두고 핸들을 먼저 반납한다.
	// 데이터 스토어 참조를 든 채 순회하면 ApplyDamage 안에서 시작된 다른 타겟팅 요청이 스토어를 재할당해 참조가 끊긴다.
	TArray<FHitResult> Results;
	TargetingSubsystem->GetTargetingResults(RequestHandle, Results);
	UTargetingSubsystem::ReleaseTargetRequestHandle(RequestHandle);

	// 아군·시체·무적 제외와 예측 키 처리는 ApplyDamage가 하므로, 여기서는 결과를 그대로 흘려보낸다.
	for (const FHitResult& Result : Results)
	{
		if (AActor* TargetActor = Result.GetActor())
		{
			UWxCombatLibrary::ApplyDamage(Owner, TargetActor, DamageDataRow, Result);
		}
	}
}

FString UWxAnimNotify_AreaDamage::GetNotifyName_Implementation() const
{
	if (DamageDataRow.IsNull())
	{
		return Super::GetNotifyName_Implementation();
	}

	return DamageDataRow.RowName.ToString();
}

#if WITH_EDITOR
void UWxAnimNotify_AreaDamage::DrawInEditor(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* MeshComp, const UAnimSequenceBase* Animation, const FAnimNotifyEvent& NotifyEvent) const
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

	constexpr float VisibleDuration = 1.f;
	const float TimeSinceTrigger = PreviewInstance->GetCurrentTime() - NotifyEvent.GetTriggerTime();
	if (TimeSinceTrigger < 0.f || TimeSinceTrigger > VisibleDuration)
	{
		return;
	}

	FTargetingSourceContext SourceContext;
	SourceContext.SourceActor = MeshComp->GetOwner();
	SourceContext.InstigatorActor = SourceContext.SourceActor;
	SourceContext.SourceLocation = MeshComp->GetComponentLocation();

	FTargetingRequestHandle RequestHandle = UTargetingSubsystem::MakeTargetRequestHandle(TargetingPreset, SourceContext);

	const FLinearColor Color(NotifyColor);
	constexpr float Thickness = 1.f;
	constexpr int32 NumSides = 24;

	for (const TObjectPtr<UTargetingTask>& Task : TargetingPreset->GetTargetingTaskSet()->Tasks)
	{
		const UTargetingSelectionTask_AOE* AOETask = Cast<UTargetingSelectionTask_AOE>(Task);
		if (!AOETask)
		{
			continue;
		}

		const FVector Center = AOETask->GetSourceLocation(RequestHandle) + AOETask->GetSourceOffset(RequestHandle);
		const FQuat Rotation = AOETask->GetSourceRotation(RequestHandle) * AOETask->GetSourceRotationOffset(RequestHandle).Quaternion();
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

		// SourceComponent 는 소스 액터에서 태그로 찾는 형상이라 프리뷰 액터에는 그릴 것이 없다.
		default:
			break;
		}
	}

	UTargetingSubsystem::ReleaseTargetRequestHandle(RequestHandle);
}
#endif
