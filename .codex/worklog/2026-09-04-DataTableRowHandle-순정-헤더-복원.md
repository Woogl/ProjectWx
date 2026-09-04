# DataTableRowHandle 순정 헤더 복원

## 계획

- DataTable 썸네일을 제거해 엔진 순정 `FDataTableRowHandle`처럼 한 줄짜리 에셋 선택기로 표시한다.
- 순정의 복사·붙여넣기, 기본값 초기화, 참조 검색 동작을 유지한다.
- `RowType` 필터와 Row 선택 UI, 펼쳤을 때 나타나는 전체 Row 읽기 전용 프리뷰를 유지한다.
- 과거 재귀 문제를 피하는 루트 핸들의 Raw Data 접근 방식은 변경하지 않는다.
- UE 5.8 `WxEditor` Development 타겟을 빌드해 컴파일을 검증한다.

## 완료

- DataTable 에셋 선택기에서 썸네일 표시를 명시적으로 끄고, 엔진 순정처럼 한 줄짜리 에셋 필드와 Row 콤보만 보이도록 복원했다.
- 루트 프로퍼티 핸들의 순정 복사·붙여넣기 및 기본값 초기화 동작을 헤더에 연결했다.
- 전체 Row 프리뷰, `RowType` 필터, 참조 검색, Undo/Redo와 Raw Data 기반 재귀 방지 경로는 그대로 유지했다.
- 정적 검사에서 `ThumbnailPool` 및 자식 `PropertyHandle` 경로가 없고, 전체 Row 캐시와 Raw Data 접근이 유지됨을 확인했다.
- UE 5.8.2 `WxEditor Win64 Development` 빌드가 성공했다. 로그: `C:\Wx\Saved\Logs\BuildDoctor\build_2026-09-04_222513_770_10640.log`.
