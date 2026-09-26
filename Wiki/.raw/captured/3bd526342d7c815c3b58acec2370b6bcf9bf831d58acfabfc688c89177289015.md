---
title: "Row 미리보기의 기본 구조체 축약"
source: "MANUAL"
type: notes
ingested: 2026-09-25
tags: [wx, editor, datatable]
summary: "Row 미리보기에서 초기 기본값과 같은 구조체를 {}로 표시하고 원문은 툴팁에 유지한다."
---

# Row 미리보기의 기본 구조체 축약

사용자 요청: 데이터가 없는 구조체의 복잡한 Row 미리보기를 `{}` 정도로 줄인다.

구현 근거: [WxDataTableRowHandleCustomization.cpp](../../../Source/WxEditor/WxDataTableRowHandleCustomization.cpp)의 `CustomizeChildren`. 2026-09-25 작업 트리 SHA-256: `25EDFEC1D1AFC46FBCDFC282882162C8378497FA027BE5BED08311416A2116E3`.

- `Column->Property`가 `FStructProperty`인 경우 `FStructOnScope`로 만든 초기 기본값과 `Identical`로 비교한다. 고정 배열은 모든 요소가 같아야 축약한다.
- 초기 기본값과 같으면 셀 텍스트만 `{}`로 바꾸고 엔진이 만든 원문은 툴팁에 남긴다. 기본값과 다른 구조체·일반 프로퍼티의 표시는 유지한다.
- 모든 바이트가 0인지의 판정이 아니다. 생성자에서 설정한 기본값도 축약 대상이다. 실제 Row 데이터는 수정하지 않는다.
- C++ 컴파일 통과. 실행 중인 에디터의 DLL 점유로 링크 실패(LNK1104). 에디터 화면은 미검증이다.
