// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AActor;
class ULevel;
class UObject;
class UWxLevelSnapshotSettings;
class FJsonObject;

/**
 * Level → JSON 스냅샷 생성기.
 * 레벨별 JSON 파일을 유지하고, 액터 단위 업데이트 또는 레벨 전체 재생성을 지원한다.
 */
class FWxLevelSnapshotExporter
{
public:
	/** ExternalActor 패키지 저장 시 대응하는 레벨 JSON의 해당 액터 항목을 업데이트한다. 실패 시 false. */
	static bool ExportActorIntoLevel(AActor* Actor);

	/** 레벨 전체를 덤프해 JSON을 재생성한다. 실패 시 false. */
	static bool ExportLevel(ULevel* Level);

	/** LevelPackageName(예: "/Game/Maps/MyLevel") 에 대응되는 스냅샷 파일 절대경로. */
	static FString ResolveSnapshotPath(const FString& LevelPackageName);

	/** 레벨 JSON에서 주어진 GUID(하이픈 포맷)를 가진 액터 항목을 제거하고 저장한다. 남은 항목이 없으면 파일 삭제. */
	static bool RemoveActorFromLevel(const FString& LevelPackageName, const FString& ActorGuidString);

	/** 레벨 JSON 파일을 통째로 삭제한다. */
	static bool DeleteLevelSnapshot(const FString& LevelPackageName);

	/** UObject의 edit/visible 프로퍼티를 재귀적으로 JSON 오브젝트로 추출한다. 인스턴스드 서브오브젝트는 `{class, properties}` 블록으로 중첩. */
	static TSharedPtr<FJsonObject> BuildProperties(const UObject* Instance);

	/** 단일 AActor를 JSON 오브젝트로 변환 (class, actorLabel, transform, attach, delta 등). */
	static TSharedPtr<FJsonObject> BuildActorJson(AActor* Actor);

	/**
	 * 액터를 레벨 JSON 내에서 식별하는 키. KeyProperties에 매칭된 규칙의 프로퍼티 값을 반환.
	 * 매칭 규칙이 없거나 프로퍼티 추출에 실패하면 빈 문자열 반환 (호출자는 스냅샷에서 스킵).
	 */
	static FString GetActorKey(AActor* Actor);

private:
	static FString SerializeJson(TSharedRef<FJsonObject> RootObject);
	static bool WriteLevelJson(const FString& LevelPackageName, TSharedRef<FJsonObject> Root);
	static TSharedPtr<FJsonObject> LoadLevelJson(const FString& LevelPackageName);
	static TSharedRef<FJsonObject> BuildLevelRoot(ULevel* Level);
};
