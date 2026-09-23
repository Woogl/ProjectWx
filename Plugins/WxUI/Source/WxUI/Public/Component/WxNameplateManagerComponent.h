// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "WxNameplateManagerComponent.generated.h"

class UUserWidget;
class UWidgetComponent;
class UWxNameplateSourceComponent;

DECLARE_DELEGATE_RetVal(USceneComponent*, FWxNameplateLockOnTargetQuery);

/**
 * 플레이어 컨트롤러에 붙어, NameplateSource(UWxNameplateSourceComponent) 중 보일 대상에만 Nameplate 위젯을 붙이고 조건을 벗어나면 뗀다.
 * LockOn 대상 지점의 주인에는 요구 태그와 무관하게 Nameplate를 붙이고, 그 지점에는 Reticle을 붙인다.
 *
 * 보는 사람마다 다른 로컬 표시라 소유 클라(리슨 호스트 포함)에서만 구동한다.
 */
UCLASS(ClassGroup = (Wx), meta = (BlueprintSpawnableComponent))
class WXUI_API UWxNameplateManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWxNameplateManagerComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 로컬 플레이어의 LockOn 대상 지점. 락온은 WxCombat이 소유하므로 게임 쪽이 바인딩한다. */
	FWxNameplateLockOnTargetQuery LockOnTargetQuery;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Character 뷰모델을 Manual 소스로 받는 위젯이어야 한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Nameplate")
	TSubclassOf<UUserWidget> NameplateWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Wx|Nameplate")
	TSubclassOf<UUserWidget> ReticleWidgetClass;

	/** 대상 ASC 태그로 판정한다. LockOn 대상은 요구 태그를 건너뛰지만 무시 태그(사망 등)는 따른다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Nameplate")
	FGameplayTagRequirements VisibilityRequirements;

	UPROPERTY(EditDefaultsOnly, Category = "Wx|Nameplate", meta = (ClampMin = "0", Units = "cm"))
	float ReferenceDistance = 1000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Wx|Nameplate", meta = (ClampMin = "0"))
	float MinScale = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Wx|Nameplate", meta = (ClampMin = "0"))
	float MaxScale = 1.f;

	/** 0이면 거리와 관계없이 숨긴다. LockOn 대상에는 적용하지 않는다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Nameplate", meta = (ClampMin = "0", Units = "cm"))
	float MaxVisibilityDistance = 3000.f;

	/** 새로 붙이려면 MaxVisibilityDistance보다 이만큼 안쪽이어야 한다. 이미 붙은 것은 MaxVisibilityDistance까지 유지한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Nameplate", meta = (ClampMin = "0", Units = "cm"))
	float VisibilityDistanceHysteresis = 200.f;

	/** 캐릭터 메시 기본 포즈의 윗면에서 Nameplate까지의 높이. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Nameplate", meta = (Units = "cm"))
	float HeadClearance = 30.f;

private:
	void UpdateNameplates(const APawn* ViewerPawn, const AActor* LockOnActor);
	void UpdateReticle(USceneComponent* LockOnTarget);

	/** 대상 액터 소유로 만들어, 대상이 파괴되면 표시도 함께 사라지게 한다. */
	UWidgetComponent* AttachWidget(USceneComponent* Parent, TSubclassOf<UUserWidget> WidgetClass) const;

	TMap<TWeakObjectPtr<UWxNameplateSourceComponent>, TWeakObjectPtr<UWidgetComponent>> Nameplates;

	TWeakObjectPtr<UWidgetComponent> Reticle;
};
