// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "WxInteractionSource.h"
#include "WxInteractionComponent.generated.h"

class UMeshComponent;

/**
 * 상호작용 컴포넌트.
 * 상호작용 가능 영역을 나타내는 수동 쿼리 볼륨(SphereComponent, Object Type=WxInteractable)이다.
 * 플레이어 측 스캐너(상호작용 어빌리티의 주기 스캔)가 이 볼륨을 OverlapMultiByObjectType 으로 수집해 레지스트리에 채운다.
 * 한 액터에 여러 인터랙션 영역을 두려면 본 컴포넌트를 영역 수만큼 추가한다.
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
class WXWORLD_API UWxInteractionComponent : public USphereComponent, public IWxInteractionSource
{
	GENERATED_BODY()

public:
	UWxInteractionComponent();

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
	FText GetInteractionText() const { return InteractionText; }

	/** 지정된 HighlightTarget 메시에 외곽선 강조를 켜고 끈다. 레지스트리가 선택 대상만 호출한다. */
	void SetHighlightEnabled(bool bNewEnabled);

	/** 강조할 메시를 지정한다. 소유 액터가 인터랙션 컴포넌트 생성·부착 직후 호출한다(미지정이면 강조하지 않는다). */
	void SetHighlightTarget(UMeshComponent* InTarget) { HighlightTarget = InTarget; }

	/** 소유 액터가 바인딩하는 상호작용 델리게이트. 서버 권한에서만 fire 된다(원격 클라에서는 호출되지 않는다). */
	UPROPERTY()
	FWxOnInteractedSignature OnInteracted;

protected:
	/** HUD 리스트에 표시할 상호작용 텍스트. 레지스트리가 GetInteractionText 로 읽는다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx", meta = (MultiLine = true))
	FText InteractionText;

	/** 외곽선 강조(Custom Depth/Stencil)를 적용할지 여부. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	bool bEnableHighlight = true;

	/** 강조(외곽선)를 적용할 메시. 소유 액터가 명시적으로 지정한다(C++ 는 SetHighlightTarget, BP 는 디테일 패널). 미지정이면 강조하지 않는다. */
	UPROPERTY(EditAnywhere, Category = "Wx", meta = (EditCondition = "bEnableHighlight"))
	TObjectPtr<UMeshComponent> HighlightTarget = nullptr;

	/** 외곽선 강조용 Custom Depth Stencil 값. 포스트프로세스 아웃라인 머티리얼이 비교하는 값과 일치시킨다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx", meta = (ClampMin = 0, ClampMax = 255, EditCondition = "bEnableHighlight"))
	int32 HighlightStencilValue = 1;

private:
	bool bInteractionEnabled;
};
