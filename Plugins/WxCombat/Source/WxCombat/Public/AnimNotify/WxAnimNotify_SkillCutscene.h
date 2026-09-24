// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "WxAnimNotify_SkillCutscene.generated.h"

class ULevelSequence;

/**
 * 궁극기 몽타주가 재생 전에 틀 컷신을 담는 표식이다. 불려도 하는 일이 없다.
 * 컷신은 시전자 포즈를 넘겨받으며 몽타주를 멈추므로, UWxAbility_Ultimate가 재생 전에 읽어 컷신부터 튼다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotify_SkillCutscene : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual FLinearColor GetEditorColor() override;
	virtual FString GetNotifyName_Implementation() const override;

	UPROPERTY(EditAnywhere, Category = "Wx")
	TObjectPtr<ULevelSequence> Sequence;
};
