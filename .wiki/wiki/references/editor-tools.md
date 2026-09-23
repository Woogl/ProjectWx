---
title: "편집기 도구 — WxEditor·WxToolset·DataTableRowFixup·BoxComponentVisualizer"
category: reference
sources:
  - "raw/notes/2026-09-22-current-editor.md"
  - "raw/notes/2026-09-22-current-foundation.md"
  - "raw/notes/2026-09-23-datatable-row-fixup.md"
created: 2026-09-22
updated: 2026-09-23
tags: [wx, editor, datatable]
aliases: ["WxEditor", "WxToolset", "DataTableRowFixup", "BoxComponentVisualizerEditor"]
confidence: medium
volatility: warm
verified: 2026-09-23
summary: "네 편집기 모듈은 속성 편집·썸네일·시각화·에셋 도구와 DataTable 행 참조 갱신을 제공하며 런타임 게임 기능과 구분된다."
---

# 편집기 도구 — WxEditor·WxToolset·DataTableRowFixup·BoxComponentVisualizer

네 편집기 모듈은 속성 편집·썸네일·시각화·에셋 도구와 DataTable 행 참조 갱신을 제공하며 런타임 게임 기능과 구분된다.

## 모듈별 책임

| 모듈 | 제공 기능 | 주요 연결점 |
|---|---|---|
| WxEditor | Actor Locator·DataTable Row Handle·StateTree 컴포넌트 이름 편집, Wx 속성 섹션, 아이템/UI 썸네일, 장치 연결 시각화 | PropertyEditor·ThumbnailManager·UnrealEd |
| WxToolset | Blueprint 변수 메타, AnimMontage 구조/노티파이, StateTree 파라미터·바인딩·컴파일 도구 | ToolsetRegistry의 AICallable 함수 |
| DataTableRowFixup | DataTable 행 이름 변경 시 그 행을 가리키던 `FDataTableRowHandle`을 새 이름으로 갱신 | DataTable 변경 리스너·Asset Registry·PropertyAccessUtil |
| BoxComponentVisualizerEditor | BoxComponent와 파생 컴포넌트용 시각화 등록 | UnrealEd ComponentVisualizer |

DataTableRowFixup은 프로젝트 범위를 넘는 범용 플러그인이라 이름과 식별자에 `Wx` 접두사를 쓰지 않는다(사용자 결정, 코딩 규칙 1의 예외). 프로젝트/플러그인 선언은 Editor와 PostEngineInit로 로드 범위를 지정한다. 게임 모듈의 런타임 의존성으로 옮길 때는 UnrealEd 등 편집기 전용 의존을 먼저 검토해야 한다.

## 등록과 정리

WxEditor는 엔진 Object Details를 감싸 원래 콜백·순서를 보관한다. Blueprint 썸네일 렌더러는 기존 등록을 해제하고 교체하며 종료 시 기본 렌더러를 복구한다. StateTree 시각화와 속성 커스터마이징도 종료 때 해제한다. 새 도구는 등록만 추가하지 말고 해제·기존 등록 복구까지 짝을 맞춘다.

DataTableRowFixup은 모듈 시작 시 DataTable 변경 리스너를 만들고 종료 시 해제한다. WxToolset은 모듈 시작 시 세 도구 클래스를 등록하고 종료 시 해제한다. 공개 계약상 Montage 구조 복사는 대상의 노티파이·블렌드 설정을 유지하며, 노티파이 복제는 이미 노티파이가 있는 섹션을 건너뛴다. StateTree 루트 파라미터 삭제 시 그 파라미터를 쓰던 바인딩은 별도 정리 대상이다. 도구 호출 가능성과 특정 에셋 변경의 성공·저장·컴파일 성공은 구분한다.

## DataTable 행 참조 갱신

엔진의 `FDataTableEditorUtils::RenameRow`는 테이블 키만 바꾸고 행 이름 리다이렉트도 없어, 행 이름을 바꾸면 그 행을 쓰던 에셋의 참조가 조용히 끊긴다. DataTableRowFixup은 이 참조를 새 이름으로 고친다. 켜고 끄는 것은 Editor Preferences > Wx > DataTable Row Fixup의 개인 설정이며 기본값은 켜짐이다.

