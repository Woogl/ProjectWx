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

IMPLEMENT_MODULE(FWxBlueprintSnapshotModule, WxBlueprintSnapshot)

DEFINE_LOG_CATEGORY_STATIC(LogWxBPSnapshot, Log, All);

void FWxBlueprintSnapshotModule::StartupModule()
{
	PackageSavedHandle = UPackage::PackageSavedWithContextEvent.AddRaw(this, &FWxBlueprintSnapshotModule::HandlePackageSaved);
	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FWxBlueprintSnapshotModule::HandleTick), 0.0f);
}

void FWxBlueprintSnapshotModule::ShutdownModule()
{
	if (PackageSavedHandle.IsValid())
	{
		UPackage::PackageSavedWithContextEvent.Remove(PackageSavedHandle);
		PackageSavedHandle.Reset();
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
	case BPTYPE_MacroLibrary:
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

	if (!Blueprint->GeneratedClass || !Blueprint->GeneratedClass->GetDefaultObject(false))
	{
		return false;
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

	if (Settings->IncludePathPrefixes.Num() > 0)
	{
		bool bIncluded = false;
		for (const FString& Prefix : Settings->IncludePathPrefixes)
		{
			if (PackageName.StartsWith(Prefix))
			{
				bIncluded = true;
				break;
			}
		}
		if (!bIncluded)
		{
			return false;
		}
	}

	for (const FString& Prefix : Settings->ExcludePathPrefixes)
	{
		if (PackageName.StartsWith(Prefix))
		{
			return false;
		}
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

void FWxBlueprintSnapshotModule::EnqueueBlueprint(UBlueprint* Blueprint)
{
	const FString Path = Blueprint->GetPathName();
	bool bAlreadyQueued = false;
	PendingPaths.Add(Path, &bAlreadyQueued);
	if (bAlreadyQueued)
	{
		return;
	}
	PendingQueue.Enqueue(TWeakObjectPtr<UBlueprint>(Blueprint));
}

bool FWxBlueprintSnapshotModule::HandleTick(float DeltaTime)
{
	TWeakObjectPtr<UBlueprint> WeakBP;
	if (!PendingQueue.Dequeue(WeakBP))
	{
		return true;
	}

	UBlueprint* Blueprint = WeakBP.Get();
	if (Blueprint)
	{
		PendingPaths.Remove(Blueprint->GetPathName());
		if (ShouldProcessBlueprint(Blueprint))
		{
			const bool bWritten = FWxBlueprintSnapshotExporter::ExportBlueprint(Blueprint);
			if (bWritten)
			{
				UE_LOG(LogWxBPSnapshot, Log, TEXT("Snapshot written for %s"), *Blueprint->GetPathName());
			}
		}
	}

	return true;
}
