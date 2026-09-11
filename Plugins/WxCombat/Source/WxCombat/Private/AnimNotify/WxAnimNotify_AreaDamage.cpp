// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotify_AreaDamage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Targeting/WxTargetingPreview.h"
#include "TargetingSystem/TargetingSubsystem.h"
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
	WxTargetingPreview::DrawDebugTargetingPreset(PDI, MeshComp, NotifyEvent, TargetingPreset, FLinearColor(NotifyColor));
}
#endif
