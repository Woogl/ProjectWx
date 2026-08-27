// Copyright Woogle. All Rights Reserved.

#include "Interaction/WxInteractionScannerComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "WxGameplayTags.h"
#include "WxInteractable.h"

UWxInteractionScannerComponent::UWxInteractionScannerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;

	// Server RPC(ServerInteract) 라우팅을 위해 복제 활성화. 복제 프로퍼티는 없다(스캐너 상태는 클라 로컬).
	// 주입으로 부착되는 동적 컴포넌트라 기본 서브오브젝트의 안정된 이름이 없다 — 원격에서 이 객체를 해소하는 수단이 복제뿐이다.
	SetIsReplicatedByDefault(true);
}

FWxOnScannerReady UWxInteractionScannerComponent::OnAnyScannerReady;

void UWxInteractionScannerComponent::BeginPlay()
{
	Super::BeginPlay();

	OnAnyScannerReady.Broadcast(this);

	if (!IsLocalController())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ScanTimerHandle, this, &UWxInteractionScannerComponent::HandleScanTimer, FMath::Max(ScanInterval, 0.01f), true);
	}

	// 설정 즉시 1회 스캔해 진입 시점의 주변 상호작용을 바로 반영한다.
	HandleScanTimer();
}

void UWxInteractionScannerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ScanTimerHandle);
	}

	// 잔여 후보를 비워 하이라이트·프롬프트·선택 VM 을 정리한다.
	UpdateInRange({});

	Super::EndPlay(EndPlayReason);
}

void UWxInteractionScannerComponent::TryInteractSelected()
{
	AActor* Selected = GetSelectedActor();
	if (!Selected)
	{
		return;
	}

	ServerInteract(Selected);
}

TArray<FText> UWxInteractionScannerComponent::GetPrompts() const
{
	TArray<FText> Prompts;
	Prompts.Reserve(InRangeActors.Num());
	for (const TWeakObjectPtr<AActor>& Weak : InRangeActors)
	{
		if (AActor* Actor = Weak.Get())
		{
			// 인덱스 정합을 위해 대상이 없으면 빈 텍스트로 자리를 채운다.
			const IWxInteractable* Target = Cast<IWxInteractable>(Actor);
			Prompts.Add(Target ? Target->GetInteractionPrompt() : FText::GetEmpty());
		}
	}
	return Prompts;
}

int32 UWxInteractionScannerComponent::GetSelectedIndex() const
{
	return SelectedIndex;
}

AActor* UWxInteractionScannerComponent::GetSelectedActor() const
{
	if (!InRangeActors.IsValidIndex(SelectedIndex))
	{
		return nullptr;
	}
	return InRangeActors[SelectedIndex].Get();
}

void UWxInteractionScannerComponent::CycleSelection(int32 Delta)
{
	const int32 Count = InRangeActors.Num();
	if (Count == 0 || Delta == 0)
	{
		return;
	}

	const int32 Base = (SelectedIndex == INDEX_NONE) ? 0 : SelectedIndex;
	const int32 NewIndex = ((Base + Delta) % Count + Count) % Count;
	UpdateSelection(NewIndex);
}

void UWxInteractionScannerComponent::ServerInteract_Implementation(AActor* Selected)
{
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

void UWxInteractionScannerComponent::HandleScanTimer()
{
	APawn* Pawn = GetOwnerPawn();
	UWorld* World = Pawn ? Pawn->GetWorld() : nullptr;
	if (!World)
	{
		// 폰이 사라지는 경로(폰 교체·언포제스·레벨 전환 대기)에서도 외곽선·목록을 남기지 않는다. 아래 상호작용 불가 게이트와 같은 정리다.
		UpdateInRange({});
		return;
	}

	if (const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn))
	{
		if (!CanActivateInteract(ASC))
		{
			UpdateInRange({});
			return;
		}
	}

	const FVector ScanOrigin = Pawn->GetActorLocation();

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WxInteractionScan), /*bTraceComplex*/ false);
	QueryParams.AddIgnoredActor(Pawn);
	World->OverlapMultiByObjectType(Overlaps, ScanOrigin, FQuat::Identity, FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllObjects), FCollisionShape::MakeSphere(ScanRadius), QueryParams);

	// 한 액터의 컴포넌트·스켈레탈 바디마다 결과가 따로 오므로 액터 단위로 한 번만 검사한다.
	// 채택 여부가 아니라 검사 여부로 걸어야, 자격 미달로 탈락하는 대상(정면을 본 적 등)의 판정이 결과 수만큼 반복되지 않는다.
	TArray<const AActor*> Examined;
	Examined.Reserve(Overlaps.Num());

	TArray<AActor*> Candidates;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		if (!Actor || Examined.Contains(Actor))
		{
			continue;
		}
		Examined.Add(Actor);

		const IWxInteractable* Target = Cast<IWxInteractable>(Actor);
		if (!Target)
		{
			continue;
		}
		
		if (Target->CanInteract(Pawn))
		{
			Candidates.Add(Actor);
		}
	}

	// 신규 후보는 이 순서 그대로 목록 뒤에 붙는다.
	Candidates.Sort([ScanOrigin](const AActor& A, const AActor& B)
	{
		return FVector::DistSquared(ScanOrigin, A.GetActorLocation()) < FVector::DistSquared(ScanOrigin, B.GetActorLocation());
	});

	UpdateInRange(Candidates);
}

