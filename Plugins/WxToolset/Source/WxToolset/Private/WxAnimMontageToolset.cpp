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
		NotifyObject->SetStringField(TEXT("object"), GetObjectPath(NotifyInstance));
		NotifyObject->SetNumberField(TEXT("time"), NotifyEvent.GetTriggerTime());
		NotifyObject->SetNumberField(TEXT("duration"), NotifyEvent.GetDuration());
		NotifyObject->SetNumberField(TEXT("endTrigger"), NotifyEvent.GetEndTriggerTime());
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

bool UWxAnimMontageToolset::AppendMontage(UAnimMontage* Target, UAnimMontage* Source, const TArray<FName>& SectionNames)
{
	if (!ValidateMontage(Target, TEXT("대상")) || !ValidateMontage(Source, TEXT("원본")))
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

	// 슬롯은 몽타주가 어느 재생 경로로 흐르는지를 정하므로 같아야 이어 붙일 수 있다.
	if (Source->SlotAnimTracks.Num() != Target->SlotAnimTracks.Num())
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("두 몽타주의 슬롯 수가 다르다."));
		return false;
	}
	for (int32 SlotIndex = 0; SlotIndex < Source->SlotAnimTracks.Num(); ++SlotIndex)
	{
		if (Source->SlotAnimTracks[SlotIndex].SlotName != Target->SlotAnimTracks[SlotIndex].SlotName)
		{
			UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("%d번 슬롯 이름이 다르다."), SlotIndex));
			return false;
		}
	}

	if (SectionNames.Num() != Source->CompositeSections.Num())
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("섹션 이름 %d개가 원본 섹션 %d개와 맞지 않다."), SectionNames.Num(), Source->CompositeSections.Num()));
		return false;
	}
	for (int32 NameIndex = 0; NameIndex < SectionNames.Num(); ++NameIndex)
	{
		const FName SectionName = SectionNames[NameIndex];
		if (SectionName.IsNone() || Target->GetSectionIndex(SectionName) != INDEX_NONE || SectionNames.Find(SectionName) != NameIndex)
		{
			UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("섹션 이름 '%s' 를 쓸 수 없다 — 비었거나 겹친다."), *SectionName.ToString()));
			return false;
		}
	}

	// 커브는 옮기지 않으므로 말없이 잃지 않게 막는다.
	if (!Source->GetCurveData().FloatCurves.IsEmpty())
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("원본에 커브가 있다. 이 도구는 커브를 옮기지 않는다."));
		return false;
	}

	Target->Modify();

	// 섹션을 추가할 때마다 엔진이 앞 섹션을 새 섹션으로 이으므로 대상의 원래 링크를 떠 둔다.
	TArray<FName> TargetNextSections;
	for (const FCompositeSection& Section : Target->CompositeSections)
	{
		TargetNextSections.Add(Section.NextSectionName);
	}

	// 몽타주 길이는 프레임 단위로 반올림될 수 있어 트랙 끝에 붙인다.
	const float Offset = Target->CalculateSequenceLength();

	for (int32 SlotIndex = 0; SlotIndex < Source->SlotAnimTracks.Num(); ++SlotIndex)
	{
		FAnimTrack& TargetTrack = Target->SlotAnimTracks[SlotIndex].AnimTrack;
		for (const FAnimSegment& SourceSegment : Source->SlotAnimTracks[SlotIndex].AnimTrack.AnimSegments)
		{
			FAnimSegment& NewSegment = TargetTrack.AnimSegments.Add_GetRef(SourceSegment);
			NewSegment.StartPos += Offset;
		}
	}

	// 엔진은 늘어난 길이를 끝에 프레임을 끼운 것으로 보고 끝에 닿은 노티파이를 늘리거나 민다. 배열 순서는 그대로 두므로 인덱스로 되돌린다.
	const TArray<FAnimNotifyEvent> SavedNotifies = Target->Notifies;
	Target->SetCompositeLength(Target->CalculateSequenceLength());
	for (int32 NotifyIndex = 0; NotifyIndex < SavedNotifies.Num(); ++NotifyIndex)
	{
		const FAnimNotifyEvent& SavedEvent = SavedNotifies[NotifyIndex];
		FAnimNotifyEvent& NotifyEvent = Target->Notifies[NotifyIndex];
		NotifyEvent.Link(Target, SavedEvent.GetTime(), SavedEvent.GetSlotIndex());
		NotifyEvent.TriggerTimeOffset = SavedEvent.TriggerTimeOffset;
		if (SavedEvent.NotifyStateClass)
		{
			NotifyEvent.EndLink.Link(Target, SavedEvent.EndLink.GetTime(), SavedEvent.EndLink.GetSlotIndex());
			NotifyEvent.SetDuration(SavedEvent.GetDuration());
			NotifyEvent.EndTriggerTimeOffset = SavedEvent.EndTriggerTimeOffset;
		}
	}

	const int32 FirstNewSectionIndex = Target->CompositeSections.Num();
	for (int32 SectionIndex = 0; SectionIndex < Source->CompositeSections.Num(); ++SectionIndex)
	{
		if (Target->AddAnimCompositeSection(SectionNames[SectionIndex], Offset + Source->GetAnimCompositeSection(SectionIndex).GetTime()) == INDEX_NONE)
		{
			UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("섹션 '%s' 를 추가하지 못했다."), *SectionNames[SectionIndex].ToString()));
			return false;
		}
	}

	for (int32 SectionIndex = 0; SectionIndex < TargetNextSections.Num(); ++SectionIndex)
	{
		Target->CompositeSections[SectionIndex].NextSectionName = TargetNextSections[SectionIndex];
	}
	for (int32 SectionIndex = 0; SectionIndex < Source->CompositeSections.Num(); ++SectionIndex)
	{
		const int32 SourceNextIndex = Source->GetSectionIndex(Source->GetAnimCompositeSection(SectionIndex).NextSectionName);
		Target->CompositeSections[FirstNewSectionIndex + SectionIndex].NextSectionName = Source->IsValidSectionIndex(SourceNextIndex) ? SectionNames[SourceNextIndex] : NAME_None;
	}

	// 없는 트랙을 가리키는 노티파이는 엔진이 보정하면서 ensure를 낸다.
	while (Target->AnimNotifyTracks.Num() < Source->AnimNotifyTracks.Num())
	{
		const FAnimNotifyTrack& SourceTrack = Source->AnimNotifyTracks[Target->AnimNotifyTracks.Num()];
		FAnimNotifyTrack NewTrack;
		NewTrack.TrackName = SourceTrack.TrackName;
		NewTrack.TrackColor = SourceTrack.TrackColor;
		Target->AnimNotifyTracks.Add(NewTrack);
	}

	for (const FAnimNotifyEvent& SourceEvent : Source->Notifies)
	{
		FAnimNotifyEvent NewEvent = SourceEvent;
		NewEvent.Guid = FGuid::NewGuid();

		// 복사한 링크는 원본 몽타주를 가리키므로 대상 기준으로 다시 건다. 구간의 끝도 따로 링크된다.
		// 트리거 오프셋은 원본 것을 둔다 — 엔진은 섹션 시작의 노티파이를 앞 섹션 끝에서 불리게 잡으므로, 다시 계산하면 단 시작의 노티파이가 앞 단으로 넘어간다.
		const float NewTime = Offset + SourceEvent.GetTime();
		NewEvent.Link(Target, NewTime, SourceEvent.GetSlotIndex());

		if (SourceEvent.Notify)
		{
			NewEvent.Notify = DuplicateObject<UAnimNotify>(SourceEvent.Notify, Target);
		}
		if (SourceEvent.NotifyStateClass)
		{
			NewEvent.NotifyStateClass = DuplicateObject<UAnimNotifyState>(SourceEvent.NotifyStateClass, Target);
			NewEvent.EndLink.Link(Target, NewTime + SourceEvent.GetDuration(), SourceEvent.EndLink.GetSlotIndex());
			NewEvent.SetDuration(SourceEvent.GetDuration());
		}

		Target->Notifies.Add(NewEvent);
	}

	// 원본 끝에 닿아 있던 구간은 이제 섹션 경계에서 끝나야 한다. 허용 오차는 한 프레임보다 한참 작게 둔다.
	SnapNotifyEndsToSections(Target, 0.002f);
	return true;
}

