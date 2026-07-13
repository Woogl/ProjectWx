// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "WxContextEffectsComponent.generated.h"

class UWxContextEffectsLibrary;

/**
 * 컨텍스트 이펙트(표면별 오디오/VFX) 재생 컴포넌트.
 *
 * WxAnimNotify_ContextEffects(또는 WxAnimNotify_Footstep 등)가 TriggerEffect 를 호출하면,
 * 지정 위치에서 하방 트레이스로 표면을 판정하고 보유 라이브러리에서 (EffectTag, 표면) 애셋을 찾아
 * 사운드와 Niagara VFX 를 로컬 재생한다(데디케이티드 서버 제외). 순수 코스메틱이라 복제하지 않는다.
 *
 * AWxCharacterBase 가 기본 부착한다. 라이브러리는 캐릭터 BP 에서 지정한다.
 */
UCLASS(ClassGroup = (Wx), meta = (BlueprintSpawnableComponent))
class WXGAME_API UWxContextEffectsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWxContextEffectsComponent();

	/** 지정 위치에서 표면을 판정해 (EffectTag, 표면)에 매핑된 사운드/VFX 를 로컬 재생한다. */
	void TriggerEffect(const FGameplayTag& EffectTag, const FVector& Location) const;

protected:
	/** 이 액터가 사용할 컨텍스트 이펙트 라이브러리 목록. 앞선 것부터 조회해 처음 매칭을 사용한다. */
	UPROPERTY(EditAnywhere, Category = "Wx|ContextEffects")
	TArray<TObjectPtr<UWxContextEffectsLibrary>> Libraries;
};
