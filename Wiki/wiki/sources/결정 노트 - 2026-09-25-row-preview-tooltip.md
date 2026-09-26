---
type: source
title: "결정 노트 - 2026-09-25-row-preview-tooltip"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "에디터"
  - "DataTable"
summary: "사용자 후속 요청으로 Row 미리보기의 축약 셀과 툴팁에 같은 텍스트를 표시하도록 바꾸어 원문 툴팁 유지 정책을 대체한 기록"
source_type: decision-note
source_id: src-175f131530c1349f27ec
sha256: d78e5dbd11d55759ed4f14308585d49e96cdfc226f79803dc7f43b7a64e6e0c9
authority: primary
independence_key: ".wiki/raw/notes/2026-09-25-row-preview-tooltip.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-25-row-preview-tooltip.md"
raw_copy: ".raw/captured/d78e5dbd11d55759ed4f14308585d49e96cdfc226f79803dc7f43b7a64e6e0c9.md"
claim_ids:
  - clm-85f381a09a-c1
  - clm-85f381a09a-c2
  - clm-85f381a09a-c3
key_claims:
  - "사용자 후속 요청으로 Row 미리보기의 셀과 툴팁은 같은 CellText를 표시하며 기본 구조체는 둘 다 {}로 보인다."
  - "툴팁 통일 변경은 축약 셀 툴팁에 엔진 원문을 남기던 앞선 정책을 대체한다."
  - "툴팁 통일 변경은 노트 시점에 정적 확인만 했고 빌드·화면 검증을 하지 않았다."
---

# 결정 노트 - 2026-09-25-row-preview-tooltip

- 원본: `.wiki/raw/notes/2026-09-25-row-preview-tooltip.md`
- 원자료 사본: `.raw/captured/d78e5dbd11d55759ed4f14308585d49e96cdfc226f79803dc7f43b7a64e6e0c9.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

옛 LLM Wiki 원자료 노트 `2026-09-25-row-preview-tooltip.md`(제목 "Row 미리보기와 툴팁 표시 통일", source `MANUAL`, ingested 2026-09-25)를 요약한다. [[결정 노트 - 2026-09-25-row-preview-empty-struct]]의 후속으로, 앞선 원문 툴팁 유지 정책을 대체한다. 정적 확인(빌드·실행 검증 아님)이며 노트 날짜 기준이라 현재 코드와 다를 수 있다.

## 사용자 요청과 결정

노트 서술: 사용자가 셀과 툴팁을 둘 다 똑같이 표시하도록 요청했다(원문 인용 표기 없음). 이로써 축약 셀의 툴팁에 엔진 원문을 남기던 정책은 대체되었다.

## 구현 관찰(정적)

- `Source/WxEditor/WxDataTableRowHandleCustomization.cpp`의 `CustomizeChildren`에서 `STextBlock.Text`와 `ToolTipText`에 같은 `CellText`를 전달한다.
- 기본 구조체는 셀과 툴팁 모두 `{}`로 표시한다.
- 별도의 `FullCellText` 변수는 제거했다.

## 검증 범위

후속 변경의 빌드·화면 검증은 수행하지 않았다.

## 관련 주제

- [[에디터 도구]]
- [[작업 - datatable-row-preview]]
- <!--wl-->결정 노트 - 2026-09-26-row-preview-acceptance

## 핵심 주장

- 사용자 후속 요청으로 Row 미리보기의 셀과 툴팁은 같은 CellText를 표시하며 기본 구조체는 둘 다 {}로 보인다. ^c1
- 툴팁 통일 변경은 축약 셀 툴팁에 엔진 원문을 남기던 앞선 정책을 대체한다. ^c2
- 툴팁 통일 변경은 노트 시점에 정적 확인만 했고 빌드·화면 검증을 하지 않았다. ^c3
