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

## 구현 관찰

- 쿨다운 통합 뒤 어빌리티 슬롯 VM은 DynamicGrantedTags로 쿨다운 GE를 세고 주기는 IWxUIData::GetCooldownTime()으로 읽는다. ([[작업 - cooldown-unification]])
- UWxViewModelResolver_Dialogue는 CreateInstance에서 세션 OnLineChanged와 VM의 OnAdvanceRequested를 잇고 DestroyInstance에서 RemoveAll(VM)로 해당 VM 구독만 끊으며 자체 상태를 갖지 않는다. ([[작업 - dialogue-presentation-vm]])
- 상호작용 행 VM은 SetPrompt·SetSelected 없이 목록 VM이 필드를 직접 채우는 불변 데이터이며, 선택 변경마다 ListView 엔트리가 새로 붙는다. ([[작업 - interaction-list-vm-simplification]])
- UWxNameplateManagerComponent는 TActorIterator<AWxEnemyCharacter>로 순회해 IsAlive() && (락온 대상 || (거리 안 && State.Engaged))일 때 Nameplate를 캡슐 반높이 + HeadClearance(기본 90) 위치에 붙인다. ([[작업 - nameplate-manager]])
- NameplateManager는 락온 대상에는 MaxVisibilityDistance를 적용하지 않고, 새로 붙일 때만 VisibilityDistanceHysteresis(기본 200cm)만큼 안쪽이어야 한다. ([[작업 - nameplate-manager]])
- Ability.Death·State.Dialogue 태그 관찰과 대화 창 수명은 UWxUIManagerSubsystem에서 UWxPlayerLayoutComponent로 옮겨졌고, 서브시스템은 레이아웃·팝업·일시정지만 맡는다. ([[작업 - player-screen-classes-to-layout-component]])

## 검증 범위

- 대화 리졸버 구조는 빌드·WBP 컴파일로 AI가 확인했고, 대화 표시·클릭 진행·재대화는 2026-09-23 사용자가 인게임에서 통과로 확인했다. ([[작업 - dialogue-presentation-vm]])
- 상호작용 목록 표시·휠 선택·범위 이탈과 리스폰 동작은 2026-09-25 이우성이 인게임에서 통과로 확인했다. ([[작업 - interaction-list-vm-simplification]])
- Nameplate 교전·락온·사망 표시, 크기·위치, 리슨 서버와 원격 클라이언트 구분은 2026-09-25 이우성이 인게임에서 통과로 확인했다. ([[작업 - nameplate-manager]])
- 사망 화면과 대화 창의 표시·닫힘·부활 후 재표시는 2026-09-23 사용자가 인게임에서 확인했다. ([[작업 - player-screen-classes-to-layout-component]])
- IWxUIData 제거 후 HUD·버프·보스·이름표 표시는 2026-09-25 이우성이 플레이로 확인했고, 자동화 Wx.UI.Presentation 3개가 통과했다. ([[작업 - ui-data-interface-removal]])

## 미결정·충돌

- Nameplate_System 기획서의 락온 시 표시 규칙은 은폐된 적을 락온 대상에서 빼는 잠정안이며 협의 예정 상태다. ([[기획서 - Nameplate_System]])
- 원격 클라이언트에서 기존 어빌리티 스펙 복제 후 슬롯 재매칭 신호가 누락되는 문제는 미해결이다. ([[작업 - ui-data-interface-removal]])

## 원자료

- [[기획서 - Nameplate_System]] — 일반·네임드 몬스터 네임플레이트의 HP·DP 표시 구성, 시야·청각·피격 인식 규칙, 표시·숨김 조건과 보스 전용 규칙을 정의한 기획서
- [[작업 - cooldown-unification]] — 쿨다운 그룹별 UWxEffect_Cooldown 파생 클래스를 공용 GE 하나로 통합하고 CooldownTags로 구분하게 바꾼 작업 기록으로, 사람 확인 4/4 통과로 완료됐다.
- [[작업 - dialogue-presentation-vm]] — Dialogue VM을 WxUI의 순수 표시 데이터로 분리하고, 화면 클래스를 거쳐 최종적으로 WxGame 리졸버 세 층 구조로 정리한 완료 작업 기록
- [[작업 - interaction-list-vm-simplification]] — 상호작용 목록 VM을 유지하되 스캐너 신호를 OnRowsChanged 하나로 합쳐 단순화하고, 문구 출처 기준과 엘리베이터 탑승칸 버튼 잠금까지 정리한 완료 작업 기록
- [[작업 - nameplate-manager]] — 적 Nameplate와 락온 레티클을 로컬 플레이어 컨트롤러의 NameplateManager가 붙이고 떼는 구조로 바꾸고 WxGame으로 옮긴 완료 작업 기록
- [[작업 - player-screen-classes-to-layout-component]] — 사망·대화 화면 클래스를 UI 개발자 설정에서 플레이어 레이아웃 컴포넌트로 옮기고 태그 관찰 책임도 함께 이동한 완료 작업 기록
- [[작업 - quest-presentation-vm]] — Quest·QuestObjective VM을 WxUI 순수 표시 데이터로 옮기고, 화면 클래스를 거쳐 WxGame 퀘스트 리졸버 세 층 구조로 정리한 완료 작업 기록
- [[작업 - ui-data-interface-removal]] — 공용 IWxUIData 인터페이스를 없애고 WxCombat 데이터·규칙, WxUI VM, WxGame 리졸버 연결로 역할을 나눈 완료 작업 기록
