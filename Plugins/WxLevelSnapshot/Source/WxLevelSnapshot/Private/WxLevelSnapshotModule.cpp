// Copyright Woogle. All Rights Reserved.

#include "WxLevelSnapshotModule.h"
#include "WxLevelSnapshotSettings.h"
#include "WxLevelSnapshotExporter.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "GameFramework/Actor.h"
#include "UObject/Package.h"
#include "UObject/ObjectSaveContext.h"
#include "Modules/ModuleManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetRegistry/AssetData.h"
#include "HAL/PlatformFileManager.h"

IMPLEMENT_MODULE(FWxLevelSnapshotModule, WxLevelSnapshot)

DEFINE_LOG_CATEGORY(LogWxLevelSnapshot);

namespace
{
	const TCHAR* ExternalActorsMarker = TEXT("/__ExternalActors__/");

	bool IsExternalActorPackage(const FString& PackageName)
	{
		return PackageName.Contains(ExternalActorsMarker);
	}

	/**
	 * ExternalActor 패키지 short name(`UAID_<32 hex>`)에서 액터 GUID를 복원한다.
	 * 실패 시 빈 문자열 반환.
	 */
	FString ParseActorGuidFromExternalActorPackage(const FString& ExternalActorPackageName)
	{
		FString ShortName = ExternalActorPackageName;
		int32 LastSlash = INDEX_NONE;
		if (ShortName.FindLastChar(TEXT('/'), LastSlash))
		{
			ShortName = ShortName.Mid(LastSlash + 1);
		}
		const FString Prefix = TEXT("UAID_");
		if (!ShortName.StartsWith(Prefix))
		{
			return FString();
		}
		const FString Digits = ShortName.Mid(Prefix.Len());
		FGuid ParsedGuid;
		if (!FGuid::ParseExact(Digits, EGuidFormats::Digits, ParsedGuid))
		{
			return FString();
		}
		return ParsedGuid.ToString(EGuidFormats::DigitsWithHyphens);
	}

	/**
	 * ExternalActor 패키지 이름에서 소유 레벨의 패키지 이름을 뽑는다.
	 * 예: "/Game/__ExternalActors__/Maps/MyLevel/AB/CDE/UAID_XXX" -> "/Game/Maps/MyLevel"
	 * 5.7 컨벤션: `__ExternalActors__` 뒤에 `<LevelPath>/<2char>/<3char>/<ShortName>` 형태로 3개 세그먼트가 후행한다.
	 */
	FString DeriveLevelPackageFromExternalActor(const FString& ExternalActorPackageName)
	{
		const int32 MarkerIdx = ExternalActorPackageName.Find(ExternalActorsMarker);
		if (MarkerIdx == INDEX_NONE)
		{
			return FString();
		}
		const FString Mount = ExternalActorPackageName.Left(MarkerIdx);
		const FString Remainder = ExternalActorPackageName.Mid(MarkerIdx + FCString::Strlen(ExternalActorsMarker));

		TArray<FString> Parts;
		Remainder.ParseIntoArray(Parts, TEXT("/"), true);
		if (Parts.Num() < 4)
		{
			return FString();
		}
		FString LevelPath;
		for (int32 i = 0; i < Parts.Num() - 3; ++i)
		{
			LevelPath += TEXT("/");
			LevelPath += Parts[i];
		}
		return Mount + LevelPath;
	}

	bool ShouldProcessContext(const FObjectPostSaveContext& Context)
	{
		if (Context.IsProceduralSave() || Context.IsCooking() || Context.IsFromAutoSave())
		{
			return false;
		}
		if (IsRunningCommandlet())
		{
			return false;
		}
		return true;
	}
}

void FWxLevelSnapshotModule::StartupModule()
{
	PackageSavedHandle = UPackage::PackageSavedWithContextEvent.AddRaw(this, &FWxLevelSnapshotModule::HandlePackageSaved);

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	AssetRemovedHandle = AssetRegistry.OnAssetRemoved().AddRaw(this, &FWxLevelSnapshotModule::HandleAssetRemoved);
}

void FWxLevelSnapshotModule::ShutdownModule()
{
	if (PackageSavedHandle.IsValid())
	{
		UPackage::PackageSavedWithContextEvent.Remove(PackageSavedHandle);
		PackageSavedHandle.Reset();
	}
	if (AssetRemovedHandle.IsValid())
	{
		if (FAssetRegistryModule* Module = FModuleManager::GetModulePtr<FAssetRegistryModule>(TEXT("AssetRegistry")))
		{
			Module->Get().OnAssetRemoved().Remove(AssetRemovedHandle);
		}
		AssetRemovedHandle.Reset();
	}
}

