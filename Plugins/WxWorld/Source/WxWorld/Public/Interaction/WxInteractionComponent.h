// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "WxInteractionSource.h"
#include "WxInteractionComponent.generated.h"

class UWxInteractionRegistrySubsystem;

/**
 * 상호작용 컴포넌트.
 * 폰 오버랩 감지(SphereComponent), 레지스트리 등록, Multicast 알림을 일괄 처리한다.
 * 한 액터에 여러 인터랙션 영역을 두려면 본 컴포넌트를 영역 수만큼 추가한다.
 *
 * 흐름:
 *  1) 폰이 본 컴포넌트와 오버랩 → 로컬 클라이언트에서 레지스트리(HUD 리스트 소스) 등록. 외곽선 강조는 레지스트리가 선택 대상만 켠다
 *  2) 플레이어가 상호작용 입력 → WxAbility_Interact가 레지스트리의 선택된 컴포넌트의 TryInteract 호출(서버 권한)
 *  3) 서버가 Multicast RPC로 OnInteracted 델리게이트를 서버+모든 클라이언트에서 fire
 *
 * 소유 액터는 OnInteracted 델리게이트에 핸들러를 바인딩해 동작을 구현한다.
 * 핸들러 내부에서 권위 로직은 GetOwner()->HasAuthority()로 분기한다.
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

	/** 상호작용 활성/비활성 전환. 비활성 시 오버랩 감지를 중단하고 등록을 해제한다. */
	UFUNCTION(BlueprintCallable, Category = "Wx")
	void SetInteractionEnabled(bool bEnabled);

	// IWxInteractionSource
	virtual FWxOnInteractedSignature& GetOnInteractedDelegate() override;

	/** 상호작용 텍스트 갱신. 레지스트리가 GetInteractionText 로 읽어 HUD 리스트를 구성한다. */
	UFUNCTION(BlueprintCallable, Category = "Wx")
	virtual void SetInteractionText(const FText& InText) override;

	/** 현재 상호작용 텍스트. 레지스트리가 HUD 리스트 구성을 위해 읽는다. */
	FText GetInteractionText() const { return InteractionText; }

	/** 소유 액터의 메시 컴포넌트에 외곽선 강조를 켜고 끈다. 레지스트리가 선택 대상만 호출한다. */
	void SetHighlightEnabled(bool bNewEnabled);

	/** 외부 리스너/소유 액터에서 바인딩. 서버+모든 클라이언트에서 fire 된다. */
	UPROPERTY(BlueprintAssignable, Category = "Wx")
	FWxOnInteractedSignature OnInteracted;

protected:
	virtual void BeginPlay() override;

	/** HUD 리스트에 표시할 상호작용 텍스트. 레지스트리가 GetInteractionText 로 읽는다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx", meta = (MultiLine = true))
	FText InteractionText;

	/** 범위 진입 시 소유 액터 메시에 외곽선 강조(Custom Depth/Stencil)를 적용할지 여부. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	bool bEnableHighlight = true;

	/** 외곽선 강조용 Custom Depth Stencil 값. 포스트프로세스 아웃라인 머티리얼이 비교하는 값과 일치시킨다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx", meta = (ClampMin = 0, ClampMax = 255, EditCondition = "bEnableHighlight"))
	int32 HighlightStencilValue = 1;

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastInteracted(AActor* InstigatorActor);

	/** 플레이어의 레지스트리 서브시스템에 자신을 등록하고 그 서브시스템을 캐시한다. */
	void RegisterWithRegistry(AActor* PlayerActor);

	/** 캐시한 레지스트리 서브시스템에서 자신을 해제한다. */
	void UnregisterFromRegistry();

	bool IsLocalPlayerPawn(const AActor* OtherActor) const;

	/** 현재 등록되어 있는 플레이어 레지스트리 서브시스템. 비활성/이탈 시 해제 대상. */
	TWeakObjectPtr<UWxInteractionRegistrySubsystem> RegisteredRegistry;

	bool bInteractionEnabled;
};
