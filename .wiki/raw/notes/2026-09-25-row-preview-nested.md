---
title: "Row 미리보기의 빈 하위 구조체 축약"
source: "MANUAL"
type: notes
ingested: 2026-09-25
tags: [wx, editor, datatable]
summary: "값이 있는 부모 구조체 안에서도 기본값과 같은 하위 JSON 객체를 각각 {}로 축약한다."
---

# Row 미리보기의 빈 하위 구조체 축약

사용자의 HGTest 화면에서는 `IgnoreTags`에 `Master.Doppelganger`가 있어 `ActivationRequirements` 전체 기본값 비교가 실패했고, 빈 `RequireTags`와 `TagQuery`가 그대로 출력됐다. 최초 구현은 부모 전체만 검사한 것이 원인이다.

[WxDataTableRowHandleCustomization.cpp](../../../Source/WxEditor/WxDataTableRowHandleCustomization.cpp)의 `MakeStructPreviewText`는 엔진의 현재 값·초기 기본값 JSON 표현을 읽고 `CollapseDefaultObjects`로 같은 이름의 하위 객체를 재귀 비교한다. 기본값과 같은 객체는 `{}`로 만들고 설정된 태그·배열·스칼라는 보존한다. 셀과 툴팁은 같은 결과를 쓴다. JSON 파싱 실패·고정 배열 프로퍼티는 엔진 원문을 유지한다. 컨테이너 내부 요소까지 재귀 축약하는 기능은 포함하지 않는다.

`Wx.Editor.RowPreview.NestedEmptyStructs` 회귀 테스트는 첨부 화면과 같은 혼합 값에서 빈 하위 객체 축약·설정된 태그 보존과 완전히 빈 부모의 축약을 검사한다. 빌드·실행 결과는 [작업 기록](../../../.agents/workflow/tasks/datatable-row-preview.md)에 남긴다.
