// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "WxInteractionComponent.generated.h"

class UMeshComponent;
class UPrimitiveComponent;

/**
 * 상호작용 컴포넌트(순수 감지·강조).
 * 쿼리 볼륨(임의 형상의 PrimitiveComponent, 보통 대상 메시)을 WxInteractable 채널에 표식해, 플레이어 측 스캐너(레지스트리)가 OverlapMultiByChannel 로 수집하게 한다.
 * 볼륨은 BeginPlay 에서 부착 부모 프리미티브로 자동 해석되며, 부착 부모가 대상 프리미티브가 아닐 때만 소유자가 SetCollisionVolume 으로 명시한다.
 * 한 액터에 상호작용 영역이 여럿이면 본 컴포넌트를 영역 수만큼 추가하고 각각 다른 볼륨에 부착한다.
 *
 * 응답·프롬프트는 컴포넌트가 아니라 소유 액터가 IWxInteractable 로 제공한다:
 *  - TryInteract(서버 권위)가 소유자의 IWxInteractable::OnInteracted(Instigator, this) 를 호출한다.
 *  - 레지스트리가 스캔 때 소유자의 IWxInteractable::GetInteractionPrompt(this) 로 HUD 프롬프트를 읽는다.
 * 컴포넌트 자신은 볼륨·외곽선 강조·활성 토글만 갖는다.
 */
UCLASS(ClassGroup = "Wx", meta = (BlueprintSpawnableComponent))
class WXWORLD_API UWxInteractionComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UWxInteractionComponent();

	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 쿼리 볼륨을 명시 지정한다(오버라이드). 기본은 BeginPlay 에서 부착 부모를 자동 채택하므로, 부착 부모가 대상 프리미티브가 아닐 때만 호출한다. */
	void SetCollisionVolume(UPrimitiveComponent* InVolume);

	/** 현재 쿼리 볼륨(미지정이면 nullptr). */
	UPrimitiveComponent* GetCollisionVolume() const;

	/** 볼륨 기준 상호작용 위치(볼륨 미지정이면 컴포넌트 위치). 스캐너 정렬·서버 사거리 검증이 읽는다. */
	FVector GetInteractionLocation() const;

	/** 서버 사거리 검증용 볼륨 바운딩 반경. 임의 형상을 바운딩 스피어로 보수적으로 감싼다(볼륨 미지정이면 0). */
	float GetInteractionReachRadius() const;

	/** 오버랩된 볼륨 프리미티브를 이를 참조하는 상호작용 컴포넌트로 역참조한다(스캐너용). 없으면 nullptr. */
	static UWxInteractionComponent* FindByCollisionVolume(const UPrimitiveComponent* Volume);

	/** 서버 권한에서 호출되는 상호작용 진입점. 권한/활성 상태를 검증한 뒤 소유자의 IWxInteractable::OnInteracted 를 호출한다. */
	void TryInteract(AActor* InstigatorActor);

	/** 상호작용 활성/비활성 전환. 비활성 시 쿼리를 꺼서 스캔에서 탈락시킨다. */
	UFUNCTION(BlueprintCallable, Category = "Wx")
	void SetInteractionEnabled(bool bEnabled);

	/** 지정된 HighlightTarget 메시에 외곽선 강조를 켜고 끈다. 레지스트리가 선택 대상만 호출한다. */
	void SetHighlightEnabled(bool bNewEnabled);

	/** 강조할 메시를 지정한다. 소유 액터가 인터랙션 컴포넌트 생성·부착 직후 호출한다(미지정이면 강조하지 않는다). */
	void SetHighlightTarget(UMeshComponent* InTarget);

	/** 외곽선 강조 게이트(bUseHighlight)를 토글한다. 끄면 이후 SetHighlightEnabled 가 무시되며, 이미 켜진 외곽선도 즉시 끈다. */
	void SetUseHighlight(bool bNewUseHighlight);

protected:
	/** 쿼리 볼륨(임의 형상). 미지정이면 BeginPlay 에서 부착 부모 프리미티브를 자동 채택한다. 자동 채택도 실패하면 스캔에 잡히지 않는다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TObjectPtr<UPrimitiveComponent> CollisionVolume = nullptr;

	/** 외곽선 강조(Custom Depth/Stencil)를 적용할지 여부. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	bool bUseHighlight = true;

	/** 강조(외곽선)를 적용할 메시. 소유 액터가 명시적으로 지정한다(C++ 는 SetHighlightTarget, BP 는 디테일 패널). 미지정이면 강조하지 않는다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx", meta = (EditCondition = "bUseHighlight"))
	TObjectPtr<UMeshComponent> HighlightTarget = nullptr;

	/** 외곽선 강조용 Custom Depth Stencil 값. 포스트프로세스 아웃라인 머티리얼이 비교하는 값과 일치시킨다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx", meta = (ClampMin = 0, ClampMax = 255, EditCondition = "bUseHighlight"))
	int32 HighlightStencilValue = 1;

private:
	UFUNCTION()
	void OnRep_InteractionEnabled();

	/** bInteractionEnabled 값에 맞춰 쿼리 콜리전을 토글한다. SetInteractionEnabled(로컬)와 OnRep(복제)이 공유한다. */
	void ApplyInteractionCollision();

	/**
	 * 상호작용 활성 여부. 서버 권위에서 SetInteractionEnabled 로 토글되고 복제된다.
	 * 서버 전용으로 구동되는 소유자(예: enemy finisher 어포던스 타이머)의 토글이 OnRep 을 통해 원격 클라의 쿼리 콜리전에 반영되게 한다.
	 */
	UPROPERTY(ReplicatedUsing = OnRep_InteractionEnabled)
	bool bInteractionEnabled = true;
};
