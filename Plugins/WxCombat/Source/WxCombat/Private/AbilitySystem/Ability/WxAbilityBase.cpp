// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystem/Effect/WxEffect_Cooldown.h"
#include "AbilitySystem/Effect/WxEffect_Cost.h"
#include "AbilitySystem/Ability/WxAbilityTableRow.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "InputAction.h"
#include "Weapon/WxProjectileBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

UWxAbilityBase::UWxAbilityBase()
{
	InstancingPolicy  = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// 기본값을 공용 GE 마커로 둔다. 마커 그대로면 프로젝트 방식(AbilityDataRow 기반), 다른 GE로 바꾸면 커스텀(엔진 순정 경로).
	CooldownGameplayEffectClass = UWxEffect_Cooldown::StaticClass();
	CostGameplayEffectClass = UWxEffect_Cost::StaticClass();
}

TSoftObjectPtr<UObject> UWxAbilityBase::GetIcon() const
{
	const FWxAbilityTableRow* Row = GetTableRow();
	return Row ? Row->Icon : nullptr;
}

float UWxAbilityBase::GetMontagePlayRate() const
{
	const UWxAbilitySystemComponent* ASC = Cast<UWxAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	return ASC ? ASC->GetMontagePlayRate() : 1.f;
}

void UWxAbilityBase::StartRecovery()
{
	// 차단 태그를 직접 해제하면 안 된다. 엔진은 어빌리티가 차단을 쥐고 있는지 따로 기억했다가 EndAbility에서 한 번 더 해제하므로,
	// 그 여분의 해제가 후딜에 캔슬로 진입한 어빌리티의 차단을 대신 풀어 버린다(공유 카운터라 주인을 가리지 않는다).
	// 이 API는 해제와 소유 표시 정리를 함께 하므로 해제가 한 번만 일어난다.
	SetShouldBlockOtherAbilities(false);
}

void UWxAbilityBase::SpawnProjectile(TSubclassOf<AWxProjectileBase> ProjectileClass, FName SpawnSocketName) const
{
	if (!ProjectileClass)
	{
		return;
	}

	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	USkeletalMeshComponent* Mesh = ActorInfo ? ActorInfo->SkeletalMeshComponent.Get() : nullptr;

	// 투사체는 서버가 스폰해 복제한다. 클라(예측 인스턴스)에선 authority 게이트로 무동작.
	if (!Avatar || !Mesh || !Avatar->HasAuthority())
	{
		return;
	}

	const FVector SpawnLocation = Mesh->GetSocketLocation(SpawnSocketName);
	const FRotator SpawnRotation = Avatar->GetActorRotation();
	const FTransform SpawnTransform(SpawnRotation, SpawnLocation);

	// 대미지는 투사체가 자기 클래스 데이터로 BeginPlay에서 준비하므로 일반 SpawnActor로 충분하다.
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Avatar;
	SpawnParams.Instigator = Cast<APawn>(Avatar);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	Avatar->GetWorld()->SpawnActor<AWxProjectileBase>(ProjectileClass, SpawnTransform, SpawnParams);
}

#if WITH_EDITOR
bool UWxAbilityBase::CanEditChange(const FProperty* InProperty) const
{
	if (!Super::CanEditChange(InProperty))
	{
		return false;
	}
	
	if (InProperty)
	{
		const FName PropertyName = InProperty->GetFName();

		// 스톡 GE 클래스 경로는 AbilityDataRow와 상호배타다. Row가 설정돼 있으면 스톡 클래스 편집을 막는다.
		static const FName CooldownGEName = TEXT("CooldownGameplayEffectClass");
		static const FName CostGEName = TEXT("CostGameplayEffectClass");
		if ((PropertyName == CooldownGEName || PropertyName == CostGEName) && !AbilityDataRow.IsNull())
		{
			return false;
		}
	}

	return true;
}

void UWxAbilityBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(UWxAbilityBase, AbilityDataRow))
	{
		if (!AbilityDataRow.IsNull())
		{
			// Row = 프로젝트 방식이므로 커스텀 GE를 걷어내고 마커로 되돌린다("Row = 마커 표시" 불변식 유지).
			CooldownGameplayEffectClass = UWxEffect_Cooldown::StaticClass();
			CostGameplayEffectClass = UWxEffect_Cost::StaticClass();
		}
	}
}

#endif

void UWxAbilityBase::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	if (ActivationPolicy == EWxAbilityActivationPolicy::OnGranted)
	{
		ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle);
	}
}

