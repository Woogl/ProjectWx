// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Spawnable/WxSpawnableInterface.h"
#include "GameFramework/Actor.h"
#include "WxPickupBase.generated.h"

class URotatingMovementComponent;
class UStaticMeshComponent;
class UWxInteractionComponent;
class UWxInteractionWidgetComponent;

/**
 * 픽업 액터 베이스 클래스.
 *
 * 플레이어가 오버랩 범위에 들어오면 상호작용 프롬프트를 표시하고,
 * 상호작용이 발생하면 서버 권한에서 OnPickedUp 가상 훅을 호출한다.
 * 구체적인 지급/소멸 로직은 파생 클래스에서 OnPickedUp 을 오버라이드하여 구현한다.
 */
UCLASS(Abstract)
class WXWORLD_API AWxPickupBase : public AActor, public IWxSpawnableInterface
{
	GENERATED_BODY()

public:
	AWxPickupBase();

	// IWxSpawnableInterface
#if WITH_EDITOR
	virtual const UMeshComponent* GetEditorPreviewMeshComponent() const override;
#endif

protected:
	virtual void BeginPlay() override;

	/** 서버 권한에서 상호작용이 확정된 뒤 호출되는 가상 훅. 파생 클래스에서 지급/소멸 로직을 구현한다. */
	virtual void OnPickedUp(AActor* InteractingActor);

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxInteractionComponent> InteractionComponent;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<UWxInteractionWidgetComponent> InteractionWidget;

	UPROPERTY(VisibleAnywhere, Category = "Wx")
	TObjectPtr<URotatingMovementComponent> RotatingMovement;

private:
	UFUNCTION()
	void HandleInteracted(AActor* InteractingActor);
};
