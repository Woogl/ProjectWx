// Copyright Woogle. All Rights Reserved.

#include "WxLevelSnapshotExporter.h"
#include "WxLevelSnapshotModule.h"
#include "WxLevelSnapshotSettings.h"
#include "GameFramework/Actor.h"
#include "Engine/Level.h"
#include "UObject/Package.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
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

	TSharedPtr<FJsonObject> MakeVectorJson(const FVector& V)
	{
		TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetNumberField(TEXT("x"), V.X);
		Obj->SetNumberField(TEXT("y"), V.Y);
		Obj->SetNumberField(TEXT("z"), V.Z);
		return Obj;
	}

	TSharedPtr<FJsonObject> MakeRotatorJson(const FRotator& R)
	{
		TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetNumberField(TEXT("pitch"), R.Pitch);
		Obj->SetNumberField(TEXT("yaw"), R.Yaw);
		Obj->SetNumberField(TEXT("roll"), R.Roll);
		return Obj;
	}
}

FString FWxLevelSnapshotExporter::GetActorKey(AActor* Actor)
{
	if (!Actor)
	{
		return FString();
	}

	const UWxLevelSnapshotSettings* Settings = GetDefault<UWxLevelSnapshotSettings>();
	if (!Settings || Settings->KeyProperties.Num() == 0)
	{
		return FString();
	}

	// 액터 클래스 체인을 따라 가장 구체적인 매칭을 찾는다.
	FName PropertyName = NAME_None;
	bool bMatched = false;
	for (UClass* Cls = Actor->GetClass(); Cls && Cls->IsChildOf(AActor::StaticClass()); Cls = Cls->GetSuperClass())
	{
		if (const FName* Found = Settings->KeyProperties.Find(TSoftClassPtr<AActor>(Cls)))
		{
			PropertyName = *Found;
			bMatched = true;
			break;
		}
	}

	if (!bMatched || PropertyName.IsNone())
	{
		return FString();
	}

	// FGuid 프로퍼티(특히 AActor::ActorGuid)는 struct export 형식 대신 하이픈 GUID로 직접 변환.
	static const FName NAME_ActorGuid(TEXT("ActorGuid"));
	if (PropertyName == NAME_ActorGuid)
	{
		const FGuid Guid = Actor->GetActorGuid();
		if (Guid.IsValid())
		{
			return Guid.ToString(EGuidFormats::DigitsWithHyphens);
		}
		return FString();
	}

	FProperty* Prop = FindFProperty<FProperty>(Actor->GetClass(), PropertyName);
	if (!Prop)
	{
		UE_LOG(LogWxLevelSnapshot, Error, TEXT("KeyProperty '%s' not found on %s"),
			*PropertyName.ToString(), *Actor->GetClass()->GetName());
		return FString();
	}

	FString OutStr;
	Prop->ExportText_Direct(OutStr, Prop->ContainerPtrToValuePtr<void>(Actor), nullptr, Actor, PPF_None);
	OutStr.TrimStartAndEndInline();
	// 따옴표 감싸진 문자열/이름 타입은 래퍼 제거.
	if (OutStr.Len() >= 2 && OutStr.StartsWith(TEXT("\"")) && OutStr.EndsWith(TEXT("\"")))
	{
		OutStr = OutStr.Mid(1, OutStr.Len() - 2);
	}
	if (OutStr.IsEmpty() || OutStr == TEXT("None"))
	{
		return FString();
	}
	return OutStr;
}

