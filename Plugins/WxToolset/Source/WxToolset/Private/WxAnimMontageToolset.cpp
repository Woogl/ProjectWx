// Copyright Woogle. All Rights Reserved.

#include "WxAnimMontageToolset.h"

#include "Animation/AnimMontage.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Dom/JsonObject.h"
#include "FileHelpers.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
	bool ValidateMontage(const UAnimMontage* Montage, const TCHAR* Label)
	{
		if (!Montage)
		{
			UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("%s 몽타주가 null 이다."), Label));
			return false;
		}
		return true;
	}

	FString GetObjectPath(const UObject* Object)
	{
		return Object ? Object->GetPathName() : FString();
	}

	/** 노티파이 오브젝트는 단발과 구간 중 한쪽에만 들어 있다. */
	const UObject* GetNotifyInstance(const FAnimNotifyEvent& NotifyEvent)
	{
		if (NotifyEvent.Notify)
		{
			return NotifyEvent.Notify;
		}
		return NotifyEvent.NotifyStateClass;
	}

	bool HasNotifyInRange(const UAnimMontage& Montage, float RangeStart, float RangeEnd)
	{
		for (const FAnimNotifyEvent& NotifyEvent : Montage.Notifies)
		{
			const float TriggerTime = NotifyEvent.GetTriggerTime();
			if (TriggerTime >= RangeStart && TriggerTime < RangeEnd)
			{
				return true;
			}
		}
		return false;
	}
}

FString UWxAnimMontageToolset::DescribeMontage(UAnimMontage* Montage)
{
	if (!ValidateMontage(Montage, TEXT("대상")))
	{
		return FString();
	}

	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("montage"), GetObjectPath(Montage));
	Root->SetNumberField(TEXT("playLength"), Montage->GetPlayLength());

	TArray<TSharedPtr<FJsonValue>> SlotValues;
	for (const FSlotAnimationTrack& SlotTrack : Montage->SlotAnimTracks)
	{
		const TSharedRef<FJsonObject> SlotObject = MakeShared<FJsonObject>();
		SlotObject->SetStringField(TEXT("slot"), SlotTrack.SlotName.ToString());

		TArray<TSharedPtr<FJsonValue>> SegmentValues;
		for (const FAnimSegment& Segment : SlotTrack.AnimTrack.AnimSegments)
		{
			const TSharedRef<FJsonObject> SegmentObject = MakeShared<FJsonObject>();
			SegmentObject->SetStringField(TEXT("anim"), GetObjectPath(Segment.GetAnimReference()));
			SegmentObject->SetNumberField(TEXT("startPos"), Segment.StartPos);
			SegmentObject->SetNumberField(TEXT("animStart"), Segment.AnimStartTime);
			SegmentObject->SetNumberField(TEXT("animEnd"), Segment.AnimEndTime);
			SegmentObject->SetNumberField(TEXT("playRate"), Segment.AnimPlayRate);
			SegmentObject->SetNumberField(TEXT("loops"), Segment.LoopingCount);
			SegmentValues.Add(MakeShared<FJsonValueObject>(SegmentObject));
		}

		SlotObject->SetArrayField(TEXT("segments"), SegmentValues);
		SlotValues.Add(MakeShared<FJsonValueObject>(SlotObject));
	}
	Root->SetArrayField(TEXT("slots"), SlotValues);

	TArray<TSharedPtr<FJsonValue>> SectionValues;
	for (int32 SectionIndex = 0; SectionIndex < Montage->CompositeSections.Num(); ++SectionIndex)
	{
		const FCompositeSection& Section = Montage->GetAnimCompositeSection(SectionIndex);
		const TSharedRef<FJsonObject> SectionObject = MakeShared<FJsonObject>();
		SectionObject->SetStringField(TEXT("name"), Section.SectionName.ToString());
		SectionObject->SetStringField(TEXT("next"), Section.NextSectionName.ToString());
		SectionObject->SetNumberField(TEXT("start"), Section.GetTime());
		SectionObject->SetNumberField(TEXT("length"), Montage->GetSectionLength(SectionIndex));
		SectionValues.Add(MakeShared<FJsonValueObject>(SectionObject));
	}
	Root->SetArrayField(TEXT("sections"), SectionValues);

	TArray<TSharedPtr<FJsonValue>> NotifyValues;
	for (const FAnimNotifyEvent& NotifyEvent : Montage->Notifies)
	{
		const TSharedRef<FJsonObject> NotifyObject = MakeShared<FJsonObject>();
		const UObject* NotifyInstance = GetNotifyInstance(NotifyEvent);
		NotifyObject->SetStringField(TEXT("name"), NotifyEvent.NotifyName.ToString());
		NotifyObject->SetStringField(TEXT("class"), NotifyInstance ? NotifyInstance->GetClass()->GetPathName() : FString());
		NotifyObject->SetNumberField(TEXT("time"), NotifyEvent.GetTriggerTime());
		NotifyObject->SetNumberField(TEXT("duration"), NotifyEvent.GetDuration());
		NotifyObject->SetNumberField(TEXT("track"), NotifyEvent.TrackIndex);

		const int32 SectionIndex = Montage->GetSectionIndexFromPosition(NotifyEvent.GetTriggerTime());
		NotifyObject->SetStringField(TEXT("section"), Montage->IsValidSectionIndex(SectionIndex) ? Montage->GetAnimCompositeSection(SectionIndex).SectionName.ToString() : FString());
		NotifyValues.Add(MakeShared<FJsonValueObject>(NotifyObject));
	}
	Root->SetArrayField(TEXT("notifies"), NotifyValues);

	FString Output;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
	FJsonSerializer::Serialize(Root, Writer);
	return Output;
}

