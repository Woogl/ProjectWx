---
type: concept
title: "UI 표시 구조"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - concept
summary: "VM·리졸버·Nameplate 등 화면 표시 연결 구조"
sources:
  - "[[결정 노트 - 2026-09-23-ability-resolver-module]]"
  - "[[결정 노트 - 2026-09-23-boss-battle-three-layer]]"
  - "[[결정 노트 - 2026-09-23-dialogue-presentation-vm]]"
  - "[[결정 노트 - 2026-09-23-interaction-list-vm]]"
  - "[[결정 노트 - 2026-09-23-item-viewmodel-unification]]"
  - "[[결정 노트 - 2026-09-23-player-screen-owner]]"
  - "[[결정 노트 - 2026-09-23-screen-classes-to-resolvers]]"
  - "[[기획서 - Nameplate_System]]"
  - "[[작업 - cooldown-unification]]"
  - "[[작업 - dialogue-presentation-vm]]"
  - "[[작업 - interaction-list-vm-simplification]]"
  - "[[작업 - nameplate-manager]]"
  - "[[작업 - player-screen-classes-to-layout-component]]"
  - "[[작업 - quest-presentation-vm]]"
  - "[[작업 - ui-data-interface-removal]]"
---

# UI 표시 구조

VM·리졸버·Nameplate 등 화면 표시 연결 구조에 관한 원자료 요약을 모은 주제 페이지입니다. 문장마다 끝의 링크가 출처이며, 절 이름으로 기획 요구사항·사람의 확정 결정·코드 구현 관찰·검증 범위·미결정을 구분합니다. 구현 관찰은 원자료 작성 시점의 코드 기준이고, 문서 갱신이나 Wiki lint 통과는 게임 동작 검증이 아닙니다.

## 요구사항

- Nameplate_System 기획서는 일반/네임드 몬스터 네임플레이트에 HP·DP 게이지를 좌측 기준 잔량 방식으로 표시하고, 이름은 표시하지 않으며, 디버프는 하단에 쿨타임 형식으로 표시하도록 요구한다. ([[기획서 - Nameplate_System]])
- Nameplate_System 기획서는 네임플레이트를 숨김 상태로 시작해 인식 또는 카메라 락온 시 표시하고, 추적 종료 시 페이드 없이 즉시 숨기도록 요구한다. ([[기획서 - Nameplate_System]])
- Nameplate_System 기획서는 한번 표시된 네임플레이트를 벽에 가려져도 가림 처리 없이 계속 보여 주고, 동시 표시 상한 없이 겹치면 카메라에 가까운 적을 위에 그리도록 요구한다. ([[기획서 - Nameplate_System]])

## 확정 결정

- 대화 화면은 사용자 결정에 따라 전용 위젯 클래스 없이 WxGame 리졸버가 WxUI 순수 표시 VM을 세션에 연결하는 세 층 구조로 정리되었다. ([[작업 - dialogue-presentation-vm]])
- 상호작용 목록은 목록 VM과 행 VM 두 클래스 구조와 Resolver를 유지하며, Resolver가 FindComponentByClass로 찾은 스캐너를 목록 VM Initialize에 넘긴다. ([[작업 - interaction-list-vm-simplification]])
- 적 Nameplate와 락온 레티클은 로컬 플레이어 컨트롤러의 NameplateManager가 동적으로 붙이고 떼며, 사용자 결정으로 NameplateManager는 WxUI에서 WxGame Controller/로 옮겨졌다. ([[작업 - nameplate-manager]])
- UI 개발자 설정에는 LayoutClass와 ConfirmationPopupClass 같은 UI 틀만 두고, 사망·대화 같은 게임 화면 클래스는 컨트롤러 BP의 UWxPlayerLayoutComponent에 둔다. ([[작업 - player-screen-classes-to-layout-component]])
- 퀘스트 추적기는 사용자 결정에 따라 전용 위젯 클래스 없이 WBP_QuestTracker가 UserWidget을 부모로 두고 리졸버가 만든 WxUI Quest VM으로 구동된다. ([[작업 - quest-presentation-vm]])
- IWxUIData 제거 뒤 UI 데이터는 WxCombat이 원본 데이터·규칙, WxUI가 VM과 GAS 공통 구독, WxGame 리졸버가 데이터 연결을 맡으며 중계 서브시스템이나 대체 인터페이스는 두지 않는다. ([[작업 - ui-data-interface-removal]])
- 사용자는 2026-09-23 VM을 전부 WxUI에 모으고, 도메인 데이터가 필요한 표시는 모델(WxGame)·연결(WxGame 리졸버)·VM(WxUI 순수 표시) 세 층으로 나누도록 결정했다. ([[결정 노트 - 2026-09-23-boss-battle-three-layer]])
- 사용자는 Dialogue VM을 자식 VM 추가 없이 순수 표시 데이터로 만들고 WxGame이 도메인과 화면을 연결하도록 정했다. ([[결정 노트 - 2026-09-23-dialogue-presentation-vm]])
- 사용자는 상호작용 목록 VM과 행 VM 두 클래스와 리졸버를 유지하기로 정했고, 행 VM 하나로 합치는 안은 역할 중복과 CoreRedirects 3개 필요로 기각했다. ([[결정 노트 - 2026-09-23-interaction-list-vm]])
- 사용자는 WxGame 인벤토리 아이템 VM을 삭제하고 값을 받기만 하는 WxUI UWxViewModel_Item으로 단일화하기로 정했다. ([[결정 노트 - 2026-09-23-item-viewmodel-unification]])
- 인벤토리 카테고리 변환 함수 라이브러리와 위젯 변수 방식은 공유 인벤토리 VM이 카테고리 상태를 갖는 더 단순한 방식으로 철회되었다. ([[결정 노트 - 2026-09-23-item-viewmodel-unification]])
- 사용자는 UWxUIDeveloperSettings에는 UI 틀(LayoutClass, ConfirmationPopupClass)만 두고 게임플레이에 반응하는 화면은 컨트롤러 BP의 UWxPlayerLayoutComponent에 두기로 정했다. ([[결정 노트 - 2026-09-23-player-screen-owner]])
- 사용자는 MVVM을 쓰므로 Widget 클래스를 늘릴 필요가 없다며 UWxDialogueScreen·UWxQuestTracker를 제거하고 WBP가 뷰모델로 구동되게 했다. ([[결정 노트 - 2026-09-23-screen-classes-to-resolvers]])

