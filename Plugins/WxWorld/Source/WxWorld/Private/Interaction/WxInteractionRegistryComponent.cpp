// Copyright Woogle. All Rights Reserved.

#include "Interaction/WxInteractionRegistryComponent.h"
#include "Interaction/WxInteractionComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "WxCollisionChannels.h"
#include "WxGameplayTags.h"

UWxInteractionRegistryComponent::UWxInteractionRegistryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// Server RPC(ServerInteract) 라우팅을 위해 복제 활성화. 복제 프로퍼티는 없다(레지스트리 상태는 클라 로컬).
	SetIsReplicatedByDefault(true);
}

void UWxInteractionRegistryComponent::BeginPlay()
{
	Super::BeginPlay();

	// 감지는 로컬 어포던스. 소유 클라(리슨호스트 포함)에서만 주기 스캔한다. 데디 서버 PC 는 스캔하지 않는다.
	const APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ScanTimerHandle, this, &UWxInteractionRegistryComponent::ScanAndPush, FMath::Max(ScanInterval, 0.01f), true);
	}

	// 설정 즉시 1회 스캔해 진입 시점의 주변 상호작용을 바로 반영한다.
	ScanAndPush();
}

void UWxInteractionRegistryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ScanTimerHandle);
	}

	// 잔여 후보를 비워 하이라이트·프롬프트·선택 VM 을 정리한다.
	UpdateInRange({});

	Super::EndPlay(EndPlayReason);
}

void UWxInteractionRegistryComponent::TryInteractSelected()
{
	// 로컬 선택을 읽어 서버로 전송한다. 선택이 없으면 무동작.
	UWxInteractionComponent* Selected = GetSelectedComponent();
	if (!Selected)
	{
		return;
	}

	ServerInteract(Selected);
}

void UWxInteractionRegistryComponent::ServerInteract_Implementation(UWxInteractionComponent* Selected)
{
	// 선택 대상을 이벤트 페이로드에 실어 폰 ASC 로 송출한다.
	// ServerOnly WxAbility_Interact 가 권위에서 트리거되어 차단태그 게이트·사거리검증 후 TryInteract 한다.
	APawn* Pawn = GetOwnerPawn();
	if (!Pawn)
	{
		return;
	}

	FGameplayEventData EventData;
	EventData.Instigator = Pawn;
	EventData.EventTag = WxGameplayTags::Event_Interact;
	EventData.OptionalObject = Selected;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Pawn, WxGameplayTags::Event_Interact, EventData);
}

void UWxInteractionRegistryComponent::ScanAndPush()
{
	APawn* Pawn = GetOwnerPawn();
	UWorld* World = Pawn ? Pawn->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	// 상호작용 불가(사망·처형 중)면 스캔하지 않고 후보를 비워 프롬프트·하이라이트를 정리한다.
	// ServerOnly 어빌리티의 ActivationBlockedTags(State.Dead/State.Finisher) 와 동일한 게이트를 클라 표시용으로 미러링한다.
	if (const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn))
	{
		if (ASC->HasMatchingGameplayTag(WxGameplayTags::State_Dead) || ASC->HasMatchingGameplayTag(WxGameplayTags::State_Finisher))
		{
			UpdateInRange({});
			return;
		}
	}

	const FVector ScanOrigin = Pawn->GetActorLocation();

	// 볼륨 메시가 WxInteractable 채널에 Overlap 응답으로 표식되므로 채널 오버랩으로 수집한다.
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WxInteractionScan), false);

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByChannel(Overlaps, ScanOrigin, FQuat::Identity, ECC_WxInteractable, FCollisionShape::MakeSphere(ScanRadius), QueryParams);

	// 오버랩 결과는 볼륨 프리미티브이므로 이를 참조하는 상호작용 컴포넌트로 역참조한다.
	// 한 액터에 여러 영역이 있으면(예: 엘리베이터) 컴포넌트 단위로 각각 수집한다.
	TArray<UWxInteractionComponent*> Candidates;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (UWxInteractionComponent* Component = UWxInteractionComponent::FindByCollisionVolume(Overlap.GetComponent()))
		{
			Candidates.AddUnique(Component);
		}
	}

	// 가까운 영역이 먼저 오도록 거리순 정렬한다(레지스트리가 신규를 이 순서로 append).
	Candidates.Sort([ScanOrigin](const UWxInteractionComponent& A, const UWxInteractionComponent& B)
	{
		return FVector::DistSquared(ScanOrigin, A.GetInteractionLocation()) < FVector::DistSquared(ScanOrigin, B.GetInteractionLocation());
	});

	UpdateInRange(Candidates);
}

