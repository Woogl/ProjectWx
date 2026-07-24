// Copyright Woogle. All Rights Reserved.

#include "Interaction/WxInteractionScannerComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "WxCollisionChannels.h"
#include "WxGameplayTags.h"
#include "WxInteractable.h"

UWxInteractionScannerComponent::UWxInteractionScannerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// Server RPC(ServerInteract) 라우팅을 위해 복제 활성화. 복제 프로퍼티는 없다(스캐너 상태는 클라 로컬).
	SetIsReplicatedByDefault(true);
}

void UWxInteractionScannerComponent::BeginPlay()
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
	// 로컬 선택을 읽어 서버로 전송한다. 선택이 없으면 무동작.
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
			// 프롬프트는 대상 액터가 IWxInteractable 로 제공한다(pull). 인덱스 정합을 위해 대상이 없으면 빈 텍스트로 자리를 채운다.
			const IWxInteractable* Target = Cast<IWxInteractable>(Mesh->GetOwner());
			Prompts.Add(Target ? Target->GetInteractionPrompt() : FText::GetEmpty());
		}
	}
	return Prompts;
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
	// 선택 대상을 이벤트 페이로드에 실어 폰 ASC 로 송출한다.
	// ServerOnly WxAbility_Interact 가 권위에서 트리거되어 차단태그 게이트·사거리·활성 검증 후 대상 인터페이스를 호출한다.
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
		return;
	}

	// 상호작용 불가면 스캔하지 않고 후보를 비워 프롬프트·하이라이트를 정리한다.
	// 게이트는 어빌리티의 CanActivateAbility 에 위임한다(차단 태그를 컴포넌트가 하드코딩하지 않는 단일 소스).
	if (const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn))
	{
		if (!CanInteractNow(ASC))
		{
			UpdateInRange({});
			return;
		}
	}

	const FVector ScanOrigin = Pawn->GetActorLocation();

	// 대상 메시가 WxInteractable 채널에 Overlap 응답으로 표식되므로 채널 오버랩으로 수집한다.
	// 오버랩 결과가 곧 상호작용 영역이다 — 한 액터에 여러 영역이 있으면(예: 엘리베이터) 메시 단위로 각각 잡힌다.
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WxInteractionScan), false);

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByChannel(Overlaps, ScanOrigin, FQuat::Identity, ECC_WxInteractable, FCollisionShape::MakeSphere(ScanRadius), QueryParams);

	TArray<UPrimitiveComponent*> Candidates;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		UPrimitiveComponent* Mesh = Overlap.GetComponent();
		if (!Mesh)
		{
			continue;
		}

		// 주체별로 자격이 갈리는 대상(예: 처형은 주체가 후방이어야 뒤잡)은 채널만으론 걸러지지 않는다.
		// 소유 폰을 주체로 물어 표시를 거른다 — 서버는 같은 함수를 실제 instigator 로 다시 물어 권위 판정한다.
		const IWxInteractable* Target = Cast<IWxInteractable>(Mesh->GetOwner());
		if (Target && !Target->CanBeInteractedBy(Pawn, Mesh))
		{
			continue;
		}

		Candidates.AddUnique(Mesh);
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

	// 이탈/파괴 제거: 새 후보 집합에 없는 기존 항목을 떼고 강조를 끈다.
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

	// 신규 추가: 기존에 없던 후보를 뒤에 붙인다(후보는 거리순이라 가까운 것부터 들어온다).
	for (UPrimitiveComponent* Candidate : InCandidates)
	{
		if (Candidate && !InRangeMeshes.Contains(Candidate))
		{
			InRangeMeshes.Add(Candidate);
			bChanged = true;
		}
	}

	if (!bChanged)
	{
		return;
	}

	// 선택 복원: 캐시한 메시가 남아 있으면 그 인덱스로, 없으면 비었을 때 INDEX_NONE / 아니면 0.
	const int32 RestoredIndex = PreviousSelected ? InRangeMeshes.IndexOfByKey(PreviousSelected) : INDEX_NONE;
	SelectedIndex = InRangeMeshes.IsEmpty() ? INDEX_NONE : (RestoredIndex != INDEX_NONE ? RestoredIndex : 0);

	ApplyHighlight();
	OnListChanged.Broadcast(GetPrompts());
	OnSelectionChanged.Broadcast(SelectedIndex);
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
	// 상호작용 어빌리티를 Ability.Interact 애셋 태그로 찾아(클래스 의존 회피, UWxBTTask_ActivateAbility 와 동일 관례)
	// 그 CanActivateAbility 를 표시 게이트로 위임한다. 차단 조건의 단일 소스는 어빌리티다.
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

	// 어빌리티가 아직 부여되지 않았으면 게이트하지 않는다(표시를 열어둔다).
	return true;
}

APawn* UWxInteractionScannerComponent::GetOwnerPawn() const
{
	const APlayerController* PC = Cast<APlayerController>(GetOwner());
	return PC ? PC->GetPawn() : nullptr;
}
