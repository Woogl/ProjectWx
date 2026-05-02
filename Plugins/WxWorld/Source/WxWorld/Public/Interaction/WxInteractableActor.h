// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/WxInteractable.h"
#include "WxInteractableActor.generated.h"

class USphereComponent;
class UWxInteractionWidgetComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWxOnInteractedSignature, AActor*, InstigatorActor);

/**
 * 상호작용 가능한 액터의 추상 베이스.
 * 오버랩 감지(USphereComponent), 프롬프트 위젯 토글, Multicast 알림을 일괄 처리한다.
 *
 * 흐름:
 *  1) 폰이 InteractionComponent와 오버랩 → 로컬 클라이언트에서 InteractionWidget 표시
 *  2) 플레이어가 상호작용 입력 → WxAbility_Interact가 IWxInteractable::TryInteract 호출(서버 권한)
 *  3) 서버가 Multicast RPC로 OnInteracted 델리게이트를 서버+모든 클라이언트에서 fire
 *
 * 파생 클래스는 동작을 구현하며, 서버 권위 로직은 콜백 내부에서 HasAuthority()로 분기한다.
 *  - C++ 파생/외부 리스너: OnInteracted 델리게이트에 바인딩
 *  - BP 파생: ReceiveInteracted 이벤트 override (Functions > Override)
 * InteractionComponent와 InteractionWidget은 베이스에서 생성하지만 부착(SetupAttachment)은 파생 클래스의 책임이다.
 * 베이스에서 bReplicates = true로 설정한다.
 */
UCLASS(Abstract)
class WXWORLD_API AWxInteractableActor : public AActor, public IWxInteractable
{
	GENERATED_BODY()

public:
	AWxInteractableActor();

	// IWxInteractable
	virtual void TryInteract(AActor* InstigatorActor) override;

	/** 상호작용 활성/비활성 전환. 비활성 시 오버랩 감지를 중단하고 프롬프트를 숨긴다. */
	void SetInteractionEnabled(bool bEnabled);

	/** 외부 리스너/C++ 파생 클래스에서 바인딩. 서버+모든 클라이언트에서 fire 된다. */
	UPROPERTY(BlueprintAssignable, Category = "Wx")
	FWxOnInteractedSignature OnInteracted;

protected:
	virtual void BeginPlay() override;

	/** BP 파생 클래스에서 override. 서버+모든 클라이언트에서 호출된다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Wx")
	void ReceiveInteracted(AActor* InstigatorActor);

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<USphereComponent> InteractionComponent;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxInteractionWidgetComponent> InteractionWidget;

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastInteracted(AActor* InstigatorActor);

	void SetInteractionWidgetVisible(bool bNewVisible);

	bool bInteractionEnabled;
};
