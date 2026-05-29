// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "WxAnimNotify_Footstep.generated.h"

class USoundBase;

/**
 * 표면 타입(EPhysicalSurface) 별 발소리 매핑 DataAsset.
 *
 * 바닥 재질에 부여된 PhysicalMaterial 의 SurfaceType 으로 재생할 사운드를 조회한다.
 * 등록되지 않은 표면(또는 트레이스 실패)에는 DefaultSound 를 사용한다.
 */
UCLASS(BlueprintType)
class WXGAME_API UWxFootstepSoundSet : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 표면에 매핑된 사운드를 반환. 없으면 DefaultSound (그것도 없으면 nullptr) */
	USoundBase* GetSound(EPhysicalSurface Surface) const;

protected:
	/** 표면 타입별 발소리. 표면 타입 이름은 Project Settings > Physics > Physical Surface 에서 정의한다 */
	UPROPERTY(EditAnywhere, Category = "Wx|Footstep")
	TMap<TEnumAsByte<EPhysicalSurface>, TObjectPtr<USoundBase>> SurfaceSounds;

	/** 매핑되지 않은 표면 또는 트레이스 실패 시 사용할 기본 발소리 */
	UPROPERTY(EditAnywhere, Category = "Wx|Footstep")
	TObjectPtr<USoundBase> DefaultSound;
};

/**
 * 발소리 AnimNotify.
 *
 * 보행/달리기 애니메이션의 발 접지 프레임에 배치한다. 두 가지를 처리한다.
 * 1) UAISense_Hearing::ReportNoiseEvent 로 소음 이벤트를 발생시켜 주변 AI 청각이 감지하게 한다(서버 전용).
 * 2) 발 아래로 라인 트레이스해 바닥 표면(EPhysicalSurface)을 알아내고, SoundSet 에서 해당 표면의 발소리를 재생한다(로컬).
 *
 * 소음은 서버에서만 보고한다(AI Perception 은 서버에서 동작). 사운드는 데디케이티드 서버를 제외한 각 머신에서 로컬 재생한다.
 * 청취 거리는 HearingDistance(cm)로 직접 지정한다. 단 엔진 구조상 청취 AI 의 HearingRange 를 초과할 수 없어, 실제 청취 거리 = min(HearingDistance, 청취자 HearingRange) 가 된다.
 * 따라서 HearingDistance 를 키우려면 청취 AI 의 HearingRange 도 그만큼 확보해야 한다.
 */
UCLASS(DisplayName = "Wx Footstep")
class WXGAME_API UWxAnimNotify_Footstep : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	/** 발소리가 들리는 거리(cm). 기본 300 = 3m (청취 AI 의 HearingRange 가 상한) */
	UPROPERTY(EditAnywhere, Category = "Wx|Footstep")
	float HearingDistance = 300.f;

	/** 표면별 발소리 매핑. 미설정 시 사운드는 재생하지 않고 소음 이벤트만 발생시킨다 */
	UPROPERTY(EditAnywhere, Category = "Wx|Footstep")
	TObjectPtr<UWxFootstepSoundSet> SoundSet;

private:
	void PlaySurfaceSound(USkeletalMeshComponent* MeshComp, const FVector& FootLocation) const;
};
