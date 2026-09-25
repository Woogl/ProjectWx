---
title: "UI 데이터 인터페이스 제거와 리졸버 연결"
source: ".agents/workflow/tasks/ui-data-interface-removal.md; Source/WxGame/MVVM; Plugins/WxUI/Source/WxUI"
type: notes
ingested: 2026-09-25
tags: [wx, ui, combat, architecture]
summary: "IWxUIData를 제거하고 WxGame 리졸버가 어빌리티·GE 데이터를 WxUI VM에 전달한다. GAS 공통 갱신·표시 필드는 WxUI에 유지한다."
---

# UI 데이터 인터페이스 제거와 리졸버 연결

## 사용자 합의

- 사용자는 IWxUIData 제거와 모듈 독립성을 요청했다. WxCombat은 어빌리티·이펙트 데이터와 규칙, WxUI는 VM·GAS 공통 구독과 갱신, WxGame은 리졸버 연결을 맡는 예시를 확인하고 "네, 그렇게 고쳐주세요"로 구현을 요청했다.
- 대체 공용 인터페이스·전역 제공자·별도 중계 서브시스템은 추가하지 않는다. 기존 GAS 처리 전체를 옮기지 않고 구체 도메인 타입을 읽는 연결만 WxGame에 둔다.

## 구현 관찰

- WxCore의 WxUIData.h/cpp를 삭제했다. UWxAbilityBase·UWxEffectComponent_UIData·AWxCharacterBase는 인터페이스를 상속하지 않는다. 어빌리티·GE getter는 자기 타입의 일반 함수로 남고, 캐릭터의 항상 빈 GetDescription은 삭제했다.
- UWxViewModelResolver_Ability와 UWxViewModelResolver_PlayerCharacter를 WxUI에서 WxGame으로 옮겼다. VM 클래스와 바인딩 필드 경로는 유지했다.
- 어빌리티 리졸버는 FWxOnBoundAbilityChanged를 정적 함수에 연결해 공유 슬롯 팩토리에 전달한다. Initialize는 이전 상태를 정리한 다음 델리게이트를 보관하고 첫 매칭을 한다. 슬롯 변경은 표시 값을 먼저 비우고 SetPresentation으로 제목·설명·아이콘·최대 충전 수·충전 한 칸의 시간을 받는다. 그 다음 GAS 비용·쿨다운·발동 가능 상태를 갱신한다. Deinitialize는 연결도 해제한다.
- UWxViewModelResolver_AbilitySystem은 GE의 UWxEffectComponent_UIData를 직접 읽는다. 아이콘이 있어야 FWxConfigureEffectViewModel이 표시 필드를 채우고 true를 반환한다. GE 구독·시간·스택·제거는 WxUI VM이 계속 처리한다. 공유 AS VM의 연결을 한 번만 설정하고, 연결 전에 목록을 조회한 경우 활성 GE를 보충해 FieldNotify한다.
- Character VM은 AS VM·이름·초상화를 직접 받는다. 공유본의 Outer는 AS VM이며 재조회로 초기화·이미지 로드를 반복하지 않는다. 보스 리졸버와 NameplateManager도 같은 AS 리졸버를 거쳐 값을 전달한다.
- WxEditor 썸네일은 UWxAbilityBase·AWxCharacterBase·UWxEffectComponent_UIData의 아이콘을 직접 조회한다. WxEditor에 WxCombat·WxGame 의존성을 추가했다. 도메인 플러그인 간 의존성은 추가하지 않았다.
- 리졸버 경로를 저장한 WBP_Ability·WBP_ItemQuickSlot·WBP_Nameplate_Player·WBP_PlayerSkills를 재저장한다. 임시 ClassRedirect는 마이그레이션에만 사용한다.

## 근거와 검증 경계

기준 HEAD는 39f3629a4fa8a5454267fec2b6e4ac9af5b66377이며 아래 미커밋 구현을 포함한다. 이 원자료는 설계 합의와 구현 관찰의 근거다. 빌드·에셋 재로드·자동화 결과와 인간 플레이 확인의 구분은 [작업 기록](../../../.agents/workflow/tasks/ui-data-interface-removal.md)에 남긴다. 기존 쿨다운·슬롯 선택·효과 표시 규칙의 기획은 바꾸지 않았다.

| 파일 | SHA-256 |
|---|---|
| [Source/WxGame/MVVM/WxViewModelResolver_Ability.cpp](../../../Source/WxGame/MVVM/WxViewModelResolver_Ability.cpp) | `cf4673c2608df40717a8ea700e2a0dc02c1f85e0211027df354fed45ef9bce44` |
| [Source/WxGame/MVVM/WxViewModelResolver_AbilitySystem.cpp](../../../Source/WxGame/MVVM/WxViewModelResolver_AbilitySystem.cpp) | `2e2e6168a1f44c0850459a11f943552253a33fe047faaa97899916668382fc4d` |
| [Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.cpp](../../../Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.cpp) | `64d197547ed3b480adae5693396f12d012248677122a9081965efc0f04794739` |
| [Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp](../../../Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp) | `0ac3aa8c3147718138fbbe5ec3e229881b55cc4fd75cf3140a4c609c09ebbc7a` |
| [Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp](../../../Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp) | `59e7a58c7fb633df68f4adf3645fa527624af7fb8e1e804ac644156dbfa180ca` |
| [Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp](../../../Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp) | `7d748245c67ca7593b9fb77a189189dc6170509eb302c57472415d424bba645c` |
| [Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp](../../../Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp) | `62417130a35c22a89d05f645d6edf70c460997282ae10411c7ca8ba477e3a1f7` |
| [Source/WxEditor/WxUIDataThumbnailRenderer.cpp](../../../Source/WxEditor/WxUIDataThumbnailRenderer.cpp) | `557cdcafccdb7814b2d21a7623873ad58008eaf2524bfdb28ba1584604bbe5e6` |
