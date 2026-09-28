---
type: source
title: "작업 - ability-resolver-to-wxui"
created: 2026-09-28
updated: 2026-09-28
status: developing
tags:
  - source
  - 작업-기록
summary: "Ability Resolver의 WxUI 이관을 과거 이력으로 닫고 IWxUIData 제거 후 WxGame 복귀와 표시 확인을 연결한 완료 기록"
source_type: task-record
source_id: src-67788c4f569f518b988d
sha256: 941a27a626444ed3f7e17b6e1fc06864e7df5690065c17126adfab9f6579d6a4
authority: primary
independence_key: ".agents/workflow/tasks/ability-resolver-to-wxui.md"
review_state: active
refresh_due: 2027-09-28
original_paths:
  - ".agents/workflow/tasks/ability-resolver-to-wxui.md"
raw_copy: ".raw/captured/941a27a626444ed3f7e17b6e1fc06864e7df5690065c17126adfab9f6579d6a4.md"
claim_ids:
  - clm-941a27a626-c1
  - clm-941a27a626-c2
  - clm-941a27a626-c3
key_claims:
  - "Ability Resolver의 2026-09-23 WxUI 이관은 과거 이력이며, 2026-09-25 IWxUIData 제거 결정 뒤 리졸버는 WxGame으로 복귀했다."
  - "이관 때 추가한 CoreRedirects는 2026-09-24 제거됐고, 작업 기록은 리다이렉트 없이 관련 WBP 6개 로드·컴파일 오류·경고 0을 기록한다."
  - "2026-09-27 완료 처리 기록은 남았던 실제 슬롯 표시 확인을 IWxUIData 제거 작업의 HUD 플레이 확인(이우성 2026-09-25 통과)으로 마무리했다고 적는다."
---

# 작업 - ability-resolver-to-wxui

- 원본: `.agents/workflow/tasks/ability-resolver-to-wxui.md`
- 원자료 사본: `.raw/captured/941a27a626444ed3f7e17b6e1fc06864e7df5690065c17126adfab9f6579d6a4.md`
- 수집: 2026-09-28 UTC · 재확인 기한: 2027-09-28

## 확정 결정과 이력

2026-09-23에는 Resolver를 WxUI의 WxViewModel_Ability.h/.cpp에 통합하고 WXUI_API로 내보냈다. 하지만 후속 책임 분리로 현재 위치는 Source/WxGame/MVVM/WxViewModelResolver_Ability.h다. 순수 표시 VM은 WxUI, 도메인 연결은 WxGame에 둔다. 나머지 VM을 옮기기 위해 WxUI에 도메인 의존성을 추가하자는 제안은 철회됐다.

> 사용자 2026-09-27: "이미 끝난것 같은데 확인해서 완료로 옮기든지 적절한 상태를 추가해주세요"

## 검증 범위

이전 이관의 Development 빌드 성공, 리다이렉트 제거 뒤 WBP 6개 로드·컴파일 결과, 후속 HUD 사람 확인을 구분한다. 원자료 아래쪽의 미확인 문장은 이전 이관 시점의 상태다. 현재 구조와 표시 확인의 상세 근거는 [[작업 - ui-data-interface-removal]]을 따른다. 이 수집은 새 코드·게임 실행 검증이 아니다.

## 관련 주제

- [[UI 표시 구조]]

## 핵심 주장

- Ability Resolver의 2026-09-23 WxUI 이관은 과거 이력이며, 2026-09-25 IWxUIData 제거 결정 뒤 리졸버는 WxGame으로 복귀했다. ^c1
- 이관 때 추가한 CoreRedirects는 2026-09-24 제거됐고, 작업 기록은 리다이렉트 없이 관련 WBP 6개 로드·컴파일 오류·경고 0을 기록한다. ^c2
- 2026-09-27 완료 처리 기록은 남았던 실제 슬롯 표시 확인을 IWxUIData 제거 작업의 HUD 플레이 확인(이우성 2026-09-25 통과)으로 마무리했다고 적는다. ^c3
