---
title: "DataTable 행 이름 변경 시 행 핸들 참조 갱신(DataTableRowFixup)"
source: "MANUAL"
type: notes
ingested: 2026-09-23
tags: [wx, editor, datatable]
summary: "행 이름을 바꾸면 그 행을 가리키던 FDataTableRowHandle을 새 이름으로 고치는 범용 에디터 플러그인의 결정·계약·검증 기록. 사용자가 에디터에서 동작을 확인했다."
revision: e165d2988
---

# DataTable 행 이름 변경 시 행 핸들 참조 갱신(DataTableRowFixup)

2026-09-23 커밋 `e165d2988` 기준의 사용자 결정과 구현·검증 기록이다. 작업 상태와 빌드 근거는 [Workflow Task](../../../.agents/workflow/tasks/datatable-row-rename-reference-update.md)에 있다.

## 엔진 사실

- UE 5.8 `FDataTableEditorUtils::RenameRow`는 테이블 키만 바꾼다. 행 이름 리다이렉트 장치는 없다.
- `FDataTableRowHandle::PostSerialize`는 저장할 때 (테이블, 행 이름)을 Asset Registry의 SearchableName 의존으로 남긴다. 데이터 테이블 에디터의 "Find Row References"도 이 기록을 쓴다.
- 패키지 헤더의 SearchableNames 절을 파싱해 보니, 프로젝트의 모든 사용처 유형에 이 기록이 있었다.
  - StateTree 태스크 InstancedStruct
  - BP CDO
  - 레벨 외부 액터
  - 몽타주 노티파이
  - GE 컴포넌트
  - DataAsset
- BP 그래프 핀 리터럴(GetDataTableRow 노드 등)은 기록되지 않는다. 현재 프로젝트에는 이런 사용처가 없다.
- 엔진 에셋 이름 변경은 `FEditorFileUtils::IsMapPackageAsset`(외부 액터 패키지 포함)에 해당하는 패키지를 로드하지 않는다.

## 결정 (사용자 확정)

- 이름 변경 시 참조를 자동으로 갱신한다. 에셋은 Dirty 상태로만 두고 저장은 사용자가 한다.
- 열려 있지 않은 레벨의 배치 액터는 건너뛰고 로그로 보고한다.
- 대화·자막 `NextRow`처럼 FName으로 행을 가리키는 필드는 제외한다. 대상은 `FDataTableRowHandle`뿐이다.
- 위험할 수 있어 별도 에디터 플러그인으로 분리하고, 에디터 개인 설정에서 켜고 끈다. 검토 후 기본값은 켜짐으로 했다. 꺼져 있으면 참조가 조용히 끊기는 편이 더 위험하기 때문이다.
- 프로젝트 범위를 넘는 범용 플러그인이라 이름을 `DataTableRowFixup`으로 하고, 플러그인 식별자에서 `Wx`를 뺀다. AGENTS.md 코딩 규칙 1(`Wx` 접두사)의 예외다. 설정 카테고리는 `Wx`다(Editor Preferences > Wx > DataTable Row Fixup).

## 확인한 계약

- 감지: `FDataTableEditorUtils::INotifyOnDataTableChanged`의 RowList 변경 전후 행 맵을 비교한다. 두 조건을 모두 만족해야 이름 변경으로 본다.
  - 행 수가 같고 사라진 이름이 하나다.
  - 새 이름이 같은 행 메모리를 가리킨다(`RenameRow`는 행 메모리를 그대로 옮긴다).
  - 재임포트·시트 전체 붙여넣기는 행을 새로 만들므로 제외하고 경고만 남긴다.
- 대상: 레지스트리 SearchableName 참조처와 Dirty 패키지를 합친다. 레지스트리는 디스크 기준이라 저장 전 변경은 Dirty 패키지에서 찾는다.
- 갱신: 구조체·배열·셋·맵·InstancedStruct를 끝까지 따라가 핸들을 찾는다. 값은 `PropertyAccessUtil::SetPropertyValue_Object`로 쓴다. 파이썬 `set_editor_property`와 같은 경로라 Modify, 변경 통지, 로드된 아키타입 인스턴스로의 전파를 엔진이 맡는다. DataTable 행 안의 핸들은 RowData 변경 알림 사이에서 직접 고친다.
- 수정한 StateTree는 `UStateTreeEditingSubsystem::CompileStateTree`로 다시 컴파일한다. 에디터 데이터만 바꾸면 컴파일 결과에 옛 값이 남는다.
- `RenameRow`의 트랜잭션 안에서 실행되므로 Ctrl+Z 한 번에 이름 변경과 참조 갱신이 함께 되돌아간다.
- 결과는 알림과 `LogDataTableRowFixup` 로그로 보고한다.

