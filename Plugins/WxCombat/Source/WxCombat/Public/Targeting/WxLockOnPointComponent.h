// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "WxLockOnPointComponent.generated.h"

/**
 * 락온 지점 컴포넌트.
 * 액터에서 "락온이 걸릴 위치"를 표시하는 마커다. 락온 대상은 이 컴포넌트 자체가 되며,
 * 카메라·캐릭터가 바라보는 지점은 이 컴포넌트의 월드 위치다(부위 위치에 그대로 조준·추적).
 * 한 액터에 붙여 부위별 락온을 구성한다. 이 컴포넌트가 없는 액터는 락온 대상이 될 수 없다.
 *
 * 배치: C++ 디폴트 서브오브젝트 또는 BP 컴포넌트 트리(SCS)로 추가한다.
 * 두 경우 모두 네트워크 주소가 안정적이라 UWxLockOnManagerComponent가 복제하는 대상 레퍼런스가 원격에서 정상 해소된다.
 * 런타임에 NewObject로 동적 생성한 지점은 복제 등록을 하지 않는 한 원격(서버 권위 소비처/시뮬프록시)에서 null로 도착하니 주의한다.
 */
UCLASS(ClassGroup = (Wx), meta = (BlueprintSpawnableComponent, DisplayName = "Wx Lock-On Point"))
class WXCOMBAT_API UWxLockOnPointComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UWxLockOnPointComponent();

	/**
	 * 액터를 락온할 때 대상이 될 SceneComponent를 반환한다.
	 * 락온 지점이 있으면 그것을, 없으면 nullptr을 반환한다(지점이 없는 액터는 락온 불가).
	 */
	static USceneComponent* ResolveLockOnTarget(const AActor* Actor);
};