void UWxInteractionScannerComponent::UpdateInRange(const TArray<AActor*>& InCandidates)
{
	// 선택 안정성을 위해 갱신 전 선택 액터를 포인터로 캐시한다. 순서가 바뀌어도 동일 액터를 다시 찾아 선택을 잇는다.
	AActor* PreviousSelected = GetSelectedActor();

	bool bChanged = false;

	for (int32 Index = InRangeActors.Num() - 1; Index >= 0; --Index)
	{
		AActor* Existing = InRangeActors[Index].Get();
		if (!Existing || !InCandidates.Contains(Existing))
		{
			SetActorHighlighted(Existing, false);
			InRangeActors.RemoveAt(Index);
			bChanged = true;
		}
	}

	for (AActor* Candidate : InCandidates)
	{
		if (Candidate && !InRangeActors.Contains(Candidate))
		{
			InRangeActors.Add(Candidate);
			bChanged = true;
		}
	}

	// 멤버십도 그대로고 볼 대상도 없으면 비교할 문구조차 없다 — 주변에 아무것도 없는 대부분의 스캔이 여기서 끝나 아래 프롬프트 수집 비용이 들지 않는다.
	if (!bChanged && InRangeActors.IsEmpty())
	{
		return;
	}

	if (bChanged)
	{
		const int32 RestoredIndex = PreviousSelected ? InRangeActors.IndexOfByKey(PreviousSelected) : INDEX_NONE;
		SelectedIndex = InRangeActors.IsEmpty() ? INDEX_NONE : (RestoredIndex != INDEX_NONE ? RestoredIndex : 0);

		ApplyHighlight();
	}

	// 프롬프트는 대상에서 pull 하는 값이라 멤버십이 그대로여도 문구만 바뀔 수 있다(상태가 바뀌어도 상호작용을 끄지 않는 장치).
	TArray<FText> Prompts = GetPrompts();
	bool bPromptsChanged = Prompts.Num() != LastPrompts.Num();
	for (int32 Index = 0; !bPromptsChanged && Index < Prompts.Num(); ++Index)
	{
		bPromptsChanged = !Prompts[Index].EqualTo(LastPrompts[Index]);
	}

	if (bPromptsChanged)
	{
		LastPrompts = MoveTemp(Prompts);
		OnListChanged.Broadcast(LastPrompts);
	}

	if (bChanged)
	{
		OnSelectionChanged.Broadcast(SelectedIndex);
	}
}

void UWxInteractionScannerComponent::UpdateSelection(int32 NewIndex)
{
	const int32 Clamped = InRangeActors.IsEmpty() ? INDEX_NONE : FMath::Clamp(NewIndex, 0, InRangeActors.Num() - 1);
	if (Clamped == SelectedIndex)
	{
		return;
	}

	SelectedIndex = Clamped;
	ApplyHighlight();
	OnSelectionChanged.Broadcast(SelectedIndex);
}

void UWxInteractionScannerComponent::ApplyHighlight()
{
	for (int32 Index = 0; Index < InRangeActors.Num(); ++Index)
	{
		SetActorHighlighted(InRangeActors[Index].Get(), Index == SelectedIndex);
	}
}

void UWxInteractionScannerComponent::SetActorHighlighted(AActor* Actor, bool bHighlighted) const
{
	if (!Actor)
	{
		return;
	}

	// 보이지 않는 것은 건너뛴다 — 캡슐·트리거처럼 렌더링되지 않는 형상까지 켜 봐야 외곽선에 기여하지 않는다.
	for (UActorComponent* Component : Actor->GetComponents())
	{
		UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component);
		if (!Primitive || !Primitive->IsVisible())
		{
			continue;
		}

		Primitive->SetRenderCustomDepth(bHighlighted);
		if (bHighlighted)
		{
			Primitive->SetCustomDepthStencilValue(HighlightStencilValue);
		}
	}
}

bool UWxInteractionScannerComponent::CanActivateInteract(const UAbilitySystemComponent* ASC) const
{
	// 애셋 태그로 어빌리티를 지목하는 것은 UWxBTTask_ActivateAbility 와 동일한 관례다.
	const FGameplayAbilityActorInfo* ActorInfo = ASC->AbilityActorInfo.Get();
	if (!ActorInfo)
	{
		return false;
	}

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->GetAssetTags().HasTag(WxGameplayTags::Ability_Interact))
		{
			return Spec.Ability->CanActivateAbility(Spec.Handle, ActorInfo);
		}
	}

	return false;
}

APawn* UWxInteractionScannerComponent::GetOwnerPawn() const
{
	const APlayerController* PC = GetController<APlayerController>();
	return PC ? PC->GetPawn() : nullptr;
}
