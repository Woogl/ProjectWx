---
type: source
title: "작업 - datatable-row-preview"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "작업-기록"
  - "DataTable"
  - "에디터"
summary: "DataTable 행 미리보기에서 데이터 없는 구조체를 {}로 축약하고 셀과 툴팁을 같은 텍스트로 맞춘 에디터 작업 기록으로, 사람 확인 3/3 통과로 완료됐다."
source_type: task-record
source_id: src-4094d8d54f71a0271819
sha256: 6ade9c1a177ad50b322a23c042f525bd04f22690394b8023b65238779f33b45e
authority: primary
independence_key: ".agents/workflow/tasks/datatable-row-preview.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".agents/workflow/tasks/datatable-row-preview.md"
raw_copy: ".raw/captured/6ade9c1a177ad50b322a23c042f525bd04f22690394b8023b65238779f33b45e.md"
claim_ids:
  - clm-b6b3742e54-c1
  - clm-b6b3742e54-c2
  - clm-b6b3742e54-c3
  - clm-b6b3742e54-c4
key_claims:
  - "datatable-row-preview 작업은 행 미리보기에서 초기 기본값과 같은 구조체를 {}로 축약하고 셀과 툴팁에 같은 텍스트를 보인다."
  - "datatable-row-preview 작업은 빈 하위 구조체를 축약하기 위해 현재 값과 초기 기본값의 엔진 JSON 표현을 재귀 비교한다."
  - "datatable-row-preview 작업의 회귀 자동화 테스트는 성공했지만 사용자 요청으로 제출 전에 제거되어 제출 코드에 포함되지 않는다."
  - "datatable-row-preview 작업의 사람 확인 3항목은 이우성이 2026-09-25 에디터에서 통과로 확인했다."
---

# 작업 - datatable-row-preview

- 원본: `.agents/workflow/tasks/datatable-row-preview.md`
- 원자료 사본: `.raw/captured/6ade9c1a177ad50b322a23c042f525bd04f22690394b8023b65238779f33b45e.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

DataTable 행 미리보기에서 데이터가 없는 구조체를 `{}`로 간략히 보이도록 한 에디터 작업 기록이다. 상태는 완료(체크리스트 3/3 통과)다. 실제 데이터는 바꾸지 않는다.

## 요청

- 데이터 없는 구조체를 `{}`로 간략히 표시(기록은 요청을 요약으로 적었다).
- 사용자 후속 요청으로 셀과 툴팁이 같은 텍스트(`{}` 포함)를 보이게 했다.
- 제출 전 사용자 요청으로 이번 세션의 회귀 테스트 코드와 AutomationTest include를 제거했다.

## 구현 결과

- `CustomizeChildren`에서 구조체의 초기 기본값과 비교해 축약한다. 툴팁은 같은 `CellText` 연결을 쓴다.
- 후속 수정: 사용자 HGTest 화면의 `IgnoreTags`가 설정돼 있어 부모 전체 비교만으로는 빈 하위 구조체를 축약하지 못했다. 현재 값과 초기 기본값의 엔진 JSON 표현을 재귀 비교해 하위 객체도 축약하고, 설정된 태그·컨테이너 값은 유지한다.

## 검증 범위

- AI 빌드: 최초 수정은 C++ 컴파일 성공, 실행 중인 에디터의 DLL 점유로 LNK1104 링크 실패(`Saved/Logs/BuildDoctor/build_2026-09-25_014435_089_12848.log`). 툴팁 변경은 정적 확인과 `git diff --check`만 했다. 후속 재귀 비교 수정은 Editor Development 빌드 성공(`Saved/Logs/BuildDoctor/build_2026-09-25_021809_315_46344.log`).
- AI 자동화 테스트: `Wx.Editor.RowPreview.NestedEmptyStructs` 성공(빈 하위 객체 축약, 설정 태그 보존, 완전히 빈 부모 축약). 이 테스트 코드는 제출 전에 제거했으므로 제출 코드에는 테스트가 없다.
- 사람 확인(통과, 이우성 2026-09-25): 빈 구조체 셀이 `{}`(HGTest IgnoreTags 같은 빈 하위 구조체 포함), 설정값은 축약되지 않고 그대로, 셀 툴팁이 셀과 같은 텍스트.

## 관련 주제

- [[에디터 도구]]
- [[결정 노트 - 2026-09-25-row-preview-empty-struct]]
- [[결정 노트 - 2026-09-25-row-preview-tooltip]]
- [[결정 노트 - 2026-09-26-row-preview-acceptance]]

## 핵심 주장

- datatable-row-preview 작업은 행 미리보기에서 초기 기본값과 같은 구조체를 {}로 축약하고 셀과 툴팁에 같은 텍스트를 보인다. ^c1
- datatable-row-preview 작업은 빈 하위 구조체를 축약하기 위해 현재 값과 초기 기본값의 엔진 JSON 표현을 재귀 비교한다. ^c2
- datatable-row-preview 작업의 회귀 자동화 테스트는 성공했지만 사용자 요청으로 제출 전에 제거되어 제출 코드에 포함되지 않는다. ^c3
- datatable-row-preview 작업의 사람 확인 3항목은 이우성이 2026-09-25 에디터에서 통과로 확인했다. ^c4
