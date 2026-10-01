// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/WxAnimNotify_AbilityEvent.h"
#include "WxAnimNotify_SpawnMinion.generated.h"

/**
 * 권위 판정과 같은 종류 교체는 이 노티파이가 아니라 UWxMinionComponent::SpawnMinion 이 한다.
 */
UCLASS()
class WXGAME_API UWxAnimNotify_SpawnMinion : public UWxAnimNotify_AbilityEvent
{
	GENERATED_BODY()

public:
	UWxAnimNotify_SpawnMinion();

#if WITH_EDITOR
	virtual FLinearColor GetEditorColor() override;
#endif

	virtual FString GetNotifyName_Implementation() const override;

	/**
	 * CDO에 MinionComponent가 네이티브로 붙어 있어야 소환된다 — 동시 유지 수와 소환 조건을 그 컴포넌트가 선언한다.
	 * 팀을 물려받을 수 있는지는 서브시스템이 런타임에 보며, 못 물려받아도 소환은 된다.
	 */
	UPROPERTY(EditAnywhere, Category = "Wx", meta = (MustImplement = "/Script/WxGame.WxSpawnable", AllowAbstract = "false"))
	TSubclassOf<APawn> MinionClass;

	/** 소환자 로컬 기준 스폰 지점. 실제 위치는 스폰 시 충돌 보정으로 밀릴 수 있다. */
	UPROPERTY(EditAnywhere, Category = "Wx")
	FTransform LocalSpawnOffset;
};