bool UWxAnimMontageToolset::RenameSection(UAnimMontage* Montage, FName OldName, FName NewName)
{
	if (!ValidateMontage(Montage, TEXT("대상")))
	{
		return false;
	}

	if (Montage->GetSectionIndex(OldName) == INDEX_NONE)
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("섹션 '%s' 가 없다."), *OldName.ToString()));
		return false;
	}

	if (NewName.IsNone() || Montage->GetSectionIndex(NewName) != INDEX_NONE)
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("섹션 이름 '%s' 를 쓸 수 없다 — 비었거나 이미 있다."), *NewName.ToString()));
		return false;
	}

	Montage->Modify();

	for (FCompositeSection& Section : Montage->CompositeSections)
	{
		if (Section.SectionName == OldName)
		{
			Section.SectionName = NewName;
		}
		if (Section.NextSectionName == OldName)
		{
			Section.NextSectionName = NewName;
		}
	}

	Montage->RefreshCacheData();
	Montage->PostEditChange();
	Montage->MarkPackageDirty();
	return true;
}

int32 UWxAnimMontageToolset::SnapNotifyEndsToSections(UAnimMontage* Montage, float Tolerance)
{
	if (!ValidateMontage(Montage, TEXT("대상")))
	{
		return 0;
	}

	// 경계 판정(CalculateOffsetFromSections)이 섹션 시각과의 정확한 일치를 보므로 섹션이 돌려주는 값을 그대로 쓴다.
	TArray<float> Boundaries;
	for (int32 SectionIndex = 1; SectionIndex < Montage->CompositeSections.Num(); ++SectionIndex)
	{
		Boundaries.Add(Montage->GetAnimCompositeSection(SectionIndex).GetTime());
	}
	Boundaries.Add(Montage->GetPlayLength());

	Montage->Modify();

	int32 SnappedCount = 0;
	for (FAnimNotifyEvent& NotifyEvent : Montage->Notifies)
	{
		if (!NotifyEvent.NotifyStateClass)
		{
			continue;
		}

		const float StartTime = NotifyEvent.GetTime();
		const float EndTime = StartTime + NotifyEvent.GetDuration();
		for (const float Boundary : Boundaries)
		{
			if (Boundary <= StartTime || FMath::Abs(EndTime - Boundary) > Tolerance)
			{
				continue;
			}

			// 상대·비율 링크는 시각을 다시 계산하며 오차를 만들므로 절대 시각으로 고정한다.
			NotifyEvent.SetDuration(Boundary - StartTime);
			NotifyEvent.EndLink.ChangeLinkMethod(EAnimLinkMethod::Absolute);
			NotifyEvent.EndLink.Link(Montage, Boundary, NotifyEvent.EndLink.GetSlotIndex());
			NotifyEvent.EndTriggerTimeOffset = GetTriggerTimeOffsetForType(EAnimEventTriggerOffsets::OffsetBefore);
			++SnappedCount;
			break;
		}
	}

	Montage->RefreshCacheData();
	Montage->PostEditChange();
	Montage->MarkPackageDirty();
	return SnappedCount;
}

