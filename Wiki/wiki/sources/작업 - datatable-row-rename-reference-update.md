---
type: source
title: "작업 - datatable-row-rename-reference-update"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "작업-기록"
  - "DataTable"
  - "에디터"
  - "플러그인"
summary: "DataTable 행 이름 변경 시 FDataTableRowHandle 참조를 자동 갱신하는 에디터 플러그인 DataTableRowFixup을 추가한 작업으로 2026-09-23 완료됐다."
source_type: task-record
source_id: src-77f121f89f4d09dc254b
sha256: 3a04a2bc1f49a84c8439bec3b090e0fe82da87dd46f6a29a9079ae0911c08704
authority: primary
independence_key: ".agents/workflow/tasks/datatable-row-rename-reference-update.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".agents/workflow/tasks/datatable-row-rename-reference-update.md"
raw_copy: ".raw/captured/3a04a2bc1f49a84c8439bec3b090e0fe82da87dd46f6a29a9079ae0911c08704.md"
claim_ids:
  - clm-ba50e69b72-c1
  - clm-ba50e69b72-c2
  - clm-ba50e69b72-c3
  - clm-ba50e69b72-c4
key_claims:
  - "datatable-row-rename-reference-update 작업은 행 이름 변경 시 FDataTableRowHandle 참조를 갱신하는 에디터 전용 플러그인 DataTableRowFixup을 추가했다."
  - "DataTableRowFixup은 갱신한 에셋을 Dirty로만 두고 저장은 사용자에게 맡기며, 열려 있지 않은 레벨의 배치 액터는 건너뛰고 로그로 보고한다."
  - "DataTableRowFixup의 설정 bUpdateReferencesOnRowRename은 사용자 결정으로 기본값이 켜짐이며 Editor Preferences의 Wx 카테고리에 있다."
  - "DataTableRowFixup은 엔진에 행 이름 변경 이벤트가 없어 RenameRow가 행 메모리를 그대로 옮기는 동작에 의존한다."
---

# 작업 - datatable-row-rename-reference-update

