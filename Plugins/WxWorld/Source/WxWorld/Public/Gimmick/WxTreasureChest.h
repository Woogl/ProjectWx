// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
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
 * 보상 지급은 GimmickStateTree(ST_TreasureChest)의 Open 상태에서 Wx Grant Reward 태스크가 수행한다(UWxRewardLibrary::GrantReward 호출).
 * 지급할 보상(RewardRow)·발사 속도(LaunchSpeed)는 이 액터의 프로퍼티로 두고, ST 태스크가 Context 액터 바인딩으로 읽는다. 스폰 오프셋(SpawnOffset)은 태스크 인스턴스 데이터로 ST 에셋에서 설정한다(별도 컴포넌트 부착 불필요).
 * 픽업 보상은 상자 위(SpawnOffset)에서 월드 Z 업으로 수직 발사되고, 비-픽업(재화 등) 보상은 로컬 플레이어 인벤토리에 직접 지급된다.
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

	//~ Begin AWxGimmick — State(EWxChestState) ↔ uint8 쓰기 매핑.
	virtual void SetGimmickState(uint8 NewStateValue) override;
	//~ End AWxGimmick

	// VisibleAnywhere + AllowPrivateAccess: StateTree 의 Wx Play Animation 이 Context 액터의 컴포넌트로 바인딩하기 위한 노출.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> MeshComponent;

	// VisibleAnywhere + AllowPrivateAccess: StateTree 의 Wx Enable Interaction 이 토글 대상으로 바인딩하기 위한 노출.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wx", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWxInteractionComponent> InteractionComponent;

	// EditAnywhere + AllowPrivateAccess: 디자이너가 값을 설정하고, StateTree 의 Wx Grant Reward 태스크가 Context 액터의 프로퍼티로 바인딩해 읽는다.
	/** 상호작용 시 지급할 보상. FWxRewardTableRow 로우. 비우면 보상 없음. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx|Reward", meta = (RowType = "/Script/WxInventory.WxRewardTableRow", AllowPrivateAccess = "true"))
	FDataTableRowHandle RewardRow;

	/** 픽업 보상의 발사 속도(cm/s). 발사 방향은 항상 월드 Z 업(수직)이다. 비-픽업(재화) 보상엔 영향 없다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wx|Reward", meta = (ClampMin = "0", AllowPrivateAccess = "true"))
	float LaunchSpeed = 300.f;

private:
	UFUNCTION()
	void HandleInteracted(AActor* InstigatorActor);

	/** 상자 권위/영속 상태(복제 + SaveGame). State 쓰기는 권위 전용(CommitGimmickState)이며, 클라는 복제 State 를 ST 의 Enum Compare 전이가 추종한다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx", Replicated, SaveGame, meta = (AllowPrivateAccess = "true"))
	EWxChestState State = EWxChestState::Closed;
};
