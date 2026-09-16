// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "WxBTService_MirrorMovement.generated.h"

class ACharacter;
class UAbilitySystemComponent;
struct FAbilityEndedData;

/** Master 옆으로 이동하며, 어빌리티 종료 또는 도달 제한 시간 초과 시 위치를 보정한다. */
UCLASS()
class WXAI_API UWxBTService_MirrorMovement : public UBTService
{
	GENERATED_BODY()
public:
	UWxBTService_MirrorMovement();
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual FString GetStaticServiceDescription() const override;
protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	UPROPERTY(EditAnywhere, Category="Wx|AI")
	FBlackboardKeySelector MirrorTarget;
	UPROPERTY(EditAnywhere, Category="Wx|AI")
	FVector LocalOffset = FVector(0.f, 100.f, 0.f);
	UPROPERTY(EditAnywhere, Category="Wx|AI", meta=(ClampMin="1.0"))
	float ArrivalRadius = 15.f;
	UPROPERTY(EditAnywhere, Category="Wx|AI", meta=(ClampMin="0.01"))
	float TeleportDelay = 1.f;
private:
	void Release(UBehaviorTreeComponent& OwnerComp);
	void HandleAbilityEnded(const FAbilityEndedData& Data);
	TWeakObjectPtr<ACharacter> Master;
	TWeakObjectPtr<ACharacter> Follower;
	TWeakObjectPtr<UAbilitySystemComponent> MasterAbilitySystem;
	TWeakObjectPtr<UAbilitySystemComponent> FollowerAbilitySystem;
	bool bPendingAbilityEndTeleport = false;
	float TravelTime = 0.f;
	int32 PreviousJumpCount = 0;
	bool bOriginalOrientToMovement = false;
	bool bOriginalControllerDesiredRotation = false;
	float OriginalMaxWalkSpeed = 0.f;
	float OriginalCrouchSpeed = 0.f;
	float OriginalGravity = 1.f;
	float OriginalJumpVelocity = 0.f;
	int32 OriginalJumpMaxCount = 1;
};
