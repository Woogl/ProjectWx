# Ability 버튼 Enhanced Input 연결

## 계획

- 승인된 UI 버튼 눌림/해제 입력을 Enhanced Input 액션에 주입한다. Light의 Pressed와 Heavy의 Hold 트리거를 재사용한다.
- WxUIData 확장 없이 WxUI 버튼에 액션을 설정하고, 해제·숨김·UI 종료·커서 모드 종료 시 주입을 정리한다.
- HUD의 히트 테스트와 활성화 바인딩을 클릭 가능한 구성으로 수정한다. 겹친 위젯은 최상단이 클릭을 받는 규칙을 유지한다.
- WxEditor Development 빌드 및 입력 수명/에셋 설정 검증을 수행한다.

## 완료

- 입력 주입 코드는 빌드 및 실제 Heavy 액션의 Hold/해제 테스트를 통과했다.
- 사용자가 공용 버튼의 복잡도를 지적하고 엔진 기본 Hold 방식으로 전환을 승인했다. 최종 적용 및 검증은 `2026-09-08-Ability-CommonUI-기본-Hold.md`에서 계속한다.