TSharedPtr<FJsonObject> FWxLevelSnapshotExporter::BuildActorJson(AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}

	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("class"), Actor->GetClass()->GetPathName());

	// 액터가 속한 레벨 패키지 경로.
	if (ULevel* OwningLevel = Actor->GetLevel())
	{
		if (UPackage* LevelPackage = OwningLevel->GetPackage())
		{
			Root->SetStringField(TEXT("level"), LevelPackage->GetName());
		}
	}

	const FString FNameStr = Actor->GetFName().ToString();
	const FString Label = Actor->GetActorLabel();
	if (!Label.IsEmpty() && Label != FNameStr)
	{
		Root->SetStringField(TEXT("label"), Label);
	}

	const FGuid ActorGuid = Actor->GetActorGuid();
	if (ActorGuid.IsValid())
	{
		Root->SetStringField(TEXT("guid"), ActorGuid.ToString(EGuidFormats::DigitsWithHyphens));
	}

	// Transform: 루트 컴포넌트 기준. 없으면 스킵.
	if (USceneComponent* RootComp = Actor->GetRootComponent())
	{
		TSharedPtr<FJsonObject> TransformJson = MakeShared<FJsonObject>();
		TransformJson->SetObjectField(TEXT("location"), MakeVectorJson(Actor->GetActorLocation()).ToSharedRef());
		TransformJson->SetObjectField(TEXT("rotation"), MakeRotatorJson(Actor->GetActorRotation()).ToSharedRef());
		TransformJson->SetObjectField(TEXT("scale"), MakeVectorJson(Actor->GetActorScale3D()).ToSharedRef());
		Root->SetObjectField(TEXT("transform"), TransformJson.ToSharedRef());
	}

	// 내부 프로퍼티를 모두 재귀적으로 덤프 (bIncludeAllProperties가 true일 때).
	const UWxLevelSnapshotSettings* Settings = GetDefault<UWxLevelSnapshotSettings>();
	if (Settings && Settings->bIncludeAllProperties)
	{
		TSharedPtr<FJsonObject> Properties = BuildProperties(Actor);
		if (Properties.IsValid() && Properties->Values.Num() > 0)
		{
			Root->SetObjectField(TEXT("properties"), Properties.ToSharedRef());
		}
	}

	return Root;
}

bool FWxLevelSnapshotExporter::ExportActorIntoLevel(AActor* Actor)
{
	if (!Actor)
	{
		return false;
	}

	// KeyProperties 규칙에서 키를 얻지 못하면 스냅샷 대상 아님.
	const FString ActorKey = GetActorKey(Actor);
	if (ActorKey.IsEmpty())
	{
		return false;
	}

	ULevel* Level = Actor->GetLevel();
	if (!Level)
	{
		return false;
	}

	UPackage* LevelPackage = Level->GetPackage();
	if (!LevelPackage)
	{
		return false;
	}

	const FString LevelPackageName = LevelPackage->GetName();

	// 기존 레벨 JSON 로드. 없으면 빈 루트로 시작.
	TSharedPtr<FJsonObject> Root = LoadLevelJson(LevelPackageName);
	if (!Root.IsValid())
	{
		Root = MakeShared<FJsonObject>();
	}

	TSharedPtr<FJsonObject> ActorJson = BuildActorJson(Actor);
	if (!ActorJson.IsValid())
	{
		return false;
	}

	// 커스텀 KeyProperty 값이 변경된 경우 구-키 엔트리가 남지 않도록 같은 GUID의 기존 엔트리를 제거한다.
	const FGuid ActorGuid = Actor->GetActorGuid();
	if (ActorGuid.IsValid())
	{
		const FString ActorGuidString = ActorGuid.ToString(EGuidFormats::DigitsWithHyphens);
		TArray<FString> KeysToRemove;
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Root->Values)
		{
			if (Pair.Key == ActorKey)
			{
				continue;
			}
			const TSharedPtr<FJsonObject>* EntryObj = nullptr;
			if (!Pair.Value.IsValid() || !Pair.Value->TryGetObject(EntryObj) || !EntryObj)
			{
				continue;
			}
			FString GuidField;
			if ((*EntryObj)->TryGetStringField(TEXT("guid"), GuidField) && GuidField == ActorGuidString)
			{
				KeysToRemove.Add(Pair.Key);
			}
		}
		for (const FString& Key : KeysToRemove)
		{
			Root->Values.Remove(Key);
		}
	}

	Root->SetObjectField(ActorKey, ActorJson.ToSharedRef());

	return WriteLevelJson(LevelPackageName, Root.ToSharedRef());
}

