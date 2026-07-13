// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "WxContextEffectsLibrary.generated.h"

class USoundBase;
class UNiagaraSystem;

/** 한 (EffectTag, 표면)에서 재생할 코스메틱 애셋. 둘 중 설정된 것만 재생된다. */
USTRUCT(BlueprintType)
struct FWxContextEffectAssets
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Wx")
	TObjectPtr<USoundBase> Sound;

	UPROPERTY(EditAnywhere, Category = "Wx")
	TObjectPtr<UNiagaraSystem> VFX;
};

/** 한 EffectTag 의 표면별 애셋 묶음. 미매핑 표면(또는 트레이스 실패)은 DefaultAssets 사용. */
USTRUCT(BlueprintType)
struct FWxContextEffectSurfaceSet
{
	GENERATED_BODY()

	/** 표면 타입별 애셋. 표면 타입 이름은 Project Settings > Physics > Physical Surface 에서 정의한다 */
	UPROPERTY(EditAnywhere, Category = "Wx")
	TMap<TEnumAsByte<EPhysicalSurface>, FWxContextEffectAssets> SurfaceAssets;

	/** 매핑되지 않은 표면에 사용할 기본 애셋 */
	UPROPERTY(EditAnywhere, Category = "Wx")
	FWxContextEffectAssets DefaultAssets;
};

/**
 * 컨텍스트 이펙트 라이브러리.
 *
 * EffectTag(예: Effect.Footstep) → 표면(EPhysicalSurface)별 사운드/VFX 매핑을 저작하는 DataAsset.
 * 컴포넌트가 (EffectTag, 표면)으로 조회해 코스메틱을 재생한다.
 */
UCLASS(BlueprintType)
class WXGAME_API UWxContextEffectsLibrary : public UDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * EffectTag+표면에 해당하는 애셋을 OutAssets 에 채운다. 표면 미매핑 시 Default 로 폴백한다.
	 * EffectTag 자체가 없거나 사운드·VFX 가 모두 비어 재생할 게 없으면 false.
	 */
	bool ResolveEffect(const FGameplayTag& EffectTag, EPhysicalSurface Surface, FWxContextEffectAssets& OutAssets) const;

protected:
	/** EffectTag → 표면별 애셋 묶음 */
	UPROPERTY(EditAnywhere, Category = "Wx|ContextEffects")
	TMap<FGameplayTag, FWxContextEffectSurfaceSet> Effects;
};
