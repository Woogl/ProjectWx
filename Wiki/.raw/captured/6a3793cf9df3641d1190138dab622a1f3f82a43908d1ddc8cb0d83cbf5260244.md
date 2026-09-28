# 서버 거절 뒤 콤보 단계 어긋남

상태: 완료 · 체크리스트 5/5 통과
다음 행동: 변경 시 기록된 테스트 범위와 제약을 참고한다.

- 계기: Exclusive 태그 차단 기록(`exclusive-tag-blocking.md`)의 헤드리스 네트워크 테스트(2026-09-28)에서 AI 항목 「예측·복제와 거절 뒤 정리」가 실패했다.
- 사용자 결정(2026-09-28): "콤보는 일감 만들어주세요. 나중에 볼게요" 그 실패의 수정을 이 일감으로 뗐다.
- 사용자 결정(2026-09-29): "더 올바른 해결 방법 있는지 찾아봐주세요" → 조사 뒤 "네. 그 방법으로 합시다." 거절 초기화(NotifyAbilityFailed) 계획을 버리고, 클라이언트가 콤보 단계를 발동 이벤트 데이터로 보내는 안으로 바꿨다. 거절 없이 완료 뒤에도 어긋나는 경우가 드러났기 때문이다.

## 요청

- 요청 · 이우성 2026-09-28

> 콤보는 일감 만들어주세요. 나중에 볼게요

## 질문

| ID | 질문 | 선택지 | 추천 | 답변 |
| --- | --- | --- | --- | --- |
| Q1 | 구현 중 거절 없이도 서버 콤보 단계가 남는 경우를 찾았다(정상 콤보가 끝난 직후 다음 공격이 client L / server LLL). 클라이언트가 먼저 몽타주를 끝내고 복제한 종료가 서버에서는 재발동 종료와 구분되지 않아서다. 어떻게 고칠까? | 클라이언트가 단계를 발동 이벤트 데이터로 보내고 서버가 따른다 / 승인된 거절 초기화만 두고 완료 뒤 어긋남은 새 일감으로 뗀다 / 서버가 원격 종료 뒤 같은 프레임의 발동만 콤보로 잇는다 | 클라이언트가 단계를 발동 이벤트 데이터로 보내고 서버가 따른다. 엔진 순정 ServerTryActivateAbilityWithEventData 경로라 새 RPC가 없고, 완료·거절 두 경우를 한 규칙으로 덮으며 NotifyAbilityFailed 재정의는 빠진다 | 클라이언트가 단계를 발동 이벤트 데이터로 보내고 서버가 따른다 (이우성 2026-09-29 "네. 그 방법으로 합시다.") |

## 구현 계획

구현 승인: 이우성 2026-09-29 ("네. 그 방법으로 합시다.")

추가 질문 없음.

1. 현상: 서버의 콤보 단계가 클라이언트와 어긋나 다음 공격에서 서버만 다른 단을 재생한다. 두 경우가 있다.
   - 서버만 콤보 재발동을 거절한 뒤(client L / server LL).
   - 거절 없이 콤보가 끝난 직후(client L / server LLL). 클라이언트가 먼저 몽타주를 끝내 종료를 복제하면 서버는 자기 몽타주 완료 전에 취소 아닌 종료를 받아 단계가 남는다.
2. 원인: "이어 가기(재발동)"인지 "끝남"인지는 클라이언트만 안다. 서버에는 두 경우 모두 취소 아닌 원격 종료로 보여 서버가 추론으로 가릴 수 없다.
3. 변경. 규칙은 "입력 발동의 콤보 단계는 발동하는 쪽이 정하고, 서버는 받은 단계를 따른다"이다.
   - `UWxAbility_Combo`: `GetNextComboIndex()`를 더한다. 활성 중인 인스턴스를 재발동할 때만 다음 단, 그 밖은 첫 단이다. `ActivateAbility`는 이벤트 데이터가 있으면 `EventMagnitude`를 단계로 쓰고(범위 밖이면 첫 단), 없으면(AI·도플갱어 미러링, 서버 단독) 기존처럼 자기 단계를 잇는다.
   - `UWxAbilitySystemComponent`: 입력 발동 두 곳(`AbilityInputActionTriggered`·`TryActivateByInputAction`)이 새 private `TryActivateInputAbility`를 거친다. 콤보면 단계를 담은 이벤트 데이터로 `InternalTryActivateAbility`를 부르고, 엔진이 이를 `ServerTryActivateAbilityWithEventData`로 서버에 넘긴다. 콤보가 아니면 기존 `TryActivateAbility`다.
   - 앞 계획의 `ResetCombo()`·`NotifyAbilityFailed` 재정의는 적용했다가 뺐다.