bool FWxLevelSnapshotExporter::ExportLevel(ULevel* Level)
{
	if (!Level)
	{
		return false;
	}

	UPackage* LevelPackage = Level->GetPackage();
	if (!LevelPackage)
	{
		return false;
	}

	const FString LevelPackageName = LevelPackage->GetName();
	const UWxLevelSnapshotSettings* Settings = GetDefault<UWxLevelSnapshotSettings>();

	if (Settings && Settings->bCombineAllLevels)
	{
		// 합쳐진 파일 로드 후 이 레벨의 기존 엔트리만 제거하고 최신 엔트리로 덮어쓴다.
		TSharedPtr<FJsonObject> Root = LoadLevelJson(LevelPackageName);
		if (!Root.IsValid())
		{
			Root = MakeShared<FJsonObject>();
		}

		TArray<FString> KeysToRemove;
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Root->Values)
		{
			const TSharedPtr<FJsonObject>* EntryObj = nullptr;
			if (!Pair.Value.IsValid() || !Pair.Value->TryGetObject(EntryObj) || !EntryObj)
			{
				continue;
			}
			FString LevelField;
			if ((*EntryObj)->TryGetStringField(TEXT("level"), LevelField) && LevelField == LevelPackageName)
			{
				KeysToRemove.Add(Pair.Key);
			}
		}
		for (const FString& Key : KeysToRemove)
		{
			Root->Values.Remove(Key);
		}

		for (AActor* Actor : Level->Actors)
		{
			if (!Actor || Actor->HasAnyFlags(RF_Transient))
			{
				continue;
			}
			const FString ActorKey = GetActorKey(Actor);
			if (ActorKey.IsEmpty())
			{
				continue;
			}
			TSharedPtr<FJsonObject> ActorJson = BuildActorJson(Actor);
			if (ActorJson.IsValid())
			{
				Root->SetObjectField(ActorKey, ActorJson.ToSharedRef());
			}
		}

		return WriteLevelJson(LevelPackageName, Root.ToSharedRef());
	}

	TSharedRef<FJsonObject> Root = BuildLevelRoot(Level);
	return WriteLevelJson(LevelPackageName, Root);
}

TSharedRef<FJsonObject> FWxLevelSnapshotExporter::BuildLevelRoot(ULevel* Level)
{
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	for (AActor* Actor : Level->Actors)
	{
		if (!Actor || Actor->HasAnyFlags(RF_Transient))
		{
			continue;
		}
		const FString ActorKey = GetActorKey(Actor);
		if (ActorKey.IsEmpty())
		{
			continue;
		}
		TSharedPtr<FJsonObject> ActorJson = BuildActorJson(Actor);
		if (ActorJson.IsValid())
		{
			Root->SetObjectField(ActorKey, ActorJson.ToSharedRef());
		}
	}
	return Root;
}

