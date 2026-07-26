// Copyright Woogle. All Rights Reserved.

#include "WxActorLocatorEditor.h"

#include "DragAndDrop/ActorDragDropOp.h"
#include "GameFramework/Actor.h"
#include "IUniversalObjectLocatorCustomization.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyHandle.h"
#include "SceneOutlinerDragDrop.h"
#include "String/Split.h"
#include "Styling/AppStyle.h"
#include "UniversalObjectLocator.h"
#include "UniversalObjectLocators/ActorLocatorFragment.h"
#include "Widgets/Layout/SBox.h"

#define LOCTEXT_NAMESPACE "WxActorLocatorEditor"

namespace
{
	/** 드래그 오퍼레이션에서 액터 드래그를 꺼낸다. 아웃라이너 래핑·단독 액터 드래그 둘 다 지원(스톡 재현). */
	TSharedPtr<FActorDragDropOp> GetActorDrag(const TSharedPtr<FDragDropOperation>& DragOperation)
	{
		if (DragOperation->IsOfType<FSceneOutlinerDragDropOp>())
		{
			FSceneOutlinerDragDropOp* SceneOutlinerOp = static_cast<FSceneOutlinerDragDropOp*>(DragOperation.Get());
			return SceneOutlinerOp->GetSubOp<FActorDragDropOp>();
		}
		if (DragOperation->IsOfType<FActorDragDropOp>())
		{
			return StaticCastSharedPtr<FActorDragDropOp>(DragOperation);
		}
		return nullptr;
	}

	/** 편집 대상 프로퍼티의 AllowedClasses 메타를 허용 클래스 목록으로 해석한다. 메타가 없으면 빈 목록(전체 허용). */
	TArray<const UClass*> ResolveAllowedClasses(const TSharedPtr<IPropertyHandle>& PropertyHandle)
	{
		TArray<const UClass*> AllowedClasses;

		const FProperty* Property = PropertyHandle.IsValid() ? PropertyHandle->GetProperty() : nullptr;
		if (!Property)
		{
			return AllowedClasses;
		}

		// 배열 필드는 원소 핸들로 들어오므로 메타는 소유 프로퍼티에서 읽는다(엔진 에셋 픽커와 동일 패턴).
		const FString& MetaString = Property->GetOwnerProperty()->GetMetaData(TEXT("AllowedClasses"));
		if (MetaString.IsEmpty())
		{
			return AllowedClasses;
		}

		TArray<FString> ClassPaths;
		MetaString.ParseIntoArray(ClassPaths, TEXT(","));
		for (FString& ClassPath : ClassPaths)
		{
			ClassPath.TrimStartAndEndInline();
			if (const UClass* Class = FSoftClassPath(ClassPath).ResolveClass())
			{
				AllowedClasses.Add(Class);
			}
		}

		return AllowedClasses;
	}

	/** 핸들의 현재 프래그먼트가 가리키는 액터를 해석한다(스톡 재현). */
	AActor* GetActorFromHandle(const TSharedPtr<UE::UniversalObjectLocator::IFragmentEditorHandle>& Handle)
	{
		if (!Handle.IsValid())
		{
			return nullptr;
		}

		const FUniversalObjectLocatorFragment& Fragment = Handle->GetFragment();
		ensure(Fragment.GetFragmentTypeHandle() == FActorLocatorFragment::FragmentType);
		const FActorLocatorFragment* Payload = Fragment.GetPayloadAs(FActorLocatorFragment::FragmentType);
		return Payload ? Cast<AActor>(Payload->Path.ResolveObject()) : nullptr;
	}
}

UE::UniversalObjectLocator::ELocatorFragmentEditorType FWxActorLocatorEditor::GetLocatorFragmentEditorType() const
{
	return UE::UniversalObjectLocator::ELocatorFragmentEditorType::Absolute;
}

bool FWxActorLocatorEditor::IsDragSupported(TSharedPtr<FDragDropOperation> InDragOperation, UObject* InContext) const
{
	const TSharedPtr<FActorDragDropOp> ActorDrag = GetActorDrag(InDragOperation);
	if (!ActorDrag)
	{
		return false;
	}

	for (const TWeakObjectPtr<AActor>& WeakActor : ActorDrag->Actors)
	{
		if (WeakActor.Get())
		{
			return true;
		}
	}

	return false;
}

UObject* FWxActorLocatorEditor::ResolveDragOperation(TSharedPtr<FDragDropOperation> InDragOperation, UObject* InContext) const
{
	const TSharedPtr<FActorDragDropOp> ActorDrag = GetActorDrag(InDragOperation);
	if (!ActorDrag)
	{
		return nullptr;
	}

	for (const TWeakObjectPtr<AActor>& WeakActor : ActorDrag->Actors)
	{
		if (AActor* Actor = WeakActor.Get())
		{
			return Actor;
		}
	}

	return nullptr;
}

