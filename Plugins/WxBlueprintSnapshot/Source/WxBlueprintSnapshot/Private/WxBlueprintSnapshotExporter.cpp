// Copyright Woogle. All Rights Reserved.

#include "WxBlueprintSnapshotExporter.h"
#include "WxBlueprintSnapshotModule.h"
#include "WxBlueprintSnapshotSettings.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Components/ActorComponent.h"
#include "WidgetBlueprint.h"
#include "UObject/Package.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/FileManager.h"
#include "Interfaces/IPluginManager.h"

namespace
{
	void SortJsonValueRecursive(const TSharedPtr<FJsonValue>& Value);

	void SortJsonObjectRecursive(TSharedPtr<FJsonObject> Obj)
	{
		if (!Obj.IsValid())
		{
			return;
		}
		Obj->Values.KeySort(TLess<FString>());
		for (auto& Pair : Obj->Values)
		{
			SortJsonValueRecursive(Pair.Value);
		}
	}

	void SortJsonValueRecursive(const TSharedPtr<FJsonValue>& Value)
	{
		if (!Value.IsValid())
		{
			return;
		}
		if (Value->Type == EJson::Object)
		{
			SortJsonObjectRecursive(Value->AsObject());
			return;
		}
		if (Value->Type == EJson::Array)
		{
			for (const TSharedPtr<FJsonValue>& Elem : Value->AsArray())
			{
				SortJsonValueRecursive(Elem);
			}
		}
	}

	void SetObjectFieldIfNonEmpty(FJsonObject& Root, const FString& Key, const TSharedPtr<FJsonObject>& Value)
	{
		if (Value.IsValid() && Value->Values.Num() > 0)
		{
			Root.SetObjectField(Key, Value.ToSharedRef());
		}
	}

	FString MakeHashedFallbackPath(UBlueprint* Blueprint)
	{
		FSHA1 DirHash;
		const FString FullPath = Blueprint->GetPathName();
		DirHash.UpdateWithString(*FullPath, FullPath.Len());
		DirHash.Final();
		uint8 Hash[FSHA1::DigestSize];
		DirHash.GetHash(Hash);
		const FString ShortDirHash = BytesToHex(Hash, 6);

		const FString FallbackDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("BlueprintSnapshots_Hashed"), ShortDirHash);
		return FallbackDir / (Blueprint->GetName() + GetDefault<UWxBlueprintSnapshotSettings>()->FileExtension);
	}
}

