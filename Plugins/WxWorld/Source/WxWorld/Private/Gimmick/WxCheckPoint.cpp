// Copyright Woogle. All Rights Reserved.

#include "Gimmick/WxCheckPoint.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "System/WxSpawnerLibrary.h"
#include "WxGameplayTags.h"

AWxCheckPoint::AWxCheckPoint()
{
	// 컴포넌트는 베이스(AWxGimmick)가 만든 SceneRoot 에 부착한다. bReplicates·SceneRoot·StateTree·WxSaveId 는 베이스가 제공한다.
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetRelativeLocation(FVector(-90.f, 0.f, 0.f));
	// 이 메시가 곧 상호작용 영역이며, 기본 활성으로 시작한다. 이후 활성/비활성은 ST 의 Enable Interaction 이 이 집합에 넣고 빼 가른다.
	ActiveInteractionMeshes.Add(MeshComponent);

	// 부활 자리. 루트와 같은 자리에서 시작하며, 메시와 형제라 토치를 두고 이 컴포넌트만 옮겨 서는 위치·방향을 잡는다.
	ResumePoint = CreateDefaultSubobject<USceneComponent>(TEXT("ResumePoint"));
	ResumePoint->SetupAttachment(SceneRoot);

	// 초기 상태는 불 꺼짐. 상호작용 시 Lit 으로 확정된다.
	State = WxGameplayTags::Gimmick_CheckPoint_Unlit;
}

void AWxCheckPoint::OnInteracted(AActor* Interactor, const UActorComponent* Source)
{
	// 서버 권위에서만 호출된다.
	// 불을 켠다. State 는 복제 + SaveGame 으로 지속돼 재로드 후에도 Lit 을 유지하며, 비주얼은 GimmickStateTree 가 적용한다.
	// 충전형 소비 아이템 리필과 세이브도 Lit 상태의 'Refill Item Charges'·'Save Game' 태스크가 맡는다.
	// 상태 이벤트는 다음 ST 틱에 처리되므로 아래 힐·리스폰이 모두 반영된 뒤 리필·저장이 이어진다.
	// 이미 Lit 이면 동일값이라 노옵이므로 그 두 태스크는 다시 돌지 않는다. 아래 회복·리스폰은 상태와 무관한 C++ 경로라 재휴식에서도 그대로 동작한다.
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

	UWxSpawnerLibrary::TryRespawnAll(this);
}
