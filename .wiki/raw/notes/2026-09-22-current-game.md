---
title: "게임 조립·새 게임·부활 정적 조사"
source: "MANUAL"
type: notes
ingested: 2026-09-22
tags: [wx, static-review, game]
summary: "게임 조립·새 게임·부활 정적 조사의 저장소 원문 발췌와 파일별 SHA-256. 정적 확인 범위이며 실행 검증이 아니다."
revision: fe8c943f49401326e1007fedd78a937c9e66db47
---

# 게임 조립·새 게임·부활 정적 조사

조사일: 2026-09-22. 기준 HEAD: `fe8c943f49401326e1007fedd78a937c9e66db47` + 현재 작업 트리. 아래 파일의 선택된 계약·실행 경로를 조사했다. SHA-256은 파일 전체 바이트의 식별값이며 파일 전체의 결함 검토를 뜻하지 않는다. 코드 원문은 해당 저장소 경로가 정본이다. 발췌 전후 생략 부분과 바이너리 에셋·실행 결과는 이 기록에 포함하지 않는다.

## Source/WxGame/WxGame.Build.cs
- [저장소 원문](<../../../Source/WxGame/WxGame.Build.cs>)
- SHA-256: `33af4f86ed48df81e926a21fb3d9db84b17c0b91d65df627f467e6173f2a9c00`
```text
...
11: 		PublicIncludePaths.AddRange(new string[] { ModuleDirectory });
12: 
13: 		PublicDependencyModuleNames.AddRange(new string[]
14: 		{
15: 			"AIModule",
16: 			"CommonUI",
17: 			"Core",
18: 			"CoreUObject",
19: 			"DeveloperSettings",
20: 			"Engine",
21: 			"GameplayAbilities",
22: 			"GameplayTags",
23: 			"GameplayTasks",
24: 			"MetaHumanSDKRuntime",
25: 			"ModelViewViewModel",
26: 			"MotionWarping",
27: 			"UMG",
28: 			"WxAI",
29: 			"WxCombat",
30: 			"WxCore",
31: 			"WxDialogue",
32: 			"WxInventory",
33: 			"WxQuest",
34: 			"WxUI",
35: 			"WxWorld",
36: 		});
37: 
38: 		PrivateDependencyModuleNames.AddRange(new string[]
39: 		{
40: 			"EnhancedInput",
41: 			"HairStrandsCore",
42: 		});
43: 	}
44: }
```
## Source/WxGame/Character/WxCharacterBase.cpp
- [저장소 원문](<../../../Source/WxGame/Character/WxCharacterBase.cpp>)
- SHA-256: `10daa82f43a6874fb3c49d4265bbe212d9577dae763d1355f6dba50fb5c47cca`
```text
...
20: #include "Component/WxMetaHumanComponent.h"
21: 
22: AWxCharacterBase::AWxCharacterBase(const FObjectInitializer& ObjectInitializer)
23: 	: Super(ObjectInitializer.SetDefaultSubobjectClass<UWxCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
24: {
25: 	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
26: 	GetMesh()->SetCollisionResponseToChannel(ECC_WxAttack, ECR_Overlap);
27: 	GetMesh()->SetGenerateOverlapEvents(true);
28: 	
29: 	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
30: 	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WxAttack, ECR_Ignore);
31: 	
32: 	AbilitySystemComponent = CreateDefaultSubobject<UWxAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
33: 	AbilitySystemComponent->SetIsReplicated(true);
34: 
35: 	CombatAttributeSet = CreateDefaultSubobject<UWxCombatAttributeSet>(TEXT("CombatAttributeSet"));
36: 
37: 	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));
38: 	LockOnComponent = CreateDefaultSubobject<UWxLockOnComponent>(TEXT("LockOnComponent"));
39: 	HitStopComponent = CreateDefaultSubobject<UWxHitStopComponent>(TEXT("HitStopComponent"));
40: 
...
50: }
51: 
52: void AWxCharacterBase::PostInitializeComponents()
53: {
54: 	Super::PostInitializeComponents();
55: 
56: 	// 래그돌 감지는 시뮬 프록시를 포함한 전 머신에서 필요하므로, 클라에선 PlayerState 복제로만 도는(에너미는 안 도는) InitAbilitySystem이 아니라 여기서 구독한다.
57: 	// 원격 머신의 초기 복제는 이 함수 뒤에 적용되므로, 늦게 참여해 이미 선 태그도 이 콜백으로 들어온다.
58: 	AbilitySystemComponent->RegisterGameplayTagEvent(WxGameplayTags::State_Ragdoll, EGameplayTagEventType::NewOrRemoved)
59: 		.AddUObject(this, &AWxCharacterBase::HandleRagdollTagChanged);
60: 
61: 	// 사망 처리도 같은 이유로 여기서 구독한다 — 무기 판정 해제와 OnDeath 방송은 시뮬 프록시를 포함한 전 머신에서 일어나야 한다.
62: 	// 보상 지급 같은 권위 전용 처리는 OnDeath 구독자(AWxEnemyCharacter::HandleOwnerDeath) 안의 HasAuthority 가드가 계속 가른다.
63: 	AbilitySystemComponent->RegisterGameplayTagEvent(WxGameplayTags::Ability_Death, EGameplayTagEventType::NewOrRemoved)
64: 		.AddUObject(this, &AWxCharacterBase::HandleDeathTagChanged);
65: 
66: 	if (!WeaponActor)
67: 	{
68: 		return;
69: 	}
70: 
71: 	if (AActor* SpawnedWeapon = WeaponActor->GetChildActor())
72: 	{
...
75: }
76: 
77: void AWxCharacterBase::PossessedBy(AController* NewController)
78: {
79: 	Super::PossessedBy(NewController);
80: 	
81: 	InitAbilitySystem();
82: }
83: 
84: void AWxCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
85: {
86: 	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
87: 
88: 	DOREPLIFETIME(AWxCharacterBase, Team);
89: }
90: 
91: void AWxCharacterBase::OnJumped_Implementation()
92: {
93: 	Super::OnJumped_Implementation();
94: 
95: 	// 후딜에 든 앞 액션은 배타 어빌리티가 그러듯 점프도 끊는다.
```
## Source/WxGame/Controller/WxPlayerController.cpp
- [저장소 원문](<../../../Source/WxGame/Controller/WxPlayerController.cpp>)
- SHA-256: `af122cc3eed03e7698093e252cd835191c383ca7422b2cd2471394aaa5397b03`
```text
...
9: #include "WxDialogueSessionComponent.h"
10: 
11: AWxPlayerController::AWxPlayerController(const FObjectInitializer& ObjectInitializer)
12: 	: Super(ObjectInitializer)
13: {
14: 	CheatClass = UWxCheatManager::StaticClass();
15: 
16: 	InventoryComponent = CreateDefaultSubobject<UWxInventoryComponent>(TEXT("InventoryComponent"));
17: 	InteractionScannerComponent = CreateDefaultSubobject<UWxInteractionScannerComponent>(TEXT("InteractionScannerComponent"));
18: 	DialogueSessionComponent = CreateDefaultSubobject<UWxDialogueSessionComponent>(TEXT("DialogueSessionComponent"));
19: 	PlayerLayoutComponent = CreateDefaultSubobject<UWxPlayerLayoutComponent>(TEXT("PlayerLayoutComponent"));
20: }
```
## Source/WxGame/Framework/WxGameState.cpp
- [저장소 원문](<../../../Source/WxGame/Framework/WxGameState.cpp>)
- SHA-256: `8f861cd73edb57b8e0d5e62b41df686c437fb029d91a1edb3500a5dba2423813`
```text
...
6: #include "Cutscene/WxSkillCutsceneComponent.h"
7: 
8: AWxGameState::AWxGameState()
9: {
10: 	SkillCutsceneComponent = CreateDefaultSubobject<UWxSkillCutsceneComponent>(TEXT("SkillCutsceneComponent"));
11: 	QuestComponent = CreateDefaultSubobject<UWxQuestComponent>(TEXT("QuestComponent"));
12: }
```
## Source/WxGame/Framework/WxGameMode.cpp
- [저장소 원문](<../../../Source/WxGame/Framework/WxGameMode.cpp>)
- SHA-256: `a7529edda15b76e4be233fc6bf06ecc93325924bed530b73746d2055a27dd13a`
```text
...
15: }
16: 
17: UClass* AWxGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
18: {
19: 	if (UWxGameFlowSubsystem* Flow = GetGameInstance()->GetSubsystem<UWxGameFlowSubsystem>())
20: 	{
21: 		if (UClass* SelectedClass = Flow->GetSelectedPawnClass(GetWorld()))
22: 		{
23: 			return SelectedClass;
24: 		}
25: 	}
26: 
27: 	return Super::GetDefaultPawnClassForController_Implementation(InController);
28: }
```
## Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp
- [저장소 원문](<../../../Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp>)
- SHA-256: `8fe2b8a5b3737f1d558b6e67d15df6e2016a78a6dcda6c2875e948353be3f998`
```text
...
18: {
19: 	Super::Initialize(Collection);
20: 	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::HandlePostLoadMap);
21: 	GEngine->OnTravelFailure().AddUObject(this, &ThisClass::HandleTravelFailure);
22: }
23: 
24: void UWxGameFlowSubsystem::Deinitialize()
25: {
26: 	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
27: 	GEngine->OnTravelFailure().RemoveAll(this);
28: 	Super::Deinitialize();
29: }
30: 
31: bool UWxGameFlowSubsystem::RequestNewGame(TSoftClassPtr<APawn> PawnClass, TSoftObjectPtr<UWorld> Level)
32: {
33: 	if (IsBusy())
34: 	{
35: 		return false;
36: 	}
37: 	UWorld* World = GetWorld();
38: 	if (!World || !World->IsNetMode(NM_Standalone))
39: 	{
40: 		StatusText = LOCTEXT("StandaloneOnly", "새 게임 진입은 싱글플레이에서 사용할 수 있습니다.");
41: 		return false;
42: 	}
43: 	if (PawnClass.IsNull() || Level.IsNull()
44: 		|| !FPackageName::DoesPackageExist(Level.ToSoftObjectPath().GetLongPackageName())
45: 		|| IsWorldPackage(World, Level))
46: 	{
47: 		StatusText = LOCTEXT("InvalidSelection", "캐릭터 또는 레벨을 확인해주세요.");
48: 		return false;
49: 	}
...
71: }
72: 
73: UClass* UWxGameFlowSubsystem::GetSelectedPawnClass(const UWorld* World) const
74: {
75: 	return IsDestinationWorld(World) ? PendingPawnClass.LoadSynchronous() : nullptr;
76: }
77: 
78: void UWxGameFlowSubsystem::HandlePostLoadMap(UWorld* World)
79: {
80: 	if (!World || World->GetGameInstance() != GetGameInstance() || PendingLevel.IsNull())
81: 	{
82: 		return;
83: 	}
84: 	if (!IsDestinationWorld(World))
85: 	{
86: 		// 전환이 어긋났든 이후의 일반 이동이든, 목적지가 아닌 맵이 열리면 선택은 여기서 끝난다.
87: 		PendingPawnClass.Reset();
88: 		PendingLevel.Reset();
89: 		return;
90: 	}
91: 	// 폰은 스폰됐지만 아직 한 틱도 돌지 않았다. 지금 지형을 올려 두면 중력으로 떨어지지 않는다.
92: 	APlayerController* Controller = GetGameInstance()->GetFirstLocalPlayerController();
93: 	if (Controller && Controller->PlayerCameraManager)
94: 	{
95: 		// 월드파티션이 볼 스트리밍 소스 위치를 폰 시점으로 확정한다.
96: 		Controller->PlayerCameraManager->UpdateCamera(0.f);
...
99: }
100: 
101: void UWxGameFlowSubsystem::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& Error)
102: {
103: 	if (!World || World->GetGameInstance() != GetGameInstance() || !IsBusy())
104: 	{
105: 		return;
106: 	}
107: 	// 출발 맵에 그대로 있으므로 선택만 버리면 메뉴가 문구를 띄우고 버튼을 다시 연다.
108: 	PendingPawnClass.Reset();
109: 	PendingLevel.Reset();
110: 	StatusText = FText::Format(LOCTEXT("TravelFailure", "레벨 전환에 실패했습니다: {0}"), FText::FromString(Error));
111: }
112: 
113: bool UWxGameFlowSubsystem::IsDestinationWorld(const UWorld* World) const
114: {
115: 	return !PendingLevel.IsNull() && IsWorldPackage(World, PendingLevel);
116: }
117: 
118: bool UWxGameFlowSubsystem::IsWorldPackage(const UWorld* World, const TSoftObjectPtr<UWorld>& Map) const
119: {
```
## Source/WxGame/Framework/WxRespawnLibrary.cpp
- [저장소 원문](<../../../Source/WxGame/Framework/WxRespawnLibrary.cpp>)
- SHA-256: `9a7b34c0ed05d2f6be55a08052dd4aa91e3db8bb5bf2b58896ee12727a182bef`
```text
...
17: #include "WxGame.h"
18: 
19: bool UWxRespawnLibrary::RequestRespawn(UCommonActivatableWidget* DeathScreen)
20: {
21: 	if (!IsValid(DeathScreen) || !DeathScreen->IsActivated())
22: 	{
23: 		return false;
24: 	}
25: 	APlayerController* Controller = DeathScreen->GetOwningPlayer();
26: 	if (!IsValid(Controller) || !Controller->IsLocalController())
27: 	{
28: 		return false;
29: 	}
30: 	UWorld* World = Controller->GetWorld();
31: 	AGameModeBase* GameMode = World ? World->GetAuthGameMode() : nullptr;
32: 	APawn* DeadPawn = Controller->GetPawn();
33: 	const UAbilitySystemComponent* DeadASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(DeadPawn);
34: 	if (!GameMode || !World->IsNetMode(NM_Standalone) || !IsValid(DeadPawn)
35: 		|| !DeadASC || !DeadASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Death)
36: 		|| !GameMode->GetDefaultPawnClassForController(Controller))
37: 	{
...
45: 	DeadPawn->SetActorEnableCollision(false);
46: 	// 엔진 RestartPlayer는 빙의 중인 Pawn이 있으면 재사용한다.
47: 	Controller->UnPossess();
48: 	if (bHasCheckpoint)
49: 	{
50: 		GameMode->RestartPlayerAtTransform(Controller, RespawnTransform);
51: 	}
52: 	else
53: 	{
54: 		GameMode->RestartPlayer(Controller);
55: 	}
56: 	APawn* NewPawn = Controller->GetPawn();
57: 	if (!IsValid(NewPawn))
58: 	{
59: 		// 생성에 실패하면 사망 화면에서 다시 시도할 수 있도록 기존 Pawn을 유지한다.
60: 		DeadPawn->SetActorEnableCollision(bHadCollision);
61: 		Controller->Possess(DeadPawn);
62: 		UE_LOG(LogWxGame, Error, TEXT("RequestRespawn: 플레이어 생성 실패."));
63: 		return false;
64: 	}
65: 	DeadPawn->Destroy();
66: 	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(NewPawn))
67: 	{
68: 		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetHPAttribute(), ASC->GetNumericAttribute(UWxCombatAttributeSet::GetMaxHPAttribute()));
69: 		ASC->SetNumericAttributeBase(UWxCombatAttributeSet::GetMPAttribute(), ASC->GetNumericAttribute(UWxCombatAttributeSet::GetMaxMPAttribute()));
70: 	}
71: 	// 다음 월드 틱 전에 새 시점의 지형 스트리밍을 완료해 낙하를 방지한다.
72: 	Controller->SetViewTarget(NewPawn);
73: 	if (Controller->PlayerCameraManager)
74: 	{
75: 		Controller->PlayerCameraManager->UpdateCamera(0.0f);
76: 	}
77: 	World->BlockTillLevelStreamingCompleted();
78: 	UWxSpawnerLibrary::TryRespawnAll(Controller);
79: 	DeathScreen->DeactivateWidget();
80: 	return true;
81: }
```
## Config/DefaultGame.ini
- [저장소 원문](<../../../Config/DefaultGame.ini>)
- SHA-256: `9d736697f8d47d82e3abced07ebca0258fb9893cf4dbfc6c20516eac827a5ccd`
```text
...
55: MetaDataTagsForAssetRegistry=()
56: 
57: [/Script/WxGame.WxFrontEndDeveloperSettings]
58: +CharacterOptions=/Game/Character/Template/Player/BP_Template.BP_Template_C
59: +CharacterOptions=/Game/Character/HGTest/BP_HGTest.BP_HGTest_C
60: +LevelOptions=/Game/Maps/LV_DevCombat.LV_DevCombat
61: +LevelOptions=/Game/Maps/LV_OpenWorld.LV_OpenWorld
```
