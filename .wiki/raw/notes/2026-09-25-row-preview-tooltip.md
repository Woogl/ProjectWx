---
title: "Row 미리보기와 툴팁 표시 통일"
source: "MANUAL"
type: notes
ingested: 2026-09-25
tags: [wx, editor, datatable]
summary: "사용자 후속 요청으로 축약된 셀과 툴팁에 같은 텍스트를 표시한다."
---

# Row 미리보기와 툴팁 표시 통일

사용자가 셀과 툴팁을 둘 다 똑같이 표시하도록 요청했다. 앞선 원문 툴팁 유지 정책을 대체한다.

[CustomizeChildren](../../../Source/WxEditor/WxDataTableRowHandleCustomization.cpp)에서 `STextBlock.Text`와 `ToolTipText`에 동일한 `CellText`를 전달한다. 기본 구조체는 둘 다 `{}`로 표시한다. 별도의 `FullCellText` 변수는 제거했다. 정적 확인이며 후속 변경의 빌드·화면 검증은 수행하지 않았다.