bool FWxLevelSnapshotExporter::RemoveActorFromLevel(const FString& LevelPackageName, const FString& ActorGuidString)
{
	if (ActorGuidString.IsEmpty())
	{
		return false;
	}

	TSharedPtr<FJsonObject> Root = LoadLevelJson(LevelPackageName);
	if (!Root.IsValid())
	{
		return false;
	}

	const UWxLevelSnapshotSettings* Settings = GetDefault<UWxLevelSnapshotSettings>();
	const bool bCombined = Settings && Settings->bCombineAllLevels;

	auto EntryBelongsToLevel = [&](const TSharedPtr<FJsonObject>& Entry) -> bool
	{
		if (!bCombined)
		{
			return true;
		}
		FString LevelField;
		return Entry->TryGetStringField(TEXT("level"), LevelField) && LevelField == LevelPackageName;
	};

	FString MatchedKey;

	// 1차: KeyProperty=ActorGuid (기본)이면 JSON 키가 곧 GUID이므로 직접 조회.
	if (const TSharedPtr<FJsonValue>* DirectVal = Root->Values.Find(ActorGuidString))
	{
		const TSharedPtr<FJsonObject>* EntryObj = nullptr;
		if (DirectVal->IsValid() && (*DirectVal)->TryGetObject(EntryObj) && EntryObj && EntryBelongsToLevel(*EntryObj))
		{
			MatchedKey = ActorGuidString;
		}
	}

	if (MatchedKey.IsEmpty())
	{
		// 2차: 커스텀 키 모드 — 각 엔트리의 guid 필드로 매칭.
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Root->Values)
		{
			const TSharedPtr<FJsonObject>* EntryObj = nullptr;
			if (!Pair.Value.IsValid() || !Pair.Value->TryGetObject(EntryObj) || !EntryObj)
			{
				continue;
			}
			FString GuidField;
			if (!(*EntryObj)->TryGetStringField(TEXT("guid"), GuidField) || GuidField != ActorGuidString)
			{
				continue;
			}
			if (!EntryBelongsToLevel(*EntryObj))
			{
				continue;
			}
			MatchedKey = Pair.Key;
			break;
		}
	}

	if (MatchedKey.IsEmpty() || !Root->Values.Remove(MatchedKey))
	{
		return false;
	}

	// 남은 엔트리가 없으면 파일 자체를 삭제.
	if (Root->Values.Num() == 0)
	{
		const FString Path = ResolveSnapshotPath(LevelPackageName);
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		if (!Path.IsEmpty() && PlatformFile.FileExists(*Path))
		{
			if (PlatformFile.IsReadOnly(*Path))
			{
				PlatformFile.SetReadOnly(*Path, false);
			}
			return PlatformFile.DeleteFile(*Path);
		}
		return true;
	}

	return WriteLevelJson(LevelPackageName, Root.ToSharedRef());
}

bool FWxLevelSnapshotExporter::DeleteLevelSnapshot(const FString& LevelPackageName)
{
	const FString Path = ResolveSnapshotPath(LevelPackageName);
	if (Path.IsEmpty())
	{
		return false;
	}

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	const UWxLevelSnapshotSettings* Settings = GetDefault<UWxLevelSnapshotSettings>();

	// 합쳐진 모드: 파일에서 해당 레벨의 엔트리만 제거. 파일이 비면 삭제.
	if (Settings && Settings->bCombineAllLevels)
	{
		TSharedPtr<FJsonObject> Root = LoadLevelJson(LevelPackageName);
		if (!Root.IsValid())
		{
			return true;
		}

		TArray<FString> KeysToRemove;
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Root->Values)
		{
			const TSharedPtr<FJsonObject>* EntryObj = nullptr;
			if (!Pair.Value.IsValid() || !Pair.Value->TryGetObject(EntryObj) || !EntryObj)
			{
				continue;
			}
			FString LevelField;
			if ((*EntryObj)->TryGetStringField(TEXT("level"), LevelField) && LevelField == LevelPackageName)
			{
				KeysToRemove.Add(Pair.Key);
			}
		}
		for (const FString& Key : KeysToRemove)
		{
			Root->Values.Remove(Key);
		}

		if (Root->Values.Num() == 0)
		{
			if (!PlatformFile.FileExists(*Path))
			{
				return true;
			}
			if (PlatformFile.IsReadOnly(*Path))
			{
				PlatformFile.SetReadOnly(*Path, false);
			}
			return PlatformFile.DeleteFile(*Path);
		}

		return WriteLevelJson(LevelPackageName, Root.ToSharedRef());
	}

	// 레벨별 모드: 파일 자체를 삭제.
	if (!PlatformFile.FileExists(*Path))
	{
		return true;
	}
	if (PlatformFile.IsReadOnly(*Path))
	{
		PlatformFile.SetReadOnly(*Path, false);
	}
	return PlatformFile.DeleteFile(*Path);
}