void FWxLevelSnapshotModule::HandlePackageSaved(const FString& PackageFileName, UPackage* Package, FObjectPostSaveContext Context)
{
	if (!Package)
	{
		return;
	}

	const UWxLevelSnapshotSettings* Settings = GetDefault<UWxLevelSnapshotSettings>();
	if (!Settings || !Settings->bEnabled)
	{
		return;
	}

	if (!ShouldProcessContext(Context))
	{
		return;
	}

	if (Package->HasAnyPackageFlags(PKG_PlayInEditor | PKG_ForDiffing | PKG_Cooked))
	{
		return;
	}

	const FString PackageName = Package->GetName();

	// ExternalActor 패키지: 해당 액터만 레벨 JSON에 merge.
	if (IsExternalActorPackage(PackageName))
	{
		const FString LevelPackageName = DeriveLevelPackageFromExternalActor(PackageName);
		if (LevelPackageName.IsEmpty() || !FWxLevelSnapshotExporter::IsLevelPackageIncluded(LevelPackageName))
		{
			return;
		}

		TArray<UObject*> Objects;
		GetObjectsWithOuter(Package, Objects, false);
		for (UObject* Obj : Objects)
		{
			AActor* Actor = Cast<AActor>(Obj);
			if (!Actor || Actor->HasAnyFlags(RF_Transient))
			{
				continue;
			}
			if (FWxLevelSnapshotExporter::ExportActorIntoLevel(Actor))
			{
				UE_LOG(LogWxLevelSnapshot, Log, TEXT("Actor snapshot updated: %s in %s"), *Actor->GetPathName(), *LevelPackageName);
			}
		}
		return;
	}

	// .umap 패키지: UWorld를 찾아 레벨 덤프. WP 레벨은 액터가 외부 파일로 따로 저장되므로 umap 이벤트에서는 스킵.
	if (!FWxLevelSnapshotExporter::IsLevelPackageIncluded(PackageName))
	{
		return;
	}

	TArray<UObject*> Objects;
	GetObjectsWithOuter(Package, Objects, false);
	for (UObject* Obj : Objects)
	{
		UWorld* World = Cast<UWorld>(Obj);
		if (!World || !World->PersistentLevel)
		{
			continue;
		}
		ULevel* Level = World->PersistentLevel;
		if (Level->IsUsingExternalActors())
		{
			// WP 레벨: 외부 액터 save 이벤트가 JSON 업데이트를 담당. umap에서는 트리거하지 않음.
			UE_LOG(LogWxLevelSnapshot, Verbose, TEXT("Skipping umap save for WP level %s"), *PackageName);
			continue;
		}
		if (FWxLevelSnapshotExporter::ExportLevel(Level))
		{
			UE_LOG(LogWxLevelSnapshot, Log, TEXT("Level snapshot written: %s"), *PackageName);
		}
	}
}

void FWxLevelSnapshotModule::HandleAssetRemoved(const FAssetData& AssetData)
{
	const UWxLevelSnapshotSettings* Settings = GetDefault<UWxLevelSnapshotSettings>();
	if (!Settings || !Settings->bEnabled)
	{
		return;
	}

	if (IsRunningCommandlet())
	{
		return;
	}

	const FString PackageName = AssetData.PackageName.ToString();

	// 레벨 삭제: 대응 JSON 전체 제거.
	if (AssetData.IsInstanceOf<UWorld>())
	{
		if (!FWxLevelSnapshotExporter::IsLevelPackageIncluded(PackageName))
		{
			return;
		}
		if (FWxLevelSnapshotExporter::DeleteLevelSnapshot(PackageName))
		{
			UE_LOG(LogWxLevelSnapshot, Log, TEXT("Level snapshot deleted: %s"), *PackageName);
		}
		return;
	}

	// ExternalActor 삭제: 소유 레벨의 JSON에서 해당 액터 entry 제거 (GUID 매칭).
	if (AssetData.IsInstanceOf<AActor>() && IsExternalActorPackage(PackageName))
	{
		const FString LevelPackageName = DeriveLevelPackageFromExternalActor(PackageName);
		if (LevelPackageName.IsEmpty() || !FWxLevelSnapshotExporter::IsLevelPackageIncluded(LevelPackageName))
		{
			return;
		}
		const FString ActorGuidString = ParseActorGuidFromExternalActorPackage(PackageName);
		if (ActorGuidString.IsEmpty())
		{
			UE_LOG(LogWxLevelSnapshot, Warning, TEXT("Failed to parse GUID from external actor package: %s"), *PackageName);
			return;
		}
		if (FWxLevelSnapshotExporter::RemoveActorFromLevel(LevelPackageName, ActorGuidString))
		{
			UE_LOG(LogWxLevelSnapshot, Log, TEXT("Actor entry removed from %s: %s"), *LevelPackageName, *ActorGuidString);
		}
	}
}
