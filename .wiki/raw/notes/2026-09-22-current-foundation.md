---
title: "공용 계약·모듈 선언 정적 조사"
source: "MANUAL"
type: notes
ingested: 2026-09-22
tags: [wx, static-review, foundation]
summary: "공용 계약·모듈 선언 정적 조사의 저장소 원문 발췌와 파일별 SHA-256. 정적 확인 범위이며 실행 검증이 아니다."
revision: fe8c943f49401326e1007fedd78a937c9e66db47
---

# 공용 계약·모듈 선언 정적 조사

조사일: 2026-09-22. 기준 HEAD: `fe8c943f49401326e1007fedd78a937c9e66db47` + 현재 작업 트리. 아래 파일의 선택된 계약·실행 경로를 조사했다. SHA-256은 파일 전체 바이트의 식별값이며 파일 전체의 결함 검토를 뜻하지 않는다. 코드 원문은 해당 저장소 경로가 정본이다. 발췌 전후 생략 부분과 바이너리 에셋·실행 결과는 이 기록에 포함하지 않는다.

## Wx.uproject
- [저장소 원문](<../../../Wx.uproject>)
- SHA-256: `c30a6e32cc9931c9ea9ce1cb57bf6a4c678cf86e453b0c57050093586df666f3`
```text
...
1: {
2: 	"FileVersion": 3,
3: 	"EngineAssociation": "5.8",
4: 	"Category": "",
5: 	"Description": "",
6: 	"Modules": [
7: 		{
8: 			"Name": "WxGame",
9: 			"Type": "Runtime",
10: 			"LoadingPhase": "Default"
11: 		},
12: 		{
13: 			"Name": "WxEditor",
14: 			"Type": "Editor",
15: 			"LoadingPhase": "PostEngineInit"
16: 		}
17: 	],
18: 	"Plugins": [
19: 		{
20: 			"Name": "ModelingToolsEditorMode",
21: 			"Enabled": true,
22: 			"TargetAllowList": [
23: 				"Editor"
24: 			]
25: 		},
26: 		{
27: 			"Name": "GameplayAbilities",
28: 			"Enabled": true
29: 		},
30: 		{
31: 			"Name": "WxCore",
```
## Plugins/WxCore/Source/WxCore/WxCore.Build.cs
- [저장소 원문](<../../../Plugins/WxCore/Source/WxCore/WxCore.Build.cs>)
- SHA-256: `9f7dbb0de6af0cc19faa353f176d6fca882e65b6399726098286bd86b7cfd52e`
```text
...
9: 		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
10: 
11: 		PublicDependencyModuleNames.AddRange(new string[]
12: 		{
13: 			"Core",
14: 			"CoreUObject",
15: 			"Engine",
16: 			"GameplayTags",
17: 		});
18: 
19: 		if (Target.bBuildEditor)
20: 		{
21: 			PublicDependencyModuleNames.AddRange(new string[]
22: 			{
23: 				"UniversalObjectLocator",
24: 			});
25: 		}
26: 	}
27: }
```
## Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h
- [저장소 원문](<../../../Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h>)
- SHA-256: `78a4536c09f65622ccba3fa214ca613136674ed15f9802cdca72043b79c52bfa`
```text
...
5: #include "NativeGameplayTags.h"
6: 
7: /** 태그 추가 시 이 파일과 WxGameplayTags.cpp에만 작성. */
8: namespace WxGameplayTags
9: {
10: 	/** 로컬 플레이어가 이 액터를 락온 중일 때 피대상 ASC에만 붙는 개인 UI 상태. */
11: 	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_LockedOn);
12: 
13: 	/** 적 AI가 유효한 전투 대상을 보유하고 있는 교전 상태. */
14: 	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Engaged);
15: 	
16: 	/**
17: 	 * 대화 세션 컴포넌트가 시작·종료에 맞춰 폰 ASC에 loose 태그로 발행한다.
18: 	 * WxAbility_Interact가 ActivationBlockedTags로 사용해 대화 중 프롬프트 표시·상호작용을 닫는다.
19: 	 */
20: 	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dialogue);
21: 
22: 	/**
23: 	 * 소환물을 보유한 주인에게 붙는다. 소환물 컴포넌트가 종류마다 하나를 선언하고 서브시스템이 로스터 변경마다 발행·복제한다.
24: 	 * 같은 입력을 쓰는 소환·명령 스킬의 발동 조건이며, 부모 State.MinionMaster 로 물으면 종류를 가리지 않는다.
25: 	 */
26: 	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_MinionMaster_Minion);
27: 
28: 	/** 소환물 컴포넌트가 선언하는 값이라 C++ 에서는 읽지 않는다. */
29: 	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_MinionMaster_Doppelganger);
30: 
31: 	/**
32: 	 * UWxAbility_Death가 서버에서 loose 태그로 발행한다(TagOnly 복제).
33: 	 * AWxCharacterBase가 구독해 전 머신에서 래그돌로 전환한다.
34: 	 */
35: 	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Ragdoll);
36: 
37: 	// GE가 부여하는 태그. 애셋 태그로도 사용한다.
38: 
39: 	/** WxEffect_Invincible이 부여하며, 구간을 연 쪽(노티파이 구간·스킬 컷신 컴포넌트의 세션·처형의 활성 구간)이 수명을 쥔다 */
40: 	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Invincible);
41: 
42: 	/**
43: 	 * 가드 어빌리티가 WxEffect_GuardReduction으로 부여하고 종료에서 걷는다.
44: 	 * SP 고갈로 가드가 깨질 때는 리액션 어빌리티가 가드를 끊어 같은 경로로 걷힌다.
45: 	 */
46: 	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_GuardReduction);
47: 
48: 	/** 가드 몽타주의 노티파이 구간이 WxEffect_PerfectGuard로 부여하고 구간 끝에서 걷어낸다 */
49: 	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_PerfectGuard);
50: 
51: 	/** SP를 소모하면 WxEffect_Exhaust가 일정 시간 부여한다. SP 자연 회복의 억제 조건 */
52: 	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Exhausted);
53: 
```
## Plugins/WxCore/Source/WxCore/Public/WxInteractable.h
- [저장소 원문](<../../../Plugins/WxCore/Source/WxCore/Public/WxInteractable.h>)
- SHA-256: `137cc33839f2996f6e8610d385c7aa6fb6365b26c52d37629bf38bd7cecd81a3`
```text
...
10: 
11: /** 상호작용 선택지 하나. 한 대상이 여럿을 내놓으면 HUD 목록에 그 수만큼 행이 생긴다. */
12: struct FWxInteractionOption
13: {
14: 	FText Prompt;
15: 
16: 	/** 고른 선택지를 대상에 되돌려 줄 때 쓰는 값. 뜻은 대상이 정한다(엘리베이터는 정차 지점 번호). 선택지가 하나뿐인 대상은 INDEX_NONE. */
17: 	int32 Value = INDEX_NONE;
18: };
19: 
20: /**
21:  * 상호작용 대상의 공용 계약. 대상 액터가 구현한다 — 컴포넌트는 구현하지 않는다.
22:  *
23:  * 능력이 컴포넌트에 담기더라도(대화·장치) 계약은 액터가 들고 그 컴포넌트로 넘긴다.
24:  * 다만 감지와 사거리는 여전히 콜리전 형상 위에서 돈다.
25:  * 액터에 쿼리 콜리전이 켜진 프리미티브가 하나도 없으면 스캔에도 사거리에도 걸리지 않는다(스켈레탈이면 피직스 애셋도 필요).
26:  * 계약이 WxCore 에 있으므로 소비 도메인(예: WxInventory 픽업)이 WxWorld 에 의존하지 않고도 자기 액터를 상호작용 대상으로 만들 수 있다.
27:  */
28: UINTERFACE(MinimalAPI, NotBlueprintable, meta = (CannotImplementInterfaceInBlueprint))
29: class UWxInteractable : public UInterface
30: {
...
41: 	 * 클라 표시 게이트와 서버 발동 검증이 같은 답을 받는다.
42: 	 */
43: 	virtual bool CanInteract(const AActor* Interactor) const;
44: 
45: 	/** 기본은 GetInteractionPrompt() 하나다. 선택지가 여럿인 대상만 재정의한다. */
46: 	virtual void GetInteractionOptions(const AActor* Interactor, TArray<FWxInteractionOption>& OutOptions) const;
47: 
48: 	/** OptionValue 는 플레이어가 고른 선택지의 Value 다. 클라가 보낸 값이지만, 호출하는 상호작용 어빌리티가 서버에서 지금의 선택지와 대조한 뒤에만 부른다. */
49: 	virtual void OnInteracted(AActor* Interactor, int32 OptionValue) = 0;
50: 
51: 	virtual FText GetInteractionPrompt() const = 0;
52: };
```
## Plugins/WxCore/Source/WxCore/Public/WxSpawnable.h
- [저장소 원문](<../../../Plugins/WxCore/Source/WxCore/Public/WxSpawnable.h>)
- SHA-256: `a5449f078c10a7aa51c1d72bb20da33866d0c3e0d87e5d5f934b996252a88b32`
```text
...
23: public:
24: 	/** 처치 판정은 스폰된 액터가 하고 이것으로 스폰 주체에 알린다. 스폰 주체의 처치 상태가 서버 권위이므로 서버에서 방송한다. */
25: 	virtual FWxOnSpawnableKilled& GetOnKilledDelegate() = 0;
26: };
```
## Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h
- [저장소 원문](<../../../Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h>)
- SHA-256: `5e217c7e03eb478e4ff1577a04d2e86923d64568b40b71b753462e114e605671`
```text
...
9:  * 항목은 Channel을 키로 명시 바인딩하므로 줄 순서는 무관하고, 그 값을 바꾸거나 항목을 지우면 아래 판정이 전부 다른 채널을 가리킨다.
10:  *
11:  * ECC_WxAttack: 무기·투사체 히트박스의 Object Type으로 사용하는 Object Channel.
12:  *               DefaultResponse=Block 이므로 별도 override 없는 프로파일(WorldStatic/WorldDynamic/BlockAll 등)은 WxAttack을 Block한다.
13:  *               캐릭터 메시는 WxAttack에 Overlap으로, 캡슐은 Ignore로 명시 override하여 메시에서만 피격 판정이 일어난다.
14:  *               투사체는 "WxProjectile" 프리셋을 사용한다.
15:  */
16: inline constexpr ECollisionChannel ECC_WxAttack = ECC_GameTraceChannel1;
```
## Config/DefaultEngine.ini
- [저장소 원문](<../../../Config/DefaultEngine.ini>)
- SHA-256: `ab39c0488d228111a0f7e80cc77fb4b866ab656570bfd05a710dcf4a86c173bd`
```text
...
37: 
38: [/Script/Engine.CollisionProfile]
39: +DefaultChannelResponses=(Channel=ECC_GameTraceChannel1,DefaultResponse=ECR_Block,bTraceType=False,bStaticObject=False,Name="WxAttack")
40: +Profiles=(Name="WxProjectile",CollisionEnabled=QueryOnly,ObjectTypeName="WxAttack",bCanModify=False,HelpMessage="Wx 투사체 히트박스: 월드에 Block, Pawn에 Overlap",CustomResponses=((Channel="WorldStatic",Response=ECR_Block),(Channel="WorldDynamic",Response=ECR_Block),(Channel="Pawn",Response=ECR_Overlap),(Channel="Visibility",Response=ECR_Ignore),(Channel="Camera",Response=ECR_Ignore),(Channel="PhysicsBody",Response=ECR_Ignore),(Channel="Vehicle",Response=ECR_Ignore),(Channel="Destructible",Response=ECR_Ignore),(Channel="WxAttack",Response=ECR_Ignore)))
41: +Profiles=(Name="WxCharacterMesh",CollisionEnabled=QueryOnly,ObjectTypeName="Pawn",bCanModify=False,HelpMessage="Wx 캐릭터 몸통 충돌: 월드/공격에 Block, Pawn 간 충돌 Ignore",CustomResponses=((Channel="WorldStatic",Response=ECR_Block),(Channel="WorldDynamic",Response=ECR_Block),(Channel="Pawn",Response=ECR_Ignore),(Channel="Visibility",Response=ECR_Block),(Channel="Camera",Response=ECR_Ignore),(Channel="PhysicsBody",Response=ECR_Block),(Channel="Vehicle",Response=ECR_Ignore),(Channel="Destructible",Response=ECR_Block),(Channel="WxAttack",Response=ECR_Block)))
42: 
43: [/Script/WindowsTargetPlatform.WindowsTargetSettings]
44: DefaultGraphicsRHI=DefaultGraphicsRHI_DX12
45: DefaultGraphicsRHI=DefaultGraphicsRHI_DX12
46: -D3D12TargetedShaderFormats=PCD3D_SM5
47: +D3D12TargetedShaderFormats=PCD3D_SM6
48: -D3D11TargetedShaderFormats=PCD3D_SM5
49: +D3D11TargetedShaderFormats=PCD3D_SM5
50: Compiler=Default
51: AudioSampleRate=48000
52: AudioCallbackBufferFrameSize=1024
53: AudioNumBuffersToEnqueue=1
54: AudioMaxChannels=0
55: AudioNumSourceWorkers=4
56: SpatializationPlugin=
57: SourceDataOverridePlugin=
```
## Config/DefaultGame.ini
- [저장소 원문](<../../../Config/DefaultGame.ini>)
- SHA-256: `9d736697f8d47d82e3abced07ebca0258fb9893cf4dbfc6c20516eac827a5ccd`
```text
...
20: [/Script/GameplayAbilities.AbilitySystemGlobals]
21: GameplayCueNotifyPaths=/Game/Character
22: AbilitySystemGlobalsClassName="/Script/WxCombat.WxAbilitySystemGlobals"
23: 
24: [/Script/UnrealEd.ProjectPackagingSettings]
25: +DirectoriesToAlwaysCook=(Path="/Game/Character")
26: 
27: [/Script/CommonInput.CommonInputSettings]
28: InputData=/Game/UI/CommonUI/WxUIInputData.WxUIInputData_C
29: bEnableEnhancedInputSupport=True
30: 
31: [CommonInputPlatformSettings_Windows CommonInputPlatformSettings]
32: DefaultInputType=MouseAndKeyboard
33: bSupportsMouseAndKeyboard=True
34: bSupportsTouch=False
35: bSupportsGamepad=True
36: DefaultGamepadName=Generic
37: bCanChangeGamepadType=True
38: +ControllerData=/Game/UI/InputData/InputData_KMB.InputData_KMB_C
39: +ControllerData=/Game/UI/InputData/InputData_Xbox.InputData_Xbox_C
40: +ControllerData=/Game/UI/InputData/InputData_PS.InputData_PS_C
...
46: +PrimaryAssetTypesToScan=(PrimaryAssetType="WxPlayerCharacter",AssetBaseClass="/Script/WxGame.WxPlayerCharacter",bHasBlueprintClasses=True,bIsEditorOnly=False,Directories=((Path="/Game")),SpecificAssets=,Rules=(Priority=-1,ChunkId=-1,bApplyRecursively=True,CookRule=AlwaysCook))
47: +PrimaryAssetTypesToScan=(PrimaryAssetType="PrimaryAssetLabel",AssetBaseClass="/Script/Engine.PrimaryAssetLabel",bHasBlueprintClasses=False,bIsEditorOnly=True,Directories=((Path="/Game")),SpecificAssets=,Rules=(Priority=-1,ChunkId=-1,bApplyRecursively=True,CookRule=Unknown))
48: +PrimaryAssetTypesToScan=(PrimaryAssetType="WxItemDefinition",AssetBaseClass="/Script/WxInventory.WxItemDefinition",bHasBlueprintClasses=False,bIsEditorOnly=False,Directories=((Path="/Game/Item")),SpecificAssets=,Rules=(Priority=-1,ChunkId=-1,bApplyRecursively=True,CookRule=Unknown))
49: +PrimaryAssetTypesToScan=(PrimaryAssetType="GameFeatureData",AssetBaseClass="/Script/GameFeatures.GameFeatureData",bHasBlueprintClasses=False,bIsEditorOnly=False,Directories=,SpecificAssets=,Rules=(Priority=-1,ChunkId=-1,bApplyRecursively=True,CookRule=AlwaysCook))
50: bOnlyCookProductionAssets=False
51: bShouldManagerDetermineTypeAndName=True
52: bShouldGuessTypeAndNameInEditor=True
53: bShouldAcquireMissingChunksOnLoad=False
54: bShouldWarnAboutInvalidAssets=True
55: MetaDataTagsForAssetRegistry=()
56: 
57: [/Script/WxGame.WxFrontEndDeveloperSettings]
58: +CharacterOptions=/Game/Character/Template/Player/BP_Template.BP_Template_C
59: +CharacterOptions=/Game/Character/HGTest/BP_HGTest.BP_HGTest_C
60: +LevelOptions=/Game/Maps/LV_DevCombat.LV_DevCombat
61: +LevelOptions=/Game/Maps/LV_OpenWorld.LV_OpenWorld
```
