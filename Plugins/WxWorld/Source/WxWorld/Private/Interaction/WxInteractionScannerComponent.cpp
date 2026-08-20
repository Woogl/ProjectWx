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
		World->GetTimerManager().SetTimer(ScanTimerHandle, this, &UWxInteractionScannerComponent::ScanAndPush, FMath::Max(ScanInterval, 0.01f), true);
	}

	// 설정 즉시 1회 스캔해 진입 시점의 주변 상호작용을 바로 반영한다.
	ScanAndPush();
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
	UPrimitiveComponent* Selected = GetSelectedMesh();
	if (!Selected)
	{
		return;
	}

	ServerInteract(Selected);
}

TArray<FText> UWxInteractionScannerComponent::GetPrompts() const
{
	TArray<FText> Prompts;
	Prompts.Reserve(InRangeMeshes.Num());
	for (const TWeakObjectPtr<UPrimitiveComponent>& Weak : InRangeMeshes)
	{
		if (const UPrimitiveComponent* Mesh = Weak.Get())
		{
			// 프롬프트는 대상이 IWxInteractable 로 제공한다(pull). 어느 영역의 문구인지는 이 메시가 정하므로 함께 넘긴다.
			// 인덱스 정합을 위해 대상이 없으면 빈 텍스트로 자리를 채운다.
			const IWxInteractable* Target = IWxInteractable::Find(Mesh);
			Prompts.Add(Target ? Target->GetInteractionPrompt(Mesh) : FText::GetEmpty());
		}
	}
	return Prompts;
}

int32 UWxInteractionScannerComponent::GetSelectedIndex() const
{
	return SelectedIndex;
}

UPrimitiveComponent* UWxInteractionScannerComponent::GetSelectedMesh() const
{
	if (!InRangeMeshes.IsValidIndex(SelectedIndex))
	{
		return nullptr;
	}
	return InRangeMeshes[SelectedIndex].Get();
}

void UWxInteractionScannerComponent::CycleSelection(int32 Delta)
{
	const int32 Count = InRangeMeshes.Num();
	if (Count == 0 || Delta == 0)
	{
		return;
	}

	const int32 Base = (SelectedIndex == INDEX_NONE) ? 0 : SelectedIndex;
	const int32 NewIndex = ((Base + Delta) % Count + Count) % Count;
	UpdateSelection(NewIndex);
}

void UWxInteractionScannerComponent::ServerInteract_Implementation(UPrimitiveComponent* Selected)
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

void UWxInteractionScannerComponent::ScanAndPush()
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
		if (!CanInteractNow(ASC))
		{
			UpdateInRange({});
			return;
		}
	}

	const FVector ScanOrigin = Pawn->GetActorLocation();

	// 오브젝트 쿼리는 셰이프의 오브젝트 타입만 보고 채널 응답 매트릭스를 보지 않으므로, 누가 대상인가가 콜리전 프리셋·응답과 무관하다는 설계가 유지된다.
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WxInteractionScan), /*bTraceComplex*/ false);
	QueryParams.AddIgnoredActor(Pawn);
	World->OverlapMultiByObjectType(Overlaps, ScanOrigin, FQuat::Identity, FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllObjects), FCollisionShape::MakeSphere(ScanRadius), QueryParams);

	// 스켈레탈은 피직스 애셋 바디마다 결과가 따로 오므로 컴포넌트 단위로 한 번만 검사한다.
	// 채택 여부가 아니라 검사 여부로 걸어야, 자격 미달로 탈락하는 대상(정면을 본 적 등)의 판정이 바디 수만큼 반복되지 않는다.
	TArray<const UPrimitiveComponent*> Examined;
	Examined.Reserve(Overlaps.Num());

	TArray<UPrimitiveComponent*> Candidates;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		UPrimitiveComponent* Mesh = Overlap.GetComponent();
		if (!Mesh || Examined.Contains(Mesh))
		{
			continue;
		}
		Examined.Add(Mesh);

		// 소유 액터도 그 컴포넌트도 계약 구현체가 아니면(바닥·벽·소품 등) 여기서 탈락한다.
		const IWxInteractable* Target = IWxInteractable::Find(Mesh);
		if (!Target)
		{
			continue;
		}

		// 겹쳤다는 것이 곧 사거리 판정이다 — 오버랩 구가 IsMeshInRange 와 같은 원점·반경·형상이라 다시 재지 않는다.
		// 주체별로 자격이 갈리는 대상(예: 처형은 주체가 후방이어야 뒤잡)은 활성 판정만으론 걸러지지 않으므로 소유 폰을 주체로 물어 표시를 거른다.
		// 서버는 같은 두 함수를 실제 instigator 로 다시 물어 권위 판정한다.
		if (Target->IsInteractionMeshActive(Mesh) && Target->CanBeInteractedBy(Pawn, Mesh))
		{
			// 한 액터에 여러 영역이 있으면(예: 엘리베이터) 메시 단위로 각각 잡힌다.
			Candidates.Add(Mesh);
		}
	}

	// 가까운 영역이 먼저 오도록 거리순 정렬한다(스캐너가 신규를 이 순서로 append).
	Candidates.Sort([ScanOrigin](const UPrimitiveComponent& A, const UPrimitiveComponent& B)
	{
		return FVector::DistSquared(ScanOrigin, A.GetComponentLocation()) < FVector::DistSquared(ScanOrigin, B.GetComponentLocation());
	});

	UpdateInRange(Candidates);
}

