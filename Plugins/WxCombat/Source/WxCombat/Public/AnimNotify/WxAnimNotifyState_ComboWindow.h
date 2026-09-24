// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "WxAnimNotifyState_ComboWindow.generated.h"

/**
 * 구간 동안 이 몽타주를 재생 중인 어빌리티의 자기 재발동을 열어 다음 콤보 단으로 넘어갈 수 있게 한다.
 * 창이 닫힌 뒤의 발동은 첫 단부터 시작한다.
 * 회피·가드 같은 남의 캔슬은 열지 않는다 — 그쪽은 더 늦게 WxAnimNotify_StartRecovery가 연다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotifyState_ComboWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual FLinearColor GetEditorColor() override;
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	/** 엔진 기본 구현은 빈 이벤트 참조를 넘겨 몽타주 인스턴스를 잃는다. */
	virtual void BranchingPointNotifyBegin(FBranchingPointNotifyPayload& BranchingPointPayload) override;
	virtual void BranchingPointNotifyEnd(FBranchingPointNotifyPayload& BranchingPointPayload) override;

private:
	void OpenWindow(USkeletalMeshComponent* MeshComp, int32 MontageInstanceID);
	void CloseWindow(USkeletalMeshComponent* MeshComp, int32 MontageInstanceID);
};