bool UWxAnimMontageToolset::MirrorMontageStructure(UAnimMontage* Source, UAnimMontage* Target)
{
	if (!ValidateMontage(Source, TEXT("원본")) || !ValidateMontage(Target, TEXT("대상")))
	{
		return false;
	}

	if (Source == Target)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("원본과 대상이 같은 몽타주다."));
		return false;
	}

	if (Source->GetSkeleton() != Target->GetSkeleton())
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("두 몽타주의 스켈레톤이 다르다."));
		return false;
	}

	if (Source->SlotAnimTracks.IsEmpty() || Source->CompositeSections.IsEmpty())
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("원본에 슬롯 트랙이나 섹션이 없다."));
		return false;
	}

	Target->Modify();

	// 시작 시각도 지속시간도 세그먼트 링크로 표현되므로, 트랙을 갈아치우기 전에 값으로 떠 둔다.
	TArray<TPair<float, float>> NotifyTimings;
	NotifyTimings.Reserve(Target->Notifies.Num());
	for (const FAnimNotifyEvent& NotifyEvent : Target->Notifies)
	{
		NotifyTimings.Emplace(NotifyEvent.GetTime(), NotifyEvent.GetDuration());
	}

	// 슬롯 이름은 대상 것을 살린다 — 슬롯은 대상 몽타주가 어느 재생 경로로 흐르는지를 정한다.
	for (int32 SlotIndex = 0; SlotIndex < Source->SlotAnimTracks.Num(); ++SlotIndex)
	{
		if (Target->SlotAnimTracks.IsValidIndex(SlotIndex))
		{
			Target->SlotAnimTracks[SlotIndex].AnimTrack = Source->SlotAnimTracks[SlotIndex].AnimTrack;
		}
		else
		{
			Target->SlotAnimTracks.Add(Source->SlotAnimTracks[SlotIndex]);
		}
	}
	Target->SlotAnimTracks.SetNum(Source->SlotAnimTracks.Num());

	float NewLength = 0.f;
	for (const FSlotAnimationTrack& SlotTrack : Target->SlotAnimTracks)
	{
		NewLength = FMath::Max(NewLength, SlotTrack.AnimTrack.GetLength());
	}
	Target->SetCompositeLength(NewLength);

	Target->CompositeSections.Reset();
	for (int32 SectionIndex = 0; SectionIndex < Source->CompositeSections.Num(); ++SectionIndex)
	{
		const FCompositeSection& SourceSection = Source->GetAnimCompositeSection(SectionIndex);
		const int32 NewSectionIndex = Target->AddAnimCompositeSection(SourceSection.SectionName, SourceSection.GetTime());
		if (NewSectionIndex == INDEX_NONE)
		{
			UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("섹션 '%s' 를 추가하지 못했다."), *SourceSection.SectionName.ToString()));
			return false;
		}
	}

	// 섹션을 하나 추가할 때마다 엔진이 앞 섹션을 새 섹션으로 이으므로, 링크는 전부 넣은 뒤에 끊는다.
	for (FCompositeSection& Section : Target->CompositeSections)
	{
		Section.NextSectionName = NAME_None;
	}

	// 세그먼트가 갈렸으므로 노티파이 링크를 떠 둔 시각으로 다시 건다.
	for (int32 NotifyIndex = 0; NotifyIndex < Target->Notifies.Num(); ++NotifyIndex)
	{
		FAnimNotifyEvent& NotifyEvent = Target->Notifies[NotifyIndex];
		NotifyEvent.Link(Target, NotifyTimings[NotifyIndex].Key);
		if (NotifyTimings[NotifyIndex].Value > 0.f)
		{
			NotifyEvent.SetDuration(NotifyTimings[NotifyIndex].Value);
		}
	}

	Target->RefreshCacheData();
	Target->PostEditChange();
	Target->MarkPackageDirty();
	return true;
}

