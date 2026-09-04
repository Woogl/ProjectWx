# DataTableRowHandle 프리뷰 기본 접힘

## 계획

- `WxPreviewRow`가 적용된 `FDataTableRowHandle` 헤더에 UE 5.8 공식 API인 `ShouldAutoExpand(false)`를 지정해 미리보기가 기본적으로 접힌 상태로 표시되게 한다.
- 사용자가 화살표를 눌러 전체 Row 미리보기를 펼치는 동작은 유지한다.
- DataTable·Row 선택기, 전체 칼럼 프리뷰, 재귀 방지 구조는 변경하지 않는다.
- UE 5.8 `WxEditor` Development 타겟을 빌드해 컴파일을 검증한다.

## 완료

- `WxPreviewRow` 커스터마이징의 헤더 행에 `ShouldAutoExpand(false)`를 지정해 전체 Row 미리보기가 기본적으로 접힌 상태로 시작하게 했다.
- 사용자가 직접 펼치는 동작과 DataTable·Row 선택기, 전체 칼럼 프리뷰, Raw Data 기반 재귀 방지 구조는 그대로 유지했다.
- 강제 펼침 경로가 없음을 정적으로 확인했다.
- UE 5.8.2 `WxEditor Win64 Development` 빌드가 성공했다. 로그: `C:\Wx\Saved\Logs\BuildDoctor\build_2026-09-04_223927_424_24600.log`.