## 구현 관찰

- 쿨다운 통합 뒤 어빌리티 슬롯 VM은 DynamicGrantedTags로 쿨다운 GE를 세고 주기는 IWxUIData::GetCooldownTime()으로 읽는다. ([[작업 - cooldown-unification]])
- UWxViewModelResolver_Dialogue는 CreateInstance에서 세션 OnLineChanged와 VM의 OnAdvanceRequested를 잇고 DestroyInstance에서 RemoveAll(VM)로 해당 VM 구독만 끊으며 자체 상태를 갖지 않는다. ([[작업 - dialogue-presentation-vm]])
- 상호작용 행 VM은 SetPrompt·SetSelected 없이 목록 VM이 필드를 직접 채우는 불변 데이터이며, 선택 변경마다 ListView 엔트리가 새로 붙는다. ([[작업 - interaction-list-vm-simplification]])
- UWxNameplateManagerComponent는 TActorIterator<AWxEnemyCharacter>로 순회해 IsAlive() && (락온 대상 || (거리 안 && State.Engaged))일 때 Nameplate를 캡슐 반높이 + HeadClearance(기본 90) 위치에 붙인다. ([[작업 - nameplate-manager]])
- NameplateManager는 락온 대상에는 MaxVisibilityDistance를 적용하지 않고, 새로 붙일 때만 VisibilityDistanceHysteresis(기본 200cm)만큼 안쪽이어야 한다. ([[작업 - nameplate-manager]])
- Ability.Death·State.Dialogue 태그 관찰과 대화 창 수명은 UWxUIManagerSubsystem에서 UWxPlayerLayoutComponent로 옮겨졌고, 서브시스템은 레이아웃·팝업·일시정지만 맡는다. ([[작업 - player-screen-classes-to-layout-component]])
- 2026-09-23 정적 조사 기준 WxViewModelResolver_Ability는 WxUI 플러그인에 있으며 위젯 소유 PC의 Pawn ASC에서 AbilitySystem VM의 슬롯 캐시를 찾아 준다. ([[결정 노트 - 2026-09-23-ability-resolver-module]])
- 2026-09-23 Ability Resolver 이동에서 기존 WxGame 클래스 경로는 DefaultEngine.ini CoreRedirects로 WxUI 경로에 연결됐다. ([[결정 노트 - 2026-09-23-ability-resolver-module]])
- 2026-09-23 WxViewModelResolver_BossCharacter는 상태를 들지 않는 공유 const 객체로, CreateInstance에서 VM을 만들어 WeakLambda로 구독하고 DestroyInstance에서 RemoveAll(ViewModel)로 그 VM 구독만 끊는다. ([[결정 노트 - 2026-09-23-boss-battle-three-layer]])
- 2026-09-23 정적 확인 기준 WxGame의 WxViewModelResolver_Dialogue가 대화 세션 OnLineChanged를 WxUI Dialogue VM의 SetLine에 연결하고, 세션이 없으면 빈 VM을 만든다. ([[결정 노트 - 2026-09-23-dialogue-presentation-vm]])
- 상호작용 목록 VM은 WxGame에 있고 스캐너를 직접 구독해 신호마다 행 VM 전체를 다시 만들며, WxUI 행 VM은 Prompt·bSelected만 가진 불변 VM이다. ([[결정 노트 - 2026-09-23-interaction-list-vm]])
- 2026-09-23 정적 조사 기준 UWxViewModel_Inventory는 PC당 공유 합성 VM으로 인벤토리 등장·제거를 관찰해 내부 연결만 바꾸며 두 리졸버가 공유본을 쓴다. ([[결정 노트 - 2026-09-23-item-viewmodel-unification]])
- UE 5.8 MVVM 변환 함수는 위젯 블루프린트의 Pure·const 함수나 BlueprintFunctionLibrary 정적 Pure 함수만 허용되고 암시적 변환기는 enum을 제외한다. ([[결정 노트 - 2026-09-23-item-viewmodel-unification]])
- 2026-09-23 정적 조사 기준 UWxPlayerLayoutComponent는 폰 교체 시 새 폰의 Ability.Death·State.Dialogue 태그 관찰로 갈아타고, 대화 창만 닫고 사망 화면은 부활 완료 때 스스로 비활성화되게 둔다. ([[결정 노트 - 2026-09-23-player-screen-owner]])
- UWxUIManagerSubsystem은 레이아웃·팝업·일시정지만 맡고 TrackedPlayerController는 일시정지에만 쓴다. ([[결정 노트 - 2026-09-23-player-screen-owner]])
- 대화·퀘스트 화면은 모델은 도메인, 연결은 WxGame 리졸버, VM은 WxUI인 세 층 구조이며 리졸버 생성·해제는 위젯 NativeConstruct·NativeDestruct를 따른다. ([[결정 노트 - 2026-09-23-screen-classes-to-resolvers]])

