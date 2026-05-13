// Copyright Woogle. All Rights Reserved.

#include "WxBlueprintSnapshotModule.h"
#include "WxBlueprintSnapshotSettings.h"
#include "WxBlueprintSnapshotExporter.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "EditorUtilityBlueprint.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/ObjectSaveContext.h"
#include "Modules/ModuleManager.h"
#include "Misc/CommandLine.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetRegistry/AssetData.h"
#include "HAL/PlatformFileManager.h"

IMPLEMENT_MODULE(FWxBlueprintSnapshotModule, WxBlueprintSnapshot)

DEFINE_LOG_CATEGORY(LogWxBPSnapshot);

void FWxBlueprintSnapshotModule::StartupModule()
{
	PackageSavedHandle = UPackage::PackageSavedWithContextEvent.AddRaw(this, &FWxBlueprintSnapshotModule::HandlePackageSaved);
	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FWxBlueprintSnapshotModule::HandleTick), 0.0f);

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	AssetRemovedHandle = AssetRegistry.OnAssetRemoved().AddRaw(this, &FWxBlueprintSnapshotModule::HandleAssetRemoved);
}

void FWxBlueprintSnapshotModule::ShutdownModule()
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
	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();
	}
	PendingQueue.Empty();
	PendingPaths.Empty();
}

bool FWxBlueprintSnapshotModule::ShouldProcessContext(const FObjectPostSaveContext& Context) const
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

bool FWxBlueprintSnapshotModule::ShouldProcessBlueprint(UBlueprint* Blueprint) const
{
	if (!Blueprint)
	{
		return false;
	}

	UPackage* Package = Blueprint->GetOutermost();
	if (Package && Package->HasAnyPackageFlags(PKG_PlayInEditor | PKG_ForDiffing | PKG_Cooked))
	{
		return false;
	}

	switch (Blueprint->BlueprintType)
	{
	case BPTYPE_Interface:
	case BPTYPE_FunctionLibrary:
		return false;
	default:
		break;
	}

	if (Blueprint->IsA<UEditorUtilityBlueprint>())
	{
		return false;
	}

	// MacroLibrary 는 CDO/SCS/Variables 가 없고 MacroGraphs 만 의미가 있다. CDO 게이트는 우회.
	if (Blueprint->BlueprintType != BPTYPE_MacroLibrary)
	{
		if (!Blueprint->GeneratedClass || !Blueprint->GeneratedClass->GetDefaultObject(false))
		{
			return false;
		}
	}

	if (Blueprint->Status == BS_Dirty || Blueprint->Status == BS_Error || Blueprint->Status == BS_Unknown)
	{
		UE_LOG(LogWxBPSnapshot, Verbose, TEXT("Skip %s: compile required (status=%d)"), *Blueprint->GetPathName(), static_cast<int32>(Blueprint->Status));
		return false;
	}

	return true;
}

bool FWxBlueprintSnapshotModule::IsPackageNameIncluded(const FString& PackageName) const
{
	const UWxBlueprintSnapshotSettings* Settings = GetDefault<UWxBlueprintSnapshotSettings>();
	if (!Settings)
	{
		return false;
	}

	const FString PackageWithSlash = PackageName + TEXT("/");
	auto MatchesAnyDir = [&PackageWithSlash](const TArray<FDirectoryPath>& Dirs) -> bool
	{
		for (const FDirectoryPath& Dir : Dirs)
		{
			if (Dir.Path.IsEmpty())
			{
				continue;
			}
			const FString Prefix = Dir.Path.EndsWith(TEXT("/")) ? Dir.Path : Dir.Path + TEXT("/");
			if (PackageWithSlash.StartsWith(Prefix))
			{
				return true;
			}
		}
		return false;
	};

	if (Settings->IncludeDirectories.Num() > 0 && !MatchesAnyDir(Settings->IncludeDirectories))
	{
		return false;
	}
	if (MatchesAnyDir(Settings->ExcludeDirectories))
	{
		return false;
	}
	return true;
}

