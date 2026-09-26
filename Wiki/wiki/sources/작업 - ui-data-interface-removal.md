---
type: source
title: "작업 - ui-data-interface-removal"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "작업-기록"
  - "UI"
  - "GAS"
  - "모듈"
summary: "공용 IWxUIData 인터페이스를 없애고 WxCombat 데이터·규칙, WxUI VM, WxGame 리졸버 연결로 역할을 나눈 완료 작업 기록"
source_type: task-record
source_id: src-fbf62a9104d918a4da32
sha256: 28130d6e3e3c13b776af54d4de37b50c73dbafcba2c69355652d63d4c915954c
authority: primary
independence_key: ".agents/workflow/tasks/ui-data-interface-removal.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".agents/workflow/tasks/ui-data-interface-removal.md"
raw_copy: ".raw/captured/28130d6e3e3c13b776af54d4de37b50c73dbafcba2c69355652d63d4c915954c.md"
claim_ids:
  - clm-eac1fbdeab-c1
  - clm-eac1fbdeab-c2
  - clm-eac1fbdeab-c3
  - clm-eac1fbdeab-c4
key_claims:
  - "IWxUIData 공용 인터페이스는 삭제되었고, UI 데이터 연결은 WxGame 리졸버의 정적 함수가 WxUI VM 델리게이트에 연결하는 방식으로 바뀌었다."
  - "IWxUIData 제거 작업에서 도메인 간 Build.cs 의존성은 추가되지 않았고 WxEditor에만 WxCombat·WxGame 의존성이 추가되었다."
  - "IWxUIData 제거 후 HUD·버프·보스·이름표 표시는 2026-09-25 이우성이 코드 리뷰와 플레이로 통과를 확인했다."
  - "원격 클라이언트의 기존 스펙 복제 후 재매칭 신호 누락은 IWxUIData 제거 작업 뒤에도 별도 미해결로 남아 있다."
---

# 작업 - ui-data-interface-removal

- 원본: `.agents/workflow/tasks/ui-data-interface-removal.md`
- 원자료 사본: `.raw/captured/28130d6e3e3c13b776af54d4de37b50c73dbafcba2c69355652d63d4c915954c.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

UI가 어빌리티·이펙트·캐릭터 표시 데이터를 읽던 공용 인터페이스 `IWxUIData`를 제거한 작업이다. 상태는 완료이며 체크리스트 1/1(사람 항목)이 통과했다.

## 요청과 합의 (2026-09-25, 확정 결정)

- WxCombat은 어빌리티·이펙트 데이터와 규칙, WxUI는 VM과 GAS 공통 구독·갱신, WxGame은 리졸버의 데이터 연결을 맡는다.
- 별도 중계 서브시스템이나 공용 대체 인터페이스를 추가하지 않는다.

> 사용자 2026-09-25: "네, 그렇게 고쳐주세요"

## 조사

- 사용처: 어빌리티 슬롯, 활성 효과 목록, 캐릭터 이름·초상화, WxEditor BP 썸네일. 슬롯·캐릭터 리졸버 경로를 저장한 WBP 4개를 확인했다. `Export-AbilitySystemLists.ps1`로 생성 목록을 갱신했으나 변경은 없었다.

## 구현

- 어빌리티 변경 델리게이트와 효과 표시 구성 델리게이트를 WxUI VM에 두고 WxGame 리졸버의 정적 함수로 연결한다.
- 어빌리티·PlayerCharacter 리졸버를 WxGame으로 옮기고 AbilitySystem 리졸버를 추가했다. 캐릭터 VM은 AS VM·이름·초상화를 직접 받는다.
- 썸네일은 WxEditor에서 WxCombat·WxGame 타입을 직접 읽는다(WxEditor에만 두 의존성 추가). 공용 `IWxUIData` h/cpp는 삭제했다.
- 수명: 슬롯은 최초 매칭 전에 연결하고 Deinitialize에서 해제한다. 공유 AS VM의 효과 연결은 한 번만 설정하고, 연결 전 조회된 빈 목록은 현재 활성 GE로 보충한다. 캐릭터 공유 VM의 Outer는 AS VM이며 재조회해도 초기화·이미지 요청을 반복하지 않는다.
- 자체 검증 중 수정: 어빌리티 제거 시 엔진이 인스턴스를 Garbage로 표시해 `CachedAbility.Get()`이 null이 되어 제목·충전 초기화가 생략됐다. `IsExplicitlyNull()`로 처음부터 빈 슬롯과 무효화된 슬롯을 구분했다.

## 검증 범위

- AI 빌드: 최종 WxEditor Win64 Development 성공(exit 0).
- AI 에셋: `WBP_Ability`·`WBP_ItemQuickSlot`·`WBP_Nameplate_Player`·`WBP_PlayerSkills`를 재저장하고, 임시 ClassRedirect를 뺀 새 프로세스에서 WBP·캐릭터 BP·GA·GE 97개를 경고를 오류로 처리해 로드·컴파일했다. `DefaultEngine.ini` 최종 변경은 없다.
- AI 자동화: `Wx.UI.Presentation` 3개 성공·0개 실패(`AbilityRebind`, `EffectLateConnection`, `CharacterSharing`).
- AI 정적: 소스의 `IWxUIData`/`UWxUIData` 참조 0건, 도메인 간 Build.cs 의존성 추가 없음.
- 사람(이우성, 2026-09-25): 코드 리뷰 후 HUD·버프·보스·이름표 표시를 플레이로 확인, 통과.

## 미결정·충돌

- 원격 클라이언트에서 기존 스펙 복제 후 재매칭 신호가 누락되는 문제는 별도 미해결 사항으로 남아 있다.

## 관련 주제

- [[UI 표시 구조]]
- [[어빌리티와 GAS]]
- [[모듈 구조와 코드 정리]]
- [[결정 노트 - 2026-09-25-ui-data-interface-removal]]
- [[결정 노트 - 2026-09-26-ui-data-display-acceptance]]

## 핵심 주장

- IWxUIData 공용 인터페이스는 삭제되었고, UI 데이터 연결은 WxGame 리졸버의 정적 함수가 WxUI VM 델리게이트에 연결하는 방식으로 바뀌었다. ^c1
- IWxUIData 제거 작업에서 도메인 간 Build.cs 의존성은 추가되지 않았고 WxEditor에만 WxCombat·WxGame 의존성이 추가되었다. ^c2
- IWxUIData 제거 후 HUD·버프·보스·이름표 표시는 2026-09-25 이우성이 코드 리뷰와 플레이로 통과를 확인했다. ^c3
- 원격 클라이언트의 기존 스펙 복제 후 재매칭 신호 누락은 IWxUIData 제거 작업 뒤에도 별도 미해결로 남아 있다. ^c4
