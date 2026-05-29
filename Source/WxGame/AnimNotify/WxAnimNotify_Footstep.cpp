// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotify_Footstep.h"

#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Hearing.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Sound/SoundBase.h"

USoundBase* UWxFootstepSoundSet::GetSound(EPhysicalSurface Surface) const
{
	if (const TObjectPtr<USoundBase>* Found = SurfaceSounds.Find(Surface))
	{
		if (*Found)
		{
			return *Found;
		}
	}

	return DefaultSound;
}

void UWxAnimNotify_Footstep::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	const FVector FootLocation = Owner->GetActorLocation();

	// 소음 보고: AI Perception 은 서버에서 동작하므로 서버에서만 보고한다.
	// Loudness 는 1 로 고정하고 MaxRange 에 절대 거리(cm)를 넘겨 거리를 직접 지정한다.
	// Instigator 로 소유 액터를 넘겨 청취자-소음원 간 팀 소속(적/아군) 판정이 가능하게 한다.
	if (Owner->HasAuthority())
	{
		UAISense_Hearing::ReportNoiseEvent(Owner, FootLocation, 1.f, Owner, HearingDistance);
	}

	// 사운드: 표면별로 다른 소리를 각 머신에서 로컬 재생(데디케이티드 서버는 오디오 없음).
	const UWorld* World = Owner->GetWorld();
	if (SoundSet && World && World->GetNetMode() != NM_DedicatedServer)
	{
		PlaySurfaceSound(MeshComp, FootLocation);
	}
}

void UWxAnimNotify_Footstep::PlaySurfaceSound(USkeletalMeshComponent* MeshComp, const FVector& FootLocation) const
{
	UWorld* World = MeshComp->GetWorld();
	if (!World)
	{
		return;
	}
	
	// 위에서 시작해 발밑 아래까지 트레이스. PhysicalMaterial 을 받아 표면 타입을 얻는다.
	const FVector TraceStart = FootLocation + FVector(0.f, 0.f, 50.f);
	const FVector TraceEnd = FootLocation - FVector(0.f, 0.f, 200.f);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WxFootstep), true, MeshComp->GetOwner());
	QueryParams.bReturnPhysicalMaterial = true;

	EPhysicalSurface Surface = SurfaceType_Default;
	FVector SoundLocation = FootLocation;

	FHitResult Hit;
	if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		Surface = UGameplayStatics::GetSurfaceType(Hit);
		SoundLocation = Hit.ImpactPoint;
	}

	if (USoundBase* Sound = SoundSet->GetSound(Surface))
	{
		UGameplayStatics::PlaySoundAtLocation(World, Sound, SoundLocation);
	}
}