## 검증 범위

- 대화 리졸버 구조는 빌드·WBP 컴파일로 AI가 확인했고, 대화 표시·클릭 진행·재대화는 2026-09-23 사용자가 인게임에서 통과로 확인했다. ([[작업 - dialogue-presentation-vm]])
- 상호작용 목록 표시·휠 선택·범위 이탈과 리스폰 동작은 2026-09-25 이우성이 인게임에서 통과로 확인했다. ([[작업 - interaction-list-vm-simplification]])
- Nameplate 교전·락온·사망 표시, 크기·위치, 리슨 서버와 원격 클라이언트 구분은 2026-09-25 이우성이 인게임에서 통과로 확인했다. ([[작업 - nameplate-manager]])
- 사망 화면과 대화 창의 표시·닫힘·부활 후 재표시는 2026-09-23 사용자가 인게임에서 확인했다. ([[작업 - player-screen-classes-to-layout-component]])
- IWxUIData 제거 후 HUD·버프·보스·이름표 표시는 2026-09-25 이우성이 플레이로 확인했고, 자동화 Wx.UI.Presentation 3개가 통과했다. ([[작업 - ui-data-interface-removal]])
- 2026-09-23 Ability Resolver 이동 노트는 정적 조사이며 기존 WBP의 로드·표시 동작은 확인하지 않았다. ([[결정 노트 - 2026-09-23-ability-resolver-module]])
- Dialogue VM 분리는 WxEditor Development 빌드와 WBP 새 프로세스 컴파일·세션 신호 전달 확인까지 했고 인게임 화면·클릭은 미검증이다. ([[결정 노트 - 2026-09-23-dialogue-presentation-vm]])
- 아이템 VM 단일화는 Development·DebugGame 빌드와 관련 위젯 6개 컴파일만 확인했고 런타임 표시·갱신은 인간 확인 대상으로 남았다. ([[결정 노트 - 2026-09-23-item-viewmodel-unification]])
- 사망·대화 화면 주인 이동 후 사용자가 2026-09-23 사망·부활·대화 동작을 인게임에서 확인했다고 노트가 기록한다. ([[결정 노트 - 2026-09-23-player-screen-owner]])
- 대화·퀘스트 화면 리졸버 전환은 WxEditor 빌드와 WBP 4개 경고-오류 컴파일을 통과했고 사용자가 2026-09-23 인게임에서 문제 없음을 확인했다. ([[결정 노트 - 2026-09-23-screen-classes-to-resolvers]])

## 미결정·충돌

