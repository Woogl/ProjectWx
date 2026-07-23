// Copyright Woogle. All Rights Reserved.

#include "WorldObject/WxCheckPoint.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Inventory/WxInventoryManagerComponent.h"
#include "System/WxSpawnerLibrary.h"
#include "WxCollisionChannels.h"
#include "WxGameplayTags.h"

AWxCheckPoint::AWxCheckPoint()
{
	// 컴포넌트는 베이스(AWxGimmick)가 만든 SceneRoot 에 부착한다. bReplicates·SceneRoot·StateTree·WxSaveId 는 베이스가 제공한다.
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetRelativeLocation(FVector(-90.f, 0.f, 0.f));
	// 이 메시가 곧 상호작용 영역이다. 활성/비활성은 ST 의 Wx Enable Interaction 이 이 응답을 토글해 가른다.
	MeshComponent->SetCollisionResponseToChannel(ECC_WxInteractable, ECR_Overlap);

	// 초기 상태는 불 꺼짐. 상호작용 시 Lit 으로 확정된다.
	State = WxGameplayTags::Gimmick_CheckPoint_Unlit;
}

void AWxCheckPoint::OnInteracted(AActor* Interactor, const UActorComponent* Source)
{
	// 서버 권위에서만 호출된다.
	// 불을 켠다. State 는 복제 + SaveGame 으로 지속돼 재로드 후에도 Lit 을 유지하며, 비주얼은 GimmickStateTree 가 적용한다.
	// 세이브도 Lit 상태의 'Wx Save Game' 태스크가 맡는다. 상태 이벤트는 다음 ST 틱에 처리되므로 아래 힐·리필·리스폰이 모두 반영된 뒤 저장된다.
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
}
