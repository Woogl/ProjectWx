---
type: source
title: "결정 노트 - 2026-09-25-ui-data-interface-removal"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "UI"
  - "MVVM"
  - "모듈"
summary: "IWxUIData 인터페이스를 제거하고 WxGame 리졸버가 어빌리티·GE 데이터를 WxUI VM에 전달하도록 모듈 책임을 나눈 사용자 합의와 구현 관찰"
source_type: decision-note
source_id: src-7ee37681d10241dffc83
sha256: c82843a97d751ca4cbeda3033a67185b680dfbba0e5b003ffc2ac2f23c040604
authority: primary
independence_key: ".wiki/raw/notes/2026-09-25-ui-data-interface-removal.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-25-ui-data-interface-removal.md"
raw_copy: ".raw/captured/c82843a97d751ca4cbeda3033a67185b680dfbba0e5b003ffc2ac2f23c040604.md"
claim_ids:
  - clm-bc73262c3c-c1
  - clm-bc73262c3c-c2
  - clm-bc73262c3c-c3
key_claims:
  - "사용자 합의에 따라 WxCore의 IWxUIData 인터페이스(WxUIData.h/cpp)가 삭제되었다."
  - "UWxViewModelResolver_Ability와 UWxViewModelResolver_PlayerCharacter는 WxUI에서 WxGame으로 옮겨졌고 VM 클래스와 바인딩 필드 경로는 유지되었다."
  - "IWxUIData 대체로 공용 인터페이스·전역 제공자·별도 중계 서브시스템을 추가하지 않기로 했다."
---

# 결정 노트 - 2026-09-25-ui-data-interface-removal

- 원본: `.wiki/raw/notes/2026-09-25-ui-data-interface-removal.md`
- 원자료 사본: `.raw/captured/c82843a97d751ca4cbeda3033a67185b680dfbba0e5b003ffc2ac2f23c040604.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

옛 LLM Wiki 원자료 노트 `2026-09-25-ui-data-interface-removal.md`(제목 "UI 데이터 인터페이스 제거와 리졸버 연결", ingested 2026-09-25)를 요약한다. frontmatter source는 `.agents/workflow/tasks/ui-data-interface-removal.md; Source/WxGame/MVVM; Plugins/WxUI/Source/WxUI`다. 기준 HEAD `39f3629a4fa8a5454267fec2b6e4ac9af5b66377`에 미커밋 구현을 포함한 설계 합의·구현 관찰 기록이며, 빌드·에셋 재로드·자동화·사람 플레이 확인 구분은 작업 기록([[작업 - ui-data-interface-removal]])에 남긴다고 적혀 있다. 이 노트 자체는 정적 조사(빌드·실행 검증 아님)이고 노트 날짜 기준이라 현재 코드와 다를 수 있다.

## 사람의 판단 원문

사용자는 IWxUIData 제거와 모듈 독립성을 요청했고, WxCombat은 어빌리티·이펙트 데이터와 규칙, WxUI는 VM·GAS 공통 구독과 갱신, WxGame은 리졸버 연결을 맡는 예시를 확인한 뒤 다음과 같이 구현을 요청했다.

> 사용자 2026-09-25: "네, 그렇게 고쳐주세요"

## 확정 결정

- 모듈 책임: WxCombat = 어빌리티·이펙트 데이터와 규칙, WxUI = VM·GAS 공통 구독·갱신, WxGame = 구체 도메인 타입을 읽는 리졸버 연결.
- 대체 공용 인터페이스·전역 제공자·별도 중계 서브시스템은 추가하지 않는다. 기존 GAS 처리 전체를 옮기지 않는다.
- 기존 쿨다운·슬롯 선택·효과 표시 규칙의 기획은 바꾸지 않았다.

## 구현 관찰

- WxCore의 `WxUIData.h/cpp`를 삭제했다. `UWxAbilityBase`·`UWxEffectComponent_UIData`·`AWxCharacterBase`는 인터페이스를 상속하지 않고, getter는 각 타입의 일반 함수로 남는다. 캐릭터의 항상 빈 `GetDescription`은 삭제했다.
- `UWxViewModelResolver_Ability`와 `UWxViewModelResolver_PlayerCharacter`를 WxUI에서 WxGame으로 옮겼다. VM 클래스와 바인딩 필드 경로는 유지했다.
- 어빌리티 리졸버는 `FWxOnBoundAbilityChanged`를 정적 함수에 연결해 공유 슬롯 팩토리에 전달한다. 슬롯 변경 시 표시 값을 비우고 `SetPresentation`으로 제목·설명·아이콘·최대 충전 수·충전 한 칸 시간을 받은 뒤 GAS 비용·쿨다운·발동 가능 상태를 갱신한다.
- `UWxViewModelResolver_AbilitySystem`은 GE의 `UWxEffectComponent_UIData`를 직접 읽고, 아이콘이 있어야 `FWxConfigureEffectViewModel`이 표시 필드를 채운다. GE 구독·시간·스택·제거는 WxUI VM이 계속 처리한다.
- Character VM은 AS VM·이름·초상화를 직접 받으며, 보스 리졸버와 NameplateManager도 같은 AS 리졸버를 거친다.
- WxEditor 썸네일은 세 타입의 아이콘을 직접 조회하며 WxEditor에 WxCombat·WxGame 의존성을 추가했다. 도메인 플러그인 간 의존성은 추가하지 않았다.
- 리졸버 경로를 저장한 `WBP_Ability`·`WBP_ItemQuickSlot`·`WBP_Nameplate_Player`·`WBP_PlayerSkills`를 재저장하며, 임시 ClassRedirect는 마이그레이션에만 쓴다.

## 검증 범위

노트는 주요 파일(리졸버 3개, WxUI VM 4개, 썸네일 렌더러)의 SHA-256로 입력을 식별할 뿐, 빌드·플레이 결과는 작업 기록으로 넘긴다.

## 관련 주제

- [[UI 표시 구조]]
- [[모듈 구조와 코드 정리]]
- [[작업 - ui-data-interface-removal]]
- [[결정 노트 - 2026-09-26-ui-data-display-acceptance]]

## 핵심 주장

- 사용자 합의에 따라 WxCore의 IWxUIData 인터페이스(WxUIData.h/cpp)가 삭제되었다. ^c1
- UWxViewModelResolver_Ability와 UWxViewModelResolver_PlayerCharacter는 WxUI에서 WxGame으로 옮겨졌고 VM 클래스와 바인딩 필드 경로는 유지되었다. ^c2
- IWxUIData 대체로 공용 인터페이스·전역 제공자·별도 중계 서브시스템을 추가하지 않기로 했다. ^c3
