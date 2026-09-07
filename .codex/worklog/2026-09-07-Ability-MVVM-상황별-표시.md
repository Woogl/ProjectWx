# Ability MVVM 상황별 표시

> 후속 사용자 승인으로 이 구현은 `2026-09-07-Ability-MVVM-개별-위젯-전환.md`의 구조로 대체되었다. 최종 결과는 후속 기록을 참조한다.

## 계획

- 사용자 수정 승인: IWxUIData와 WxAbilityBase 확장은 철회하고 Resolver에 후보별 필요·차단 태그를 설정한다. ViewModel은 전달받은 후보로 선택하고 MVVM Conversion Library가 bool을 가시성으로 변환한다.
- WxViewModel_Ability는 발동 가능 여부와 별개로 표시 조건에 맞는 후보를 선택하고, 후보가 없으면 첫 후보로 대체하지 않는다.
- HasMatchingAbility를 FieldNotify로 제공하고 HUD의 가시성 및 CanActivate 바인딩을 설정한다.
- 지상·공중 평타의 표시 조건과 공통 슬롯 태그를 확인·설정한다.
- UE 5.8 WxEditor Development 빌드와 가능한 에셋·전환 검증을 수행한다.

## 완료

- IWxUIData 확장을 제거했다. 표시 조건은 WxUI의 FWxAbilityDisplayCandidate로 정의하고 WxGame Resolver의 DisplayCandidates에서 설정한다.
- 후보 목록이 있는 슬롯은 배열 순서대로 AbilityTags/RequiredTags/BlockedTags를 검사하며, 부여된 후보가 없으면 HasMatchingAbility=false로 표시 데이터를 비운다. 후보 미설정 슬롯은 기존 발동 태그 요건 선택을 유지한다.
- 공유 캐시에 후보 목록과 순서를 포함해 같은 슬롯 태그라도 다른 표시 설정이 섞이지 않도록 했다.
- WBP_PlayerSkills Attack Resolver에 공중(Ability.Attack.Air, Movement.InAir 필요)과 지상(Ability.Attack.Light, Movement.InAir 차단) 후보를 저장했다. 두 GA의 기존 발동 태그 설정은 이미 적합했다.
- WBP_Ability에 HasMatchingAbility → Conv_MatchingAbilityToSlateVisibility → Visibility, CanActivate → bIsEnabled 바인딩을 저장했다. 변환 함수는 SelfHitTestInvisible/Collapsed를 반환한다.
- 후보 전환·해제 시 이전 쿨다운 및 표시 상태를 초기화한다.
- UE 5.8.2 WxEditor Win64 Development 빌드 성공: Saved/Logs/BuildDoctor/build_2026-09-07_231652_834_*.log.
- 자동화 테스트 Wx.MVVM.Ability.ContextSelection 성공: Saved/Logs/ability_context_test.log. 실제 검/도끼 아이콘으로 지상·공중·착지 선택, 발동 차단 중 표시 유지, 후보 부재 Collapse, 설정별 공유 캐시, 해제 상태를 검증했다.
- 저장한 WBP_Ability/WBP_PlayerSkills/WBP_GameHUD 재로드·컴파일 성공: Saved/Logs/verify_ability_hud.log.
- 일회성 에셋 바인딩 생성 C++ 도구와 해당 추가 의존성은 제거했다. 회귀 테스트에 필요한 GameplayTags 의존성만 WxEditor에 추가했다.
- PIE 화면의 육안 확인은 수행하지 않았다. 기존 미니언 컴포넌트 에셋 로드 경고는 작업 범위 밖이며 테스트 성공에 영향을 주지 않았다.