4. 검증 방법: WxEditor Development 빌드, 헤드리스 에디터 PIE 리슨 서버 + 원격 클라이언트(정상 콤보·완료 뒤 다음 입력·첫 발동 거절·재발동 거절·후딜 회피·호스트 선입력), 단독 PIE 도플갱어 미러링. 경합이 섞였으므로 반복 실행한다. 끝나면 `exclusive-tag-blocking.md`의 「예측·복제와 거절 뒤 정리」 결과를 갱신한다.

## 테스트 체크리스트

| 항목 | 확인 방법 | 담당 | 결과 | 근거 |
| --- | --- | --- | --- | --- |
| Editor Development 빌드 | 임시 테스트 제거 후 build-doctor 실행 | AI | 통과 | build_2026-09-29_024010_844_4080.log: Succeeded. Source/WxGame/Tests 없음, WxGame.Build.cs 임시 의존 되돌림 |
| 완료·거절 뒤 콤보 단계(네트워크) | 헤드리스 에디터 PIE 리슨 서버 + 원격 클라이언트(LV_DevCombat): A 정상 L→LL, A2 콤보 끝난 뒤 다음 입력, B 서버만 첫 발동 거절(공중 태그), C 서버만 콤보 재발동 거절(약공격 차단) 뒤 양쪽 다음 입력·차단 태그 | AI | 통과 | 임시 Wx.Temp.ComboRejectionNet 27회 중 26회 전 항목 통과. A2·B·C의 다음 입력은 27회 모두 client/server AM_Template_Attack_L, 거절 뒤 차단 없음, C는 서버가 Ability.Action.Attack.Light로 거절한 것을 확인. 수정 전 대조 실행은 A2·B에서 client L / server LLL로 실패. 1회는 C의 첫 입력이 클라이언트에서 발동·RPC 없이 사라져 콤보 창을 못 봤고(단계와 무관, 원인 미상), 로그를 더한 뒤 15회 연속 재현되지 않음 |
| 네트워크·호스트 회귀 | 같은 PIE: D 클라이언트 후딜 회피와 그 뒤 입력, E 호스트 창 재입력, F1 호스트 본동작 선입력(선입력 컴포넌트), F2 호스트 회피 본동작 중 선입력 → 후딜 재시도 | AI | 통과 | D 양쪽 AM_Shared_Dodge 뒤 양쪽 L, E·F1 AM_Template_Attack_LL, F2 회피 중 3회 버퍼 뒤 AM_Template_Attack_L |
| 도플갱어 미러링 | 헤드리스 에디터 단독 PIE: BP_HGTest 빙의, 궁극기 1로 분신 소환, 약공격 1단 → 창 재입력 2단 → 본동작 선입력 3단 → 끝난 뒤 다시 1단에서 분신 몽타주 비교 | AI | 통과 | 임시 Wx.Temp.DoppelgangerMirror: 주인·분신 모두 AM_HGTest_Attack_2_L → _LL → _LLL, 끝난 뒤 둘 다 _L |
| 코드 리뷰 | 변경 파일: WxAbility_Combo.h/.cpp(GetNextComboIndex, 이벤트 데이터 단계), WxAbilitySystemComponent.h/.cpp(TryActivateInputAbility). 볼 점: 입력 발동만 이벤트 데이터를 싣고 AI·미러링은 기존 경로인지, 클라이언트가 보낸 단계를 서버가 범위 검사만 하고 믿는 것(PvE 전제), TryActivateAbility 대신 InternalTryActivateAbility를 부르는 것 | 사람 | 통과 | woogle 2026-09-28 |

## 조사 · 2026-09-28

