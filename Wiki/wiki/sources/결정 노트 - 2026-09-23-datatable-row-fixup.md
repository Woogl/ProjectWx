---
type: source
title: "결정 노트 - 2026-09-23-datatable-row-fixup"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "DataTable"
  - "에디터"
  - "플러그인"
summary: "DataTable 행 이름을 바꾸면 그 행을 가리키던 FDataTableRowHandle을 자동 갱신하는 범용 에디터 플러그인 DataTableRowFixup의 결정·계약·검증 기록."
source_type: decision-note
source_id: src-245cd48e09d2ff9ec0b3
sha256: 5f1fe579585183e16b3c259cd42c04eeada23d513ad170f721be6913b822b5b8
authority: primary
independence_key: ".wiki/raw/notes/2026-09-23-datatable-row-fixup.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-23-datatable-row-fixup.md"
raw_copy: ".raw/captured/5f1fe579585183e16b3c259cd42c04eeada23d513ad170f721be6913b822b5b8.md"
claim_ids:
  - clm-ec14f48fcf-c1
  - clm-ec14f48fcf-c2
  - clm-ec14f48fcf-c3
  - clm-ec14f48fcf-c4
key_claims:
  - "DataTableRowFixup 플러그인은 DataTable 행 이름 변경 시 FDataTableRowHandle 참조만 자동 갱신하고 FName 행 참조(NextRow 등)는 제외한다."
  - "DataTableRowFixup 플러그인은 에디터 개인 설정으로 켜고 끄며 기본값은 켜짐이다."
  - "DataTableRowFixup 플러그인 이름에서 Wx 접두사를 뺀 것은 AGENTS.md 코딩 규칙 1의 사용자 확정 예외다."
  - "사용자는 2026-09-23 에디터에서 DataTableRowFixup 동작을 확인했지만 확인한 구체 시나리오는 기록되지 않았다."
---

# 결정 노트 - 2026-09-23-datatable-row-fixup

