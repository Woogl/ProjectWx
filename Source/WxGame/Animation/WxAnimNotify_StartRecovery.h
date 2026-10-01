// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/WxAnimNotify_AbilityEvent.h"
#include "WxAnimNotify_StartRecovery.generated.h"

/**
 * 후딜은 몽타주의 마지막 구간이라 종료가 곧 어빌리티 종료다.
 * 닫는 지점이 따로 필요 없어 State가 아닌 단발 Notify로 둔다.
 */
UCLASS()
class WXGAME_API UWxAnimNotify_StartRecovery : public UWxAnimNotify_AbilityEvent
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;
#if WITH_EDITOR
	virtual FLinearColor GetEditorColor() override;
#endif

};
