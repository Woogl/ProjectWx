// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "WxInteractionComponent.generated.h"

class UWidgetComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWxOnInteractedSignature, AActor*, InstigatorActor);

/**
 * 액터의 상호작용을 노출하는 컴포넌트.
 * USphereComponent를 상속해 폰의 접근을 자체적으로 감지하며,
 * 오버랩 시 Owner 액터가 소유한 UWidgetComponent를 찾아 가시성을 토글한다.
 * (UI 위젯의 생성/배치/WidgetClass 지정은 Owner 액터의 책임)
 *
 * 흐름:
 *  1) 폰이 본 컴포넌트와 오버랩 → 로컬 클라이언트에서 Owner의 WidgetComponent를 표시
 *  2) 플레이어가 상호작용 입력 → WxAbility_Interact(서버 권한 실행)가 TryInteract 호출
 *  3) 서버가 내부 Multicast RPC로 서버+모든 클라이언트에서 OnInteracted 델리게이트를 fire
 *
 * 사용처(Owner 액터)는 OnInteracted에 바인딩만 하면 되며, 권한/복제는 컴포넌트가 처리한다.
 * 서버 권위 로직이 필요하면 콜백 내부에서 HasAuthority()로 분기한다.
 * 부착 액터는 반드시 Replicate 되어야 한다.
 */
UCLASS(ClassGroup = "Wx", meta = (BlueprintSpawnableComponent, PrioritizeCategories = "Wx"))
class WXWORLD_API UWxInteractionComponent : public USphereComponent
{
	GENERATED_BODY()

public:
	UWxInteractionComponent();

	/** 서버 권한에서 호출. 서버+모든 클라이언트에서 OnInteracted 델리게이트가 fire된다. */
	void TryInteract(AActor* InstigatorActor);

	UPROPERTY(BlueprintAssignable, Category = "Wx")
	FWxOnInteractedSignature OnInteracted;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastInteracted(AActor* InstigatorActor);

	void SetPromptVisible(bool bNewVisible);

	TWeakObjectPtr<UWidgetComponent> CachedPromptWidget;
};
