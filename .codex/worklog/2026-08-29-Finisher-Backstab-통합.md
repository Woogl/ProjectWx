# Finisher Backstab 통합

## 계획

- `UWxAbility_Backstab`의 중복 공격자 어빌리티 구현을 `UWxAbility_Finisher`로 통합한다.
- 기존 앞잡·뒤잡 에셋은 모두 `UWxAbility_Finisher`를 사용하게 전환하고, 두 경우 모두 종료 시 대상의 GP를 초기화한다.
- Backstab 전용 클래스·분기·게임플레이 태그 의존성을 제거하고, 기존 몽타주와 피해 데이터는 유지한다.
- WxEditor(Development) 타깃을 빌드해 컴파일을 검증한다.

## 완료

- `UWxAbility_Backstab`과 `GA_Backstab`을 제거했다.
- `UWxAbility_Finisher`가 대상의 그로기 태그로 앞잡/뒤잡을 구분해 각 몽타주 쌍을 재생하도록 통합했다.
- 뒤잡 몽타주와 대미지 행을 `GA_Finisher`의 전용 슬롯으로 옮기고, `ABS_Player`의 Backstab 어빌리티 부여를 제거했다.
- 앞잡·뒤잡 모두 종료 시 GP 초기화 효과를 적용하도록 유지했다.
- `WxEditor Win64 Development` 빌드 성공으로 컴파일을 검증했다.