- 원본: `.agents/workflow/tasks/datatable-row-rename-reference-update.md`
- 원자료 사본: `.raw/captured/3a04a2bc1f49a84c8439bec3b090e0fe82da87dd46f6a29a9079ae0911c08704.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

DataTable에서 RowName을 바꾸면 그 행을 쓰던 에셋의 `FDataTableRowHandle`도 새 이름을 가리키도록 갱신하는 에디터 전용 플러그인 `DataTableRowFixup`을 만든 작업 기록이다. 상태는 완료(2026-09-23)이며 커밋 `e165d2988`로 `origin/main`에 푸시됐다.

## 요청과 조사

- 요청: RowName 변경을 그 행을 쓰던 에셋에도 반영하기를 원했다(기록은 요약으로 적었다).
- 조사(구현 관찰): UE 5.8 `FDataTableEditorUtils::RenameRow`는 테이블 키만 바꾸고 행 이름 리다이렉트 장치도 없다. `FDataTableRowHandle::PostSerialize`는 저장할 때 (테이블, 행 이름)을 SearchableName으로 남기고, 엔진 Find Row References도 이 기록을 쓴다.
- 실측: 패키지 헤더 SearchableNames를 파싱해 StateTree 태스크 InstancedStruct(`ST_Quest_Main1`), BP CDO(`BP_Template`), 레벨 외부 액터(`LV_DevCombat`), 몽타주 노티파이(`AM_Template_Attack_L`), GE 컴포넌트(`GE_Shared_GuardReduction`), DataAsset(`ABS_Template`) 모두 기록이 있음을 확인했다. BP 그래프 핀 리터럴은 기록되지 않지만 프로젝트 사용처는 0건이다.

## 확정 결정

- 사용자: 자동으로 갱신하되 에셋은 Dirty로만 두고 저장은 사용자가 한다.
- 사용자: 열려 있지 않은 레벨의 배치 액터는 건너뛰고 로그로 보고한다(`FEditorFileUtils::IsMapPackageAsset`이면 로드하지 않음).
- 사용자: 대화·자막 `NextRow`(FName)는 제외하고 `FDataTableRowHandle` 참조만 갱신한다.
- 사용자: 위험할 수 있으므로 별도 에디터 플러그인으로 분리하고 Editor Preferences에서 켜고 끈다. 이름은 범용 플러그인이라 `Wx`를 뺀 `DataTableRowFixup`.
- AI: 같은 이유로 플러그인 안 클래스·모듈·로그 이름에서도 `Wx`를 뺀다(AGENTS.md 코딩 규칙 1의 예외, 저작권 머리말은 유지).
- 기본값: AI가 처음 꺼짐으로 정했으나 사용자가 켜짐으로 바꿨다. 꺼져 있으면 참조가 조용히 끊기는 편이 더 위험하다는 검토 뒤의 결정이다. 설정 카테고리는 `Wx`.

## 설계와 구현

- 감지: `FDataTableEditorUtils::INotifyOnDataTableChanged` 리스너. 행 수가 같고 사라진 이름이 하나이며, 새 이름이 옛 이름과 같은 행 메모리를 가리키면 이름 변경으로 판정한다. 재임포트·전체 붙여넣기는 제외하고 경고 로그를 남긴다.
- 대상: Asset Registry `GetReferencers(FAssetIdentifier(테이블, 옛 이름), SearchableName)`와 Dirty 패키지를 합친다. 레벨·외부 액터는 로드하지 않는다.
- 갱신: 구조체·배열·셋·맵·InstancedStruct를 따라가며 핸들을 찾아 `PropertyAccessUtil::SetPropertyValue_Object`로 설정한다. DataTable 행 안의 핸들은 RowData 변경 알림 사이에서 고친다. 수정한 StateTree는 `UStateTreeEditingSubsystem::CompileStateTree`로 재컴파일한다.
- 되돌리기: `RenameRow` 트랜잭션 안에서 실행되어 Ctrl+Z 한 번에 함께 되돌아간다. 보고는 알림 1개와 `LogDataTableRowFixup` 로그.
- 구현: `Plugins/DataTableRowFixup`(Editor 전용, PostEngineInit). `DataTableRowFixupModule`, `DataTableRowFixupSettings`(`UDeveloperSettings`, `Config = EditorPerProjectUserSettings`, `bUpdateReferencesOnRowRename` 기본 true), `DataTableRowReferenceUpdater`. `Wx.uproject`에 활성화 항목을 더했고 WxEditor는 바꾸지 않았다.

## 검증 범위

- AI 빌드: WxEditor Win64 Development 성공(`Saved/Logs/BuildDoctor/build_2026-09-23_133144_700_45400.log`). 첫 빌드의 `FStateTreeCompilerLog` 링크 오류는 `PropertyBindingUtils` 의존성으로 해결했다.
- AI 임시 자동화 테스트 `Wx.DataTableRowRename.Verify`(헤드리스, 삭제함, 이름 변경 전 식별자로 실행): CDO 갱신과 Dirty, 레벨 외부 액터 건너뜀, 저장 전 재변경, Undo, StateTree 재컴파일 결과, 몽타주 노티파이 스테이트 갱신, 설정 꺼짐 시 무동작을 확인했다.
- 사람 확인:

> 사용자 2026-09-23: "네. 테스트도 완료했습니다. 잘 되네요"

  확인한 구체 시나리오가 명시되지 않아, 기록은 실제 에디터 UI 동작·Editor Preferences 표시·열린 레벨 배치 액터 갱신 같은 미확인 항목을 모두 확인한 것으로 넓히지 않는다.
- Wiki 링크 검사의 1 error는 다른 세션이 삭제한 파일을 가리키는 범위 밖 오류였다.

## 미결정·제약

- 엔진에 이름 변경 이벤트가 없어 `RenameRow`가 행 메모리를 옮기는 동작에 의존한다. 엔진이 복사 방식으로 바뀌면 갱신은 멈추고 경고만 남는다.
- 열려 있지 않은 레벨의 값 지정 배치 액터, BP 그래프 핀 리터럴은 갱신하지 않는다.
- 저장 직후 1~2초 안에 같은 행을 다시 바꾸면 레지스트리 재스캔 전이라 방금 저장한 에셋을 놓칠 수 있다.
- 로드된 인스턴스로의 전파는 트랜잭션에 기록되지 않고, 참조 에셋을 동기로 로드한다.

## 관련 주제

- [[에디터 도구]]
- [[모듈 구조와 코드 정리]]
- [[결정 노트 - 2026-09-23-datatable-row-fixup]]

## 핵심 주장

- datatable-row-rename-reference-update 작업은 행 이름 변경 시 FDataTableRowHandle 참조를 갱신하는 에디터 전용 플러그인 DataTableRowFixup을 추가했다. ^c1
- DataTableRowFixup은 갱신한 에셋을 Dirty로만 두고 저장은 사용자에게 맡기며, 열려 있지 않은 레벨의 배치 액터는 건너뛰고 로그로 보고한다. ^c2
- DataTableRowFixup의 설정 bUpdateReferencesOnRowRename은 사용자 결정으로 기본값이 켜짐이며 Editor Preferences의 Wx 카테고리에 있다. ^c3
- DataTableRowFixup은 엔진에 행 이름 변경 이벤트가 없어 RenameRow가 행 메모리를 그대로 옮기는 동작에 의존한다. ^c4
