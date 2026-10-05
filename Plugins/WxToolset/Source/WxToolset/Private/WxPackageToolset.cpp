// Copyright Woogle. All Rights Reserved.

#include "WxPackageToolset.h"

#include "FileHelpers.h"
#include "HAL/FileManager.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/ObjectSaveContext.h"
#include "UObject/Package.h"

TArray<FString> UWxPackageToolset::SavePackages(const TArray<UObject*>& Objects)
{
	TArray<UPackage*> Packages;
	for (const UObject* Object : Objects)
	{
		if (!Object)
		{
			UKismetSystemLibrary::RaiseScriptError(TEXT("Objects 에 null 이 있다."));
			return {};
		}
		// 외부 액터의 GetPackage 는 맵이 아니라 액터 자신의 패키지를 돌려준다.
		Packages.AddUnique(Object->GetPackage());
	}

	if (Packages.IsEmpty())
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Objects 가 비었다."));
		return {};
	}

	TArray<FString> Filenames;
	for (const UPackage* Package : Packages)
	{
		const FString Extension = Package->ContainsMap() ? FPackageName::GetMapPackageExtension() : FPackageName::GetAssetPackageExtension();
		FString Filename;
		if (!FPackageName::TryConvertLongPackageNameToFilename(Package->GetName(), Filename, Extension))
		{
			UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("'%s' 의 파일 경로를 알 수 없다."), *Package->GetName()));
			return {};
		}
		Filename = FPaths::ConvertRelativePathToFull(Filename);

		// 엔진 저장은 읽기 전용 파일에서 이유 없이 false 만 돌려주므로 저장 전에 막는다.
		if (IFileManager::Get().IsReadOnly(*Filename))
		{
			UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("'%s' 가 읽기 전용이라 아무것도 저장하지 않았다. 읽기 전용을 풀고 다시 부른다."), *Filename));
			return {};
		}

		Filenames.Add(Filename);
	}

	// 엔진 반환값은 패키지별 성패를 알려 주지 않고 파일 시각은 초 단위라, 저장 통지로 실제로 쓴 패키지를 모은다.
	TArray<const UPackage*> SavedPackages;
	const FDelegateHandle SavedHandle = UPackage::PackageSavedWithContextEvent.AddLambda([&SavedPackages](const FString&, UPackage* Package, FObjectPostSaveContext)
	{
		SavedPackages.Add(Package);
	});
	UEditorLoadingAndSavingUtils::SavePackages(Packages, /*bOnlyDirty*/ false);
	UPackage::PackageSavedWithContextEvent.Remove(SavedHandle);

	TArray<FString> UnsavedFilenames;
	for (int32 Index = 0; Index < Packages.Num(); ++Index)
	{
		if (!SavedPackages.Contains(Packages[Index]))
		{
			UnsavedFilenames.Add(Filenames[Index]);
		}
	}
	if (!UnsavedFilenames.IsEmpty())
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("저장되지 않았다. 출력 로그를 확인한다: %s"), *FString::Join(UnsavedFilenames, TEXT(", "))));
		return {};
	}
	return Filenames;
}
