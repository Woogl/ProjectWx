// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gimmick/WxGimmick.h"
#include "WxTreasureChest.generated.h"

class USkeletalMeshComponent;
class UWxInteractionComponent;

UENUM()
enum class EWxChestState : uint8
{
	/** 닫힘 — 초기/기본. 인터랙션 활성. */
	Closed,
	/** 열림 — 1회성 발동 완료. 인터랙션 비활성, 열린 포즈로 스냅. */
	Open
};

/**
 * 보물 상자.
 * 플레이어가 InteractionComponent 범위에 진입하면 프롬프트 위젯이 표시되고, 상호작용 입력 시 권위 측이 State 를 Open 으로 확정한다.
 * 상태는 자체 EWxChestState(State) 가 권위 원천이며, 복제·SaveGame 으로 보존된다.
 * 열기 애니메이션과 인터랙션 비활성은 GimmickStateTree(ST_TreasureChest)가 복제 State 를 추종해 적용한다(라이브 발동=처음부터 재생, 복원=끝 프레임 스냅).
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

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	// VisibleAnywhere + AllowPrivateAccess: StateTree 의 Wx Play Animation 이 Context 액터의 컴포넌트로 바인딩하기 위한 노출.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> MeshComponent;

	// VisibleAnywhere + AllowPrivateAccess: StateTree 의 Wx Enable Interaction 이 토글 대상으로 바인딩하기 위한 노출.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWxInteractionComponent> InteractionComponent;

private:
	UFUNCTION()
	void HandleInteracted(AActor* InstigatorActor);

	/** 권위 측에서 State 를 Open 으로 확정한다. 동일값/비권위면 노옵. 열기 애니·인터랙션 토글은 StateTree 가 복제 State 를 추종해 적용한다. */
	void SetChestState(EWxChestState NewState);

	/** 상자 권위/영속 상태. 클라는 복제 State 를 ST 의 Enum Compare 전이가 추종한다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx", Replicated, SaveGame, meta = (AllowPrivateAccess = "true"))
	EWxChestState State = EWxChestState::Closed;
};
