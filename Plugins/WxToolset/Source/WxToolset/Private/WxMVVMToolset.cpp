// Copyright Woogle. All Rights Reserved.

#include "WxMVVMToolset.h"

#include "Dom/JsonObject.h"
#include "Editor.h"
#include "Kismet/KismetSystemLibrary.h"
#include "MVVMBlueprintFunctionReference.h"
#include "MVVMBlueprintPin.h"
#include "MVVMBlueprintView.h"
#include "MVVMBlueprintViewBinding.h"
#include "MVVMBlueprintViewConversionFunction.h"
#include "MVVMBlueprintViewModelContext.h"
#include "MVVMEditorSubsystem.h"
#include "MVVMPropertyPath.h"
#include "MVVMWidgetBlueprintExtension_View.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Types/MVVMFieldVariant.h"
#include "WidgetBlueprint.h"

bool UWxMVVMToolset::SetBindingConversionFunction(UWidgetBlueprint* WidgetBlueprint, const FString& BindingId, const FString& FunctionPath, const FString& ArgumentsJson)
{
	const UMVVMWidgetBlueprintExtension_View* Extension = WidgetBlueprint ? UWidgetBlueprintExtension::GetExtension<UMVVMWidgetBlueprintExtension_View>(WidgetBlueprint) : nullptr;
	UMVVMBlueprintView* View = Extension ? const_cast<UMVVMWidgetBlueprintExtension_View*>(Extension)->GetBlueprintView() : nullptr;
	if (!View)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("WidgetBlueprint 에 MVVM 뷰가 없다."));
		return false;
	}

	FGuid Id;
	FMVVMBlueprintViewBinding* Binding = FGuid::Parse(BindingId, Id) ? View->GetBinding(Id) : nullptr;
	if (!Binding)
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("바인딩 '%s' 가 없다."), *BindingId));
		return false;
	}

	const UFunction* Function = Cast<UFunction>(FSoftObjectPath(FunctionPath).ResolveObject());
	if (!Function)
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("함수 '%s' 를 찾지 못했다."), *FunctionPath));
		return false;
	}

	TSharedPtr<FJsonObject> ArgumentsObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ArgumentsJson);
	if (!FJsonSerializer::Deserialize(Reader, ArgumentsObject) || !ArgumentsObject.IsValid())
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("JSON 파싱 실패: %s"), *ArgumentsJson));
		return false;
	}

	// 서브시스템 호출은 트랜잭션·재컴파일 마킹을 일으키므로, 중간에 끊겨 절반만 연결되지 않게 경로를 전부 해석한 뒤 시작한다.
	TArray<TPair<FName, FMVVMBlueprintPropertyPath>> Arguments;
	for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : ArgumentsObject->Values)
	{
		// 엔진은 없는 핀 이름을 check 로 받아 에디터가 크래시한다.
		const FProperty* Parameter = FindFProperty<FProperty>(Function, FName(*Pair.Key));
		if (!Parameter || !Parameter->HasAnyPropertyFlags(CPF_Parm) || Parameter->HasAnyPropertyFlags(CPF_ReturnParm))
		{
			UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("'%s' 에 입력 파라미터 '%s' 가 없다."), *FunctionPath, *Pair.Key));
			return false;
		}

		FString PathString;
		TArray<FString> Segments;
		if (!Pair.Value->TryGetString(PathString) || PathString.ParseIntoArray(Segments, TEXT(".")) < 2)
		{
			UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("인자 '%s' 의 경로는 \"소스.필드\" 문자열이어야 한다."), *Pair.Key));
			return false;
		}

		FMVVMBlueprintPropertyPath Path;
		const UStruct* Owner = nullptr;
		if (Segments[0] == TEXT("Self"))
		{
			Path.SetSelfContext();
			Owner = WidgetBlueprint->SkeletonGeneratedClass;
		}
		else if (const FMVVMBlueprintViewModelContext* Context = View->FindViewModel(FName(*Segments[0])))
		{
			Path.SetViewModelId(Context->GetViewModelId());
			Owner = Context->GetViewModelClass();
		}

		for (int32 Index = 1; Index < Segments.Num(); ++Index)
		{
			const UClass* OwnerClass = Cast<UClass>(Owner);
			const FProperty* Property = Owner ? FindFProperty<FProperty>(Owner, FName(*Segments[Index])) : nullptr;
			const UFunction* Getter = !Property && OwnerClass ? OwnerClass->FindFunctionByName(FName(*Segments[Index])) : nullptr;
			if (Property)
			{
				Path.AppendPropertyPath(WidgetBlueprint, UE::MVVM::FMVVMConstFieldVariant(Property));
				const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property);
				Owner = ObjectProperty ? ObjectProperty->PropertyClass : nullptr;
			}
			else if (Getter)
			{
				Path.AppendPropertyPath(WidgetBlueprint, UE::MVVM::FMVVMConstFieldVariant(Getter));
				const FObjectPropertyBase* ReturnProperty = CastField<FObjectPropertyBase>(Getter->GetReturnProperty());
				Owner = ReturnProperty ? ReturnProperty->PropertyClass : nullptr;
			}
			else
			{
				UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("인자 '%s' 의 경로 '%s' 에서 '%s' 를 찾지 못했다."), *Pair.Key, *PathString, *Segments[Index]));
				return false;
			}
		}
		Arguments.Emplace(FName(*Pair.Key), Path);
	}

	UMVVMEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UMVVMEditorSubsystem>();
	Subsystem->SetSourceToDestinationConversionFunction(WidgetBlueprint, *Binding, FMVVMBlueprintFunctionReference(WidgetBlueprint, Function));

	// 서브시스템은 반환 타입이 목적지와 맞지 않으면 변환 함수를 조용히 비운다.
	if (!Binding->Conversion.GetConversionFunction(true))
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("'%s' 를 이 바인딩의 변환 함수로 쓸 수 없다. 반환 타입이 목적지와 맞는지 확인한다."), *FunctionPath));
		return false;
	}

	for (const TPair<FName, FMVVMBlueprintPropertyPath>& Argument : Arguments)
	{
		Subsystem->SetPathForConversionFunctionArgument(WidgetBlueprint, *Binding, FMVVMBlueprintPinId(TArray<FName>{Argument.Key}), Argument.Value, true);
	}
	return true;
}
