// Copyright Woogle. All Rights Reserved.

#pragma once

#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Engine/EngineTypes.h"
#include "WxAnimNotifyState_Rush.generated.h"

UENUM(BlueprintType)
enum class EWxRushTarget : uint8
{
	LockOnTarget,
	Master,
	Minion,
};

/** 돌진 구간의 시작·끝과 설정을 알린다. 대상 결정과 이동 실행은 몽타주 소유 어빌리티가 맡는다. */
UCLASS()
class WXGAME_API UWxAnimNotifyState_Rush : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;
	UWxAnimNotifyState_Rush();
#if WITH_EDITOR
	virtual FLinearColor GetEditorColor() override;
#endif
	virtual void BranchingPointNotifyBegin(FBranchingPointNotifyPayload& Payload) override;
	virtual void BranchingPointNotifyEnd(FBranchingPointNotifyPayload& Payload) override;
	UPROPERTY(EditAnywhere, Category = "Wx")
	EWxRushTarget TargetSource = EWxRushTarget::LockOnTarget;

	/** 대상 앞에서 멈출 거리. 교차 돌진에는 0을 사용한다. */
	UPROPERTY(EditAnywhere, Category = "Wx", meta = (ClampMin = "0"))
	float StopDistance = 0.f;

	/** 돌진 구간 동안 캡슐이 무시할 오브젝트 종류. 빈 목록은 기존 충돌을 유지한다. */
	UPROPERTY(EditAnywhere, Category = "Wx")
	TArray<TEnumAsByte<EObjectTypeQuery>> IgnoreCollisions;

private:
	void SendSignal(const FBranchingPointNotifyPayload& Payload, bool bBegin) const;
};
