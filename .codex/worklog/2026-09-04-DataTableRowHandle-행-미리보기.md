# DataTableRowHandle 행 미리보기

## 계획

- 에디터에 노출된 프로젝트 `FDataTableRowHandle` 프로퍼티를 명시적인 Wx 메타데이터로 식별하고, 그 필드에만 전체 행 미리보기 커스터마이징을 적용한다.
- 테이블과 행 선택기는 헤더에 유지하고, 필드를 펼치면 선택한 행의 모든 데이터 테이블 칼럼을 `칼럼명: 값` 형태로 읽기 전용 표시한다.
- 데이터 테이블 에디터와 동일하게 Deprecated 및 `HideFromDataTableEditorColumn` 칼럼은 미리보기에서 제외한다.
- 선택한 행 하나만 데이터 테이블 편집용 텍스트로 변환하며, 자식 `PropertyHandle`이나 중첩 디테일 뷰를 만들지 않아 과거 전역 커스터마이징의 재귀 진입을 피한다.
- 기존 `RowType` 테이블 필터, Undo/Redo, 행 참조 검색을 보존하고, 메타데이터가 없는 `FDataTableRowHandle`은 엔진 기본 UI를 유지한다.
- 단일 칼럼용 `RowPreviewProperty`는 사용하지 않는다. 행 데이터 편집, 에셋 스키마 변경, 엔진 소스 수정은 범위에서 제외한다.
- UE 5.8 `WxEditor` Development 타겟을 빌드해 컴파일을 검증한다.

## 완료

- `WxPreviewRow` 메타데이터가 붙은 `FDataTableRowHandle`만 가로채는 조건부 프로퍼티 커스터마이징을 `WxEditor`에 등록했다.
- 테이블·행 선택기는 루트 핸들의 Raw Data를 통해 읽고 쓰도록 구성해 자식 `PropertyHandle` 생성으로 인한 재귀 경로를 제거했다.
- 선택한 행 하나를 `FDataTableEditorUtils::CacheDataForEditing`으로 변환해, 데이터 테이블 에디터와 동일한 칼럼명과 표시 값 전체를 펼친 읽기 전용 자식 행으로 출력한다.
- 현재 에디터에 노출된 프로젝트 행 핸들 12곳에 `WxPreviewRow`를 지정했고, 단일 칼럼용 `RowPreviewProperty`는 남기지 않았다.
- 테이블 변경 시 유효하지 않은 행 이름 정리, `RowType` 기반 테이블 필터, 다중 선택, Undo/Redo 변경 통지, 행 참조 검색을 유지했다.
- `git diff --check`와 메타데이터 적용 누락 검사를 통과했다.
- UE 5.8.2 `WxEditor Win64 Development` 빌드가 성공했다. 로그: `C:\Wx\Saved\Logs\BuildDoctor\build_2026-09-04_220343_379_25528.log`.
