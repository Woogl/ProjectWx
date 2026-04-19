// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/EngineTypes.h"
#include "WxLevelSnapshotSettings.generated.h"

/**
 * 레벨 액터 스냅샷을 JSON으로 기록하는 플러그인 설정.
 * 저장된 JSON은 diff로 변경점을 추적하거나 서버에게 레벨 정보를 전달할 때 사용 가능.
 */
UCLASS(Config = Editor, Meta = (DisplayName = "Wx Level Snapshot"))
class WXLEVELSNAPSHOT_API UWxLevelSnapshotSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UWxLevelSnapshotSettings();

	/** 전체 기능 on/off */
	UPROPERTY(EditAnywhere, Config, Category = "General")
	bool bEnabled = true;

	/** 스냅샷 파일 확장자. 점을 포함해 입력 (예: .json) */
	UPROPERTY(EditAnywhere, Config, Category = "General")
	FString FileExtension = TEXT(".json");

	/** 스냅샷을 저장할 폴더. 비워두면 `<Plugin>/Snapshots`를 사용 */
	UPROPERTY(EditAnywhere, Config, Category = "General")
	FDirectoryPath OutputDirectory;

	/**
	 * 모든 레벨의 액터를 하나의 JSON 파일로 합쳐서 저장할지 여부.
	 * true(기본)면 `<OutputDirectory>/AllLevels<FileExtension>` 단일 파일로 합침. false면 레벨별 별도 파일.
	 */
	UPROPERTY(EditAnywhere, Config, Category = "General")
	bool bCombineAllLevels = true;

	/** 대상 레벨 폴더. 비어있으면 모든 레벨이 대상 */
	UPROPERTY(EditAnywhere, Config, Category = "Filter", Meta = (ContentDir, LongPackageName))
	TArray<FDirectoryPath> IncludeDirectories;

	/** 제외할 레벨 폴더 */
	UPROPERTY(EditAnywhere, Config, Category = "Filter", Meta = (ContentDir, LongPackageName))
	TArray<FDirectoryPath> ExcludeDirectories;

	/**
	 * 액터 내부 프로퍼티를 모두 재귀적으로 추출해 `properties` 필드에 기록할지 여부.
	 * false면 class/fname/label/transform/attach만 기록. true면 컴포넌트·서브오브젝트·구조체·배열까지 전부 덤프.
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Scope")
	bool bIncludeAllProperties = false;

	/**
	 * 클래스별 키 프로퍼티.
	 * 지정된 액터 클래스의 인스턴스는 FName 대신 해당 프로퍼티 값을 JSON 키로 사용한다.
	 * 프로퍼티를 찾지 못하거나 값이 비어있으면 FName으로 폴백.
	 * 클래스 체인을 따라 가장 구체적인 매칭이 우선.
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Key")
	TMap<TSoftClassPtr<AActor>, FName> KeyProperties;
};