- 원본: `.wiki/raw/notes/2026-09-23-datatable-row-fixup.md`
- 원자료 사본: `.raw/captured/5f1fe579585183e16b3c259cd42c04eeada23d513ad170f721be6913b822b5b8.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: 옛 LLM Wiki `.wiki/raw/notes/2026-09-23-datatable-row-fixup.md` (source: MANUAL, ingested: 2026-09-23, revision: `e165d2988`).
- 2026-09-23 커밋 `e165d2988` 기준의 사용자 결정과 구현·검증 기록이다. 작업 상태와 빌드 근거는 `.agents/workflow/tasks/datatable-row-rename-reference-update.md`에 있다고 적는다.
- 노트 날짜 기준 기록이라 현재 코드와 다를 수 있다.

## 엔진 사실(노트의 조사)

- UE 5.8 `FDataTableEditorUtils::RenameRow`는 테이블 키만 바꾸며 행 이름 리다이렉트 장치가 없다.
- `FDataTableRowHandle::PostSerialize`는 저장 시 (테이블, 행 이름)을 Asset Registry의 SearchableName 의존으로 남긴다. 에디터의 Find Row References도 이 기록을 쓴다.
- 프로젝트의 모든 사용처 유형(StateTree 태스크 InstancedStruct, BP CDO, 레벨 외부 액터, 몽타주 노티파이, GE 컴포넌트, DataAsset)에 이 기록이 있었다. BP 그래프 핀 리터럴(GetDataTableRow 노드 등)은 기록되지 않지만 현재 프로젝트에 그런 사용처는 없다.
- 엔진 에셋 이름 변경은 `FEditorFileUtils::IsMapPackageAsset`에 해당하는 패키지(외부 액터 패키지 포함)를 로드하지 않는다.

## 확정 결정(사용자 확정으로 기록됨)

노트의 결정 문장 원문:

> "이름 변경 시 참조를 자동으로 갱신한다. 에셋은 Dirty 상태로만 두고 저장은 사용자가 한다."
> "열려 있지 않은 레벨의 배치 액터는 건너뛰고 로그로 보고한다."
> "대화·자막 `NextRow`처럼 FName으로 행을 가리키는 필드는 제외한다. 대상은 `FDataTableRowHandle`뿐이다."
> "위험할 수 있어 별도 에디터 플러그인으로 분리하고, 에디터 개인 설정에서 켜고 끈다. 검토 후 기본값은 켜짐으로 했다. 꺼져 있으면 참조가 조용히 끊기는 편이 더 위험하기 때문이다."
> "프로젝트 범위를 넘는 범용 플러그인이라 이름을 `DataTableRowFixup`으로 하고, 플러그인 식별자에서 `Wx`를 뺀다."

- 이는 AGENTS.md 코딩 규칙 1(`Wx` 접두사)의 예외다. 설정 위치는 Editor Preferences > Wx > DataTable Row Fixup.

## 구현 관찰(확인한 계약)

- 감지: `INotifyOnDataTableChanged`의 RowList 변경 전후 행 맵을 비교해, 행 수가 같고 사라진 이름이 하나이며 새 이름이 같은 행 메모리를 가리킬 때만 이름 변경으로 본다. 재임포트·시트 전체 붙여넣기는 행을 새로 만들어 제외하고 경고만 남긴다.
- 대상: 레지스트리 SearchableName 참조처와 Dirty 패키지를 합친다(레지스트리는 디스크 기준).
- 갱신: 구조체·배열·셋·맵·InstancedStruct를 끝까지 따라가 핸들을 찾고 `PropertyAccessUtil::SetPropertyValue_Object`로 쓴다. 수정한 StateTree는 `UStateTreeEditingSubsystem::CompileStateTree`로 재컴파일한다.
- `RenameRow`의 트랜잭션 안에서 실행되어 Ctrl+Z 한 번에 함께 되돌아간다. 결과는 알림과 `LogDataTableRowFixup` 로그로 보고한다.

## 검증 범위

- 헤드리스 에디터 임시 자동화 테스트 통과(BP CDO 2개 `BP_Template`·`BP_Soldier`, 닫힌 레벨 외부 액터 건너뜀, 저장 전 연속 변경, Undo, StateTree 2개 에디터·컴파일 데이터, 몽타주 노티파이, 설정 끔). 테스트 코드는 삭제했다.
- 사람의 에디터 확인:

> 사용자 2026-09-23: "테스트도 완료했습니다. 잘 되네요"

- 사용자가 확인한 구체 시나리오는 명시되지 않았다.

## 미결정·충돌(남은 제약)

- `RenameRow`가 행 메모리를 옮기는 엔진 동작에 의존한다. 엔진이 바뀌면 갱신이 멈추고 경고만 남는다.
- 닫힌 레벨의 직접 지정 배치 액터, BP 그래프 리터럴, FName 행 참조, `FCurveTableRowHandle`은 갱신하지 않는다.
- 저장 직후 레지스트리 재스캔 전에 같은 행을 또 바꾸면 방금 저장한 에셋을 놓칠 수 있다. 로드된 인스턴스로의 전파는 트랜잭션에 기록되지 않는다.

## 관련 주제

- [[에디터 도구]]
- [[모듈 구조와 코드 정리]]
- [[작업 - datatable-row-rename-reference-update]]

## 핵심 주장

- DataTableRowFixup 플러그인은 DataTable 행 이름 변경 시 FDataTableRowHandle 참조만 자동 갱신하고 FName 행 참조(NextRow 등)는 제외한다. ^c1
- DataTableRowFixup 플러그인은 에디터 개인 설정으로 켜고 끄며 기본값은 켜짐이다. ^c2
- DataTableRowFixup 플러그인 이름에서 Wx 접두사를 뺀 것은 AGENTS.md 코딩 규칙 1의 사용자 확정 예외다. ^c3
- 사용자는 2026-09-23 에디터에서 DataTableRowFixup 동작을 확인했지만 확인한 구체 시나리오는 기록되지 않았다. ^c4
