// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/WxAnimNotify_AbilityEvent.h"
#include "WxAnimNotifyState_ComboWindow.generated.h"

/**
 * 구간 동안 이 몽타주를 재생 중인 콤보 어빌리티가 다음 타 입력을 받아, 한 활성화 안에서 다음 단으로 넘어간다.
 * 창이 닫힌 뒤의 발동은 첫 단부터 시작한다.
 * 회피·가드 같은 남의 캔슬은 열지 않는다 — 그쪽은 더 늦게 WxAnimNotify_StartRecovery가 연다.
 */
UCLASS()
class WXGAME_API UWxAnimNotifyState_ComboWindow : public UWxAnimNotifyState_AbilityEvent
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;
#if WITH_EDITOR
	virtual FLinearColor GetEditorColor() override;
#endif

};
