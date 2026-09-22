---
title: "WxCore 태그 정의·로케이터 표시·픽업 상호작용 조사"
source: "MANUAL"
type: notes
ingested: 2026-09-22
tags: [wx, static-review, foundation]
summary: "WxCore 태그 정의부·로케이터 표시 헬퍼와 WxCore 상호작용 계약을 쓰는 아이템 픽업의 저장소 원문 발췌와 파일별 SHA-256. 정적 확인 범위이며 실행 검증이 아니다."
revision: 60c324c714b1dab10cd48d36cabad63ace232716
---

# WxCore 태그 정의·로케이터 표시·픽업 상호작용 조사

조사일: 2026-09-22. 기준 HEAD: `60c324c714b1dab10cd48d36cabad63ace232716`, 대상 파일은 작업 트리 변경 없음. 아래 파일의 선택된 계약·실행 경로를 조사했다. SHA-256은 파일 전체 바이트의 식별값이며 파일 전체의 결함 검토를 뜻하지 않는다. 코드 원문은 해당 저장소 경로가 정본이다. 발췌 전후 생략 부분과 바이너리 에셋·실행 결과는 이 기록에 포함하지 않는다.

## Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp
- [저장소 원문](<../../../Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp>)
- SHA-256: `309ffc2d5aed7959224879576eb2188ef1697d1dc0cebb22560a118828e31c6a`
```text
1: // Copyright Woogle. All Rights Reserved.
2: 
3: #include "WxGameplayTags.h"
4: 
5: namespace WxGameplayTags
6: {
7: 	UE_DEFINE_GAMEPLAY_TAG(State_LockedOn, "State.LockedOn");
8: 	UE_DEFINE_GAMEPLAY_TAG(State_Engaged, "State.Engaged");
9: 	UE_DEFINE_GAMEPLAY_TAG(State_Dialogue, "State.Dialogue");
10: 	UE_DEFINE_GAMEPLAY_TAG(State_MinionMaster_Minion, "State.MinionMaster.Minion");
11: 	UE_DEFINE_GAMEPLAY_TAG(State_MinionMaster_Doppelganger, "State.MinionMaster.Doppelganger");
12: 	UE_DEFINE_GAMEPLAY_TAG(State_Ragdoll, "State.Ragdoll");
...
```

## Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h
- [저장소 원문](<../../../Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h>)
- SHA-256: `31bee10aee2866366f43613a728b62ebc624917eb15abf932b38967218ce6e3a`
```text
...
9: /** 저작 도구에 로케이터를 보여주기 위한 헬퍼. */
10: struct WXCORE_API FWxLocatorUtils
11: {
12: #if WITH_EDITOR
13: 	/** 해석되면 액터 라벨(아웃라이너와 동일), 미해석이면 경로 끝 오브젝트 이름, 빈 로케이터는 unset. */
14: 	static FText GetDisplayName(const FUniversalObjectLocator& Locator);
15: 
16: 	/** 표시명 3개까지 나열하고 초과분은 +N 으로 줄인다. */
17: 	static FText GetDisplayNames(const TArray<FUniversalObjectLocator>& Locators);
18: #endif
19: };
```

## Plugins/WxCore/Source/WxCore/Private/WxLocatorUtils.cpp
- [저장소 원문](<../../../Plugins/WxCore/Source/WxCore/Private/WxLocatorUtils.cpp>)
- SHA-256: `2795099f42459f4683177454bc04ba7cc253e9f360edb3049bf0c9559fc86211`
```text
...
5: #if WITH_EDITOR
6: #include "GameFramework/Actor.h"
7: #include "UniversalObjectLocator.h"
8: #include "UniversalObjectLocators/ActorLocatorFragment.h"
9: 
10: FText FWxLocatorUtils::GetDisplayName(const FUniversalObjectLocator& Locator)
11: {
12: 	if (Locator.IsEmpty())
13: 	{
14: 		return INVTEXT("unset");
15: 	}
16: 
17: 	if (const AActor* Actor = Cast<AActor>(Locator.SyncFind()))
18: 	{
19: 		return FText::FromString(Actor->GetActorNameOrLabel());
20: 	}
21: 
22: 	const FUniversalObjectLocatorFragment* Fragment = Locator.GetLastFragment();
23: 	const FActorLocatorFragment* Payload = nullptr;
24: 	if (Fragment && Fragment->TryGetPayloadAs(FActorLocatorFragment::FragmentType, Payload) && Payload)
25: 	{
26: 		const FString SubPath = Payload->Path.GetSubPathString();
27: 		int32 DotIndex = INDEX_NONE;
28: 		return FText::FromString(SubPath.FindLastChar(TEXT('.'), DotIndex) ? SubPath.Mid(DotIndex + 1) : SubPath);
29: 	}
30: 
31: 	return INVTEXT("unresolved");
32: }
...
```

## Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h
- [저장소 원문](<../../../Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h>)
- SHA-256: `786a909b91dd0b7f566301c62aeef1a6a77aceb982d56d4c370eb8ae1322b1ff`
```text
...
14: /**
15:  * 아이템(또는 재화) 지급용 픽업.
16:  *
17:  * 상시 활성이며, 쿼리 콜리전이 켜진 메시 덕에 스캐너에 잡힌다 — 계약이 WxCore 에 있어 WxWorld 를 참조하지 않고도 대상이 된다.
18:  * 상호작용 시 Interactor 의 인벤토리에 ItemDef 를 지급한 뒤 파괴된다.
19:  *
20:  * 외부 스포너(예: UWxRewardLibrary::GrantReward) 가 SetItemDef 로 지급 데이터를 주입하고 LaunchInDirection 으로 물리 발사한다.
21:  */
22: UCLASS(Abstract)
23: class WXINVENTORY_API AWxItemPickup : public AActor, public IWxInteractable
24: {
25: 	GENERATED_BODY()
26: 
27: public:
28: 	AWxItemPickup();
29: 
30: 	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
31: 
32: 	/**
33: 	 * 외부 스포너가 SpawnActorDeferred → FinishSpawning 사이에 지급할 아이템과 수량을 주입할 때 사용.
34: 	 * 서버 권한에서만 호출.
35: 	 */
36: 	void SetItemDef(UWxItemDefinition* InItemDef, int32 InQuantity = 1);
37: 
38: 	/** 서버 권한에서만 동작한다. */
39: 	void LaunchInDirection(const FVector& Direction, float Speed);
40: 
41: 	//~ Begin IWxInteractable
42: 	virtual void OnInteracted(AActor* Interactor, int32 OptionValue) override;
43: 	virtual FText GetInteractionPrompt() const override;
44: 	//~ End IWxInteractable
...
```