void FWxBlueprintSnapshotModule::HandlePackageSaved(const FString& PackageFileName, UPackage* Package, FObjectPostSaveContext Context)
{
	if (!Package)
	{
		return;
	}

	const UWxBlueprintSnapshotSettings* Settings = GetDefault<UWxBlueprintSnapshotSettings>();
	if (!Settings || !Settings->bEnabled)
	{
		return;
	}

	if (!ShouldProcessContext(Context))
	{
		return;
	}

	const FString PackageName = Package->GetName();
	if (!IsPackageNameIncluded(PackageName))
	{
		return;
	}

	TArray<UObject*> AssetsInPackage;
	GetObjectsWithOuter(Package, AssetsInPackage, false);
	for (UObject* Asset : AssetsInPackage)
	{
		UBlueprint* Blueprint = Cast<UBlueprint>(Asset);
		if (!Blueprint)
		{
			continue;
		}
		if (!ShouldProcessBlueprint(Blueprint))
		{
			continue;
		}
		EnqueueBlueprint(Blueprint);
	}
}

void FWxBlueprintSnapshotModule::HandleAssetRemoved(const FAssetData& AssetData)
{
	const UWxBlueprintSnapshotSettings* Settings = GetDefault<UWxBlueprintSnapshotSettings>();
	if (!Settings || !Settings->bEnabled)
	{
		return;
	}

	if (IsRunningCommandlet())
	{
		return;
	}

	// Blueprint 계열 자산만 대상. UWidgetBlueprint 등 파생 클래스도 포함.
	if (!AssetData.IsInstanceOf<UBlueprint>())
	{
		return;
	}

	const FString PackageName = AssetData.PackageName.ToString();
	if (!IsPackageNameIncluded(PackageName))
	{
		return;
	}

	const FString SnapshotPath = FWxBlueprintSnapshotExporter::ResolveSnapshotPath(PackageName);
	if (SnapshotPath.IsEmpty())
	{
		return;
	}

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.FileExists(*SnapshotPath))
	{
		return;
	}

	if (PlatformFile.IsReadOnly(*SnapshotPath))
	{
		PlatformFile.SetReadOnly(*SnapshotPath, false);
	}

	if (PlatformFile.DeleteFile(*SnapshotPath))
	{
		UE_LOG(LogWxBPSnapshot, Log, TEXT("Snapshot deleted for %s"), *PackageName);
	}
	else
	{
		UE_LOG(LogWxBPSnapshot, Warning, TEXT("Failed to delete snapshot %s"), *SnapshotPath);
	}

	// 삭제된 BP가 큐에 대기 중이면 제거하여 뒤늦은 재기록을 방지.
	PendingPaths.Remove(AssetData.GetObjectPathString());
}

void FWxBlueprintSnapshotModule::EnqueueBlueprint(UBlueprint* Blueprint)
{
	FString Path = Blueprint->GetPathName();
	bool bAlreadyQueued = false;
	PendingPaths.Add(Path, &bAlreadyQueued);
	if (bAlreadyQueued)
	{
		return;
	}
	PendingQueue.Enqueue({ TWeakObjectPtr<UBlueprint>(Blueprint), MoveTemp(Path) });
}

bool FWxBlueprintSnapshotModule::HandleTick(float DeltaTime)
{
	FPendingEntry Entry;
	if (!PendingQueue.Dequeue(Entry))
	{
		return true;
	}

	PendingPaths.Remove(Entry.Path);

	UBlueprint* Blueprint = Entry.Blueprint.Get();
	if (Blueprint && ShouldProcessBlueprint(Blueprint))
	{
		const bool bWritten = FWxBlueprintSnapshotExporter::ExportBlueprint(Blueprint);
		if (bWritten)
		{
			UE_LOG(LogWxBPSnapshot, Log, TEXT("Snapshot written for %s"), *Blueprint->GetPathName());
		}
	}

	return true;
}
