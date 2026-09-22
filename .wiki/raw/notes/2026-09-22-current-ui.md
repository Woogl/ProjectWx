---
title: "레이어·대화 화면·HUD·속성 표시 수명 조사"
source: "MANUAL"
type: notes
ingested: 2026-09-22
tags: [wx, static-review, ui]
summary: "레이어·대화 화면·HUD·속성 표시 수명 조사의 저장소 원문 발췌와 파일별 SHA-256. 정적 확인 범위이며 실행 검증이 아니다."
revision: fe8c943f49401326e1007fedd78a937c9e66db47
---

# 레이어·대화 화면·HUD·속성 표시 수명 조사

조사일: 2026-09-22. 기준 HEAD: `fe8c943f49401326e1007fedd78a937c9e66db47` + 현재 작업 트리. 아래 파일의 선택된 계약·실행 경로를 조사했다. SHA-256은 파일 전체 바이트의 식별값이며 파일 전체의 결함 검토를 뜻하지 않는다. 코드 원문은 해당 저장소 경로가 정본이다. 발췌 전후 생략 부분과 바이너리 에셋·실행 결과는 이 기록에 포함하지 않는다.

## Plugins/WxUI/Source/WxUI/WxUI.Build.cs
- [저장소 원문](<../../../Plugins/WxUI/Source/WxUI/WxUI.Build.cs>)
- SHA-256: `66bc3aff2bcacbdf8d1f396a36851699a92fb3e33318e03fa7cf320a41bfd9c6`
```text
...
9: 		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
10: 
11: 		PublicDependencyModuleNames.AddRange(
12: 			new string[]
13: 			{
14: 				"CommonInput",
15: 				"CommonUI",
16: 				"Core",
17: 				"CoreUObject",
18: 				"DeveloperSettings",
19: 				"Engine",
20: 				"GameplayAbilities",
21: 				"GameplayTags",
22: 				"ModelViewViewModel",
23: 				"StateTreeModule",
24: 				"UMG",
25: 				"UniversalObjectLocator",
26: 				"WxCore",
27: 			}
28: 		);
29: 
30: 		PrivateDependencyModuleNames.AddRange(
31: 			new string[]
32: 			{
33: 				"Slate",
34: 				"SlateCore",
35: 			}
36: 		);
37: 	}
38: }
```
## Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h
- [저장소 원문](<../../../Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h>)
- SHA-256: `e1d6d2f8c815a86ea31ccb49252c312817201690631f8a44dc071055499c53b0`
```text
...
35: 	/** 배열 순서가 z-order다(0 = 최하단). */
36: 	UPROPERTY(EditDefaultsOnly, Category = "UI|Layers", meta = (Categories = "UI.Layer"))
37: 	TArray<FGameplayTag> LayerTags;
38: 
39: private:
40: 	UPROPERTY(Transient)
41: 	TMap<FGameplayTag, TObjectPtr<UCommonActivatableWidgetStack>> LayerMap;
42: };
```
## Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h
- [저장소 원문](<../../../Plugins/WxUI/Source/WxUI/Public/System/WxUIManagerSubsystem.h>)
- SHA-256: `b9fc2ca2fe072b9a17fe65cc898d77cbf089d35edc486bd5f9f7c642742b2c8b`
```text
...
74: 
75: 	/**
76: 	 * 로컬 플레이어 하나를 전제로 레이아웃과 아래 추적 상태를 단수로 둔다.
77: 	 * 스플릿스크린이 필요해지면 이것들을 ULocalPlayer 키로 묶어야 한다 — 지금 구조에선 나중에 붙은 플레이어가 앞의 것을 갈아치운다.
78: 	 */
79: 	UPROPERTY()
80: 	TObjectPtr<UWxPrimaryGameLayout> PrimaryGameLayout;
81: 
82: 	UPROPERTY(Transient)
83: 	TSubclassOf<UWxGamePopup> ConfirmationPopupClass;
84: 
85: 	/** 빙의를 구독해 둔 로컬 PC. 교체·종료 때 같은 PC 에서 끊기 위해 기억한다. */
86: 	TWeakObjectPtr<APlayerController> TrackedPlayerController;
87: 
88: 	/** 상태 태그를 구독해 둔 폰 ASC. 폰이 바뀌면 같은 ASC 에서 끊기 위해 기억한다. */
89: 	TWeakObjectPtr<UAbilitySystemComponent> WatchedAbilitySystem;
90: 
91: 	FDelegateHandle DeathTagHandle;
92: 
93: 	FDelegateHandle DialogueTagHandle;
94: 
```
## Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp
- [저장소 원문](<../../../Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp>)
- SHA-256: `953d54ea1a5bc544d66f51e480dbd1d869b8bc9d46fb546babd09fbaf8f8f5a4`
```text
...
102: }
103: 
104: bool UWxUIManagerSubsystem::IsMenuLayerActive() const
105: {
106: 	// GameMenu 는 아이템 획득 알림처럼 화면을 덮지 않는 자리라 메뉴로 세지 않는다.
107: 	return HasActiveWidgetInLayer(WxGameplayTags::UI_Layer_Menu) || HasActiveWidgetInLayer(WxGameplayTags::UI_Layer_Modal);
108: }
109: 
110: bool UWxUIManagerSubsystem::HasActiveWidgetInLayer(FGameplayTag LayerTag) const
111: {
112: 	if (!PrimaryGameLayout)
113: 	{
114: 		return false;
115: 	}
116: 
117: 	const UCommonActivatableWidgetStack* Stack = PrimaryGameLayout->GetLayerWidgetStack(LayerTag);
118: 	if (!Stack)
119: 	{
120: 		return false;
121: 	}
122: 
...
148: }
149: 
150: void UWxUIManagerSubsystem::RefreshGamePause()
151: {
152: 	UGameInstance* GameInstance = GetGameInstance();
153: 	UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
154: 	if (!World || !World->IsNetMode(NM_Standalone))
155: 	{
156: 		return;
157: 	}
158: 
159: 	APlayerController* PC = TrackedPlayerController.Get();
160: 	if (!PC)
161: 	{
162: 		return;
163: 	}
164: 
165: 	// 해제 조건을 대리자로 걸어 두면 게임모드가 해제 직전 되물으므로, 우리 해제가 남의 정지를 지우지 않고 남의 해제도 우리 정지를 지우지 않는다.
166: 	if (WantsGamePause())
167: 	{
168: 		PC->SetPause(true, FCanUnpause::CreateUObject(this, &ThisClass::HandleCanUnpause));
...
298: }
299: 
300: void UWxUIManagerSubsystem::HandleDialogueTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
301: {
302: 	if (NewCount <= 0)
303: 	{
304: 		CloseDialogueScreen();
305: 		return;
306: 	}
307: 
308: 	// 대화 위젯은 Game 레이어 스택 top 에 얹혀 HUD 를 잠시 가리고, 닫히면 HUD 가 복귀한다.
309: 	// 위젯의 뷰모델이 생성 시점에 세션의 현재 대사를 pull 하므로, 세션이 다 채워진 뒤에 오는 이 신호로 띄운다.
310: 	PendingDialogueScreenPush = UWxAsyncAction_PushWidgetToLayer::PushWidgetToLayer(
311: 		this, WxGameplayTags::UI_Layer_Game, GetDefault<UWxUIDeveloperSettings>()->DialogueScreenClass);
312: 	PendingDialogueScreenPush->SetCompletionCallback(
313: 		FWxPushWidgetToLayerNativeDelegate::CreateUObject(this, &ThisClass::HandleDialogueScreenPushCompleted));
314: 	PendingDialogueScreenPush->Activate();
315: }
316: 
317: void UWxUIManagerSubsystem::HandleDialogueScreenPushCompleted(UCommonActivatableWidget* Widget)
318: {
...
333: }
334: 
335: void UWxUIManagerSubsystem::CloseDialogueScreen()
336: {
337: 	if (PendingDialogueScreenPush)
338: 	{
339: 		PendingDialogueScreenPush->Cancel();
340: 		PendingDialogueScreenPush = nullptr;
341: 	}
342: 
343: 	// 띄운 쪽에서 닫는다. 태그가 걷히는 어느 경로로 끝나든 창이 남지 않는다.
344: 	if (UCommonActivatableWidget* Screen = DialogueScreen.Get())
345: 	{
346: 		Screen->DeactivateWidget();
347: 	}
348: 	DialogueScreen.Reset();
349: }
350: 
351: void UWxUIManagerSubsystem::CreateLayoutForPlayer(APlayerController* PC)
352: {
353: 	const UWxUIDeveloperSettings* UISettings = GetDefault<UWxUIDeveloperSettings>();
```
## Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp
- [저장소 원문](<../../../Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp>)
- SHA-256: `3280a0009008fa0107492122b3551d4acf7d7d22ea2101e9144b862950195fdc`
```text
...
22: 	}
23: 
24: 	OwningController->OnPossessedPawnChanged.AddDynamic(this, &ThisClass::HandlePossessedPawnChanged);
25: 
26: 	// BeginPlay 가 빙의보다 늦으면 신호가 다시 오지 않으므로, 지금 폰으로 따라잡는다.
27: 	HandlePossessedPawnChanged(nullptr, OwningController->GetPawn());
28: }
29: 
30: void UWxPlayerLayoutComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
31: {
32: 	if (APlayerController* OwningController = Cast<APlayerController>(GetOwner()))
33: 	{
34: 		OwningController->OnPossessedPawnChanged.RemoveDynamic(this, &ThisClass::HandlePossessedPawnChanged);
35: 	}
36: 
37: 	ClearLayout();
38: 
39: 	Super::EndPlay(EndPlayReason);
40: }
41: 
42: void UWxPlayerLayoutComponent::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
43: {
44: 	// ViewModel은 생성 당시 Pawn의 ASC를 소유자로 삼으므로 빙의 해제·교체 때 함께 걷는다.
45: 	if (OldPawn != NewPawn)
...
81: }
82: 
83: void UWxPlayerLayoutComponent::ClearLayout()
84: {
85: 	if (PendingLayoutPush)
86: 	{
87: 		PendingLayoutPush->Cancel();
88: 		PendingLayoutPush = nullptr;
89: 	}
90: 	UWxPrimaryGameLayout* Layout = UWxUILibrary::GetPrimaryGameLayout(this);
91: 	UCommonActivatableWidgetStack* Stack = Layout ? Layout->GetLayerWidgetStack(WxGameplayTags::UI_Layer_Game) : nullptr;
92: 	if (UCommonActivatableWidget* Widget = LayoutWidget.Get())
93: 	{
94: 		Widget->DeactivateWidget();
95: 		if (Stack)
96: 		{
97: 			Stack->RemoveWidget(*Widget);
98: 		}
99: 	}
100: 	LayoutWidget.Reset();
101: }
```
## Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp
- [저장소 원문](<../../../Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp>)
- SHA-256: `ca63ad2b6782720c05ebdaa146dfce4ebfffb034c56133e0e2d8e6c33819b548`
```text
...
9: #include "GameFramework/PlayerController.h"
10: 
11: void UWxViewModel_Attribute::Initialize(UAbilitySystemComponent* InASC, FGameplayAttribute InAttribute, FGameplayAttribute InMaxAttribute)
12: {
13: 	Deinitialize();
14: 	if (!InASC || !InAttribute.IsValid())
15: 	{
16: 		return;
17: 	}
18: 
19: 	InMaxAttribute = InMaxAttribute.IsValid() ? InMaxAttribute : InAttribute;
20: 
21: 	CachedASC = InASC;
22: 	BoundAttribute = InAttribute;
23: 	BoundMaxAttribute = InMaxAttribute;
24: 
25: 	const float InitialValue = InASC->GetNumericAttribute(InAttribute);
26: 	const float InitialMaxValue = InASC->GetNumericAttribute(InMaxAttribute);
27: 	SetAttributeAmount(InitialValue);
28: 	SetMaxAttributeAmount(InitialMaxValue);
29: 	SetIsAttributeEmpty(InitialValue <= 0.f);
...
37: }
38: 
39: void UWxViewModel_Attribute::Deinitialize()
40: {
41: 	if (UAbilitySystemComponent* ASC = CachedASC.Get())
42: 	{
43: 		if (BoundAttribute.IsValid())
44: 		{
45: 			ASC->GetGameplayAttributeValueChangeDelegate(BoundAttribute).RemoveAll(this);
46: 		}
47: 
48: 		if (BoundMaxAttribute.IsValid())
49: 		{
50: 			ASC->GetGameplayAttributeValueChangeDelegate(BoundMaxAttribute).RemoveAll(this);
51: 		}
52: 	}
53: 
54: 	CachedASC.Reset();
55: 	BoundAttribute = FGameplayAttribute();
56: 	BoundMaxAttribute = FGameplayAttribute();
57: 
```
## Plugins/WxCore/Source/WxCore/Public/WxUIData.h
- [저장소 원문](<../../../Plugins/WxCore/Source/WxCore/Public/WxUIData.h>)
- SHA-256: `50a747dae82436fa8d9b940b1c28b209023607050316772f9d699351e4bf2a2a`
```text
...
19: };
20: 
21: class WXCORE_API IWxUIData
22: {
23: 	GENERATED_BODY()
24: 
25: public:
26: 	virtual FText GetTitle() const = 0;
27: 
28: 	virtual FText GetDescription() const = 0;
29: 
30: 	/** 텍스처나 머티리얼 */
31: 	virtual TSoftObjectPtr<UObject> GetIcon() const = 0;
32: 
33: 	/** 충전 개념이 없는 구현체는 기본값 1. */
34: 	virtual int32 GetMaxRecharges() const;
35: };
```
