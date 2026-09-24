# DataTable Row 미리보기

- 요청: 데이터 없는 구조체를 `{}`로 간략히 표시.
- 구현: `CustomizeChildren`에서 구조체의 초기 기본값과 비교해 축약. 사용자 후속 요청에 따라 셀과 툴팁 모두 같은 텍스트(`{}` 포함)를 표시. 실제 데이터 변경 없음.
- 검증: 최초 수정 C++ 컴파일 성공. 실행 중인 에디터의 DLL 점유로 LNK1104 링크 실패. 후속 툴팁 변경은 같은 `CellText` 연결을 정적으로 확인하고 `git diff --check`로 검증; 재빌드 미실행.
- 로그: `Saved/Logs/BuildDoctor/build_2026-09-25_014435_089_12848.log`.
- 남은 확인: 에디터 종료 후 재빌드 및 화면 확인. 사용자 화면 수용 미확인.
- 지식: `.wiki/wiki/references/editor-tools.md`에 표시 기준 반영.

## 빈 하위 구조체 미처리 후속 수정

- 사용자 HGTest 화면의 `IgnoreTags`가 설정돼 있어 부모 전체 비교만으로는 빈 하위 구조체를 축약하지 못했다.
- 현재 값과 초기 기본값의 엔진 JSON 표현을 재귀 비교해 하위 객체도 축약한다. 설정된 태그·컨테이너 값은 유지한다. 셀·툴팁은 동일하다.
- Editor Development 빌드 성공: `Saved/Logs/BuildDoctor/build_2026-09-25_021809_315_46344.log`.
- 회귀 테스트 `Wx.Editor.RowPreview.NestedEmptyStructs` 성공, 프로세스 종료 코드 0. 빈 하위 객체 축약·설정 태그 보존·완전히 빈 부모 축약 확인. 로그: `Saved/Logs/RowPreviewAutomation.log`. `git diff --check` 통과.
- 실제 Slate 화면은 아직 확인하지 않았다.

- 제출 전 사용자 요청에 따라 이번 세션의 회귀 테스트 코드와 AutomationTest include를 제거했다. 위 테스트 성공은 제거 전 검증 이력이며 제출 코드에는 테스트를 포함하지 않는다.
