// Copyright Woogle. All Rights Reserved.

#include "WorldObject/WxCheckPoint.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Interaction/WxInteractionComponent.h"
#include "Inventory/WxInventoryManagerComponent.h"
#include "System/WxSpawnerLibrary.h"
#include "WxGameplayTags.h"
#include "WxSaveGameSubsystem.h"

AWxCheckPoint::AWxCheckPoint()
{
	// 컴포넌트는 베이스(AWxGimmick)가 만든 SceneRoot 에 부착한다. bReplicates·SceneRoot·StateTree·WxSaveId 는 베이스가 제공한다.
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetRelativeLocation(FVector(-90.f, 0.f, 0.f));

	InteractionComponent = CreateDefaultSubobject<UWxInteractionComponent>(TEXT("InteractionComponent"));
	InteractionComponent->SetupAttachment(MeshComponent);
	InteractionComponent->SetHighlightTarget(MeshComponent);

	// 초기 상태는 불 꺼짐. 상호작용 시 Lit 으로 확정된다.
	State = WxGameplayTags::Gimmick_CheckPoint_Unlit;
}

void AWxCheckPoint::OnInteracted(AActor* Interactor, UActorComponent* Source)
{
	// 서버 권위(TryInteract)에서만 호출된다.
	// 불을 켠다. State 는 복제 + SaveGame 으로 지속돼 재로드 후에도 Lit 을 유지하며, 비주얼은 GimmickStateTree 가 적용한다.
	CommitGimmickState(WxGameplayTags::Gimmick_CheckPoint_Lit);

	if (HealEffect)
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Interactor))
		{
			FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
			Context.AddSourceObject(this);

			FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(HealEffect, 1.0f, Context);
			if (SpecHandle.IsValid())
			{
				ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}

	// 에스트병 등 충전형 소비 아이템의 충전을 가득 채운다(다크소울 모닥불 방식).
	// 충전형이 아닌 아이템은 내부에서 무시된다.
	if (UWxInventoryManagerComponent* Inventory = UWxInventoryManagerComponent::FindInventory(Interactor))
	{
		for (UWxItemInstance* Item : Inventory->GetAllItems())
		{
			Inventory->RefillItemCharges(Item);
		}
	}

	UWxSpawnerLibrary::TryRespawnAll(this);

	// 리셋된 월드 상태 + Lit State 를 활성 슬롯에 저장한다(HasAuthority 게이트 안이라 서버 전용).
	// 재개 지점은 세이브 플러시가 플레이어 위치로 캡처한다 — 지금 플레이어가 이 앞에 서 있으므로 그 값이 곧 이 체크포인트 자리다.
	// CommitGimmickState 이후 실행돼 순서가 보장된다(슬롯 정체성은 활성 SaveGame 이 보유).
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UWxSaveGameSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UWxSaveGameSubsystem>())
		{
			SaveSubsystem->SaveToFile();
		}
	}
}
