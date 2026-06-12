// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Blueprint/UserWidget.h"
#include "WxAbilityTask_LockOnTarget.generated.h"

class UInputAction;
class USceneComponent;
class UWidgetComponent;
class UWxLockOnManagerComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWxOnTargetLost);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWxOnRetargetRequested, FVector2D, ScreenDirection);

/**
 * 락온 대상을 매 프레임 추적하는 AbilityTask.
 * 컨트롤러 회전과 캐릭터 몸체를 각각 타겟 방향으로 보간하여 카메라와 캐릭터가 부드럽게 적을 향하게 한다.
 * 타겟이 파괴되거나 무효화되면 OnTargetLost를 브로드캐스트.
 */
UCLASS()
class WXCOMBAT_API UWxAbilityTask_LockOnTarget : public UAbilityTask
{
	GENERATED_BODY()

public:
	static UWxAbilityTask_LockOnTarget* CreateTask(UGameplayAbility* OwningAbility, USceneComponent* InTarget, float InInterpSpeed = 10.f, float InPitchOffset = -15.f, float InMaxDistance = 2000.f, float InCharacterInterpSpeed = 10.f, TSubclassOf<UUserWidget> InReticleWidgetClass = nullptr, UInputAction* InLookAction = nullptr, float InRetargetLookThreshold = 40.f);

	UPROPERTY()
	FWxOnTargetLost OnTargetLost;

	/** IA_Look 입력을 임계값 이상 누적했을 때 정규화된 화면 기준 방향을 전달하며 재탐색을 요청한다. */
	UPROPERTY()
	FWxOnRetargetRequested OnRetargetRequested;

	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

protected:
	virtual void Activate() override;

private:
	/** 락온 컴포넌트의 대상 변경(로컬 예측 + 서버 복제 정합)을 받아 추적 대상을 교체한다. */
	UFUNCTION()
	void HandleLockOnTargetChanged(USceneComponent* NewTarget);

	UFUNCTION()
	void HandleTargetDestroyed(AActor* DestroyedActor);

	void HandleTargetDeathTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	void BindTarget();
	void UnbindTarget();

	void CreateReticleWidget();
	void DestroyReticleWidget();

	/** 추적 대상 컴포넌트(부위). 위치는 GetComponentLocation(), 소유 액터는 GetOwner()로 얻는다. */
	TWeakObjectPtr<USceneComponent> Target;
	/** BindTarget 시점의 소유 액터. 부위 컴포넌트만 파괴되고 액터는 살아있는 경우에도 OnDestroyed/사망 태그 바인딩을 정확히 해제하기 위해 캐시한다. */
	TWeakObjectPtr<AActor> BoundTargetActor;
	TWeakObjectPtr<UWxLockOnManagerComponent> LockOnManagerComponent;
	float InterpSpeed = 8.f;
	float CharacterInterpSpeed = 10.f;
	float PitchOffset = -15.f;
	float MaxDistanceSquared = 2000.f * 2000.f;
	float RetargetLookThreshold = 40.f;
	FVector2D AccumulatedLook = FVector2D::ZeroVector;
	TSubclassOf<UUserWidget> ReticleWidgetClass;

	UPROPERTY()
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY()
	TObjectPtr<UWidgetComponent> ReticleWidgetComponent;
};