void UWxInteractionScannerComponent::UpdateInRange(const TArray<UPrimitiveComponent*>& InCandidates)
{
	// 선택 안정성을 위해 갱신 전 선택 메시를 포인터로 캐시한다. 순서가 바뀌어도 동일 메시를 다시 찾아 선택을 잇는다.
	UPrimitiveComponent* PreviousSelected = GetSelectedMesh();

	bool bChanged = false;

	for (int32 Index = InRangeMeshes.Num() - 1; Index >= 0; --Index)
	{
		UPrimitiveComponent* Existing = InRangeMeshes[Index].Get();
		if (!Existing || !InCandidates.Contains(Existing))
		{
			SetMeshHighlighted(Existing, false);
			InRangeMeshes.RemoveAt(Index);
			bChanged = true;
		}
	}

	for (UPrimitiveComponent* Candidate : InCandidates)
	{
		if (Candidate && !InRangeMeshes.Contains(Candidate))
		{
			InRangeMeshes.Add(Candidate);
			bChanged = true;
		}
	}

	// 멤버십도 그대로고 볼 대상도 없으면 비교할 문구조차 없다 — 주변에 아무것도 없는 대부분의 스캔이 여기서 끝나 아래 프롬프트 수집 비용이 들지 않는다.
	if (!bChanged && InRangeMeshes.IsEmpty())
	{
		return;
	}

	if (bChanged)
	{
		const int32 RestoredIndex = PreviousSelected ? InRangeMeshes.IndexOfByKey(PreviousSelected) : INDEX_NONE;
		SelectedIndex = InRangeMeshes.IsEmpty() ? INDEX_NONE : (RestoredIndex != INDEX_NONE ? RestoredIndex : 0);

		ApplyHighlight();
	}

	// 프롬프트는 대상에서 pull 하는 값이라 멤버십이 그대로여도 문구만 바뀔 수 있다(상태가 바뀌어도 그 영역을 끄지 않는 기믹).
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
	const int32 Clamped = InRangeMeshes.IsEmpty() ? INDEX_NONE : FMath::Clamp(NewIndex, 0, InRangeMeshes.Num() - 1);
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
	for (int32 Index = 0; Index < InRangeMeshes.Num(); ++Index)
	{
		SetMeshHighlighted(InRangeMeshes[Index].Get(), Index == SelectedIndex);
	}
}

void UWxInteractionScannerComponent::SetMeshHighlighted(UPrimitiveComponent* Mesh, bool bHighlighted) const
{
	if (!Mesh)
	{
		return;
	}

	Mesh->SetRenderCustomDepth(bHighlighted);
	if (bHighlighted)
	{
		Mesh->SetCustomDepthStencilValue(HighlightStencilValue);
	}
}

bool UWxInteractionScannerComponent::CanInteractNow(const UAbilitySystemComponent* ASC) const
{
	// 클래스 의존을 피해 Ability.Interact 애셋 태그로 찾는다(UWxBTTask_ActivateAbility 와 동일 관례).
	const FGameplayAbilityActorInfo* ActorInfo = ASC->AbilityActorInfo.Get();
	if (!ActorInfo)
	{
		return true;
	}

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->GetAssetTags().HasTag(WxGameplayTags::Ability_Interact))
		{
			return Spec.Ability->CanActivateAbility(Spec.Handle, ActorInfo);
		}
	}

	return true;
}

APawn* UWxInteractionScannerComponent::GetOwnerPawn() const
{
	const APlayerController* PC = GetController<APlayerController>();
	return PC ? PC->GetPawn() : nullptr;
}
