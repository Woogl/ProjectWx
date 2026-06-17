// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "WxAbilityTargetData_Interaction.generated.h"

class UWxInteractionComponent;

/**
 * 선택된 상호작용 컴포넌트를 전달하는 TargetData.
 * 클라이언트가 로컬 레지스트리에서 고른 대상 컴포넌트를 서버로 전송할 때 사용.
 * 컴포넌트는 복제 객체이므로 포인터를 PackageMap으로 직렬화한다.
 */
USTRUCT()
struct WXGAME_API FWxAbilityTargetData_Interaction : public FGameplayAbilityTargetData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TObjectPtr<UWxInteractionComponent> Component = nullptr;

	virtual UScriptStruct* GetScriptStruct() const override;
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FWxAbilityTargetData_Interaction> : public TStructOpsTypeTraitsBase2<FWxAbilityTargetData_Interaction>
{
	enum
	{
		WithNetSerializer = true,
	};
};