void UWxInteractionRegistryComponent::UpdateInRange(const TArray<UWxInteractionComponent*>& InCandidates)
{
	// 선택 안정성을 위해 갱신 전 선택 컴포넌트를 포인터로 캐시한다. 순서가 바뀌어도 동일 컴포넌트를 다시 찾아 선택을 잇는다.
	UWxInteractionComponent* PreviousSelected = GetSelectedComponent();

	bool bChanged = false;

	// 이탈/파괴 제거: 새 후보 집합에 없는 기존 항목을 떼고 강조를 끈다.
	for (int32 Index = InRangeComponents.Num() - 1; Index >= 0; --Index)
	{
		UWxInteractionComponent* Existing = InRangeComponents[Index].Get();
		if (!Existing || !InCandidates.Contains(Existing))
		{
			if (Existing)
			{
				Existing->SetHighlightEnabled(false);
			}
			InRangeComponents.RemoveAt(Index);
			bChanged = true;
		}
	}

	// 신규 추가: 기존에 없던 후보를 뒤에 붙인다(후보는 거리순이라 가까운 것부터 들어온다).
	for (UWxInteractionComponent* Candidate : InCandidates)
	{
		if (Candidate && !InRangeComponents.Contains(Candidate))
		{
			InRangeComponents.Add(Candidate);
			bChanged = true;
		}
	}

	if (!bChanged)
	{
		return;
	}

	// 선택 복원: 캐시한 컴포넌트가 남아 있으면 그 인덱스로, 없으면 비었을 때 INDEX_NONE / 아니면 0.
	const int32 RestoredIndex = PreviousSelected ? InRangeComponents.IndexOfByKey(PreviousSelected) : INDEX_NONE;
	SelectedIndex = InRangeComponents.IsEmpty() ? INDEX_NONE : (RestoredIndex != INDEX_NONE ? RestoredIndex : 0);

	ApplyHighlight();
	OnListChanged.Broadcast(GetPrompts());
	OnSelectionChanged.Broadcast(SelectedIndex);
}

TArray<FText> UWxInteractionRegistryComponent::GetPrompts() const
{
	TArray<FText> Prompts;
	Prompts.Reserve(InRangeComponents.Num());
	for (const TWeakObjectPtr<UWxInteractionComponent>& Weak : InRangeComponents)
	{
		if (const UWxInteractionComponent* Component = Weak.Get())
		{
			Prompts.Add(Component->GetInteractionText());
		}
	}
	return Prompts;
}

UWxInteractionComponent* UWxInteractionRegistryComponent::GetSelectedComponent() const
{
	if (!InRangeComponents.IsValidIndex(SelectedIndex))
	{
		return nullptr;
	}
	return InRangeComponents[SelectedIndex].Get();
}

void UWxInteractionRegistryComponent::CycleSelection(int32 Delta)
{
	const int32 Count = InRangeComponents.Num();
	if (Count == 0 || Delta == 0)
	{
		return;
	}

	const int32 Base = (SelectedIndex == INDEX_NONE) ? 0 : SelectedIndex;
	const int32 NewIndex = ((Base + Delta) % Count + Count) % Count;
	UpdateSelection(NewIndex);
}

void UWxInteractionRegistryComponent::UpdateSelection(int32 NewIndex)
{
	const int32 Clamped = InRangeComponents.IsEmpty() ? INDEX_NONE : FMath::Clamp(NewIndex, 0, InRangeComponents.Num() - 1);
	if (Clamped == SelectedIndex)
	{
		return;
	}

	SelectedIndex = Clamped;
	ApplyHighlight();
	OnSelectionChanged.Broadcast(SelectedIndex);
}

void UWxInteractionRegistryComponent::ApplyHighlight()
{
	for (int32 Index = 0; Index < InRangeComponents.Num(); ++Index)
	{
		if (UWxInteractionComponent* Component = InRangeComponents[Index].Get())
		{
			Component->SetHighlightEnabled(Index == SelectedIndex);
		}
	}
}

APawn* UWxInteractionRegistryComponent::GetOwnerPawn() const
{
	const APlayerController* PC = Cast<APlayerController>(GetOwner());
	return PC ? PC->GetPawn() : nullptr;
}
