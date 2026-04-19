// Copyright Woogle. All Rights Reserved.

#include "WxLevelSnapshotSettings.h"
#include "GameFramework/Actor.h"

UWxLevelSnapshotSettings::UWxLevelSnapshotSettings()
{
	CategoryName = TEXT("Wx");
	SectionName = TEXT("WxLevelSnapshot");
	OutputDirectory.Path = TEXT("Plugins/WxLevelSnapshot/Snapshots");

	// 디폴트: 모든 액터는 ActorGuid를 키로 사용. 하위 클래스에서 덮어써 커스텀 프로퍼티 지정 가능.
	KeyProperties.Add(TSoftClassPtr<AActor>(AActor::StaticClass()), FName(TEXT("ActorGuid")));
}
