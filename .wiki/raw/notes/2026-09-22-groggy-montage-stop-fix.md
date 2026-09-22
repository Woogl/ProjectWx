---
title: "그로기 종료 시 몽타주 정지 경로 수정 조사"
source: "Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp"
type: notes
ingested: 2026-09-22
tags: [wx, static-review, groggy]
summary: "커밋 c2b1088a0으로 그로기 종료가 ASC 현재 몽타주 기준 정지 대신 AnimInstance에서 그로기 몽타주를 직접 정지하도록 바뀐 저장소 원문 발췌와 SHA-256. 정적 확인 범위이며 실행 검증이 아니다."
revision: c2b1088a0965d098a0c744f6aae7d753ad31ed08
sha: 4e919a7072e8d71a989c6cac51720e2c7bd1c1d5d2cf81d58983fce44d956535
---

# 그로기 종료 시 몽타주 정지 경로 수정 조사

조사일: 2026-09-22. 기준 HEAD: `60c324c714b1dab10cd48d36cabad63ace232716`, 해당 파일은 작업 트리 변경 없음. 대상 파일의 마지막 변경 커밋은 `c2b1088a0965d098a0c744f6aae7d753ad31ed08`("가산 피격 중 그로기 몽타주가 종료되지 않는 문제를 수정", 2026-09-22 18:36 +0900)이다. [그로기 기획과 활성화 수명 조사](2026-09-22-current-groggy.md)가 기록한 같은 파일의 SHA-256 `80f51ee7fd57c124574e686f7e35eb8290bbda98591c9071dd1d55092c57409e` 이후 버전이다. SHA-256은 파일 전체 바이트의 식별값이며 파일 전체의 결함 검토를 뜻하지 않는다. 코드 원문은 해당 저장소 경로가 정본이다. 발췌 전후 생략 부분과 바이너리 에셋·실행 결과는 이 기록에 포함하지 않는다.

## 커밋 변경

```diff
@@ -5,6 +5,7 @@
 #include "AbilitySystem/Attribute/WxCombatAttributeSet.h"
 #include "AbilitySystem/Effect/WxEffect_DrainGP.h"
 #include "AIController.h"
+#include "Animation/AnimInstance.h"
 #include "BrainComponent.h"
 #include "Engine/World.h"
 #include "GameFramework/Pawn.h"
@@ -79,9 +80,11 @@ void UWxAbility_Groggy::EndAbility(const FGameplayAbilitySpecHandle Handle, cons
 			UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
 
 			// GroggyMontage 미설정 경로에서는 즉시 종료될 수 있다.
-			if (GroggyMontage)
+			// 가산 슬롯 피격이 ASC의 현재 몽타주 자리를 차지하면 StopMontageIfCurrent는 그 아래에서 루프 중인 그로기 몽타주를 놓친다.
+			UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
+			if (GroggyMontage && AnimInstance)
 			{
-				ASC->StopMontageIfCurrent(*GroggyMontage);
+				AnimInstance->Montage_Stop(GroggyMontage->GetDefaultBlendOutTime(), GroggyMontage);
 			}
 
 			StopGroggyDrain(*ASC);
```

## Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp
- [저장소 원문](<../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp>)
- SHA-256: `4e919a7072e8d71a989c6cac51720e2c7bd1c1d5d2cf81d58983fce44d956535`
```text
...
70: void UWxAbility_Groggy::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
71: {
72: 	StopMontagePolling();
73: 
74: 	if (ActorInfo)
75: 	{
76: 		SetAILogicPaused(ActorInfo, false);
77: 
78: 		if (ActorInfo->AbilitySystemComponent.IsValid())
79: 		{
80: 			UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
81: 
82: 			// GroggyMontage 미설정 경로에서는 즉시 종료될 수 있다.
83: 			// 가산 슬롯 피격이 ASC의 현재 몽타주 자리를 차지하면 StopMontageIfCurrent는 그 아래에서 루프 중인 그로기 몽타주를 놓친다.
84: 			UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
85: 			if (GroggyMontage && AnimInstance)
86: 			{
87: 				AnimInstance->Montage_Stop(GroggyMontage->GetDefaultBlendOutTime(), GroggyMontage);
88: 			}
89: 
90: 			StopGroggyDrain(*ASC);
91: 		}
92: 	}
93: 
94: 	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
95: }
...
105: void UWxAbility_Groggy::HandleMontagePollTick()
106: {
107: 	UAbilitySystemComponent* ASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;
108: 	if (!ASC || ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Death))
109: 	{
110: 		// 사망 어빌리티가 Override 그로기를 취소하지 못해 여기서 종료한다.
111: 		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
112: 		return;
113: 	}
114: 
115: 	if (ASC->GetCurrentMontage() != nullptr)
116: 	{
117: 		return;
118: 	}
119: 
120: 	ASC->PlayMontage(this, CurrentActivationInfo, GroggyMontage, 1.f);
...
```
