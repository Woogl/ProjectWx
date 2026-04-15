// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "WxBlueprintSnapshotSettings.generated.h"

/**
 * Project Settings > Plugins > Wx Blueprint Snapshot.
 * 스냅샷 활성화, 대상 경로, 히스토리 보관 개수를 제어한다.
 */
UCLASS(Config = Editor, DefaultConfig, Meta = (DisplayName = "Wx Blueprint Snapshot"))
class WXBLUEPRINTSNAPSHOT_API UWxBlueprintSnapshotSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UWxBlueprintSnapshotSettings();

	virtual FName GetCategoryName() const override;

	/** 전체 기능 on/off */
	UPROPERTY(EditAnywhere, Config, Category = "General")
	bool bEnabled = true;

	/** 대상 BP 에셋 경로 prefix. 비어있으면 모든 BP가 대상 */
	UPROPERTY(EditAnywhere, Config, Category = "Filter")
	TArray<FString> IncludePathPrefixes;

	/** 제외할 BP 에셋 경로 prefix */
	UPROPERTY(EditAnywhere, Config, Category = "Filter")
	TArray<FString> ExcludePathPrefixes;

	/** JSON 키를 정렬해 git diff 친화적으로 출력 */
	UPROPERTY(EditAnywhere, Config, Category = "Output")
	bool bGitFriendly = true;

	/** SimpleConstructionScript 컴포넌트 트리 포함 */
	UPROPERTY(EditAnywhere, Config, Category = "Scope")
	bool bIncludeComponents = true;

	/** BP NewVariables 포함 */
	UPROPERTY(EditAnywhere, Config, Category = "Scope")
	bool bIncludeVariables = true;

	/** ImplementedInterfaces 포함 */
	UPROPERTY(EditAnywhere, Config, Category = "Scope")
	bool bIncludeInterfaces = true;

	/** WBP의 WidgetTree(위젯 계층 + 각 위젯의 CDO delta) 포함 */
	UPROPERTY(EditAnywhere, Config, Category = "Scope")
	bool bIncludeWidgetTree = true;

	/** WBP의 MVVM View 확장 (ViewModel 컨텍스트 + 바인딩) 포함 */
	UPROPERTY(EditAnywhere, Config, Category = "Scope")
	bool bIncludeMvvm = true;

	/** Event Graph / Function Graph를 의사 코드 문자열로 포함 */
	UPROPERTY(EditAnywhere, Config, Category = "Scope")
	bool bIncludeGraphs = true;
};
