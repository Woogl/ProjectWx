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
class UWxItemDefinition;

/**
 * 픽업 아이템 베이스 클래스.
 *
 * 플레이어가 오버랩 범위에 들어오면 상호작용 프롬프트를 표시하고,
 * 상호작용 어빌리티를 통해 수집(획득)한다. 수집 시 Interactor 의
 * PlayerState(또는 본체) 에서 UWxInventoryManagerComponent 를 찾아
 * ItemDef 를 Count 만큼 지급한 뒤 파괴된다.
 */
UCLASS(Abstract, meta = (PrioritizeCategories = "Wx"))
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

	/** 지급할 아이템 정의. Currency Fragment 가 붙은 재화도 동일하게 사용. */
	UPROPERTY(EditAnywhere, Category = "Wx|Pickup")
	TObjectPtr<UWxItemDefinition> ItemDef;

	/** 지급 수량. */
	UPROPERTY(EditAnywhere, Category = "Wx|Pickup", meta = (ClampMin = "1"))
	int32 Count = 1;

private:
	UFUNCTION()
	void HandleInteracted(AActor* InteractingActor);
};
