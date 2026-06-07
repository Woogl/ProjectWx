// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "WxEffectZone.generated.h"

class UGameplayEffect;
class USceneComponent;

/**
 * 접촉 시 GameplayEffect를 적용하는 베이스 액터.
 *
 * 콜리전·외형 컴포넌트는 Blueprint에서 구성한다. NotifyActorBeginOverlap에서 ApplyEffect를 호출하므로
 * GenerateOverlapEvents가 켜진 모든 컴포넌트의 진입이 자동으로 처리된다.
 */
UCLASS(Abstract, Blueprintable)
class WXCOMBAT_API AWxEffectZone : public AActor
{
	GENERATED_BODY()

public:
	AWxEffectZone();

	/** 접촉한 대상에게 적용할 GameplayEffect */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wx")
	TSubclassOf<UGameplayEffect> EffectClass;

	/** Spec에 세팅할 SetByCaller 값 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "SetByCaller"))
	TMap<FGameplayTag, float> SetByCallers;

	/** Spec에 추가할 DynamicAssetTags. 예: Event.HitReact.Knockback, Damage.Unblockable */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTagContainer EffectAssetTags;

	/** true 면 같은 액터에게는 한 번만 GE 를 적용한다. 일격 트랩, 통과형 위해(레이저 벽 등) 에서 사용. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wx")
	bool bApplyOncePerTarget = false;

	void ApplyEffect(AActor* Target);

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx")
	TObjectPtr<USceneComponent> SceneRoot;

private:
	/** bApplyOncePerTarget 일 때 이미 GE 가 적용된 대상 추적용. */
	TSet<TWeakObjectPtr<AActor>> AppliedTargets;
};
