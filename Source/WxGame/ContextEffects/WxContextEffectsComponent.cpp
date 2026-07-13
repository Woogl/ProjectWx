// Copyright Woogle. All Rights Reserved.

#include "ContextEffects/WxContextEffectsComponent.h"

#include "ContextEffects/WxContextEffectsLibrary.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Sound/SoundBase.h"

UWxContextEffectsComponent::UWxContextEffectsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWxContextEffectsComponent::TriggerEffect(const FGameplayTag& EffectTag, const FVector& Location) const
{
	// 순수 코스메틱이므로 데디케이티드 서버에선 재생하지 않는다.
	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_DedicatedServer || !EffectTag.IsValid())
	{
		return;
	}

	// 위에서 발밑 아래까지 트레이스해 바닥 표면과 스폰 위치를 얻는다.
	const FVector TraceStart = Location + FVector(0.f, 0.f, 50.f);
	const FVector TraceEnd = Location - FVector(0.f, 0.f, 200.f);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WxContextEffect), true, GetOwner());
	QueryParams.bReturnPhysicalMaterial = true;

	EPhysicalSurface Surface = SurfaceType_Default;
	FVector SpawnLocation = Location;

	FHitResult Hit;
	if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		Surface = UGameplayStatics::GetSurfaceType(Hit);
		SpawnLocation = Hit.ImpactPoint;
	}

	// 라이브러리를 순서대로 조회해 처음 매칭되는 애셋을 재생한다.
	for (const UWxContextEffectsLibrary* Library : Libraries)
	{
		FWxContextEffectAssets Assets;
		if (Library && Library->ResolveEffect(EffectTag, Surface, Assets))
		{
			if (Assets.Sound)
			{
				UGameplayStatics::PlaySoundAtLocation(World, Assets.Sound, SpawnLocation);
			}
			if (Assets.VFX)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, Assets.VFX, SpawnLocation);
			}
			return;
		}
	}
}
