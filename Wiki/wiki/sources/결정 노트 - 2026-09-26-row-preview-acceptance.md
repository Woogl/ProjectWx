---
type: source
title: "결정 노트 - 2026-09-26-row-preview-acceptance"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "DataTable"
  - "에디터"
  - "테스트"
summary: "DataTable Row 미리보기의 빈 구조체 축약·설정값 표시·셀과 툴팁 일치를 사람이 확인했고 과거 자동화 테스트는 제출 전 제거된 이력임을 정리"
source_type: decision-note
source_id: src-ad6b6deaa490937b16da
sha256: c5d4d393d3f2facb9de1a3e5c4a7d48b40298d89fcdf6b82d89f93486332ff4e
authority: primary
independence_key: ".wiki/raw/notes/2026-09-26-row-preview-acceptance.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-26-row-preview-acceptance.md"
raw_copy: ".raw/captured/c5d4d393d3f2facb9de1a3e5c4a7d48b40298d89fcdf6b82d89f93486332ff4e.md"
claim_ids:
  - clm-873556746a-c1
  - clm-873556746a-c2
  - clm-873556746a-c3
key_claims:
  - "사람은 DataTable Row 미리보기에서 빈 하위 구조체를 포함한 데이터 없는 구조체 셀이 `{}`로 표시됨을 확인했다."
  - "사람은 DataTable Row 미리보기의 셀과 툴팁이 같은 텍스트를 보여 줌을 확인했다."
  - "Wx.Editor.RowPreview.NestedEmptyStructs 자동화 테스트는 사용자 요청으로 제출 전 제거되어 현재 제출 코드의 회귀 테스트가 아니다."
---

# 결정 노트 - 2026-09-26-row-preview-acceptance

- 원본: `.wiki/raw/notes/2026-09-26-row-preview-acceptance.md`
- 원자료 사본: `.raw/captured/c5d4d393d3f2facb9de1a3e5c4a7d48b40298d89fcdf6b82d89f93486332ff4e.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: 옛 LLM Wiki 결정 노트 `2026-09-26-row-preview-acceptance.md`(제목 "DataTable Row 미리보기의 사람 확인 범위", 수집일 2026-09-26).
- 출처: 작업 기록 `.agents/workflow/tasks/datatable-row-preview.md`의 상태·다음 행동·테스트 체크리스트·후속 수정·사용자 테스트 결과를 2026-09-26 읽었다. SHA-256 `06eb2648bc32a108d118245ccdb155966a27e8121986aecf4e5fbe4077cf1238`로 접수 데이터 해시와 일치한다.
- 체크리스트 사람 항목 3/3 통과, 근거 `이우성 2026-09-25`, 결과 기록 시각 `2026-09-25T17:08:00.518Z`.
- 관련 작업: [[작업 - datatable-row-preview]]

## 사람의 판단 원문

> 사용자(이우성) 2026-09-25: "통과 · 빈 구조체 축약"
> 사용자(이우성) 2026-09-25: "통과 · 설정값 표시"
> 사용자(이우성) 2026-09-25: "통과 · 툴팁"

## 확인한 것

- 빈 하위 구조체를 포함한 데이터 없는 구조체 셀의 `{}` 표시.
- 설정된 태그·컨테이너 값의 표시 유지.
- 셀과 툴팁의 동일 텍스트.
- 이는 실제 데이터 변경이 아닌 미리보기 표시 계약의 확인이며, 기록에 남은 이전 화면 미확인 설명을 이 세 항목 범위에서 대체한다.

## 자동화 테스트 이력

- 작업 기록에는 후속 수정의 Editor Development 빌드 성공과 `Wx.Editor.RowPreview.NestedEmptyStructs` 성공·종료 코드 0이 남아 있다.
- 같은 기록에 따르면 해당 회귀 테스트 코드와 AutomationTest include는 사용자 요청으로 제출 전 제거했다. 테스트 성공은 제거 전 검증 이력이며 현재 제출 코드에 유지되는 회귀 테스트가 아니다.

## 확인하지 않은 것

- 이번 수집에서 빌드·자동화·화면 검증을 재실행하거나 과거 로그를 다시 검사하지 않았다.
- 고정 배열·JSON 파싱 실패 시 원문 유지, 컨테이너 요소 내부 재귀 축약 미지원은 기존 구현 근거의 제약으로 남으며, 이번 사람 결과를 그 경계 조건의 추가 검증으로 확대하지 않는다.
- 작업 상태와 사람 판단의 정본은 작업 기록이다.

## 관련 주제

- [[에디터 도구]]
- [[결정 노트 - 2026-09-25-row-preview-empty-struct]]
- [[결정 노트 - 2026-09-25-row-preview-tooltip]]

## 핵심 주장

- 사람은 DataTable Row 미리보기에서 빈 하위 구조체를 포함한 데이터 없는 구조체 셀이 `{}`로 표시됨을 확인했다. ^c1
- 사람은 DataTable Row 미리보기의 셀과 툴팁이 같은 텍스트를 보여 줌을 확인했다. ^c2
- Wx.Editor.RowPreview.NestedEmptyStructs 자동화 테스트는 사용자 요청으로 제출 전 제거되어 현재 제출 코드의 회귀 테스트가 아니다. ^c3
