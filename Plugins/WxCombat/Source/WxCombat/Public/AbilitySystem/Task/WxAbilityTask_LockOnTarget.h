// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "WxAbilityTask_LockOnTarget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWxOnTargetLost);

/**
 * 락온 대상을 매 프레임 카메라로 추적하는 AbilityTask.
 * 컨트롤러 회전을 타겟 방향으로 보간하여 카메라가 적을 따라가도록 한다.
 * 타겟이 파괴되거나 무효화되면 OnTargetLost를 브로드캐스트.
 */
UCLASS()
class WXCOMBAT_API UWxAbilityTask_LockOnTarget : public UAbilityTask
{
	GENERATED_BODY()

public:
	static UWxAbilityTask_LockOnTarget* CreateTask(UGameplayAbility* OwningAbility, AActor* InTarget, float InInterpSpeed = 10.f, float InMaxPitchOffset = 20.f);

	UPROPERTY()
	FWxOnTargetLost OnTargetLost;

protected:
	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	UFUNCTION()
	void HandleTargetDestroyed(AActor* DestroyedActor);

	TWeakObjectPtr<AActor> Target;
	float InterpSpeed = 10.f;
	float MaxPitchOffset = 20.f;
};
