// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/WxAnimNotify_AbilityEvent.h"
#include "WxAnimNotifyState_ComboWindow.generated.h"

/**
 * 구간 동안 이 몽타주를 재생 중인 어빌리티의 자기 재발동을 열어 다음 콤보 단으로 넘어갈 수 있게 한다.
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