TSharedPtr<SWidget> FWxActorLocatorEditor::MakeEditUI(const FEditUIParameters& InParameters)
{
	AActor* InitialActor = GetActorFromHandle(InParameters.Handle);
	const bool bAllowClear = true;
	const bool bAllowPickingLevelInstanceContent = true;
	const bool bDontDisplayUseSelected = false;
	const FOnActorSelected OnActorSelected = FOnActorSelected::CreateSP(this, &FWxActorLocatorEditor::HandleActorSelected, TWeakPtr<UE::UniversalObjectLocator::IFragmentEditorHandle>(InParameters.Handle));
	const FOnShouldFilterActor OnShouldFilterActor = FOnShouldFilterActor::CreateSP(this, &FWxActorLocatorEditor::HandleShouldFilterActor, ResolveAllowedClasses(InParameters.Customization->GetProperty()));

	return
		SNew(SBox)
		.MinDesiredWidth(400.0f)
		.MaxDesiredWidth(400.0f)
		[
			PropertyCustomizationHelpers::MakeActorPickerWithMenu(
				InitialActor,
				bAllowClear,
				bAllowPickingLevelInstanceContent,
				OnShouldFilterActor,
				OnActorSelected,
				FSimpleDelegate(),
				FSimpleDelegate(),
				bDontDisplayUseSelected)
		];
}

FText FWxActorLocatorEditor::GetDisplayText(const FUniversalObjectLocatorFragment* InFragment) const
{
	if (InFragment)
	{
		ensure(InFragment->GetFragmentTypeHandle() == FActorLocatorFragment::FragmentType);
		const FActorLocatorFragment* Payload = InFragment->GetPayloadAs(FActorLocatorFragment::FragmentType);
		if (Payload)
		{
			const FString& SubPathString = Payload->Path.GetSubPathString();
			if (!SubPathString.IsEmpty())
			{
				FStringView LevelStringView, ActorStringView;
				UE::String::SplitLast(SubPathString, TEXT("."), LevelStringView, ActorStringView);
				if (!ActorStringView.IsEmpty())
				{
					return FText::FromStringView(ActorStringView);
				}
			}
		}
	}

	return LOCTEXT("ActorLocatorName", "Actor");
}

FText FWxActorLocatorEditor::GetDisplayTooltip(const FUniversalObjectLocatorFragment* InFragment) const
{
	if (InFragment)
	{
		ensure(InFragment->GetFragmentTypeHandle() == FActorLocatorFragment::FragmentType);
		const FActorLocatorFragment* Payload = InFragment->GetPayloadAs(FActorLocatorFragment::FragmentType);
		if (Payload && Payload->Path.IsValid())
		{
			static const FTextFormat TextFormat(LOCTEXT("ActorLocatorTooltipFormat", "A reference to actor {0}"));
			return FText::Format(TextFormat, FText::FromString(Payload->Path.ToString()));
		}
	}

	return LOCTEXT("ActorLocatorTooltip", "An actor reference");
}

FSlateIcon FWxActorLocatorEditor::GetDisplayIcon(const FUniversalObjectLocatorFragment* InFragment) const
{
	return FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.Actor");
}

UClass* FWxActorLocatorEditor::ResolveClass(const FUniversalObjectLocatorFragment& InFragment, UObject* InContext) const
{
	if (UClass* Class = ILocatorFragmentEditor::ResolveClass(InFragment, InContext))
	{
		return Class;
	}

	return AActor::StaticClass();
}

FUniversalObjectLocatorFragment FWxActorLocatorEditor::MakeDefaultLocatorFragment() const
{
	FUniversalObjectLocatorFragment NewFragment(FActorLocatorFragment::FragmentType);
	return NewFragment;
}

bool FWxActorLocatorEditor::HandleShouldFilterActor(const AActor* InActor, TArray<const UClass*> AllowedClasses) const
{
	if (AllowedClasses.IsEmpty())
	{
		return true;
	}

	for (const UClass* Class : AllowedClasses)
	{
		if (InActor && InActor->IsA(Class))
		{
			return true;
		}
	}

	return false;
}

void FWxActorLocatorEditor::HandleActorSelected(AActor* InActor, TWeakPtr<UE::UniversalObjectLocator::IFragmentEditorHandle> InWeakHandle)
{
	const TSharedPtr<UE::UniversalObjectLocator::IFragmentEditorHandle> Handle = InWeakHandle.Pin();
	if (!Handle)
	{
		return;
	}

	FUniversalObjectLocatorFragment NewFragment(FActorLocatorFragment::FragmentType);
	FActorLocatorFragment* Payload = NewFragment.GetPayloadAs(FActorLocatorFragment::FragmentType);
	Payload->Path = InActor;
	Handle->SetValue(NewFragment);
}

#undef LOCTEXT_NAMESPACE
