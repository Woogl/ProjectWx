// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "WxInteractionComponent.generated.h"

class UUserWidget;
class UWidgetComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWxOnInteractedSignature, AActor*, InstigatorActor);

/**
 * 상호작용 컴포넌트.
 * 폰 오버랩 감지(SphereComponent), 프롬프트 위젯 토글, Multicast 알림을 일괄 처리한다.
 * 한 액터에 여러 인터랙션 영역을 두려면 본 컴포넌트를 영역 수만큼 추가한다.
 *
 * 흐름:
 *  1) 폰이 본 컴포넌트와 오버랩 → 로컬 클라이언트에서 프롬프트 위젯 표시
 *  2) 플레이어가 상호작용 입력 → WxAbility_Interact가 가장 가까운 컴포넌트의 TryInteract 호출(서버 권한)
 *  3) 서버가 Multicast RPC로 OnInteracted 델리게이트를 서버+모든 클라이언트에서 fire
 *
 * 소유 액터는 OnInteracted 델리게이트에 핸들러를 바인딩해 동작을 구현한다.
 * 핸들러 내부에서 권위 로직은 GetOwner()->HasAuthority()로 분기한다.
 *
 * 프롬프트 위젯 컴포넌트는 OnRegister 시점에 동적으로 생성·부착한다.
 */
UCLASS(ClassGroup = "Wx", meta = (BlueprintSpawnableComponent))
class WXWORLD_API UWxInteractionComponent : public USphereComponent
{
	GENERATED_BODY()

public:
	UWxInteractionComponent();

	/** 서버 권한에서 호출되는 상호작용 진입점. 권한/활성 상태를 검증한 뒤 Multicast 알림을 발사한다. */
	void TryInteract(AActor* InstigatorActor);

	/** 상호작용 활성/비활성 전환. 비활성 시 오버랩 감지를 중단하고 프롬프트를 숨긴다. */
	UFUNCTION(BlueprintCallable, Category = "Wx")
	void SetInteractionEnabled(bool bEnabled);

	/** 프롬프트 위젯 텍스트 갱신. 위젯이 이미 생성되어 있으면 즉시 반영한다. */
	UFUNCTION(BlueprintCallable, Category = "Wx")
	void SetInteractionText(const FText& InText);

	/** 외부 리스너/소유 액터에서 바인딩. 서버+모든 클라이언트에서 fire 된다. */
	UPROPERTY(BlueprintAssignable, Category = "Wx")
	FWxOnInteractedSignature OnInteracted;

protected:
	virtual void BeginPlay() override;
	virtual void OnRegister() override;

	/** 프롬프트 위젯 BP 클래스. IWxInteractionWidgetInterface 구현체만 지정 가능. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx", meta = (MustImplement = "/Script/WxWorld.WxInteractionWidgetInterface"))
	TSubclassOf<UUserWidget> PromptWidgetClass;

	/** 프롬프트 위젯이 IWxInteractionWidgetInterface 를 구현하면 자동 전달되는 텍스트. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx", meta = (MultiLine = true))
	FText InteractionText;

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastInteracted(AActor* InstigatorActor);

	void SetInteractionWidgetVisible(bool bNewVisible);

	UPROPERTY(Transient)
	TObjectPtr<UWidgetComponent> InteractionWidget;

	bool bInteractionEnabled;
};
