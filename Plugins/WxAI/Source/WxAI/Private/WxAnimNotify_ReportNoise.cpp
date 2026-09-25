// Copyright Woogle. All Rights Reserved.

#include "WxAnimNotify_ReportNoise.h"
#if WITH_EDITOR
#include "WxAnimNotifySettings.h"
#endif

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Perception/AISense_Hearing.h"

#if WITH_EDITOR
FLinearColor UWxAnimNotify_ReportNoise::GetEditorColor()
{
	return GetDefault<UWxAnimNotifySettings>()->MiscColor;
}
#endif

void UWxAnimNotify_ReportNoise::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner)
	{
		return;
	}

	// AI Perception 은 서버에서만 동작한다.
	// Loudness 1 고정이라 MaxRange 가 곧 절대 거리(cm)이고, Instigator=소유 액터로 청취자-소음원 팀 소속을 판정한다.
	if (Owner->HasAuthority())
	{
		UAISense_Hearing::ReportNoiseEvent(Owner, Owner->GetActorLocation(), 1.f, Owner, HearingDistance);
	}
}

FString UWxAnimNotify_ReportNoise::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Noise: %scm"), *FString::SanitizeFloat(HearingDistance, 0));
}
