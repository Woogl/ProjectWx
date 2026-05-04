// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Effect/WxEffect_Damage.h"
#include "AbilitySystem/Effect/WxExecCalc_Damage.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"
#include "WxGameplayTags.h"

UWxEffect_Damage::UWxEffect_Damage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayEffectExecutionDefinition ExecDef;
	ExecDef.CalculationClass = UWxExecCalc_Damage::StaticClass();
	Executions.Add(ExecDef);

	// 사망 타겟에는 적용 자체를 차단
	UTargetTagRequirementsGameplayEffectComponent* TagReqComp = CreateDefaultSubobject<UTargetTagRequirementsGameplayEffectComponent>(TEXT("TargetTagReq"));
	TagReqComp->ApplicationTagRequirements.IgnoreTags.AddTag(WxGameplayTags::State_Dead);
	GEComponents.Add(TagReqComp);
}