- 재현: 헤드리스 에디터 PIE(리슨 서버 + 원격 클라이언트, 한 프로세스)에서 한다.
  1. 클라이언트가 약공격 L을 친다(서버도 L).
  2. 콤보 창이 열리면 서버 ASC에만 `BlockAbilitiesWithTags(Ability.Attack.Light)`를 걸고 클라이언트가 다음 단을 누른다.
  3. 서버가 `InternalServerTryActivateAbility`를 `Ability.Attack.Light`로 거절하고, 클라이언트는 `ClientActivateAbilityFailed`를 받는다.
  4. 양쪽이 모두 멈춘 뒤 차단을 풀고 다시 누르면, 추적 결과는 client L / server LL이다.
- 차단 태그는 양쪽 모두 남지 않았다. 첫 발동을 거절할 때(서버 ASC에만 `Movement.InAir`)는 어긋나지 않는다. 그때는 서버의 단계 값이 이미 비어 있기 때문이다.
- 엔진 경로(UE 5.8 GameplayAbilities)
  - 클라이언트: `ClientActivateAbilityFailed`가 예측 키 거절을 먼저 알린다. 예측 몽타주가 멈추고 몽타주 태스크의 OnInterrupted가 불린다. 이어 `HandleMontageInterrupted`가 취소 종료로 끝내고 `ComboIndex`를 비운다.
  - 서버: 앞 단 종료는 `ServerEndAbility` → `RemoteEndOrCancelAbility`가 취소가 아닌 종료로 처리한다. 뒤이은 발동은 `InternalTryActivateAbility`의 CanActivate 검사에서 거절되고, 이때 `NotifyAbilityFailed(Handle, 인스턴스, 실패 태그)`가 불린다.
- 실제 게임에서 생기는 경우: 클라이언트의 콤보 재발동이 서버만 아는 차단과 겹칠 때다. 예를 들어 아직 복제되지 않은 GE 차단(컷신·반응 등), 서버만 공중으로 판정한 순간, 비용 판정 차이가 있다.
- 기각한 안
  - 콤보 어빌리티가 ASC의 실패 델리게이트를 직접 구독하는 안: 같은 규칙이지만 부여·제거 수명에 맞춰 구독을 관리해야 해 코드가 늘어난다.
  - 클라이언트가 단계 번호를 TargetData로 보내는 안: 새 동기화 통로라 쓰지 않는다.
  - 종료와 같은 프레임의 발동만 콤보로 이어 주는 시간 조건: 복제 RPC 도착 시점에 기대 불안정하다.
- 약점: 클라이언트 쪽 초기화는 엔진의 예측 몽타주 거절 경로에 기댄다. 클라이언트가 예측하는 공격·스킬은 발동하자마자 몽타주를 재생하므로 늘 이 경로를 탄다.

## 구현 · 2026-09-29

- 승인된 변경(`ResetCombo()`, `UWxAbilitySystemComponent::NotifyAbilityFailed` 재정의)을 적용했다. 작업 트리에 그대로 있다.
- 임시 에디터 자동화 테스트(`Wx.Temp.ComboRejectionNet`, 리슨 서버 + 원격 클라이언트, `LV_DevCombat`)로 검증했다. 시나리오: A 정상 콤보 L→LL, A2 콤보가 끝난 뒤 다음 입력, B 서버만 첫 발동 거절(공중 태그), C 서버만 콤보 재발동 거절(약공격 차단), D 후딜 회피, E 호스트의 선입력 콤보.
- 수정 적용: B·C·D·E는 매번 통과했다. C는 서버가 `Ability.Action.Attack.Light`로 거절(비활성 상태)한 뒤 다음 입력이 양쪽 모두 `AM_Template_Attack_L`이다. 거절 뒤 양쪽 모두 차단이 남지 않는다.
- 새로 발견: A2는 두 번 중 한 번 실패했다(client `AM_Template_Attack_L` / server `AM_Template_Attack_LLL`). 수정을 끈 대조 실행에서도 A 뒤 B의 다음 입력이 client L / server LLL이었다. 원인(추정, 로그의 몽타주로 판단): 클라이언트가 몽타주를 먼저 끝내 단계를 비우고 종료를 복제하면, 서버는 자기 몽타주가 끝나기 전에 취소가 아닌 종료를 받아 단계가 남는다. 이 종료는 재발동 때의 종료와 서버에서 구분되지 않는다. 서버 몽타주는 편도 지연만큼 늦게 시작하고 클라이언트 종료도 그만큼 늦게 도착하므로, 지연과 무관하게 거의 동시 도착의 경합이다(관측 2회 중 1회). HEAD의 `WxAbility_Attack`에도 같은 단계 로직이 있어 이번 작업 전부터 있던 문제다.
- 거절 초기화는 이 경우를 덮지 못한다. 거절이 없기 때문이다. 방향을 Q1로 묻는다.
- 엔진 확인: `InternalTryActivateAbility`는 이벤트 데이터가 있으면 `ServerTryActivateAbilityWithEventData`로 서버에 보낸다(AbilitySystemComponent_Abilities.cpp 1937).
- 임시 테스트는 저장소에서 지우고 `WxGame.Build.cs`의 임시 UnrealEd 의존도 되돌렸다. 지운 상태의 빌드: build_2026-09-29_020334_536_28884.log Succeeded.