- Nameplate_System 기획서의 락온 시 표시 규칙은 은폐된 적을 락온 대상에서 빼는 잠정안이며 협의 예정 상태다. ([[기획서 - Nameplate_System]])
- 원격 클라이언트에서 기존 어빌리티 스펙 복제 후 슬롯 재매칭 신호가 누락되는 문제는 미해결이다. ([[작업 - ui-data-interface-removal]])
- 상호작용 목록은 선택 변경마다 ListView 엔트리가 새로 붙으므로 선택 전환 애니메이션을 넣으려면 선택 신호를 다시 나눠야 한다. ([[결정 노트 - 2026-09-23-interaction-list-vm]])
- 사망·대화 화면 클래스 값이 ini에서 컨트롤러 BP 에셋으로 옮겨져 git diff로 리뷰할 수 없다. ([[결정 노트 - 2026-09-23-player-screen-owner]])

## 원자료

- [[결정 노트 - 2026-09-23-ability-resolver-module]] — Ability ViewModel Resolver를 WxGame에서 WxUI로 옮기고 CoreRedirects로 기존 클래스 경로를 호환시킨 2026-09-23 정적 확인 기록
- [[결정 노트 - 2026-09-23-boss-battle-three-layer]] — 보스 표시를 UWxBattleSubsystem·WxGame 리졸버·WxUI Character VM 세 층으로 재구성하고 보스 식별을 IdentityTags로 바꾼 2026-09-23 결정
- [[결정 노트 - 2026-09-23-dialogue-presentation-vm]] — Dialogue VM을 WxUI의 순수 표시 데이터로 만들고 세션 연결은 WxGame Resolver, 진행 입력은 화면이 맡게 한 모듈 경계 결정과 정적 확인 기록.
- [[결정 노트 - 2026-09-23-interaction-list-vm]] — 상호작용 스캐너 신호를 OnRowsChanged 하나로 합쳐 목록 VM이 행 VM을 전부 재생성하게 한 구조와 문구 출처 원칙에 대한 사용자 결정·정적 조사 기록.
- [[결정 노트 - 2026-09-23-item-viewmodel-unification]] — WxGame 인벤토리 아이템 VM을 WxUI 아이템 VM으로 단일화하고 PC당 공유 인벤토리 VM이 값을 공급하게 한 결정, MVVM 변환 함수 제약, WxToolset 도구 기록.
- [[결정 노트 - 2026-09-23-player-screen-owner]] — 사망·대화 화면 클래스와 태그 관찰을 UIManager 서브시스템·전역 설정에서 컨트롤러 BP의 UWxPlayerLayoutComponent로 옮긴 결정과 정적 조사 기록.
- [[결정 노트 - 2026-09-23-screen-classes-to-resolvers]] — UWxDialogueScreen·UWxQuestTracker C++ 위젯 클래스를 제거하고 WBP가 WxGame 리졸버가 연결한 WxUI 뷰모델로 구동되게 한 사용자 결정과 구현 기록.
- [[기획서 - Nameplate_System]] — 일반·네임드 몬스터 네임플레이트의 HP·DP 표시 구성, 시야·청각·피격 인식 규칙, 표시·숨김 조건과 보스 전용 규칙을 정의한 기획서
- [[작업 - cooldown-unification]] — 쿨다운 그룹별 UWxEffect_Cooldown 파생 클래스를 공용 GE 하나로 통합하고 CooldownTags로 구분하게 바꾼 작업 기록으로, 사람 확인 4/4 통과로 완료됐다.
- [[작업 - dialogue-presentation-vm]] — Dialogue VM을 WxUI의 순수 표시 데이터로 분리하고, 화면 클래스를 거쳐 최종적으로 WxGame 리졸버 세 층 구조로 정리한 완료 작업 기록
- [[작업 - interaction-list-vm-simplification]] — 상호작용 목록 VM을 유지하되 스캐너 신호를 OnRowsChanged 하나로 합쳐 단순화하고, 문구 출처 기준과 엘리베이터 탑승칸 버튼 잠금까지 정리한 완료 작업 기록
- [[작업 - nameplate-manager]] — 적 Nameplate와 락온 레티클을 로컬 플레이어 컨트롤러의 NameplateManager가 붙이고 떼는 구조로 바꾸고 WxGame으로 옮긴 완료 작업 기록
- [[작업 - player-screen-classes-to-layout-component]] — 사망·대화 화면 클래스를 UI 개발자 설정에서 플레이어 레이아웃 컴포넌트로 옮기고 태그 관찰 책임도 함께 이동한 완료 작업 기록
- [[작업 - quest-presentation-vm]] — Quest·QuestObjective VM을 WxUI 순수 표시 데이터로 옮기고, 화면 클래스를 거쳐 WxGame 퀘스트 리졸버 세 층 구조로 정리한 완료 작업 기록
- [[작업 - ui-data-interface-removal]] — 공용 IWxUIData 인터페이스를 없애고 WxCombat 데이터·규칙, WxUI VM, WxGame 리졸버 연결로 역할을 나눈 완료 작업 기록
