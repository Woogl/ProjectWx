---
type: source
title: "결정 노트 - 2026-09-25-row-preview-empty-struct"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "에디터"
  - "DataTable"
summary: "DataTable Row 미리보기에서 초기 기본값과 같은 구조체 셀을 {}로 축약하도록 한 사용자 요청과 구현 관찰, 컴파일만 통과한 상태"
source_type: decision-note
source_id: src-05a7632d596861e25362
sha256: 3bd526342d7c815c3b58acec2370b6bcf9bf831d58acfabfc688c89177289015
authority: primary
independence_key: ".wiki/raw/notes/2026-09-25-row-preview-empty-struct.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-25-row-preview-empty-struct.md"
raw_copy: ".raw/captured/3bd526342d7c815c3b58acec2370b6bcf9bf831d58acfabfc688c89177289015.md"
claim_ids:
  - clm-d2863def61-c1
  - clm-d2863def61-c2
  - clm-d2863def61-c3
key_claims:
  - "Row 미리보기 커스터마이제이션은 FStructProperty 셀 값이 FStructOnScope 초기 기본값과 Identical이면 셀 텍스트를 {}로 축약한다."
  - "기본 구조체 축약 판정은 모든 바이트가 0인지가 아니라 생성자 기본값을 포함한 초기 기본값과의 비교다."
  - "노트 시점에 이 변경은 컴파일만 통과했고 링크는 LNK1104로 실패했으며 에디터 화면은 확인되지 않았다."
---

# 결정 노트 - 2026-09-25-row-preview-empty-struct

- 원본: `.wiki/raw/notes/2026-09-25-row-preview-empty-struct.md`
- 원자료 사본: `.raw/captured/3bd526342d7c815c3b58acec2370b6bcf9bf831d58acfabfc688c89177289015.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

옛 LLM Wiki 원자료 노트 `2026-09-25-row-preview-empty-struct.md`(제목 "Row 미리보기의 기본 구조체 축약", source `MANUAL`, ingested 2026-09-25)를 요약한다. 구현 근거는 `Source/WxEditor/WxDataTableRowHandleCustomization.cpp`의 `CustomizeChildren`이고 2026-09-25 작업 트리 SHA-256은 `25EDFEC1D1AFC46FBCDFC282882162C8378497FA027BE5BED08311416A2116E3`이다. 노트 날짜 기준이라 현재 코드와 다를 수 있다. 툴팁 정책은 후속 노트 [[결정 노트 - 2026-09-25-row-preview-tooltip]]가 대체했다.

## 사용자 요청

노트 서술: 데이터가 없는 구조체의 복잡한 Row 미리보기를 `{}` 정도로 줄인다(원문 인용 표기 없이 요약된 요청).

## 구현 관찰

- `Column->Property`가 `FStructProperty`면 `FStructOnScope`로 만든 초기 기본값과 `Identical`로 비교한다. 고정 배열은 모든 요소가 같아야 축약한다.
- 기본값과 같으면 셀 텍스트만 `{}`로 바꾸고 엔진이 만든 원문은 툴팁에 남긴다(이 툴팁 정책은 후속 요청으로 대체됨). 기본값과 다른 구조체·일반 프로퍼티 표시는 유지한다.
- 모든 바이트가 0인지 판정하는 것이 아니며, 생성자에서 설정한 기본값도 축약 대상이다. 실제 Row 데이터는 수정하지 않는다.

## 검증 범위

- C++ 컴파일 통과. 실행 중인 에디터의 DLL 점유로 링크 실패(LNK1104).
- 에디터 화면은 미검증이다.

## 관련 주제

- [[에디터 도구]]
- [[작업 - datatable-row-preview]]
- [[결정 노트 - 2026-09-26-row-preview-acceptance]]

## 핵심 주장

- Row 미리보기 커스터마이제이션은 FStructProperty 셀 값이 FStructOnScope 초기 기본값과 Identical이면 셀 텍스트를 {}로 축약한다. ^c1
- 기본 구조체 축약 판정은 모든 바이트가 0인지가 아니라 생성자 기본값을 포함한 초기 기본값과의 비교다. ^c2
- 노트 시점에 이 변경은 컴파일만 통과했고 링크는 LNK1104로 실패했으며 에디터 화면은 확인되지 않았다. ^c3
