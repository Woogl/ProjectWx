# AttackButton 제거

## 계획

- 사용자의 명시적 제거 요청에 따라 UWxAttackButton과 이를 위해 추가한 HUD 입력 바인딩·전용 입력 테스트를 제거한다. 대체 클래스나 신규 WBP는 만들지 않는다.
- 기존 AttackVariants를 원래 부모에 직접 배치하고 MVVM 표시 바인딩은 유지한다. 에셋 참조를 먼저 제거한 다음 C++ 클래스를 삭제한다.
- WxEditor Development 빌드 및 남은 MVVM 회귀 테스트로 검증한다.

## 완료

- UWxAttackButton 헤더·구현, 전용 입력 테스트와 테스트 어빌리티, 테스트 전용 CommonUI/CommonInput 에디터 의존성을 제거했다. 대체 클래스와 신규 WBP는 추가하지 않았다.
- WBP_PlayerSkills의 AttackSlot을 제거하고 원래 AttackVariants를 부모에 직접 배치했다. 해당 입력 바인딩 3개와 위젯 GUID를 제거했다. 기존 아이콘·MVVM 표시 바인딩은 유지했다. 에셋 처리용 임시 도구도 제거했다.
- WxEditor Development 빌드 성공: Saved/Logs/BuildDoctor/build_2026-09-08_015527_754_32944.log.
- SeparateWidgets / SlotVisibility 2종 자동화 성공: Saved/Logs/test_removed_attack_button.log. 삭제된 클래스 없이 실제 HUD 에셋 로딩 및 표시 바인딩을 검증했다.
- 이 클래스가 제공하던 HUD의 Light/Air/Hold 입력 경로는 제거된 상태다. 기존 게임 입력 코드는 변경하지 않았다.
