# Ability CommonUI 기본 Hold

## 계획

- 사용자가 승인한 엔진 기본 방식으로 전환한다. WxButtonBase의 입력 주입, Ticker, 클릭 우회와 그 전용 의존성·테스트를 제거한다.
- HeavyAttack에 CommonButtonBase의 RequiresHold와 HoldData를 설정하고, 기존 OnClicked → ViewModel.TryActivateAbility 연결을 사용한다. Light/Air는 일반 클릭을 사용한다.
- 사용자 요청대로 겹친 배치를 유지한다. 각 공격 VM의 CanActivate를 엔진 기본 Visibility 변환에 연결해, 불가능한 앞 버튼을 Collapsed 처리하고 뒤쪽 버튼이 클릭을 받게 한다. WxUIData 및 공용 버튼의 새 기능은 추가하지 않는다.
- WxEditor Development 빌드, Blueprint 재로딩·컴파일, Hold 설정 및 클릭 경로를 검증한다. 에셋 수정용 임시 도구는 제거한다.

## 완료

- 후속 수정: 아래 CanActivate → Visibility 연결은 공격 중 슬롯 소실을 일으켜 `2026-09-08-공격-슬롯-표시-조건-분리.md`에서 상황 태그 기반 표시로 교체했다. 아래 내용은 당시 적용 이력이다.
- WxButtonBase의 입력 주입, Ticker, 클릭 우회 및 전용 의존성을 모두 제거했다. 버튼 소스·헤더와 WxUI 모듈·플러그인 설정은 변경 전과 동일하다.
- `Content/UI/InputData/HoldData_AttackHeavy.uasset`를 CommonUIHoldData의 Blueprint로 만들었다. 키보드/마우스·패드·터치 HoldTime은 현재 IA_Attack_Heavy와 같은 0.35초, HoldRollbackTime은 0이다.
- WBP_PlayerSkills의 Heavy만 RequiresHold를 켰다. Light/Air는 일반 클릭이며, 기존 WBP_Ability의 OnButtonBaseClicked → TryActivateAbility MVVM 이벤트를 사용한다. 게임 입력과 중복 바인딩하지 않도록 TriggeringEnhancedInputAction은 비워 둔다.
- 겹친 배치를 유지하고, 각 공격 VM의 CanActivate를 엔진 UMVVMConversionLibrary.Conv_BoolToSlateVisibility에 연결했다. 참은 SelfHitTestInvisible, 거짓은 Collapsed이다. 초기 상태는 Collapsed이며, 부모 패널은 자식 히트 테스트를 허용한다. 둘 다 가능하면 기존 순서의 최상단 버튼이 클릭을 받는다.
- 에셋 재로딩으로 세 ViewModel GUID·CanActivate 바인딩, Hold 데이터, 기존 클릭 이벤트, 활성화/히트 테스트 설정을 검증했다. WBP_Ability·WBP_PlayerSkills·WBP_GameHUD 컴파일 성공: `Saved/Logs/verify_commonui_hold.log`의 `WXSTOCK VERIFY SUCCESS`.
- 임시 에셋 수정 테스트와 임시 모듈 의존성을 제거했다. 최종 UE 5.8 WxEditor Win64 Development 빌드 성공: `Saved/Logs/BuildDoctor/build_2026-09-08_002638_045_26964.log`.
- 기존 `Wx.MVVM.Ability.SeparateWidgets` 회귀 테스트 성공: `Saved/Logs/test_stock_hold_ability_mvvm.log`.
- 실제 PIE에서 Alt 커서로 클릭/길게 누르는 수동 플레이 검증은 수행하지 않았다. HoldData는 입력 액션의 Hold 트리거와 별도의 엔진 설정이며 자동 동기화되지 않는다.