TSharedPtr<FJsonObject> FWxLevelSnapshotExporter::LoadLevelJson(const FString& LevelPackageName)
{
	const FString Path = ResolveSnapshotPath(LevelPackageName);
	if (Path.IsEmpty())
	{
		return nullptr;
	}

	FString Contents;
	if (!FFileHelper::LoadFileToString(Contents, *Path))
	{
		return nullptr;
	}

	TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(Contents);
	TSharedPtr<FJsonObject> Parsed;
	if (!FJsonSerializer::Deserialize(Reader, Parsed) || !Parsed.IsValid())
	{
		UE_LOG(LogWxLevelSnapshot, Warning, TEXT("Failed to parse existing level JSON at %s"), *Path);
		return nullptr;
	}
	return Parsed;
}

bool FWxLevelSnapshotExporter::WriteLevelJson(const FString& LevelPackageName, TSharedRef<FJsonObject> Root)
{
	const FString Path = ResolveSnapshotPath(LevelPackageName);
	if (Path.IsEmpty())
	{
		return false;
	}

	const int32 MaxPathLen = 240;
	if (Path.Len() > MaxPathLen)
	{
		UE_LOG(LogWxLevelSnapshot, Error,
			TEXT("Snapshot path exceeds %d chars (len=%d), skipping. Level=%s  Path=%s"),
			MaxPathLen, Path.Len(), *LevelPackageName, *Path);
		return false;
	}

	IFileManager& FileManager = IFileManager::Get();
	FileManager.MakeDirectory(*FPaths::GetPath(Path), true);

	const FString Json = SerializeJson(Root);

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	bool bClearedReadOnly = false;
	if (PlatformFile.FileExists(*Path) && PlatformFile.IsReadOnly(*Path))
	{
		PlatformFile.SetReadOnly(*Path, false);
		bClearedReadOnly = true;
	}

	if (!FFileHelper::SaveStringToFile(Json, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		if (bClearedReadOnly)
		{
			PlatformFile.SetReadOnly(*Path, true);
		}
		UE_LOG(LogWxLevelSnapshot, Warning, TEXT("Failed to write %s"), *Path);
		return false;
	}

	return true;
}

FString FWxLevelSnapshotExporter::SerializeJson(TSharedRef<FJsonObject> RootObject)
{
	SortJsonObjectRecursive(RootObject);

	FString Out;
	TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&Out);
	FJsonSerializer::Serialize(RootObject, Writer);
	return Out;
}

FString FWxLevelSnapshotExporter::ResolveSnapshotPath(const FString& LevelPackageName)
{
	const UWxLevelSnapshotSettings* Settings = GetDefault<UWxLevelSnapshotSettings>();
	if (!Settings)
	{
		return FString();
	}

	FString RootDir = Settings->OutputDirectory.Path;
	if (RootDir.IsEmpty())
	{
		TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("WxLevelSnapshot"));
		if (!Plugin.IsValid())
		{
			return FString();
		}
		RootDir = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Snapshots"));
	}
	else if (FPaths::IsRelative(RootDir))
	{
		RootDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), RootDir);
	}

	// 합쳐진 모드: LevelPackageName과 무관하게 단일 파일 경로.
	if (Settings->bCombineAllLevels)
	{
		return FPaths::Combine(RootDir, TEXT("AllLevels")) + Settings->FileExtension;
	}

	if (LevelPackageName.IsEmpty())
	{
		return FString();
	}

	FString PackagePath = LevelPackageName;
	if (PackagePath.StartsWith(TEXT("/")))
	{
		PackagePath.RemoveAt(0);
	}

	return FPaths::Combine(RootDir, PackagePath) + Settings->FileExtension;
}
