---
title: "전투 자원 기획·보고와 속성 계약 조사"
source: "MANUAL"
type: notes
ingested: 2026-09-22
tags: [wx, static-review, resources]
summary: "전투 자원 기획·보고와 속성 계약 조사의 저장소 원문 발췌와 파일별 SHA-256. 정적 확인 범위이며 실행 검증이 아니다."
revision: fe8c943f49401326e1007fedd78a937c9e66db47
---

# 전투 자원 기획·보고와 속성 계약 조사

조사일: 2026-09-22. 기준 HEAD: `fe8c943f49401326e1007fedd78a937c9e66db47` + 현재 작업 트리. 아래 파일의 선택된 계약·실행 경로를 조사했다. SHA-256은 파일 전체 바이트의 식별값이며 파일 전체의 결함 검토를 뜻하지 않는다. 코드 원문은 해당 저장소 경로가 정본이다. 발췌 전후 생략 부분과 바이너리 에셋·실행 결과는 이 기록에 포함하지 않는다.

## Docs/SystemDesign/Core_Combat_System.md
- [저장소 원문](<../../../Docs/SystemDesign/Core_Combat_System.md>)
- SHA-256: `b83eee16c6d0686181d1b07b97fc8d5270dccf18f788cac445a3ec07ccf62afe`
```text
...
17: | 자원 | 명칭 | 역할 및 특징 | 주요 수급/소모처 |
18: | :--- | :--- | :--- | :--- |
19: | **HP** | **Health Points** | 캐릭터의 생존 수치 | **소모:** 피격 시 감소 (0이 되면 사망)<br>**회복:** 아이템 및 특정 스킬 (자연 회복 없음) |
20: | **MP** | **Magic Points** | **스킬(4종)** 발동 자원 | **수급:** 평타 적중, 패링 성공, 극한 회피 성공<br>**소모:** 캐릭터 고유 스킬(4종) 발동 시 소모 |
21: | **UP** | **Ultimate Points** | **궁극기** 발동 자원 | **수급:** MP 소모 스킬이 적에게 적중했을 때만 충전<br>**소모:** 최대치 도달 시 전량 소모하여 궁극기 발동 |
22: 
23: ---
24: 
25: ## 3. 6대 핵심 액션 (Core Combat Actions)
26: 모든 플레이어블 캐릭터가 공유하는 공통 조작 및 판정 규칙입니다.
27: 
28: ### 3.1 일반 공격 (Normal Attack)
29: *   가장 빠르고 기본적인 공격 형태.
30: *   **MP 수급의 주력 수단**이며, 콤보를 통해 전투의 리듬을 유지함.
31: 
32: ### 3.2 강공격 (Heavy Attack)
33: *   평타보다 느리지만 강력한 위력과 넓은 공격 범위를 가짐.
34: *   평타 콤보 도중 특정 타이밍에 입력 시 **특수 파생기(Branching)** 발동하여 콤보의 다양성 확보.
35: 
36: ### 3.3 가드 및 패링 (Guard & Parry)
37: *   **일반 가드:** 데미지 일부 경감. MP/UP 수급 없음.
38: *   **패링 (Just Guard):** 적중 직전 가드 시 발동. **데미지 0 + MP 대량 수급 + UP 소량 수급 + 적 경직.**
39: *   **참고:** 패링 시의 특수 상호작용 및 연출은 적/투사체별 개별 기획에 따름.
```
## Docs/Meeting/2026-09-19 회의자료.md
- [저장소 원문](<../../../Docs/Meeting/2026-09-19 회의자료.md>)
- SHA-256: `5caa534e9fb9ca8fe82731fc93733b0ef4d0682f7a4228d279c25a0ba3e2b83c`
```text
...
5: 첫 캐릭터부터 전용 자원과 전용 궁극기 코스트 규칙을 추가하면, 자원 관리 규칙이 꼬이게 될 것 같아서 단순화했습니다.
6: 
7: 궁극기 1은 MP 3이 코스트이고, 궁극기 2는 UP 100이 코스트입니다.
8: * MP: 분신과 협공할 때마다 1씩 회복
9: * UP: 도플갱어 있을 때 적에게 대미지 가할 때마다 10씩 회복
10: 
11: 앞으로도 자원이나 코스트를 특별하게 쓰는 캐릭터가 대다수일거라면 자원 시스템을 재설계해야할 것 같은데, 필요하시다면 저와 논의헤보죠. 
12: * 예전 기획 규칙에 의하면 스킬은 MP, 궁극기는 UP만 사용하기로 기획해서, 이에 맞춰서 구현함
13: * 고유 자원의 종류가 계속 추가되거나, 예외적으로 사용되는 자원 규칙이 많다면 어빌리티 테이블에서 코스트를 빼고 BP 스크립팅으로 설계 전환해야함
14: 
15: ## 현광 소환수(분신, 도플갱어) 구현 완료
16: 
17: 분신 및 협동 스킬의 조작감이 영 좋지 않은데 전투 기획자가 직접 플레이하면서 느껴보셔야할 것 같아서 수정하지 않았습니다.
18: 
19: 궁극기 도플갱어 위치가 꼬이거나, 타겟팅이 꼬이는 경우도 마찬가지 이유로 수정하지 않았습니다.
20: 
21: 기획 QA 및 피드백 주시면 개선작업 진행 예정입니다.
22: 
23: ## 현광 관련 후속 작업
24: 
25: TestHG를 참고해서 정식 현광 캐릭터를 새로 제작 작업 부탁드립니다.
26: * 작업하다가 질문사항, 기획과 다른 부분 등 문의사항은 언제든 편하게 물어봐주세요.
27: 
28: 정식 현광 캐릭터 완성될 때까지 전투 밸런싱 가능하도록 준비하고 있겠습니다.
29: 
```
## Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp
- [저장소 원문](<../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp>)
- SHA-256: `aee562971380734774f9c67d444101cc53de6f6ac43f295906bafa44451be2be`
```text
...
138: float UWxCombatAttributeSet::ClampAttributeValue(const FGameplayAttribute& Attribute, float NewValue) const
139: {
140: 	const float MinimumValue = Attribute == GetASPDAttribute() ? 0.001f : 0.f;
141: 	NewValue = FMath::Max(NewValue, MinimumValue);
142: 
143: 	const FWxMaxAttributePair* Pair = FindMaxAttributePair(Attribute);
144: 	const UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
145: 	if (Pair && Pair->Attribute == Attribute && ASC)
146: 	{
147: 		NewValue = FMath::Min(NewValue, ASC->GetNumericAttribute(Pair->MaxAttribute));
148: 	}
149: 
150: 	return NewValue;
151: }
152: 
153: void UWxCombatAttributeSet::AdjustAttributeForMaxChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
154: {
155: 	const FWxMaxAttributePair* Pair = FindMaxAttributePair(Attribute);
156: 	if (!Pair || Pair->MaxAttribute != Attribute || OldValue <= 0.f || FMath::IsNearlyEqual(OldValue, NewValue))
157: 	{
158: 		return;
...
166: 
167: 	// 현재값이 아니라 베이스를 읽어 베이스에 쓴다. 현재값을 읽으면 지속형 모디파이어 몫까지 베이스로 굳어 GE가 걷혀도 남는다.
168: 	const float ScaledBase = ASC->GetNumericAttributeBase(Pair->Attribute) * NewValue / OldValue;
169: 	ASC->SetNumericAttributeBase(Pair->Attribute, ScaledBase);
170: }
171: 
172: void UWxCombatAttributeSet::OnRep_HP(const FGameplayAttributeData& OldHP)
173: {
174: 	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxCombatAttributeSet, HP, OldHP);
175: }
176: 
177: void UWxCombatAttributeSet::OnRep_MaxHP(const FGameplayAttributeData& OldMaxHP)
178: {
179: 	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxCombatAttributeSet, MaxHP, OldMaxHP);
180: }
181: 
182: void UWxCombatAttributeSet::OnRep_SP(const FGameplayAttributeData& OldSP)
183: {
184: 	GAMEPLAYATTRIBUTE_REPNOTIFY(UWxCombatAttributeSet, SP, OldSP);
185: }
186: 
```
## Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityTableRow.h
- [저장소 원문](<../../../Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityTableRow.h>)
- SHA-256: `4df0f2070c457e6ec374139afe559f6092c1f9dda88f550aa41b01c0439223cd`
```text
...
8: 
9: UENUM(BlueprintType)
10: enum class EWxAbilityCostResource : uint8
11: {
12: 	Custom,
13: 	SP,
14: 	MP,
15: 	UP,
16: };
17: 
18: /** RowName 예시: GA_HGTest_Skill_1, GA_HGTest_Ultimate_1 */
19: USTRUCT(BlueprintType)
20: struct WXCOMBAT_API FWxAbilityTableRow : public FTableRowBase
21: {
22: 	GENERATED_BODY()
23: 
24: 	/** 쿨다운 시간(초). 0 이하이면 쿨다운 미적용 */
25: 	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooldown")
26: 	float CooldownTime = 0.f;
27: 
28: 	/** 1이면 단일 쿨다운 */
...
31: 
32: 	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost")
33: 	EWxAbilityCostResource CostResource = EWxAbilityCostResource::Custom;
34: 
35: 	/** 질주처럼 지속 소모하는 어빌리티에서는 진입 비용이다 */
36: 	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost", meta = (ClampMin = "0.0"))
37: 	float CostAmount = 0.f;
38: 
39: 	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display")
40: 	FText Title;
41: 
42: 	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display", meta = (MultiLine = true))
43: 	FText Description;
44: 
45: 	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display", meta = (AllowedClasses = "/Script/Engine.Texture2D,/Script/Engine.MaterialInterface"))
46: 	TSoftObjectPtr<UObject> Icon;
47: };
```