## 대안 조사 · 2026-09-29

- 요청: "더 올바른 해결 방법 있는지 찾아봐주세요"
- 근본 원인: 콤보 단계를 양쪽이 각자 기억하고, "이어 가기(재발동)"인지 "끝남(완료)"인지는 클라이언트만 안다. 서버에는 두 경우 모두 취소 아닌 원격 종료로 보인다. 따라서 서버가 추론만으로 맞히는 안은 없고, 클라이언트의 의도가 발동과 함께 가야 한다.
- 검토한 안
  - 클라이언트가 단계를 발동 이벤트 데이터로 보냄(추천 유지): 엔진은 이벤트 데이터가 있으면 `ServerTryActivateAbilityWithEventData`로 보내고(1937), 서버 발동에 그대로 넘긴다(2109). 추가 검사는 `ShouldAbilityRespondToEvent`뿐이며 기본은 참이고, GA_ BP 중 이를 재정의하거나 `ActivateAbilityFromEvent`를 쓰는 것은 없다. 콤보 계열(Attack·Skill·Pattern)은 TriggerEventData를 읽지 않는다. 입력 경로 두 곳(`AbilityInputActionTriggered`·`TryActivateByInputAction`)만 바뀌고 AI·도플갱어(서버 단독 발동)는 그대로다. 거절 초기화(`NotifyAbilityFailed`)는 불필요해 뺀다.
  - 한 번의 발동이 콤보 전체를 맡고 다음 단은 복제 입력 이벤트로 넘김(GAS 교과서형): 종료가 한 번뿐이라 어긋날 틈이 없지만, 09-25 결정(콤보 창 자기 재발동)·선입력 재시도·도플갱어의 발동 단위 미러링을 모두 다시 짜야 한다.
  - 완료 종료만 복제하지 않고 양쪽이 각자 몽타주 완료로 끝냄: 완료 직후(지연 구간) 누른 입력이 서버에서는 재발동으로 처리돼 반대 방향으로 어긋난다.
  - 재발동 종료만 복제하지 않고 서버도 자체 재발동을 타게 함: 재발동 여부를 알 멤버 플래그가 필요하고 엔진 재발동 경로 내부에 기댄다.
  - 원격 종료와 같은 프레임의 발동만 잇기: 신뢰 RPC가 한 패킷에 실리는 것에 기대며, 패킷이 나뉘면 틀린다.

## 재구현 · 2026-09-29

- 결정에 따라 `ResetCombo()`·`NotifyAbilityFailed` 재정의를 빼고, 입력 발동이 콤보 단계를 이벤트 데이터로 싣게 바꿨다(구현 계획 3).
- 테스트 결과는 체크리스트에 있다. 임시 테스트 두 개(Wx.Temp.ComboRejectionNet·Wx.Temp.DoppelgangerMirror)와 WxGame.Build.cs의 임시 UnrealEd 의존은 지웠다.
- 원인 미상 1회: C의 첫 입력에서 클라이언트 `AbilityInputActionTriggered`가 발동도 서버 RPC도 없이 끝나 5초 동안 콤보 창을 보지 못했다. 그 뒤 같은 실행에서 거절·다음 입력은 정상이었다. 클라이언트 실패 통지와 입력 결과 로그를 더한 15회에서는 재현되지 않았다. 이번 변경은 콤보의 발동 가능 판정을 바꾸지 않아 무관하다고 보지만 확인하지는 못했다.

