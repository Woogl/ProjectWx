// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WxAbilityComponent.generated.h"

/**
 * 어빌리티에 부착하는 컴포넌트의 추상 베이스.
 * UGameplayAbility 는 액터가 아니라 ActorComponent 를 붙일 수 없어, 어빌리티(UWxAbilityBase)가 이 타입 배열을 Instanced 서브오브젝트로 보유한다.
 * UGameplayEffectComponent 가 GameplayEffect 에 대해 하는 역할을 어빌리티에 대해 수행한다.
 *
 * 실제 데이터/로직을 가진 구체 컴포넌트는 각 도메인 모듈에서 이 클래스를 상속해 정의한다.  (예: UI 표시 데이터는 WxUI 의 UWxAbilityComponent_UIData)
 * WxCombat 이 다른 도메인을 참조할 수 없으므로, 도메인 간 공유 앵커로서 WxCore 에 둔다.
 */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, BlueprintType)
class WXCORE_API UWxAbilityComponent : public UObject
{
	GENERATED_BODY()
};