bool FWxBlueprintSnapshotExporter::ExportBlueprint(UBlueprint* Blueprint)
{
	if (!Blueprint)
	{
		return false;
	}

	const UWxBlueprintSnapshotSettings* Settings = GetDefault<UWxBlueprintSnapshotSettings>();
	if (!Settings)
	{
		return false;
	}

	TSharedRef<FJsonObject> Root = BuildSnapshot(Blueprint, *Settings);

	FString LatestPath = ResolveLatestPath(Blueprint);
	if (LatestPath.IsEmpty())
	{
		return false;
	}

	// Windows MAX_PATH 가드: 경로가 과도하게 길면 BP 경로 해시로 폴더를 단축한다.
	const int32 MaxPathLen = 240;
	if (LatestPath.Len() > MaxPathLen)
	{
		LatestPath = MakeHashedFallbackPath(Blueprint);
	}

	IFileManager& FileManager = IFileManager::Get();
	FileManager.MakeDirectory(*FPaths::GetPath(LatestPath), true);

	const FString Json = SerializeJson(Root);

	// Read-only (SCC 추적 등) 인 경우 해제 시도. 저장 실패하면 원상 복구.
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	bool bClearedReadOnly = false;
	if (PlatformFile.FileExists(*LatestPath) && PlatformFile.IsReadOnly(*LatestPath))
	{
		PlatformFile.SetReadOnly(*LatestPath, false);
		bClearedReadOnly = true;
	}

	if (!FFileHelper::SaveStringToFile(Json, *LatestPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		if (bClearedReadOnly)
		{
			PlatformFile.SetReadOnly(*LatestPath, true);
		}
		UE_LOG(LogWxBPSnapshot, Warning, TEXT("Failed to write %s"), *LatestPath);
		return false;
	}

	return true;
}

TSharedRef<FJsonObject> FWxBlueprintSnapshotExporter::BuildSnapshot(UBlueprint* Blueprint, const UWxBlueprintSnapshotSettings& Settings)
{
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("blueprintPath"), Blueprint->GetPathName());
	Root->SetStringField(TEXT("parentClass"), Blueprint->ParentClass ? Blueprint->ParentClass->GetPathName() : TEXT(""));

	UBlueprintGeneratedClass* GeneratedClass = Cast<UBlueprintGeneratedClass>(Blueprint->GeneratedClass);
	if (GeneratedClass && Blueprint->ParentClass)
	{
		UObject* InstanceCDO = GeneratedClass->GetDefaultObject(false);
		UObject* ParentCDO = Blueprint->ParentClass->GetDefaultObject(false);
		if (InstanceCDO && ParentCDO)
		{
			// NewVariables는 별도 `variables` 필드에 기록되므로 classDefaults 델타에서 중복 제외.
			TSet<FName> NewVariableNames;
			NewVariableNames.Reserve(Blueprint->NewVariables.Num());
			for (const FBPVariableDescription& Var : Blueprint->NewVariables)
			{
				NewVariableNames.Add(Var.VarName);
			}
			SetObjectFieldIfNonEmpty(*Root, TEXT("classDefaults"), BuildClassDefaults(InstanceCDO, ParentCDO, NewVariableNames));
		}
	}

	if (Settings.bIncludeComponents)
	{
		if (USimpleConstructionScript* SCS = Blueprint->SimpleConstructionScript)
		{
			SetObjectFieldIfNonEmpty(*Root, TEXT("components"), BuildComponentsJson(SCS));
		}
	}

	if (Settings.bIncludeVariables)
	{
		SetObjectFieldIfNonEmpty(*Root, TEXT("newVariables"), BuildVariablesJson(Blueprint));
	}

	if (Settings.bIncludeInterfaces)
	{
		SetObjectFieldIfNonEmpty(*Root, TEXT("interfaces"), BuildInterfacesJson(Blueprint));
	}

	if (Settings.bIncludeGraphs)
	{
		SetObjectFieldIfNonEmpty(*Root, TEXT("graphs"), BuildGraphsJson(Blueprint));
	}

	if (UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(Blueprint))
	{
		if (Settings.bIncludeWidgetTree)
		{
			SetObjectFieldIfNonEmpty(*Root, TEXT("widgetTree"), BuildWidgetTreeJson(WidgetBlueprint->WidgetTree));
		}

		if (Settings.bIncludeMVVM)
		{
			SetObjectFieldIfNonEmpty(*Root, TEXT("mvvm"), BuildMvvmJson(WidgetBlueprint));
		}
	}

	return Root;
}

TSharedPtr<FJsonObject> FWxBlueprintSnapshotExporter::BuildComponentsJson(USimpleConstructionScript* SCS)
{
	if (!SCS)
	{
		return nullptr;
	}

	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	for (USCS_Node* Node : SCS->GetAllNodes())
	{
		TSharedPtr<FJsonObject> NodeJson = BuildScsNodeJson(Node);
		if (NodeJson.IsValid())
		{
			Root->SetObjectField(Node->GetVariableName().ToString(), NodeJson.ToSharedRef());
		}
	}
	return Root;
}