## 검증

- 헤드리스 에디터 임시 자동화 테스트가 통과했다. 테스트 코드는 삭제했다.
  - BP CDO 2개(`BP_Template`·`BP_Soldier`) 갱신
  - 열려 있지 않은 레벨 외부 액터 건너뜀
  - 저장 전 연속 변경
  - Undo
  - StateTree 2개의 에디터·컴파일 데이터 갱신
  - 몽타주 노티파이 갱신
  - 설정을 끄면 갱신하지 않음
- 사용자가 에디터에서 동작을 확인하고 테스트를 수용했다(2026-09-23, "테스트도 완료했습니다. 잘 되네요"). 확인한 구체 시나리오는 명시되지 않았다.

## 남은 제약

- `RenameRow`가 행 메모리를 옮기는 엔진 동작에 의존한다. 엔진이 바뀌면 갱신이 멈추고 경고만 남는다.
- 열려 있지 않은 레벨에서 값을 직접 지정한 배치 액터, BP 그래프 리터럴, FName 행 참조, CurveTable 행(`FCurveTableRowHandle`)은 갱신하지 않는다.
- 저장 직후 레지스트리를 다시 스캔하기 전에 같은 행을 또 바꾸면 방금 저장한 에셋을 놓칠 수 있다.
- 로드된 인스턴스로의 전파는 트랜잭션에 기록되지 않는다. 엔진 디테일 패널과 같은 동작이다.

## 코드 근거

| 파일 | 확인 범위 | SHA-256 |
|---|---|---|
| [DataTableRowFixup.uplugin](../../../Plugins/DataTableRowFixup/DataTableRowFixup.uplugin) | Editor 전용·PostEngineInit | `b4aab1ca07f47afddfde48d819e7233cf89041913e68b66baae8bea75541322b` |
| [DataTableRowFixup.Build.cs](../../../Plugins/DataTableRowFixup/Source/DataTableRowFixup/DataTableRowFixup.Build.cs) | 의존 모듈 | `743eeb335f2e438944128e6c23dcb21c5bd99c0ca4d37b47290db7e3f74738a6` |
| [DataTableRowFixupSettings.h](../../../Plugins/DataTableRowFixup/Source/DataTableRowFixup/Private/DataTableRowFixupSettings.h) | 개인 설정·기본값 | `72a732babdfbf9621dca61c284b3a4225a1d67f3bdd95e7f4db8bbc083cff07e` |
| [DataTableRowFixupSettings.cpp](../../../Plugins/DataTableRowFixup/Source/DataTableRowFixup/Private/DataTableRowFixupSettings.cpp) | 카테고리 | `97d7a9e95d4cee3a2895f7481a03b8f04f5ebbd08b39e7e118f54ec28adc8386` |
| [DataTableRowReferenceUpdater.h](../../../Plugins/DataTableRowFixup/Source/DataTableRowFixup/Private/DataTableRowReferenceUpdater.h) | 리스너 계약 | `44c37ac0336951067a98cc5e5db7f794088d339f3232b903e62188a79e5ad2b7` |
| [DataTableRowReferenceUpdater.cpp](../../../Plugins/DataTableRowFixup/Source/DataTableRowFixup/Private/DataTableRowReferenceUpdater.cpp) | 감지·대상 수집·갱신·StateTree 컴파일·보고 | `23aa6daa38f09cc30c44c160cfd738d70c3dd9dc522bab561bff1421a124726a` |
| [DataTableRowFixupModule.cpp](../../../Plugins/DataTableRowFixup/Source/DataTableRowFixup/Private/DataTableRowFixupModule.cpp) | 리스너 수명 | `fb62b1bf3d34fbc852552b54038a782c9dce52b85693d4a5d1eb9691d80cfebb` |
