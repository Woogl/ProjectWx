---
title: "GAS 부여·어빌리티 계약 정적 조사"
source: "MANUAL"
type: notes
ingested: 2026-09-22
tags: [wx, static-review, combat]
summary: "GAS 부여·어빌리티 계약 정적 조사의 저장소 원문 발췌와 파일별 SHA-256. 정적 확인 범위이며 실행 검증이 아니다."
revision: fe8c943f49401326e1007fedd78a937c9e66db47
---

# GAS 부여·어빌리티 계약 정적 조사

조사일: 2026-09-22. 기준 HEAD: `fe8c943f49401326e1007fedd78a937c9e66db47` + 현재 작업 트리. 아래 파일의 선택된 계약·실행 경로를 조사했다. SHA-256은 파일 전체 바이트의 식별값이며 파일 전체의 결함 검토를 뜻하지 않는다. 코드 원문은 해당 저장소 경로가 정본이다. 발췌 전후 생략 부분과 바이너리 에셋·실행 결과는 이 기록에 포함하지 않는다.

## Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs
- [저장소 원문](<../../../Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs>)
- SHA-256: `00ec1004d18b5e68628935bd6f63e56bfa194c9436beb540c2542d07a346ec4d`
```text
...
9: 		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
10: 
11: 		PublicDependencyModuleNames.AddRange(new string[]
12: 		{
13: 			"AIModule",
14: 			"Core",
15: 			"CoreUObject",
16: 			"DeveloperSettings",
17: 			"EnhancedInput",
18: 			"Engine",
19: 			"GameplayAbilities",
20: 			"GameplayTags",
21: 			"GameplayTasks",
22: 			"MotionWarping",
23: 			"TargetingSystem",
24: 			"UMG",
25: 			"WxCore",
26: 		});
27: 
28: 		PrivateDependencyModuleNames.AddRange(new string[]
29: 		{
```
## Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h
- [저장소 원문](<../../../Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h>)
- SHA-256: `b5486078b330156e3d43919a625a5bb2897dd2d511b36f13050955b8346a1cb5`
```text
...
24:  */
25: UENUM()
26: enum class EWxAbilityActivationGroup : uint8
27: {
28: 	/** 막지도 막히지도 않는다. */
29: 	Independent,
30: 
31: 	/** 배타적으로 다른 Exclusive 어빌리티 발동을 막는다. */
32: 	Exclusive,
33: 
34: 	/** Exclusive 점유를 덮어쓰고 발동하며 캔슬되지도 않는다. 주로 HitReact, Groggy, Death에서 사용. */
35: 	Override,
36: };
37: 
38: /**
39:  * Exclusive 어빌리티가 발동 한 번 동안 밟는 캔슬 창.
40:  * 몽타주 노티파이가 닫힘에서 열림 순으로 전이시킨다 — Blocking → ComboWindow → Recovery.
41:  */
42: enum class EWxAbilityActionPhase : uint8
43: {
44: 	/** 본동작. 남의 배타 발동을 막는다. */
45: 	Blocking,
46: 
47: 	/** 콤보 창. 자기 재발동만 통과시키고, 남의 발동은 본동작처럼 막는다. */
48: 	ComboWindow,
49: 
50: 	/** 액션을 캔슬할 수 있게 된 후딜레이. 점유를 놓아 다른 배타 어빌리티가 끊고 들어올 수 있다. */
51: 	Recovery,
52: };
53: 
54: UCLASS(Abstract, BlueprintType, Blueprintable)
55: class WXCOMBAT_API UWxAbilityBase : public UGameplayAbility, public IWxUIData
56: {
57: 	GENERATED_BODY()
58: 
59: public:
60: 	UWxAbilityBase();
...
81: 	 */
82: 	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx")
83: 	TArray<TSubclassOf<UGameplayEffect>> ActivationOwnedEffects;
84: 
85: 	//~ Begin IWxUIData
86: 	virtual FText GetTitle() const override;
87: 	virtual FText GetDescription() const override;
88: 	virtual TSoftObjectPtr<UObject> GetIcon() const override;
89: 	virtual int32 GetMaxRecharges() const override;
90: 	//~ End IWxUIData
91: 
92: 	/** 충전 1개의 회복 시간(초). 테이블에 수치가 없으면 0 */
93: 	float GetCooldownTime() const;
94: 
95: 	/** 일반적으로는 ASPD가 반영된 몽타주 재생 속도 사용. */
96: 	virtual float GetMontagePlayRate() const;
97: 
98: 	void OpenComboWindow();
99: 	void CloseComboWindow();
100: 
101: 	/**
```
## Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp
- [저장소 원문](<../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp>)
- SHA-256: `d1a87819a988fb77a44da5da8ce5da954a3923302259f1478369d12438f6ad4c`
```text
...
16: #include "WxGameplayTags.h"
17: 
18: UWxAbilityBase::UWxAbilityBase()
19: {
20: 	InstancingPolicy  = EGameplayAbilityInstancingPolicy::InstancedPerActor;
21: 	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
22: 
23: 	// 쿨다운 GE는 각 어빌리티가 지정한다 — 엔진이 스택을 GE 클래스 단위로 병합해서, 여기에 공용 기본값을 두면 어빌리티끼리 쿨다운이 섞인다.
24: 	// 코스트는 Instant라 병합될 것이 없어 공용 GE 하나로 충분하다.
25: 	CostGameplayEffectClass = UWxEffect_Cost::StaticClass();
26: }
27: 
28: FText UWxAbilityBase::GetTitle() const
29: {
30: 	const FWxAbilityTableRow* Row = GetTableRow();
31: 	return Row ? Row->Title : FText::GetEmpty();
32: }
33: 
34: FText UWxAbilityBase::GetDescription() const
35: {
36: 	const FWxAbilityTableRow* Row = GetTableRow();
...
85: }
86: 
87: void UWxAbilityBase::OpenComboWindow()
88: {
89: 	// 배타 본동작에서만 연다 — 콤보가 없는 어빌리티의 몽타주에 노티파이가 섞여도 Independent를 점유자로 승격시키지 않는다.
90: 	if (ActivationGroup == EWxAbilityActivationGroup::Exclusive && ActionPhase == EWxAbilityActionPhase::Blocking)
91: 	{
92: 		SetActionPhase(EWxAbilityActionPhase::ComboWindow);
93: 
94: 		// 재발동이 이 인스턴스를 그대로 되살리므로 전이 뒤에는 아무것도 쓰지 않는다.
95: 		const AActor* Avatar = GetAvatarActorFromActorInfo();
96: 		if (UWxInputBufferComponent* InputBuffer = Avatar ? Avatar->FindComponentByClass<UWxInputBufferComponent>() : nullptr)
97: 		{
98: 			InputBuffer->FlushBufferedInputs();
99: 		}
100: 	}
101: }
102: 
103: void UWxAbilityBase::CloseComboWindow()
104: {
105: 	// 창이 아직 열려 있을 때만 되돌린다 — 창이 후딜보다 늦게 닫히는 배치가 정상이라 무조건 되돌리면 후딜을 도로 닫는다.
...
110: }
111: 
112: void UWxAbilityBase::StartRecovery()
113: {
114: 	// 배타 어빌리티만 후딜로 — 엉뚱한 노티파이가 Independent를 점유자로 승격시키거나 Override의 캔슬 면역을 벗기지 않게 한다.
115: 	if (ActivationGroup == EWxAbilityActivationGroup::Exclusive && ActionPhase != EWxAbilityActionPhase::Recovery)
116: 	{
117: 		SetActionPhase(EWxAbilityActionPhase::Recovery);
118: 
119: 		// 성립한 어빌리티가 이 인스턴스를 끊으므로 전이 뒤에는 아무것도 쓰지 않는다.
120: 		const AActor* Avatar = GetAvatarActorFromActorInfo();
121: 		if (UWxInputBufferComponent* InputBuffer = Avatar ? Avatar->FindComponentByClass<UWxInputBufferComponent>() : nullptr)
122: 		{
123: 			InputBuffer->FlushBufferedInputs();
124: 		}
125: 	}
126: }
127: 
128: const UWxAbilityBase* UWxAbilityBase::FindActivationGroupBlocker(const UAbilitySystemComponent& ASC, const UWxAbilityBase* Candidate)
129: {
130: 	for (const FGameplayAbilitySpec& Spec : ASC.GetActivatableAbilities())
131: 	{
132: 		if (!Spec.IsActive())
133: 		{
134: 			continue;
135: 		}
136: 
137: 		// 모든 Wx 어빌리티는 기반 생성자가 InstancedPerActor를 강제하므로 스펙당 인스턴스는 하나뿐이다.
138: 		const UWxAbilityBase* Occupant = Cast<UWxAbilityBase>(Spec.GetPrimaryInstance());
139: 		if (!Occupant || !Occupant->IsActive())
140: 		{
141: 			continue;
142: 		}
143: 
144: 		const bool bOccupying = Occupant->ActivationGroup == EWxAbilityActivationGroup::Override
145: 			|| (Occupant->ActivationGroup == EWxAbilityActivationGroup::Exclusive && Occupant->ActionPhase != EWxAbilityActionPhase::Recovery);
146: 		if (!bOccupying)
```
## Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp
- [저장소 원문](<../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp>)
- SHA-256: `a6238cca604793a641176c408b21ac2e7e8b4e40a0e89ad8391072754e5c3025`
```text
...
7: #include "AbilitySystem/Attribute/WxCombatAttributeInitTableRow.h"
8: 
9: void UWxAbilitySet::GiveToAbilitySystem(UWxAbilitySystemComponent* ASC) const
10: {
11: 	if (!ASC)
12: 	{
13: 		return;
14: 	}
15: 	
16: 	if (const FWxCombatAttributeInitTableRow* Row = AttributeInitRow.GetRow<FWxCombatAttributeInitTableRow>(ANSI_TO_TCHAR(__FUNCTION__)))
17: 	{
18: 		// 현재값이 먼저 오면 옛 Max로 클램프된 뒤, 이어지는 Max 기록이 PostAttributeChange의 비례 스케일을 깨워 방금 넣은 값을 덮어쓴다.
19: 		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetMaxHPAttribute(), Row->MaxHP);
20: 		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetHPAttribute(), Row->HP);
21: 		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetMaxSPAttribute(), Row->MaxSP);
22: 		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetSPAttribute(), Row->SP);
23: 		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetMaxGPAttribute(), Row->MaxGP);
24: 		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetGPAttribute(), Row->GP);
25: 		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetMaxMPAttribute(), Row->MaxMP);
26: 		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetMPAttribute(), Row->MP);
27: 		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetMaxUPAttribute(), Row->MaxUP);
28: 		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetUPAttribute(), Row->UP);
29: 		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetATKAttribute(), Row->ATK);
30: 		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetDEFAttribute(), Row->DEF);
31: 		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetCritRateAttribute(), Row->CritRate);
32: 		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetCritDMGAttribute(), Row->CritDMG);
33: 	}
34: 
35: 	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
36: 	Context.AddSourceObject(ASC->GetOwner());
37: 
38: 	for (const TSubclassOf<UGameplayEffect>& Effect : GrantedEffects)
39: 	{
40: 		if (!Effect)
41: 		{
42: 			continue;
43: 		}
44: 
45: 		const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Effect, 1, Context);
46: 		if (Spec.IsValid())
47: 		{
48: 			ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
49: 		}
50: 	}
51: 
52: 	for (const TSubclassOf<UWxAbilityBase>& AbilityClass : GrantedAbilities)
53: 	{
54: 		if (!AbilityClass)
55: 		{
56: 			continue;
57: 		}
58: 
59: 		FGameplayAbilitySpec Spec(AbilityClass, 1);
60: 
61: 		ASC->GiveAbility(Spec);
62: 	}
63: }
64: 
65: void UWxAbilitySet::AppendInputActions(TArray<const UInputAction*>& OutInputActions) const
66: {
67: 	for (const TSubclassOf<UWxAbilityBase>& AbilityClass : GrantedAbilities)
68: 	{
69: 		const UWxAbilityBase* AbilityCDO = AbilityClass.GetDefaultObject();
70: 		if (AbilityCDO && AbilityCDO->ActivationInputAction)
```
## Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp
- [저장소 원문](<../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp>)
- SHA-256: `842ea43ca28185fd93b6a7c2eb779e918a96d82f70e2531851f1fdb45f7421fc`
```text
...
18: 
19: 	GetGameplayAttributeValueChangeDelegate(UWxCombatAttributeSet::GetSPAttribute())
20: 		.AddUObject(this, &UWxAbilitySystemComponent::HandleSPChanged);
21: }
22: 
23: float UWxAbilitySystemComponent::PlayMontage(UGameplayAbility* AnimatingAbility, FGameplayAbilityActivationInfo ActivationInfo, UAnimMontage* Montage, float InPlayRate, FName StartSectionName, float StartTimeSeconds)
24: {
25: 	const float Duration = Super::PlayMontage(AnimatingAbility, ActivationInfo, Montage, InPlayRate, StartSectionName, StartTimeSeconds);
26: 	if (Duration > 0.f && AnimatingAbility != nullptr)
27: 	{
28: 		EnableAnimatingMontageMeshTick();
29: 	}
30: 
31: 	return Duration;
32: }
33: 
34: void UWxAbilitySystemComponent::ClearAnimatingAbility(UGameplayAbility* Ability)
35: {
36: 	const bool bClearingCurrentAbility = IsAnimatingAbility(Ability);
37: 	Super::ClearAnimatingAbility(Ability);
38: 
...
43: }
44: 
45: void UWxAbilitySystemComponent::GiveAbilitySets()
46: {
47: 	// ASC는 캐릭터 서브오브젝트라 재빙의 후에도 앞서 부여한 어빌리티를 그대로 쥐고 있다.
48: 	// 다시 부여하면 어빌리티·GE가 중복되고 어트리뷰트 초기화가 HP/SP를 초기값으로 되돌린다.
49: 	if (bAbilitySetsGranted)
50: 	{
51: 		return;
52: 	}
53: 
54: 	bAbilitySetsGranted = true;
55: 
56: 	for (const TObjectPtr<UWxAbilitySet>& Set : AbilitySets)
57: 	{
58: 		if (Set)
59: 		{
60: 			Set->GiveToAbilitySystem(this);
61: 		}
62: 	}
63: }
64: 
65: void UWxAbilitySystemComponent::HandleSPChanged(const FOnAttributeChangeData& ChangeData)
66: {
67: 	// 클라도 복제 수신으로 이 콜백을 지난다.
68: 	if (ChangeData.NewValue >= ChangeData.OldValue || !IsOwnerActorAuthoritative())
69: 	{
70: 		return;
71: 	}
72: 
73: 	if (GetNumericAttribute(UWxCombatAttributeSet::GetMaxSPAttribute()) > 0.f)
74: 	{
75: 		UWxEffect_Exhaust::ApplyTo(this);
76: 	}
77: }
78: 
79: void UWxAbilitySystemComponent::EnableAnimatingMontageMeshTick()
80: {
81: 	if (MontageTickMesh.IsValid())
82: 	{
83: 		return;
...
210: }
211: 
212: bool UWxAbilitySystemComponent::TryActivateByInputAction(const UInputAction* Action)
213: {
214: 	// AbilityInputActionTriggered와 같은 이유로 락을 건다.
215: 	ABILITYLIST_SCOPE_LOCK();
216: 
217: 	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
218: 	{
219: 		const UWxAbilityBase* Ability = Cast<UWxAbilityBase>(Spec.Ability);
220: 		if (!Ability || Ability->ActivationInputAction.Get() != Action)
221: 		{
222: 			continue;
223: 		}
224: 
225: 		// 신규 발동과 콤보 재발동은 엔진이 bRetriggerInstancedAbility로 가르므로 호출이 같다.
226: 		if (TryActivateAbility(Spec.Handle))
227: 		{
228: 			return true;
229: 		}
230: 	}
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
29: 	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooldown")
30: 	int32 MaxRecharges = 1;
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