## 이전 계획 · 2026-09-28 (2026-09-29 폐기)

- 폐기 이유: 거절이 없는 완료 뒤 어긋남을 덮지 못한다(위 「구현 · 2026-09-29」). 원문은 아래와 같다.

당시 승인: 이우성 2026-09-29 ("네")

추가 질문 없음.

1. 현상: 소유 클라이언트가 콤보 창에서 다음 단을 누른 순간에 서버만 그 재발동을 거절하면, 다음 공격에서 클라이언트는 첫 단(L)을, 서버는 둘째 단(LL)을 재생한다. 그 콤보 동안 서버의 무기 판정 시점과 피해 행이 화면과 다르고, 그 몽타주가 끝나면 다시 맞는다.
2. 원인: 클라이언트는 재발동하려고 앞 단을 끝내고(취소 아님, 서버로 복제) 다음 단을 예측 재생한다. 서버가 거절하면 클라이언트는 예측 몽타주가 끊기며 인터럽트 종료(취소)로 `ComboIndex`를 비운다. 서버는 앞 단이 복제된 종료(취소 아님)로 끝나 `ComboIndex`가 남는다. 다음 단 발동은 CanActivate에서 거절되므로 서버에는 이 값을 비울 경로가 없다.
3. 변경. 규칙은 "활성 상태가 아닌 콤보의 발동이 거절되면 다음 발동은 첫 단부터"이고, 서버·클라이언트에 똑같이 적용된다.
   - `UWxAbility_Combo`(`WxAbility_Combo.h/.cpp`): 콤보 단계를 비우는 공개 함수 `ResetCombo()`를 더한다.
   - `UWxAbilitySystemComponent`(`WxAbilitySystemComponent.h/.cpp`): 엔진의 `NotifyAbilityFailed`를 재정의한다. 거절된 어빌리티가 콤보이고 활성 상태가 아니면 `ResetCombo()`를 부른다. 엔진은 CanActivate가 거절하면 이 통지를 인스턴스로 부른다.
   - 활성 중인 거절은 건드리지 않는다(본동작에서 누른 재입력, 도플갱어의 따라 쓰기 재시도). 따라서 선입력과 재시도 동작은 그대로다.
4. 검증 방법
   - WxEditor Development 빌드.
   - 헤드리스 에디터 PIE(리슨 서버 + 원격 클라이언트): 서버만 콤보 재발동을 거절한 뒤 다음 공격이 양쪽 모두 L인지 본다. 콤보·후딜 회피와 첫 발동 거절 뒤 정리가 그대로인지도 본다.
   - 헤드리스 게임(단독) 회귀: 콤보, 선입력, 회피 대기, 후딜 액션, 반응, 도플갱어 재시도, UI 조회.
   - `exclusive-tag-blocking.md`의 실패 항목 「예측·복제와 거절 뒤 정리」를 다시 테스트해 결과를 그 기록에 적는다.
   - 테스트 체크리스트 초안
     - AI 항목: 빌드, 거절 뒤 콤보 단계(네트워크), 네트워크 회귀, 단독 회귀.
     - 사람 항목: 코드 리뷰. 변경 파일은 4개이고, 볼 점은 초기화 조건이 활성 여부만 보는지와 엔진 통지가 인스턴스로 오는지다.
5. 사전 확인(2026-09-28): 위 변경을 임시로 적용해 두 테스트를 돌렸고, 적용분은 되돌렸다.
   - 네트워크 테스트: 다음 공격이 양쪽 모두 L이고, 거절 뒤에 차단이 남지 않았다.
   - 단독 테스트: 모두 통과했다.


## 사용자 테스트 결과 · 2026-09-28T17:49:58.499Z

<!-- test-feedback:request-44411192-9566-46c2-b6b1-02e54325c5ba:submitted -->
- 전달한 사람: woogle

> 통과 · 코드 리뷰
