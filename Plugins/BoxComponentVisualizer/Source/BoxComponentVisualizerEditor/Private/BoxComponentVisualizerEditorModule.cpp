// Copyright Woogle. All Rights Reserved.

#include "BoxComponentVisualizerEditorModule.h"

#include "BoxComponentVisualizer.h"
#include "Components/BoxComponent.h"
#include "Editor/UnrealEdEngine.h"
#include "Modules/ModuleManager.h"
#include "UnrealEdGlobals.h"

IMPLEMENT_MODULE(FBoxComponentVisualizerEditorModule, BoxComponentVisualizerEditor)

void FBoxComponentVisualizerEditorModule::StartupModule()
{
	// 엔진은 UBoxComponent 자리를 비워 두었고, 에디터가 클래스 사슬을 거슬러 찾으므로 파생 박스까지 함께 덮인다.
	if (GUnrealEd)
	{
		GUnrealEd->RegisterComponentVisualizer(UBoxComponent::StaticClass()->GetFName(), MakeShared<FBoxComponentVisualizer>());
	}
}

void FBoxComponentVisualizerEditorModule::ShutdownModule()
{
	if (GUnrealEd)
	{
		GUnrealEd->UnregisterComponentVisualizer(UBoxComponent::StaticClass()->GetFName());
	}
}
