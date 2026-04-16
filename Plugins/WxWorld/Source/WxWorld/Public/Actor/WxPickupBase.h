// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Actor/WxSpawnableInterface.h"
#include "GameFramework/Actor.h"
#include "WxPickupBase.generated.h"

class URotatingMovementComponent;
class UStaticMeshComponent;
class UWxInteractionComponent;
class UWxInteractionWidgetComponent;

/**
 * 픽업 아이템 베이스 클래스.
 * 플레이어가 오버랩 범위에 들어오면 상호작용 프롬프트를 표시하고,
 * 상호작용 어빌리티를 통해 수집(획득)한다.
 * 서브클래스에서 OnPickedUp을 구현하여 수집 시 동작을 정의한다.
 */
UCLASS(Abstract, meta = (PrioritizeCategories = "Wx"))
class WXWORLD_API AWxPickupBase : public AActor, public IWxSpawnableInterface
{
	GENERATED_BODY()

public:
	AWxPickupBase();

	// IWxSpawnableInterface
#if WITH_EDITOR
	virtual UStreamableRenderAsset* GetEditorPreviewMesh() const override;
#endif

protected:
	virtual void BeginPlay() override;

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
