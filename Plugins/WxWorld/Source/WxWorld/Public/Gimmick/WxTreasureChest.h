// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gimmick/WxGimmick.h"
#include "WxTreasureChest.generated.h"

class UStaticMeshComponent;
class UWxInteractionComponent;

/**
 * 보물 상자.
 * 플레이어가 InteractionComponent 범위에 진입하면 프롬프트 위젯이 표시되고,
 * 상호작용 입력 시 1회성 발동(bTriggered) 후 상호작용을 비활성화한다.
 *
 * 보상 컴포넌트(WxInventory 의 WxRewardComponent)는 플러그인 간 참조 금지 규칙 때문에 C++ 가 아니라 상속 BP 에서 추가한다.
 * 보상 컴포넌트가 InteractionComponent 에 자가 바인딩해 상호작용 시 보상 픽업을 흩뿌리므로 BP 그래프 배선은 필요 없다.
 * 보상 컴포넌트의 배치/회전이 드랍 위치와 발사 방향을 결정한다.
 */
UCLASS(Abstract)
class WXWORLD_API AWxTreasureChest : public AWxGimmick
{
	GENERATED_BODY()

public:
	AWxTreasureChest();

protected:
	virtual void BeginPlay() override;
	virtual void ApplyState() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx")
	TObjectPtr<UWxInteractionComponent> InteractionComponent;

private:
	UFUNCTION()
	void HandleInteracted(AActor* InstigatorActor);
};