int32 UWxAnimMontageToolset::ReplicateNotifiesToSections(UAnimMontage* Montage, FName SourceSectionName)
{
	if (!ValidateMontage(Montage, TEXT("대상")))
	{
		return 0;
	}

	const int32 SourceSectionIndex = Montage->GetSectionIndex(SourceSectionName);
	if (!Montage->IsValidSectionIndex(SourceSectionIndex))
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("섹션 '%s' 가 없다."), *SourceSectionName.ToString()));
		return 0;
	}

	float SourceStart = 0.f;
	float SourceEnd = 0.f;
	Montage->GetSectionStartAndEndTime(SourceSectionIndex, SourceStart, SourceEnd);
	const float SourceLength = SourceEnd - SourceStart;
	if (SourceLength <= 0.f)
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("섹션 '%s' 의 길이가 0 이다."), *SourceSectionName.ToString()));
		return 0;
	}

	// 복제하면서 배열이 늘어나므로 기준 섹션의 노티파이를 먼저 값으로 떠 둔다.
	TArray<FAnimNotifyEvent> SourceNotifies;
	for (const FAnimNotifyEvent& NotifyEvent : Montage->Notifies)
	{
		const float TriggerTime = NotifyEvent.GetTriggerTime();
		if (TriggerTime >= SourceStart && TriggerTime < SourceEnd)
		{
			SourceNotifies.Add(NotifyEvent);
		}
	}

	if (SourceNotifies.IsEmpty())
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("섹션 '%s' 에 노티파이가 없다."), *SourceSectionName.ToString()));
		return 0;
	}

	Montage->Modify();

	int32 AddedCount = 0;
	for (int32 SectionIndex = 0; SectionIndex < Montage->CompositeSections.Num(); ++SectionIndex)
	{
		if (SectionIndex == SourceSectionIndex)
		{
			continue;
		}

		float TargetStart = 0.f;
		float TargetEnd = 0.f;
		Montage->GetSectionStartAndEndTime(SectionIndex, TargetStart, TargetEnd);
		const float TargetLength = TargetEnd - TargetStart;
		if (TargetLength <= 0.f || HasNotifyInRange(*Montage, TargetStart, TargetEnd))
		{
			continue;
		}

		const float LengthRatio = TargetLength / SourceLength;
		for (const FAnimNotifyEvent& SourceEvent : SourceNotifies)
		{
			FAnimNotifyEvent NewEvent = SourceEvent;
			NewEvent.Guid = FGuid::NewGuid();
			if (SourceEvent.Notify)
			{
				NewEvent.Notify = DuplicateObject<UAnimNotify>(SourceEvent.Notify, Montage);
			}
			if (SourceEvent.NotifyStateClass)
			{
				NewEvent.NotifyStateClass = DuplicateObject<UAnimNotifyState>(SourceEvent.NotifyStateClass, Montage);
			}

			// 섹션 길이가 방향마다 달라 절대 오프셋 대신 비율로 옮긴다.
			const float OffsetRatio = (SourceEvent.GetTriggerTime() - SourceStart) / SourceLength;
			NewEvent.Link(Montage, TargetStart + OffsetRatio * TargetLength);
			NewEvent.SetDuration(SourceEvent.GetDuration() * LengthRatio);

			Montage->Notifies.Add(NewEvent);
			++AddedCount;
		}
	}

	Montage->RefreshCacheData();
	Montage->PostEditChange();
	Montage->MarkPackageDirty();
	return AddedCount;
}