TSharedPtr<FJsonObject> FWxBlueprintSnapshotExporter::BuildScsNodeJson(USCS_Node* Node)
{
	if (!Node)
	{
		return nullptr;
	}

	TSharedPtr<FJsonObject> NodeJson = MakeShared<FJsonObject>();
	NodeJson->SetStringField(TEXT("variableName"), Node->GetVariableName().ToString());

	UActorComponent* Template = Node->ComponentTemplate;
	if (Template)
	{
		NodeJson->SetStringField(TEXT("componentClass"), Template->GetClass()->GetPathName());

		UObject* ClassDefaults = Template->GetClass()->GetDefaultObject(false);
		TSharedPtr<FJsonObject> TemplateDelta = BuildClassDefaults(Template, ClassDefaults);
		if (TemplateDelta.IsValid() && TemplateDelta->Values.Num() > 0)
		{
			NodeJson->SetObjectField(TEXT("delta"), TemplateDelta.ToSharedRef());
		}
	}

	if (USimpleConstructionScript* SCS = Node->GetSCS())
	{
		if (USCS_Node* Parent = SCS->FindParentNode(Node))
		{
			NodeJson->SetStringField(TEXT("attachParent"), Parent->GetVariableName().ToString());
		}
	}

	if (!Node->AttachToName.IsNone())
	{
		NodeJson->SetStringField(TEXT("attachSocket"), Node->AttachToName.ToString());
	}

	return NodeJson;
}

TSharedPtr<FJsonObject> FWxBlueprintSnapshotExporter::BuildVariablesJson(UBlueprint* Blueprint)
{
	auto FormatTerminal = [](const FName& Category, const TWeakObjectPtr<UObject>& SubCatObj) -> FString
	{
		if (UObject* Obj = SubCatObj.Get())
		{
			return Obj->GetName();
		}
		return Category.ToString();
	};

	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	for (const FBPVariableDescription& Var : Blueprint->NewVariables)
	{
		FString TypeStr = FormatTerminal(Var.VarType.PinCategory, Var.VarType.PinSubCategoryObject);

		if (Var.VarType.IsArray())
		{
			TypeStr = FString::Printf(TEXT("%s[]"), *TypeStr);
		}
		else if (Var.VarType.IsSet())
		{
			TypeStr = FString::Printf(TEXT("Set<%s>"), *TypeStr);
		}
		else if (Var.VarType.IsMap())
		{
			const FString ValueStr = FormatTerminal(Var.VarType.PinValueType.TerminalCategory, Var.VarType.PinValueType.TerminalSubCategoryObject);
			TypeStr = FString::Printf(TEXT("Map<%s, %s>"), *TypeStr, *ValueStr);
		}

		TSharedPtr<FJsonObject> Entry = MakeShared<FJsonObject>();
		Entry->SetStringField(TEXT("type"), TypeStr);
		Entry->SetStringField(TEXT("value"), Var.DefaultValue);
		Root->SetObjectField(Var.VarName.ToString(), Entry.ToSharedRef());
	}
	return Root;
}

TSharedPtr<FJsonObject> FWxBlueprintSnapshotExporter::BuildInterfacesJson(UBlueprint* Blueprint)
{
	TArray<TSharedPtr<FJsonValue>> Arr;
	for (const FBPInterfaceDescription& Desc : Blueprint->ImplementedInterfaces)
	{
		if (Desc.Interface)
		{
			Arr.Add(MakeShared<FJsonValueString>(Desc.Interface->GetPathName()));
		}
	}

	if (Arr.Num() == 0)
	{
		return nullptr;
	}

	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetArrayField(TEXT("implemented"), Arr);
	return Root;
}

FString FWxBlueprintSnapshotExporter::SerializeJson(TSharedRef<FJsonObject> RootObject)
{
	SortJsonObjectRecursive(RootObject);

	FString Out;
	TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&Out);
	FJsonSerializer::Serialize(RootObject, Writer);
	return Out;
}

FString FWxBlueprintSnapshotExporter::ResolveLatestPath(UBlueprint* Blueprint)
{
	if (!Blueprint)
	{
		return FString();
	}

	UPackage* Package = Blueprint->GetOutermost();
	if (!Package)
	{
		return FString();
	}

	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("WxBlueprintSnapshot"));
	if (!Plugin.IsValid())
	{
		return FString();
	}

	// /Game/UI/Widget/WBP_Ability -> Game/UI/Widget/WBP_Ability{Ext}
	FString PackagePath = Package->GetName();
	if (PackagePath.StartsWith(TEXT("/")))
	{
		PackagePath.RemoveAt(0);
	}

	return FPaths::Combine(Plugin->GetBaseDir(), TEXT("Snapshots"), PackagePath) + GetDefault<UWxBlueprintSnapshotSettings>()->FileExtension;
}
