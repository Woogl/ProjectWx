// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "WxBTService_MirrorMovement.generated.h"

class AAIController;
class APawn;

/** 대상의 한 프레임. 재생 시점에 이 값들이 그대로 폰에게 옮겨진다. */
struct FWxTrailSample
{
	double Time = 0.0;

	FVector Location = FVector::ZeroVector;

	/** 오프셋을 미는 기준 프레임이자, 따라 볼 방향이다. */
	float Yaw = 0.f;

	float Speed = 0.f;

	bool bCrouched = false;

	bool bAirborne = false;

	/** 이 프레임에 대상이 스스로 떴다. 재생에서 한 번 소비하고 지운다. */
	bool bJumped = false;
};

struct FWxMirrorMovementMemory
{
	/** 오래된 것이 앞. 맨 앞이 곧 지금 재생할 표본이다. */
	TArray<FWxTrailSample> Trail;

	/** 시선과 회전 모드를 걸어 둔 폰. 비어 있으면(IsExplicitlyNull) 적용 기록이 없다는 뜻이다. */
	TWeakObjectPtr<APawn> FacingPawn;
};

/**
 * BT Service: Blackboard 로 지목한 대상의 자취를 기록해, Delay 초 뒤에 그 자리를 되밟는다.
 *
 * 걷기·달리기·점프·앉기는 어빌리티가 아니라 ACharacter/CMC 의 네이티브 상태라 UWxBTTask_MirrorAbility 가 잡지 못한다.
 * 그래서 상태를 태그로 묻는 대신 대상의 자취를 그대로 기록해 되밟는다. 대상이 밟은 길만 밟으므로 벽·낭떠러지를 따로 피할 필요가 없다.
 *
 * 설 자리는 대상의 로컬 프레임에서 LocalOffset 만큼 민다 — 옆에 나란히 서는 분신이 된다.
 * 이동 입력 스케일이 "기록 속력 / 내 상한" 이라 걷기와 달리기의 속도 차이가 저절로 재현되지만, 상한이 대상보다 낮으면 그만큼 뒤처진다.
 *
 * 태스크가 아니라 서비스인 이유는 두 가지다.
 * 태스크로 만들면 어빌리티 미러가 브랜치를 쥔 동안 기록이 끊겨 자취에 구멍이 생기고, 그동안 폰이 멈춘다.
 * 같은 이유로 이 서비스는 루트 컴포지트에 붙여야 한다 — 하위 브랜치에 붙이면 그 브랜치가 쉴 때 기록이 끊긴다.
 *
 * bMirrorFacing 은 컨트롤러 포커스와 폰의 회전 모드를 쓰므로 UWxBTService_LockOn 과 한 트리에 두지 않는다.
 */
UCLASS()
class WXAI_API UWxBTService_MirrorMovement : public UBTService
{
	GENERATED_BODY()

public:
	UWxBTService_MirrorMovement();

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

	virtual uint16 GetInstanceMemorySize() const override;

	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;

	virtual void CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryClear::Type CleanupType) const override;

	/** 엔진이 여기에 노드 타입명과 틱 주기를 덧붙여 그래프에 그린다. 최상위 GetStaticDescription 을 덮으면 그 둘이 사라진다. */
	virtual FString GetStaticServiceDescription() const override;

	virtual void DescribeRuntimeValues(const UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const override;

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/** 소환물이라면 소환자를 담는 Master 가 기본값이다. */
	UPROPERTY(EditAnywhere, Category = "Wx|AI")
	FBlackboardKeySelector MirrorTarget;

	/** 대상의 로컬 기준으로 설 자리. X 는 앞뒤, Y 는 좌우다. 대상이 방향을 틀면 이 자리도 함께 돈다. */
	UPROPERTY(EditAnywhere, Category = "Wx|AI")
	FVector LocalOffset = FVector(0.f, -120.f, 0.f);

	/** 몇 초 전의 대상을 재현하는가. 0 이면 지금 대상 옆에 즉시 붙는다. */
	UPROPERTY(EditAnywhere, Category = "Wx|AI", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float Delay = 0.5f;

	/** 설 자리에 이만큼 다가서면 입력을 넣지 않는다. */
	UPROPERTY(EditAnywhere, Category = "Wx|AI", meta = (ClampMin = "0.0"))
	float ArrivalRadius = 60.f;

	/** 이보다 벌어지면 기록 속력을 무시하고 전력으로 붙는다. 루트모션 어빌리티가 추적을 멈춘 만큼을 여기서 메운다. */
	UPROPERTY(EditAnywhere, Category = "Wx|AI", meta = (ClampMin = "0.0"))
	float CatchUpDistance = 400.f;

	/** 대상이 보던 방향을 따라 본다. 컨트롤러 포커스와 회전 모드를 쓰므로 Lock On 서비스와 한 트리에 두지 않는다. */
	UPROPERTY(EditAnywhere, Category = "Wx|AI")
	bool bMirrorFacing = true;

private:
	AActor* FindMirrorTarget(const UBehaviorTreeComponent& OwnerComp) const;

	void RecordSample(const AActor& Target, double Now, FWxMirrorMovementMemory& Memory) const;

	/** 표본 하나를 폰에게 옮긴다. 점프만 1회성이라 밖에서 소비해 넘겨준다. */
	void MirrorSample(APawn& Pawn, const FWxTrailSample& Sample, bool bShouldJump) const;

	/** 포커스는 컨트롤러가, 그 방향으로의 회전은 폰 CMC 가 담당하므로 둘을 한 쌍으로 건다. */
	void ApplyFacing(AAIController& AIController, APawn& Pawn, float Yaw, FWxMirrorMovementMemory& Memory) const;

	/**
	 * 적용 기록이 있을 때만 되돌린다. 멱등이라 틱과 브랜치 이탈 양쪽에서 불러도 안전하다.
	 * 컨트롤러가 없어도 폰의 회전 모드는 되돌려야 하므로 컨트롤러를 선택 인자로 받는다.
	 */
	void ReleaseFacing(AAIController* AIController, FWxMirrorMovementMemory& Memory) const;
};