bool UWxAnimMontageToolset::AddNotify(UAnimMontage* Montage, TSubclassOf<UAnimNotify> NotifyClass, FName SectionName, float OffsetInSection, int32 TrackIndex)
{
	if (!ValidateMontage(Montage, TEXT("대상")))
	{
		return false;
	}

	if (!NotifyClass)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("노티파이 클래스가 비었다."));
		return false;
	}

	const int32 SectionIndex = Montage->GetSectionIndex(SectionName);
	if (!Montage->IsValidSectionIndex(SectionIndex))
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("섹션 '%s' 가 없다."), *SectionName.ToString()));
		return false;
	}

	float SectionStart = 0.f;
	float SectionEnd = 0.f;
	Montage->GetSectionStartAndEndTime(SectionIndex, SectionStart, SectionEnd);
	const float TriggerTime = SectionStart + OffsetInSection;
	if (OffsetInSection < 0.f || TriggerTime >= SectionEnd)
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("오프셋 %.3f 이 섹션 '%s' 밖이다."), OffsetInSection, *SectionName.ToString()));
		return false;
	}

	if (TrackIndex < 0)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("트랙 인덱스가 음수다."));
		return false;
	}

	Montage->Modify();

	// 에디터 트랙 표시는 AnimNotifyTracks 를 따르므로 지목한 인덱스까지 채운다.
	while (!Montage->AnimNotifyTracks.IsValidIndex(TrackIndex))
	{
		FAnimNotifyTrack NewTrack;
		NewTrack.TrackName = *FString::FromInt(Montage->AnimNotifyTracks.Num() + 1);
		NewTrack.TrackColor = FLinearColor::White;
		Montage->AnimNotifyTracks.Add(NewTrack);
	}

	FAnimNotifyEvent& NewEvent = Montage->Notifies.AddDefaulted_GetRef();
	NewEvent.Guid = FGuid::NewGuid();
	NewEvent.TrackIndex = TrackIndex;
	NewEvent.Notify = NewObject<UAnimNotify>(Montage, NotifyClass, NAME_None, RF_Transactional);
	NewEvent.NotifyName = FName(*NewEvent.Notify->GetNotifyName());
	NewEvent.Link(Montage, TriggerTime);
	NewEvent.TriggerTimeOffset = GetTriggerTimeOffsetForType(Montage->CalculateOffsetForNotify(TriggerTime));

	Montage->RefreshCacheData();
	Montage->PostEditChange();
	Montage->MarkPackageDirty();
	return true;
}

bool UWxAnimMontageToolset::SaveMontage(UAnimMontage* Montage)
{
	if (!ValidateMontage(Montage, TEXT("대상")))
	{
		return false;
	}

	UPackage* Package = Montage->GetPackage();
	if (!Package)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("몽타주의 패키지를 찾지 못했다."));
		return false;
	}

	TArray<UPackage*> PackagesToSave;
	PackagesToSave.Add(Package);
	return UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, false);
}