- **감지:** RowList 변경 전후의 행 맵을 비교한다. 행 수가 같고, 사라진 이름 하나와 새 이름 하나가 같은 행 메모리를 가리킬 때만 이름 변경으로 본다. 재임포트·시트 전체 붙여넣기는 행을 새로 만들므로 제외하고 경고만 남긴다.
- **대상:** `FDataTableRowHandle`이 저장 때 남기는 SearchableName 기록(엔진 "Find Row References"와 같은 기록)에서 참조 에셋을 찾는다. 기록이 디스크 기준이라 저장하지 않은 변경은 Dirty 패키지에서 찾는다. 열려 있지 않은 레벨·외부 액터는 엔진 에셋 이름 변경처럼 로드하지 않고 보고만 한다.
- **갱신:** 값은 `PropertyAccessUtil::SetPropertyValue_Object`로 써서, 변경 통지와 값을 물려받은 로드된 인스턴스로의 전파를 엔진이 맡는다. 수정한 StateTree는 다시 컴파일한다. 에셋은 Dirty로만 두고 저장은 사용자가 하며, Ctrl+Z 한 번에 이름 변경과 함께 되돌아간다.
- **범위 밖:** 대화·자막 `NextRow` 같은 FName 행 참조, BP 그래프에 직접 입력한 행 이름, CurveTable 행, 열려 있지 않은 레벨에서 값을 직접 지정한 배치 액터는 고치지 않는다.

## 확인 범위와 진입점

이번에는 모듈 선언·등록 수명과 공개 헤더 계약을 확인했다. 모든 에셋 편집 함수의 내부 구현·실제 MCP 연결·Unreal Editor 화면 동작을 실행 검증하지 않았다. 예외로 DataTableRowFixup은 헤드리스 에디터 자동화로 실제 에셋의 갱신·되돌리기를 확인했고, 사용자가 에디터에서 동작을 확인했다(2026-09-23). 확인한 구체 시나리오는 원자료에 따른다.

- [WxEditor 등록](../../../Source/WxEditor/WxEditor.cpp)
- [WxToolset 공개 계약과 구현](../../../Plugins/WxToolset/Source/WxToolset/Private)
- [DataTableRowFixup 감지·갱신](../../../Plugins/DataTableRowFixup/Source/DataTableRowFixup/Private/DataTableRowReferenceUpdater.cpp)
- [BoxComponentVisualizer 등록](../../../Plugins/BoxComponentVisualizer/Source/BoxComponentVisualizerEditor/Private/BoxComponentVisualizerEditorModule.cpp)

## 관련 문서

- [[foundation|WxCore — 공용 계약과 설정]] ([WxCore — 공용 계약과 설정](../topics/foundation.md))
- [[wiki-operation|WX Wiki 운영과 재생성]] ([WX Wiki 운영과 재생성](../references/wiki-operation.md))
- [[wiki-workflow|Wiki·Workflow 도구 구조]] ([Wiki·Workflow 도구 구조](../references/wiki-workflow.md))
- [[world|WxWorld — 장치와 상호작용]] ([WxWorld — 장치와 상호작용](../topics/world.md))

## Sources

- [근거 1](../../raw/notes/2026-09-22-current-editor.md)
- [근거 2](../../raw/notes/2026-09-22-current-foundation.md)
- [근거 3](../../raw/notes/2026-09-23-datatable-row-fixup.md)

<details id="document-notes">
<summary>출처·검증 및 참고 정보</summary>

2026-09-22 현재 작업 트리 정적 조사·재편찬. 기준 HEAD `fe8c943f49401326e1007fedd78a937c9e66db47`에 미커밋 문서·도구 변경을 포함하며, 정확한 입력은 출처의 파일별 SHA-256과 발췌 범위로 식별한다. 문서의 `confidence: medium`은 제한된 정적 근거에 대한 표시다. `verified`는 순정 규칙에 따른 편찬일이다.

빌드·게임 실행·멀티플레이·BP/WBP·DataTable·BT/StateTree 바이너리 내부는 이번에 검증하지 않았다. 2026-09-23에 DataTableRowFixup 원자료(커밋 `e165d2988`)를 편찬해 추가했다. 기획·회의 보고·확정 판단·코드 관찰을 서로 대체하지 않는다.

</details>
