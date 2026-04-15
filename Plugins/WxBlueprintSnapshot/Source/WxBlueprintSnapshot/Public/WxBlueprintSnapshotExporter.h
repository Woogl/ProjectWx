// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UBlueprint;
class UClass;
class UObject;
class USimpleConstructionScript;
class USCS_Node;
class UWidgetBlueprint;
class UWidget;
class UWidgetTree;
class FJsonObject;

/**
 * Blueprint → JSON 스냅샷 생성기.
 * CDO delta, SCS 컴포넌트 트리, 변수 선언, 인터페이스를 추출한다.
 */
class FWxBlueprintSnapshotExporter
{
public:
	/** Blueprint를 스냅샷 JSON으로 추출해 설정된 경로에 기록한다. 변경 없으면 skip하고 false 반환 */
	static bool ExportBlueprint(UBlueprint* Blueprint);

private:
	static TSharedRef<FJsonObject> BuildSnapshot(UBlueprint* Blueprint);

	static TSharedPtr<FJsonObject> BuildClassDefaults(const UObject* Instance, const UObject* Defaults);

	static TSharedPtr<FJsonObject> BuildComponentsJson(USimpleConstructionScript* SCS);
	static TSharedPtr<FJsonObject> BuildScsNodeJson(USCS_Node* Node);

	static TSharedPtr<FJsonObject> BuildVariablesJson(UBlueprint* Blueprint);
	static TSharedPtr<FJsonObject> BuildInterfacesJson(UBlueprint* Blueprint);

	static TSharedPtr<FJsonObject> BuildWidgetTreeJson(UWidgetTree* WidgetTree);
	static TSharedPtr<FJsonObject> BuildWidgetJson(UWidget* Widget);
	static TSharedPtr<FJsonObject> BuildMvvmJson(UWidgetBlueprint* WidgetBlueprint);

	static TSharedPtr<FJsonObject> BuildGraphsJson(UBlueprint* Blueprint);

	static FString SerializeJson(TSharedRef<FJsonObject> RootObject, bool bSortKeys);

	static FString ResolveLatestPath(UBlueprint* Blueprint);
};
