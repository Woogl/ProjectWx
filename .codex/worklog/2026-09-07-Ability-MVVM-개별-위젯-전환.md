# Ability MVVM 개별 위젯 전환

## 계획

- 사용자 승인: Resolver와 Ability ViewModel은 지정 태그의 한 어빌리티를 담당하고 상황별 후보 선택을 제거한다.
- DisplayCandidates와 후보별 공유 캐시, HasMatchingAbility, 전용 bool 가시성 변환 함수를 제거한다.
- 지상·공중 평타 위젯을 동일 Overlay에 배치하고 각각 별도 Ability Resolver로 연결한다.
- 기존 Conv_GameplayTagToSlateVisibility에 AbilitySystem.OwnedTags와 Movement.InAir를 바인딩해 두 위젯의 가시성을 반대로 설정한다. CanActivate 비활성 표시는 유지한다.
- UE 5.8 WxEditor Development 빌드, 위젯 재로드·컴파일 및 변경 구조에 맞는 회귀 검증을 수행한다.

## 완료

- DisplayCandidates, FWxAbilityDisplayCandidate, 후보별 공유 캐시, HasMatchingAbility, Conv_MatchingAbilityToSlateVisibility를 제거했다. IWxUIData 및 기존 Conversion Library는 확장하지 않는다.
- Resolver는 구분 태그 하나로 조회한다. Ability ViewModel은 일치하는 첫 어빌리티를 유지하고, 상태 태그 변경 시 발동 가능 여부만 갱신한다. 어빌리티 부여 변경 시의 재조회는 유지한다.
- WBP_PlayerSkills의 기존 Attack 위치에 AttackVariants Overlay를 두고 Attack(지상)과 Attack_Air(공중)를 배치했다. 부모 슬롯 배치는 보존했다.
- 지상 Resolver는 Ability.Attack.Light, 공중 Resolver는 Ability.Attack.Air를 조회한다.
- 두 위젯 Visibility에 VM_PlayerCharacter.AbilitySystem.OwnedTags → Conv_GameplayTagToSlateVisibility를 연결했다. Movement.InAir가 있으면 공중만 표시하고, 없으면 지상만 표시한다.
- WBP_Ability의 자체 가시성 바인딩을 제거하여 부모의 상태별 바인딩과 충돌하지 않도록 했다. CanActivate → bIsEnabled는 유지한다.
- 어빌리티 해제 및 쿨다운 초기화는 유지한다. 일회성 에셋 생성 도구와 관련 모듈 의존성은 최종 코드에서 제거했다.
- UE 5.8.2 WxEditor Win64 Development 빌드 성공: Saved/Logs/BuildDoctor/build_2026-09-07_232713_976_12088.log.
- Wx.MVVM.Ability.SeparateWidgets 테스트 성공: Saved/Logs/separate_ability_widget_test.log. 두 VM의 독립성, 동일 태그 공유, 공중 진입·착지 가시성 반전, 태그 변경에도 각 아이콘 유지, 부여 해제 영향 범위를 확인했다.
- 저장본 재로드에서 Overlay 자식 2개, 별도 Resolver 및 각 변환 인자·OwnedTags 경로를 확인하고 WBP_Ability/WBP_PlayerSkills/WBP_GameHUD 모두 컴파일 성공: Saved/Logs/verify_separate_attacks.log.
- 에셋 생성 중 GUID 보정 및 이전 바인딩 잔존 문제는 수정했고, 최종 재로드 검증은 오류 없이 종료했다. PIE 화면 육안 검증은 수행하지 않았다.
