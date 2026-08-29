# DataTableRowHandle 커스터마이징 제거

## 계획

- 전역 `DataTableRowHandle` 프로퍼티 커스터마이징 등록을 제거하고 엔진 기본 편집 UI로 복귀한다.
- 재귀 진입의 원인이 된 `WxDataTableRowHandleCustomization` 구현 파일을 제거한다.
- `WxEditor` Development와 DebugGame 타겟을 빌드해 에디터 모듈 컴파일을 확인한다.

## 완료

- `WxEditor` 모듈에서 `DataTableRowHandle` 전역 커스터마이징 등록과 해제를 제거했다.
- `WxDataTableRowHandleCustomization` 헤더·구현 파일을 제거해 재귀적인 프로퍼티 편집기 진입 경로를 없앴다.
- `WxEditor Win64 Development`, `WxEditor Win64 DebugGame` 빌드가 모두 성공했다.
