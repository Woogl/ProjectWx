# DataTableRowHandle 다중 값 표시

## 계획

- `FDataTableRowHandle`의 `DataTable`과 `RowName` 자식 `IPropertyHandle`을 선택 위젯에 직접 연결한다.
- 다중 객체 선택에서 서로 다른 DataTable 또는 Row가 설정된 경우 엔진 기본 `Multiple Values` 표시를 사용한다.
- 수동 문자열 경로 및 RawData 기반 선택 처리를 제거하고 엔진 기본 트랜잭션·Undo/Redo·변경 알림을 사용한다.
- 값이 하나로 확정되지 않은 경우 프리뷰를 생성하지 않으며, 기존 접힘 상태와 Row 변경 새로고침을 유지한다.
- UE 5.8 `WxEditor` Development 타겟을 빌드해 컴파일을 검증한다.

## 완료

- `DataTable`과 `RowName` 자식 `IPropertyHandle`을 각각 에셋 선택기와 Row 콤보박스에 직접 연결했다.
- 다중 객체의 값이 다르면 엔진 기본 `Multiple Values` 상태를 표시하도록 했다.
- 수동 문자열 경로, RawData 직접 수정 및 별도 트랜잭션 처리를 제거하고 엔진 기본 속성 변경 흐름으로 통합했다.
- DataTable 변경 시 공통 Row가 새 테이블에 없으면 `RowName`을 초기화하는 엔진 기본 동작을 유지했다.
- 기존 `ShouldAutoExpand(false)`, 전체 Row 프리뷰 및 DataTable Row 변경 새로고침을 유지했다.
- UE 5.8 `WxEditor` Development 빌드 성공.
  - 로그: `C:\Wx\Saved\Logs\BuildDoctor\build_2026-09-04_233936_605_24400.log`
