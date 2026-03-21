// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Blueprint/UserWidget.h"
#include "WxAbilityTask_LockOnTarget.generated.h"

class UWidgetComponent;

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
	static UWxAbilityTask_LockOnTarget* CreateTask(UGameplayAbility* OwningAbility, AActor* InTarget, float InInterpSpeed = 10.f, float InPitchOffset = -15.f, float InMaxDistance = 2000.f, TSubclassOf<UUserWidget> InReticleWidgetClass = nullptr);

	UPROPERTY()
	FWxOnTargetLost OnTargetLost;

protected:
	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	UFUNCTION()
	void HandleTargetDestroyed(AActor* DestroyedActor);

	void HandleTargetDeathTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	void CreateReticleWidget();
	void DestroyReticleWidget();

	TWeakObjectPtr<AActor> Target;
	float InterpSpeed = 8.f;
	float PitchOffset = -15.f;
	float MaxDistanceSquared = 2000.f * 2000.f;
	TSubclassOf<UUserWidget> ReticleWidgetClass;

	UPROPERTY()
	TObjectPtr<UWidgetComponent> ReticleWidgetComponent;
};
