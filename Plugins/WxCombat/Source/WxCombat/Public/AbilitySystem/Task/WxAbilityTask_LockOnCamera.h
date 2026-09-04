// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Blueprint/UserWidget.h"
#include "WxAbilityTask_LockOnCamera.generated.h"

class USceneComponent;
class UWidgetComponent;
class UWxLockOnComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWxOnTargetLost);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWxOnRetargetRequested, FVector2D, ScreenDirection);

/** 컨트롤러 회전을 타겟 방향으로 보간하고, 대상이 파괴·무효화되면 OnTargetLost를 쏜다. */
UCLASS()
class WXCOMBAT_API UWxAbilityTask_LockOnCamera : public UAbilityTask
{
	GENERATED_BODY()

public:
	static UWxAbilityTask_LockOnCamera* CreateTask(UGameplayAbility* OwningAbility, USceneComponent* InTarget, float InInterpSpeed = 10.f, float InPitchOffset = -15.f, float InMaxDistance = 2000.f, TSubclassOf<UUserWidget> InReticleWidgetClass = nullptr, float InRetargetLookThreshold = 40.f);

	UPROPERTY()
	FWxOnTargetLost OnTargetLost;

	/** 시선 입력을 임계값 이상 누적했을 때 정규화된 화면 기준 방향을 전달하며 재탐색을 요청한다. */
	UPROPERTY()
	FWxOnRetargetRequested OnRetargetRequested;

	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

protected:
	virtual void Activate() override;

private:
	/** 로컬 예측과 서버 복제 양쪽의 대상 변경을 여기서 받는다 */
	UFUNCTION()
	void HandleLockOnTargetChanged(USceneComponent* NewTarget);

	UFUNCTION()
	void HandleTargetDestroyed(AActor* DestroyedActor);

	void BindTarget();
	void UnbindTarget();

	void CreateReticleWidget();
	void DestroyReticleWidget();

	TWeakObjectPtr<USceneComponent> Target;
	TWeakObjectPtr<AActor> BoundTargetActor;
	TWeakObjectPtr<UWxLockOnComponent> LockOnComponent;
	float InterpSpeed = 8.f;
	float PitchOffset = -15.f;
	float MaxDistanceSquared = 2000.f * 2000.f;
	float RetargetLookThreshold = 40.f;
	FVector2D AccumulatedLook = FVector2D::ZeroVector;

	UPROPERTY()
	TSubclassOf<UUserWidget> ReticleWidgetClass;

	UPROPERTY()
	TObjectPtr<UWidgetComponent> ReticleWidgetComponent;
};
