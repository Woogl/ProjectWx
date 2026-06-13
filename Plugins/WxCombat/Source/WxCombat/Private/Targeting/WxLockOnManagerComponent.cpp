// Copyright Woogle. All Rights Reserved.

#include "Targeting/WxLockOnManagerComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "WxGameplayTags.h"

UWxLockOnManagerComponent::UWxLockOnManagerComponent()
{
	// Server RPC 라우팅과 LockOnTarget 복제를 위해 컴포넌트 복제를 켠다.
	SetIsReplicatedByDefault(true);
}

UWxLockOnManagerComponent* UWxLockOnManagerComponent::FindComponent(const AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}

	return Actor->FindComponentByClass<UWxLockOnManagerComponent>();
}

void UWxLockOnManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 서버 권위 소비처(발사체 방향/호밍)와 시뮬프록시 소비처(몽타주 스냅)가 모두 읽으므로 전 대상에 복제한다.
	DOREPLIFETIME(UWxLockOnManagerComponent, LockOnTarget);
}

void UWxLockOnManagerComponent::SetLockOnTarget(USceneComponent* InTarget)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		// 권위(서버/리슨 서버 호스트): 직접 반영하면 전 클라로 복제된다.
		ApplyLockOnTarget(InTarget);
	}
	else
	{
		// 소유 클라: 응답성을 위해 로컬에 즉시 반영(예측)한 뒤 서버에 권위 설정을 요청한다.
		// 서버 복제값이 도착하면 OnRep 으로 정합한다.
		ApplyLockOnTarget(InTarget);
		ServerSetLockOnTarget(InTarget);
	}
}

void UWxLockOnManagerComponent::ServerSetLockOnTarget_Implementation(USceneComponent* InTarget)
{
	ApplyLockOnTarget(InTarget);
}

void UWxLockOnManagerComponent::OnRep_LockOnTarget()
{
	// 복제가 LockOnTarget 값을 이미 대입한 뒤 호출된다. 구독자(락온 태스크)는 변경 없을 시 무시하도록 idempotent 하게 처리한다.
	OnLockOnTargetChanged.Broadcast(LockOnTarget);
}

void UWxLockOnManagerComponent::ApplyLockOnTarget(USceneComponent* InTarget)
{
	if (LockOnTarget == InTarget)
	{
		return;
	}

	// 락온 피대상 표시는 시각적·개인 UI라, 로컬 플레이어의 락온만 대상 ASC 에 로컬 태그로 노출한다(타 클라의 락온은 이 화면에 표시하지 않는다).
	// 대상 변경의 공통 경로이므로 여기서 이전 대상의 태그를 거두고 새 대상에 부여한다.
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn && OwnerPawn->IsLocallyControlled())
	{
		if (LockOnTarget)
		{
			if (UAbilitySystemComponent* PrevASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(LockOnTarget->GetOwner()))
			{
				PrevASC->RemoveLooseGameplayTag(WxGameplayTags::State_LockedOn);
			}
		}

		if (InTarget)
		{
			if (UAbilitySystemComponent* NextASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InTarget->GetOwner()))
			{
				NextASC->AddLooseGameplayTag(WxGameplayTags::State_LockedOn);
			}
		}
	}

	LockOnTarget = InTarget;

	// 권위 측은 OnRep 이 호출되지 않으므로 여기서 직접 브로드캐스트해 서버 태스크를 갱신한다.
	OnLockOnTargetChanged.Broadcast(LockOnTarget);
}

USceneComponent* UWxLockOnManagerComponent::GetLockOnTarget() const
{
	return LockOnTarget;
}
