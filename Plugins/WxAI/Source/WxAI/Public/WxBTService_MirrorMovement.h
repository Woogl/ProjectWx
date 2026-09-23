// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayTagContainer.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "WxBTService_MirrorMovement.generated.h"

class ACharacter;
class UAbilitySystemComponent;
class UGameplayAbility;
class UGameplayEffect;
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
	/** 일치하는 AssetTags의 발동 시 전방으로 이동한다. 같은 소유 태그가 유지되는 동안 추적 대신 마스터를 바라본다. */
	UPROPERTY(EditAnywhere, Category="Wx|AI")
	FGameplayTagContainer FaceMasterAbilityTags;
	UPROPERTY(EditAnywhere, Category="Wx|AI", meta=(ClampMin="1.0", Units="cm"))
	float AbilityTeleportDistance = 500.f;
	/**
	 * Master 속도를 따르도록 SPD 를 SetByCaller 로 덮어쓰고 서비스가 끝나면 제거한다.
	 * WxAI 는 WxCombat 에 의존하지 않으므로 디자이너가 BT 에디터에서 지정한다(WxEffect_MoveSpeedOverride). 지정하지 않으면 자기 속도로 따라간다.
	 */
	UPROPERTY(EditAnywhere, Category="Wx|AI")
	TSubclassOf<UGameplayEffect> MoveSpeedEffect;
private:
	friend class FWxMirrorMovementAbilityTeleportTest;
	void Release(UBehaviorTreeComponent& OwnerComp);
	void HandleAbilityActivated(UGameplayAbility* Ability);
	void HandleAbilityEnded(const FAbilityEndedData& Data);
	TWeakObjectPtr<ACharacter> Master;
	TWeakObjectPtr<ACharacter> Follower;
	TWeakObjectPtr<UAbilitySystemComponent> FollowerAbilitySystem;
	FActiveGameplayEffectHandle MoveSpeedEffectHandle;
	bool bPendingAbilityEndTeleport = false;
	float TravelTime = 0.f;
	int32 PreviousJumpCount = 0;
};
