# DataTableRowHandle Row 변경 새로고침

## 계획

- 현재 선택된 공통 DataTable의 `UDataTable::OnDataTableChanged()`를 구독한다.
- Row 값·추가·삭제 등 테이블 변경 시 전체 Row 미리보기를 강제 새로고침한다.
- 다른 DataTable 선택 또는 커스터마이징 종료 시 기존 구독을 해제한다.
- `ShouldAutoExpand(false)`와 기존 DataTable·Row 선택기 및 프리뷰 동작은 유지한다.
- UE 5.8 `WxEditor` Development 타겟을 빌드해 컴파일을 검증한다.

## 완료

- 현재 선택된 공통 DataTable의 `OnDataTableChanged()`에 커스터마이징 인스턴스를 구독시켰다.
- DataTable Row 변경 시 디테일 패널을 새로고침해 선택된 Row의 모든 칼럼 값을 다시 생성하도록 했다.
- DataTable 선택 변경 및 커스터마이징 종료 시 기존 델리게이트 구독을 해제하도록 수명 관리를 추가했다.
- `ShouldAutoExpand(false)`와 기존 선택기·미리보기 UI는 변경하지 않았다.
- UE 5.8 `WxEditor` Development 빌드 성공.
  - 로그: `C:\Wx\Saved\Logs\BuildDoctor\build_2026-09-04_230402_585_2616.log`