int32 UWxAnimMontageToolset::SnapNotifyStartsToSections(UAnimMontage* Montage, float Tolerance, float Offset)
{
	if (!ValidateMontage(Montage, TEXT("대상")))
	{
		return 0;
	}

	if (Offset <= 0.f)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Offset은 0보다 커야 한다."));
		return 0;
	}

	TArray<float> Boundaries;
	for (int32 SectionIndex = 1; SectionIndex < Montage->CompositeSections.Num(); ++SectionIndex)
	{
		Boundaries.Add(Montage->GetAnimCompositeSection(SectionIndex).GetTime());
	}

	Montage->Modify();

	int32 SnappedCount = 0;
	for (FAnimNotifyEvent& NotifyEvent : Montage->Notifies)
	{
		if (!NotifyEvent.NotifyStateClass)
		{
			continue;
		}

		// 옮겨 온 노티파이는 섹션 시작에 놓이고도 뒤로 잡힌 오프셋으로 그 섹션에서 시작할 수 있어, 실제 트리거 시각으로 넘어가는 구간만 고른다.
		const float StartTriggerTime = NotifyEvent.GetTriggerTime();
		const float EndTriggerTime = NotifyEvent.GetEndTriggerTime();
		for (const float Boundary : Boundaries)
		{
			const float NewStartTime = Boundary + Offset;
			if (StartTriggerTime >= Boundary || Boundary - StartTriggerTime > Tolerance || EndTriggerTime <= NewStartTime)
			{
				continue;
			}

			// 상대·비율 링크는 시각을 다시 계산하며 오차를 만들므로 절대 시각으로 고정한다. 끝은 실제로 불리던 시각에 남긴다.
			NotifyEvent.ChangeLinkMethod(EAnimLinkMethod::Absolute);
			NotifyEvent.Link(Montage, NewStartTime, NotifyEvent.GetSlotIndex());
			NotifyEvent.TriggerTimeOffset = GetTriggerTimeOffsetForType(Montage->CalculateOffsetForNotify(NewStartTime));
			NotifyEvent.SetDuration(EndTriggerTime - NotifyEvent.EndTriggerTimeOffset - NotifyEvent.GetTriggerTime());
			++SnappedCount;
			break;
		}
	}

	Montage->RefreshCacheData();
	Montage->PostEditChange();
	Montage->MarkPackageDirty();
	return SnappedCount;
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
