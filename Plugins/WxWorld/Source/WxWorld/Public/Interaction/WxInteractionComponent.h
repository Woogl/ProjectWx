// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "WxInteractionSource.h"
#include "WxInteractionComponent.generated.h"

class UMeshComponent;
class UPrimitiveComponent;

/**
 * 상호작용 컴포넌트.
 * 상호작용 상태·로직만 보유하는 SceneComponent 이며, 쿼리 볼륨 자체는 CollisionVolume(임의 형상의 PrimitiveComponent,
 * 보통 기존 메시)을 재사용한다. 볼륨은 BeginPlay 에서 부착 부모 프리미티브로 자동 해석되며(대부분 대상 메시에 직접 부착됨),
 * 부착 부모가 대상 프리미티브가 아닐 때만 소유자가 SetCollisionVolume 으로 명시한다. 볼륨은 WxInteractable 채널에 Overlap
 * 응답으로 표식되고, 플레이어 측 스캐너(상호작용 어빌리티의 주기 스캔)가 OverlapMultiByChannel(WxInteractable) 으로 수집한다.
 * 한 액터에 여러 인터랙션 영역을 두려면 본 컴포넌트를 영역 수만큼 추가하고 각각 다른 볼륨에 부착한다.
 *
 * 흐름:
 *  1) 플레이어 스캐너가 주변 볼륨을 수집 → 로컬 레지스트리(HUD 리스트 소스)에 채운다. 외곽선 강조는 레지스트리가 선택 대상만 켠다
 *  2) 플레이어가 상호작용 입력 → WxAbility_Interact가 (원격 클라는) 레지스트리의 선택 컴포넌트를 TargetData로 서버에 전달, 서버 권한에서 TryInteract 호출
 *  3) 서버 권한에서 OnInteracted 델리게이트를 fire(서버 전용). 클라 비주얼은 각 대상의 복제 상태(기믹 State, 픽업 Destroy 등)로 수렴한다
 *
 * 소유 액터는 OnInteracted 델리게이트에 핸들러를 바인딩해 동작을 구현한다.
 * OnInteracted는 서버 권한에서만 fire되므로 핸들러는 권위 로직을 그대로 수행한다(클라에서는 호출되지 않는다).
 *
 * 프롬프트 표시는 본 컴포넌트가 아니라 플레이어 HUD 리스트(WBP_InteractionList)가 담당한다.
 */
UCLASS(ClassGroup = "Wx", meta = (BlueprintSpawnableComponent))
class WXWORLD_API UWxInteractionComponent : public USceneComponent, public IWxInteractionSource
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

	/** 서버 권한에서 호출되는 상호작용 진입점. 권한/활성 상태를 검증한 뒤 Multicast 알림을 발사한다. */
	void TryInteract(AActor* InstigatorActor);

	/** 상호작용 활성/비활성 전환. 비활성 시 쿼리를 꺼서 스캔에서 탈락시킨다. */
	UFUNCTION(BlueprintCallable, Category = "Wx")
	void SetInteractionEnabled(bool bEnabled);

	// IWxInteractionSource
	virtual FWxOnInteractedSignature& GetOnInteractedDelegate() override;

	/** 상호작용 텍스트 갱신. 레지스트리가 GetInteractionText 로 읽어 HUD 리스트를 구성한다. */
	UFUNCTION(BlueprintCallable, Category = "Wx")
	virtual void SetInteractionText(const FText& InText) override;

	/** 현재 상호작용 텍스트. 레지스트리가 HUD 리스트 구성을 위해 읽는다. */
	FText GetInteractionText() const;

	/** 지정된 HighlightTarget 메시에 외곽선 강조를 켜고 끈다. 레지스트리가 선택 대상만 호출한다. */
	void SetHighlightEnabled(bool bNewEnabled);

	/** 강조할 메시를 지정한다. 소유 액터가 인터랙션 컴포넌트 생성·부착 직후 호출한다(미지정이면 강조하지 않는다). */
	void SetHighlightTarget(UMeshComponent* InTarget);

	/** 외곽선 강조 게이트(bUseHighlight)를 토글한다. 끄면 이후 SetHighlightEnabled 가 무시되며, 이미 켜진 외곽선도 즉시 끈다. */
	void SetUseHighlight(bool bNewUseHighlight);

	/** 소유 액터가 바인딩하는 상호작용 델리게이트. 서버 권한에서만 fire 된다(원격 클라에서는 호출되지 않는다). */
	UPROPERTY()
	FWxOnInteractedSignature OnInteracted;

protected:
	/** 쿼리 볼륨(임의 형상). 미지정이면 BeginPlay 에서 부착 부모 프리미티브를 자동 채택한다. 자동 채택도 실패하면 스캔에 잡히지 않는다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TObjectPtr<UPrimitiveComponent> CollisionVolume = nullptr;

	/** HUD 리스트에 표시할 상호작용 텍스트. 레지스트리가 GetInteractionText 로 읽는다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx", meta = (MultiLine = true))
	FText InteractionText;

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
