// Copyright Woogle. All Rights Reserved.

#include "Device/WxDeviceComponentName.h"

#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "UObject/UnrealType.h"

USceneComponent* FWxStateTreeComponentName::Resolve(const AActor* Actor) const
{
	if (!Actor || Name.IsNone())
	{
		return nullptr;
	}

	// 드롭다운이 고르는 것이 액터 클래스의 컴포넌트 프로퍼티라, 그 프로퍼티를 되읽는 것이 정확한 역이다(컴포넌트 오브젝트 이름 대조는 이름 충돌 접미사에 걸린다).
	const FObjectPropertyBase* ComponentProperty = FindFProperty<FObjectPropertyBase>(Actor->GetClass(), Name);
	if (!ComponentProperty)
	{
		return nullptr;
	}

	return Cast<USceneComponent>(ComponentProperty->GetObjectPropertyValue_InContainer(Actor));
}
