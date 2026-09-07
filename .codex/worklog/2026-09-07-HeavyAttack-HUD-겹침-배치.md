# HeavyAttack HUD 겹침 배치

## 계획

- 승인된 지상·공중 평타 Overlay에 HeavyAttack 위젯과 별도 Resolver(Ability.Attack.Heavy)를 추가한다.
- HeavyAttack은 기존 발동 조건에 맞춰 지상에서 표시하고 공중에서는 Collapse한다. 지상에서는 LightAttack과 HeavyAttack을 함께 겹쳐 표시한다.
- 공격 위젯 3개의 표시 Visibility를 HitTestInvisible로 설정하여 자식 위젯까지 마우스 입력 대상에서 제외한다.
- 기존 GameplayTag MVVM Conversion Library와 OwnedTags 바인딩을 재사용한다.
- UE 5.8 빌드와 에셋 재로드·컴파일 및 설정 검증을 수행한다.

## 완료

- WBP_PlayerSkills의 AttackVariants Overlay에 Attack_Heavy를 추가했다. 자식은 Attack, Attack_Air, Attack_Heavy의 3개다.
- WxViewModel_Ability_AttackHeavy Resolver는 Ability.Attack.Heavy를 조회하고 Attack_Heavy의 뷰모델 입력에 연결했다.
- 지상에서는 Light/Heavy가 함께 표시되고 공중에서는 Air만 표시된다. 기존 OwnedTags → Conv_GameplayTagToSlateVisibility를 재사용했다.
- 세 공격 위젯의 표시값을 HitTestInvisible로 지정했고 Overlay도 HitTestInvisible로 설정했다. 자식까지 마우스 히트 테스트에서 제외한다. CanActivate 바인딩은 유지했다.
- 저장본 검증 성공: Saved/Logs/verify_heavy_hud.log. Overlay 자식 3개, Heavy Resolver 태그, 3개 변환 함수의 OwnedTags 경로·Movement.InAir·HitTestInvisible/Collapsed 인자 및 기본 가시성을 확인했다.
- WBP_Ability/WBP_PlayerSkills/WBP_GameHUD 재로드·컴파일 성공. 에셋 생성 중 InputData_KMB의 중첩 컴파일 ensure가 있었으나 최종 저장본 재로드·컴파일은 오류 없이 종료했다.
- 일회성 에셋 설정 C++ 도구와 임시 모듈 의존성을 제거했다. 이번 작업의 제품 변경은 WBP_PlayerSkills 에셋이다.
- 최종 UE 5.8.2 WxEditor Win64 Development 빌드 성공: Saved/Logs/BuildDoctor/build_2026-09-07_234019_315_12184.log.
- PIE에서 마우스 클릭 통과를 직접 조작하는 검증은 수행하지 않았다.