UGameplayEffect* UWxAbilityBase::GetCooldownGameplayEffect() const
{
	if (CooldownGameplayEffectClass && CooldownGameplayEffectClass != UWxEffect_Cooldown::StaticClass())
	{
		return Super::GetCooldownGameplayEffect();
	}

	const FWxAbilityTableRow* Row = GetTableRow();
	if (!Row || Row->CooldownTime <= 0.f)
	{
		return nullptr;
	}

	// 단일 충전이면 공유 CDO로 충분하다(ViewModel이 Max(1, StackLimitCount)=1로 읽는다). per-ability 인스턴스는 다중 충전에서만 StackLimitCount 전달용으로 만든다.
	const int32 MaxRecharges = FMath::Max(1, Row->MaxRecharges);
	if (MaxRecharges <= 1)
	{
		return Super::GetCooldownGameplayEffect();
	}

	if (!CooldownEffect)
	{
		CooldownEffect = NewObject<UWxEffect_Cooldown>(const_cast<UWxAbilityBase*>(this), TEXT("CooldownEffect"));
	}

	CooldownEffect->StackLimitCount = MaxRecharges;
	return CooldownEffect;
}

bool UWxAbilityBase::CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (CooldownGameplayEffectClass && CooldownGameplayEffectClass != UWxEffect_Cooldown::StaticClass())
	{
		return Super::CheckCooldown(Handle, ActorInfo, OptionalRelevantTags);
	}

	const FWxAbilityTableRow* Row = GetTableRow();
	if (!Row || Row->CooldownTime <= 0.f)
	{
		return true;
	}

	const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		return true;
	}

	float LongestRemaining = 0.f;
	float LongestDuration = 0.f;
	if (QueryActiveCooldowns(*ASC, LongestRemaining, LongestDuration) >= FMath::Max(1, Row->MaxRecharges))
	{
		// 엔진 순정 CheckCooldown과 동일하게 실패 사유 태그를 채워 OnAbilityFailed 파이프라인(실패 피드백 UI 등)에 전달한다
		if (OptionalRelevantTags)
		{
			const FGameplayTag& FailCooldownTag = UAbilitySystemGlobals::Get().ActivateFailCooldownTag;
			if (FailCooldownTag.IsValid())
			{
				OptionalRelevantTags->AddTag(FailCooldownTag);
			}
		}
		return false;
	}

	return true;
}

float UWxAbilityBase::GetCooldownTimeRemaining(const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (CooldownGameplayEffectClass && CooldownGameplayEffectClass != UWxEffect_Cooldown::StaticClass())
	{
		return Super::GetCooldownTimeRemaining(ActorInfo);
	}

	float TimeRemaining = 0.f;
	float Duration = 0.f;
	if (const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		QueryActiveCooldowns(*ASC, TimeRemaining, Duration);
	}
	return TimeRemaining;
}

void UWxAbilityBase::GetCooldownTimeRemainingAndDuration(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, float& TimeRemaining, float& CooldownDuration) const
{
	if (CooldownGameplayEffectClass && CooldownGameplayEffectClass != UWxEffect_Cooldown::StaticClass())
	{
		Super::GetCooldownTimeRemainingAndDuration(Handle, ActorInfo, TimeRemaining, CooldownDuration);
		return;
	}

	TimeRemaining = 0.f;
	CooldownDuration = 0.f;
	if (const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		QueryActiveCooldowns(*ASC, TimeRemaining, CooldownDuration);
	}
}

const FWxAbilityTableRow* UWxAbilityBase::GetTableRow() const
{
	if (AbilityDataRow.IsNull())
	{
		return nullptr;
	}
	return AbilityDataRow.GetRow<FWxAbilityTableRow>(TEXT("WxAbilityBase::GetTableRow"));
}

int32 UWxAbilityBase::QueryActiveCooldowns(const UAbilitySystemComponent& ASC, float& OutLongestRemaining, float& OutLongestDuration) const
{
	OutLongestRemaining = 0.f;
	OutLongestDuration = 0.f;

	const UGameplayAbility* AbilityCDO = GetClass()->GetDefaultObject<UGameplayAbility>();
	const float WorldTime = ASC.GetWorld()->GetTimeSeconds();

	FGameplayEffectQuery Query;
	Query.EffectDefinition = UWxEffect_Cooldown::StaticClass();

	int32 ActiveCount = 0;
	for (const FActiveGameplayEffectHandle& ActiveHandle : ASC.GetActiveEffects(Query))
	{
		const FActiveGameplayEffect* ActiveGE = ASC.GetActiveGameplayEffect(ActiveHandle);
		if (!ActiveGE || ActiveGE->Spec.GetEffectContext().GetAbility() != AbilityCDO)
		{
			continue;
		}

		// 만료됐지만 아직 제거되지 않은 GE(클라이언트는 제거가 리플리케이션으로 도착할 때까지 지연됨)는 회복된 충전으로 취급한다
		const float Remaining = (ActiveGE->StartWorldTime + ActiveGE->Spec.GetDuration()) - WorldTime;
		if (Remaining <= 0.f)
		{
			continue;
		}

		++ActiveCount;
		if (Remaining > OutLongestRemaining)
		{
			OutLongestRemaining = Remaining;
			OutLongestDuration = ActiveGE->Spec.GetDuration();
		}
	}

	return ActiveCount;
}
