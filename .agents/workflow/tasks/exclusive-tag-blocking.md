# Exclusive 어빌리티 태그 차단 전환

상태: 확인 대기 · 체크리스트 11/14 통과 · 실패 1
다음 행동: 코드 리뷰와 화면 표시 모양을 확인하고, 실패한 「예측·복제와 거절 뒤 정리」(서버 거절 뒤 콤보 단계 어긋남)의 수정 방향을 정한다.

- 사용자 결정(2026-09-25): "콤보 구간 동안에는 자기 재발동 허용되게 합시다."
- 현재 범위: 순정 태그 차단 전환·콤보 예외·도플갱어 효과 이관에 이어, 자식의 공통 차단 설정을 베이스 계산과 ASC 순정 확장 지점으로 옮겼다. 기존 자식 클래스 자체의 통합은 후속 검토다.
- 사용자 승인(2026-09-25): "네. 그렇게 진행해주세요. `IgnoreAbilityTags` 는 `IgnoreAbilityActivationTags` 로 하세요." 도플갱어 정책과 구현 진행을 승인했다.

## 테스트 체크리스트

| 항목 | 확인 방법 | 담당 | 결과 | 근거 |
| --- | --- | --- | --- | --- |
| Editor Development 빌드 | 테스트 코드 제거 후 build-doctor 실행 | AI | 통과 | `build_2026-09-25_234334_925_50992.log`: Succeeded, exit 0. 대기 중 선행 빌드가 완료되어 Target is up to date |
| GA 차단 관계·점프 기본값 | 리다이렉트 없는 에디터에서 GA 40개 CDO의 계산 목록 대조 | AI | 통과 | GA 쌍 1,600건·점프 40건, `Saved/Tests/ExclusiveAbilityAssets-AscHook.json`. 개별 차단은 Death의 Ability뿐이며 재저장 불필요 |
| 효과 이름·에셋 참조 | ABS_Doppelganger 저장 후 ClassRedirect 제거·새 프로세스 재로드·옛 이름 검색 | AI | 통과 | 네 효과 중 새 IgnoreAbilityActivationTags 참조 확인, Source·Plugins·Config·Content 옛 참조 없음 |
| 콤보·외부 차단·면제 범위 | 제거 전 `Wx.Combat.AbilityBlocking.Lifecycle` 임시 자동화 | AI | 통과 | 자기/동일 태그 다른 스펙 구분·추가 차단·컷신 GE·소유자 조건 면제·Source 조건 검사 |
| 취소·후딜·즉시 종료 수명 | 제거 전 `Lifecycle`·`AssetDefaults` 임시 자동화 | AI | 통과 | 재발동·Light→Heavy·Recovery 교체·취소·즉시 종료, `ExclusiveAbilityBlocking-AscHook/index.json`: 전체 성공 3·실패 0·경고 0 |
| 공통 규칙·ASC 확장 경계 | 제거 전 `Wx.Combat.AbilityBlocking.HookRules` 임시 자동화 | AI | 통과 | 그룹·태그 기반 규칙, 명시 태그 보존, 비 Wx/빈 요청자, 적용·해제 일치 확인 |
| 테스트 제거·주석 전용 변경 | 테스트 소스·friend 참조 검색, 주석 제외 코드 해시 비교 | AI | 통과 | C++ 2개·Python 2개 제거, 테스트 friend 참조 0, 주석 대상 12개 모두 코드 동일, 정정 21건·압축 4건·보류 0 |
| 코드 리뷰 | `WxAbilityBase`·`WxAbilitySystemComponent`·각 자식 생성자: 공통 목록의 안정성, 콤보 자기 기여 제외, 순정 차단·취소 위임 | 사람 | 대기 | |
| 콤보와 선입력 | 헤드리스 게임(LV_DevCombat -game -nullrhi): 실제 입력 경로(선입력 컴포넌트)로 콤보 창 안/밖 재입력, 콤보 창에서 누른 회피, 후딜의 스킬·강공격·점프와 본동작의 점프 | AI | 통과 | 2026-09-28 임시 자동화 테스트(확인 뒤 삭제): 창 안 재입력은 LL로 이어진다. 본동작에서 누른 재입력은 거절(같은 몽타주 인스턴스)되어 선입력으로 남았다가 창이 열리는 0.34초에 LL로 이어진다. LL 창(0.33초)에서 누른 회피는 창에서는 나가지 않고 후딜 시작(0.42초)에 나가 공격을 취소한다. 후딜의 스킬·강공격은 바로 나가 공격을 취소하고, 점프는 본동작에서 막히며 후딜에서는 뛰면서 공격을 취소한다. 매번 끝난 뒤 차단 태그가 남지 않는다 |
| 반응 중첩·강공격 취소 | 헤드리스 게임: 약공격 본동작 중 강공격, 가드 중 피격(일반·퍼펙트 창), 공격 중 피격과 반응 중 재피격, 적 패턴 중 그로기·사망, 그로기 적 처형(상호작용 스캐너로 실행), 플레이어 사망 | AI | 통과 | 강공격은 막히지 않고 약공격을 취소한다. 가드 중 피격은 가드를 유지한 채 가드 반응(GuardHit·PerfectGuard)으로 들어간다. 공격 중 피격은 공격을 취소하고 Normal 반응으로, 반응 중 재피격은 반응을 새 인스턴스로 다시 시작한다. 패턴 중 그로기·사망은 패턴을 취소하고, 사망하면 다음 패턴 발동이 막힌다. 그로기 적 앞 상호작용으로 처형이 나간다. 플레이어가 죽은 뒤 공격·회피 입력은 나가지 않는다. 각 동작이 끝난 뒤 플레이어 차단 태그가 비어 있다 |
| 도플갱어 미러링 | 헤드리스 게임: HGTest 궁극기 1로 분신을 불러 주인의 약공격 콤보(창 재입력·본동작 선입력)를 따라 쓰는지, 분신에 컷신 GE·사망과 같은 Ability 전체 차단을 걸면 따라 쓰기가 막히는지 본다 | AI | 통과 | 분신이 있으면 주인은 GA_HGTest_Attack_Light_2(Master.Doppelganger 조건)를 쓰고, 분신도 L→LL→LLL→LLLL을 같은 몽타주로 따라 쓴다. 선입력으로 이은 3단은 주인 노티파이 안의 커밋이라 분신이 한 번 거절된 뒤 0.007초 만에 재시도로 따라 쓴다. 컷신 GE·Ability 전체 차단 중에는 재시도가 0.401초에 만료되어 따라 쓰지 않는다. 차단을 푼 뒤 지난 입력을 늦게 쓰지 않고, 다음 입력은 다시 따라 쓴다 |
| 예측·복제와 거절 뒤 정리 | 헤드리스 에디터 PIE(리슨 서버 + 원격 클라이언트, 한 프로세스): 소유 클라이언트의 콤보·후딜 회피, 서버만 첫 발동 거절(서버에만 공중 태그), 서버만 콤보 재발동 거절(서버에만 약공격 차단) 뒤 양쪽 차단 태그와 다음 입력 | AI | 실패 | 콤보·후딜 회피는 서버가 같게 따라가고 양쪽에 차단이 남지 않는다. 첫 발동을 거절하면 클라이언트 예측이 되돌려지고 차단이 남지 않으며 회피와 다음 입력도 정상이다. 콤보 재발동을 거절한 뒤에도 차단은 남지 않는다. 그러나 다음 공격에서 클라이언트는 L, 서버는 LL을 재생해 콤보 단계가 어긋난다. 원인과 검증한 수정안은 아래 「헤드리스 테스트로 옮김」에 있다 |
| UI·상호작용 가용성 | 헤드리스 게임: 상호작용 대상(BP_CheckPoint) 옆에서 약공격 L→LL을 끝까지 두고, 단계마다 회피·스킬 어빌리티 뷰모델의 CanActivate와 상호작용 스캐너 목록을 본다. 4단 콤보 동안에는 매 틱 모든 어빌리티의 CanActivateAbility를 50번씩 더 부른다 | AI | 통과 | 본동작·콤보 창에서는 회피·스킬 CanActivate가 false이고 상호작용 목록이 0개다. 후딜·종료에서는 true이고 목록이 1개다. 조회를 약 110만 번 더해도 L→LL→LLL→LLLL과 단계 전환이 그대로이고, 끝난 뒤 차단이 남지 않는다 |
| 화면 표시 모양 | 에디터 플레이로 본동작·콤보 창·후딜 전환 중 스킬·회피 아이콘의 사용 가능 표시와 상호작용 프롬프트가 자연스럽게 바뀌는지 본다 | 사람 | 대기 | |

## 합의와 검토 이력

### 2026-09-25 — 테스트 정리와 제출 요청

사용자: "테스트 코드 제거하고, 주석 정정해주세요.", "다 끝나면 제출해주세요." 이번 차단 작업의 임시 C++ 테스트 2개와 테스트 전용 friend, Saved/Tests의 GA 검증 Python 2개를 제거한다. 검증 로그·보고서는 과거 실행 근거로 보존한다. 차단 관련 주석만 현재 태그 계산과 GAS 수명에 맞춰 정정하며, 다른 작업 변경은 제출에서 제외한다. 이 요청은 사람의 플레이·네트워크 테스트 통과로 해석하지 않는다.

### 2026-09-25 — ASC 확장 지점으로 공통화 승인

사용자: "네. 그렇게 합시다." → ApplyAbilityBlockAndCancelTags에서 계산한 목록을 Super에 전달하는 구조를 승인했다. 후속으로 "자식클래스를 최대한 축소하는걸 원해요", "자식클래스를 축소하려는 이유는 어빌리티의 공통 규칙이기 때문이에요"라고 명시했다. 범위 질문에는 "이번에는 자식의 차단 코드만 제거"라고 답했고, "나중에는 저런 자식클래스들도 없앨 수 있으면 없애고 싶어요"라고 장기 방향을 밝혔다.

그룹별 차단과 태그 관계 계산은 베이스의 공통 함수 하나가 담당하며 자식의 함수 호출·재정의를 제거한다. ASC는 순정 차단·해제에 연결하고 콤보 예외도 같은 최종 목록을 사용한다. 동작 중 바뀌는 월드·액션 상태로 차단 목록을 계산하지 않는다. 기존 공격 타입의 고유 기능 통합은 이번 공통화의 범위에 포함하지 않는다.

기존 검증은 이전 생성자 선언 방식의 결과이므로 새 구현으로 빌드·차단 관계·수명 테스트를 다시 실행했다. 최종 결과는 위 체크리스트와 아래 ASC 공통화 검증 로그에 있다.


### 2026-09-25 — 차단 전환 영향 조사

사용자는 "Exclusive 어빌리티의 발동 차단을 태그 방식으로 수정할까요? 엔진의 순정 규칙을 따르는게 나을 것 같은 느낌이 들어서요"라고 제안했고, 후속으로 "좋습니다. 계획대로 했을 때 문제 있을지 점검해주세요."라고 요청했다.

현재 작업 트리와 로컬 UE 5.8 엔진 소스를 정적으로 대조했다. 기존 미커밋 변경은 수정하지 않았다.

- `UWxAbilityBase::FindActivationGroupBlocker`는 점유자 전원을 검사하며 자기 콤보 재발동과 `CancelAbilitiesWithTag` 우선 진입을 허용한다.
- 엔진은 `CanActivateAbility`를 통과한 뒤 `PreActivate`에서 차단·취소 태그를 적용한다. 취소 선언만으로 차단을 뚫지 못한다. 약공격이 강공격을 처음부터 차단하지 않는 관계가 필요하다.
- 공통 배타 태그는 자기 재발동도 막는다. 콤보 구간에서 전체 차단을 풀면 다른 액션까지 허용한다. 같은 태그를 공유하는 다른 GA와 자신을 구분하고, 외부 차단은 유지해야 한다.
- `SetShouldBlockOtherAbilities(false)`는 해당 어빌리티의 차단만 해제한다. 후딜 액션의 효과·태스크 종료는 별도이므로 `CancelRecoveringAbilities`가 맡는 역할을 보존한다.
- `ActivationGroup`은 Override 취소 면역·입력 버퍼 분류·점프 제한에도 쓰인다. 그룹 자체 제거는 이번 차단 전환과 별도 범위다.
- `BlockedAbilityTags`는 ASC의 소유 태그 컨테이너와 별개다. 단계 변경 이벤트·입력 버퍼 재시도 순서와 서버·소유 클라이언트의 실행/종료 처리를 보존해야 한다.

### 2026-09-25 — 콤보 결정과 도플갱어 제안

사용자: "콤보 구간 동안에는 자기 재발동 허용되게 합시다. 도플갱어 같은 경우는 어떻게 해야할까요?"

콤보 구간의 자기 재발동 허용은 결정으로 반영한다. 외부 차단까지 무시하라는 요청으로 해석하지 않는다.

도플갱어에 대한 당시 AI 제안(이후 위 사용자 승인으로 확정):

- `WxBTTask_MirrorAbility`는 주인의 `AbilityCommittedCallbacks`에서 같은 GA 클래스·레벨을 자기 ASC에 부여하고 `TryActivateAbility`로 실행한다. `Master.Doppelganger` 등 주인에게 성립한 GA 선택 조건을 도플갱어의 소유 태그로 다시 검사하면 따라 쓰기가 막힐 수 있다.
- 기존 `IgnoreAbilityTags`의 면제 범위를 소유자의 발동 태그 조건(`ActivationRequiredTags`·`ActivationBlockedTags`)으로 좁힌다. 이름도 예를 들어 `IgnoreActivationTagRequirements`로 맞춘다. 기존 효과 하나의 의미를 정리하는 제안이며 추가 효과를 중첩하는 제안이 아니다.
- `BlockAbilitiesWithTag`와 GE의 어빌리티 차단, 도플갱어 자신의 본동작·콤보 창·후딜 규칙은 지킨다. 실제로 걸린 사망·컷신 차단도 검사한다.
- 비용·쿨다운 면제는 ABS_Doppelganger의 기존 전용 효과를 유지한다.
- 도플갱어의 콤보 창이 아직 열리지 않았으면 기존 `RetryDuration` 기본값 0.4초의 재시도를 사용한다. 이 재시도는 최신 실패 요청 하나만 유지하며 임의 지연의 완전 동기화를 보장하지 않는다.
- 현재 `DoesAbilitySatisfyTagRequirements`는 `IgnoreAbilityTags`가 있으면 즉시 true를 반환해 엔진 차단도 우회한다. 제안은 이 동작을 좁히는 변경이므로, 완전한 기존 동작 보존이라고 표현하지 않는다.

## 구현

- `FindActivationGroupBlocker`와 베이스 `CanActivateAbility` 재정의를 제거했다. 처음에는 생성자에 차단 목록을 선언했으나 후속 승인으로 `UWxAbilityBase::GetAbilityBlockTags`와 ASC의 `ApplyAbilityBlockAndCancelTags`로 공통화했다.
- `GetAbilityBlockTags`는 명시한 `BlockAbilitiesWithTag`에 ActivationGroup·에셋 태그의 공통 규칙을 합친다. Independent는 명시 목록만, Exclusive·Override는 공통 액션 목록도 사용한다. 구체 클래스 분기나 자식 재정의가 없고, 적용·해제·콤보 조회가 같은 함수를 쓴다.
- ASC는 전달된 BlockTags도 보존하면서 최종 목록을 보완해 Super에 넘긴다. 발동·후딜·종료의 카운트와 취소는 엔진이 처리한다. 비 Wx 또는 빈 요청자는 순정 인자를 그대로 사용한다.
- 13개 자식 생성자의 공통 함수 호출과 약공격의 개별 차단 수정 코드를 제거했다. 사망 고유의 Ability 부모 차단은 기존 명시 규칙으로 유지한다.
- 일반 액션·패턴·점프를 공통 차단 대상으로 선언한다. 가드 반응·피격·처형 등 Override의 중첩 진입과 취소 면역은 유지한다. Death는 기존 `Ability` 부모 차단을 유지한다.
- Exclusive이면서 Ability.Attack.Light 태그를 가진 어빌리티는 공통 목록에서 Heavy를 제외한다. 에셋이 명시한 추가 차단은 삭제하지 않는다. 발동 검사를 통과한 강공격이 순정 `CancelAbilitiesWithTag`로 약공격을 취소한다.
- 콤보는 활성 인스턴스 자기 차단 기여 1건만 읽기 전용 조회에서 제외한다. 다른 스펙·같은 태그의 외부 기여·GE 차단은 남는다. 첫 발동에서 확인한 소유자 조건을 콤보 다음 단에서 면제하는 기존 동작도 유지한다.
- Recovery는 순정 `SetShouldBlockOtherAbilities(false)`로 자기 차단만 해제한다. 다음 발동/점프의 `CancelRecoveringAbilities`와 입력 버퍼 재시도 순서는 보존한다.
- 점프는 `Ability.Jump` 차단을 조회한다. `ActivationGroup`은 입력 버퍼·액션 단계·Override 취소 면역에 남는다.
- `UWxEffect_IgnoreAbilityActivationTags`와 `Effect.IgnoreAbilityActivationTags`로 이름을 변경했다. 도플갱어 효과는 소유자 ActivationRequired/Blocked 조건만 면제하고, 전달된 Source/Target 조건과 어빌리티·GE 차단은 검사한다.
- `ABS_Doppelganger`를 새 클래스 참조로 저장하고 임시 ClassRedirect를 제거했다. 리다이렉트 없는 새 에디터 프로세스에서 네 효과 참조와 GA 40개의 1,600개 차단 관계·점프 차단 기본값을 대조했다.
- 임시 회귀 테스트는 `Source/WxEditor/Tests/WxAbilityBlockingTests.cpp`와 `WxAbilityBlockingTestTypes.h`에서 실행했고 사용자 요청으로 제출 전에 제거했다. 첫 수명 테스트는 테스트용 ASC의 속성 세트 누락으로 중단됐고, 실제 비용 검사를 위한 `UWxCombatAttributeSet`을 추가한 뒤 재빌드·재실행해 두 테스트가 통과했다.

## 구현 후 확인 대상

AI 검증은 위 체크리스트와 아래 로그에 기록했다. 렌더링·몽타주를 생략한 ServerOnly 테스트이므로 실제 노티파이와 입력 버퍼 타이밍, 도플갱어 BT 재시도, 네트워크 예측/복제, 화면 표시는 별도 확인이 필요하다. 사람 항목은 아직 미실행이다. 2026-09-28에 이 가운데 헤드리스로 판정할 수 있는 부분을 AI 항목으로 옮겨 테스트했다(아래 「헤드리스 테스트로 옮김」).

## 검증 로그

### 제출 전 정리 — 2026-09-25

- 테스트 제거 후 빌드 `Saved/Logs/BuildDoctor/build_2026-09-25_234334_925_50992.log`: Result Succeeded, BUILD_DOCTOR_EXIT_CODE=0. 선행 빌드 대기 후 Target is up to date로 검증을 마쳤다.
- C++ 테스트·타입 파일 2개, 테스트 전용 friend, Saved의 GA 검증 Python 2개를 제거했다. Source·Plugins에서 해당 타입/테스트 참조가 남지 않았음을 검색했다.
- 주석 대상 12개 중 11개 파일 수정, 정정 21건·압축 4건·보류 0. 테스트 friend 제거 직후와 비교해 주석 외 코드 해시가 12개 모두 동일하다.
- 기존 변경과 정리 diff가 겹친 파일: `WxAbilityBase.h/.cpp`, `WxAbilitySystemComponent.h`, `WxAbility_Finisher.cpp`, `WxAbility_GuardReact.cpp`, `WxAbility_Guard.cpp/.h`, `WxAbility_Death.h`, `WxAbility_UseItem.cpp`, `WxAbility_Interact.cpp`, `WxCharacterBase.cpp`. `WxAbilitySystemComponent.cpp`는 점검만 했고 추가 주석 수정은 없다.
- 다른 작업의 방향 몽타주·콤보 구성·에셋·Workflow 변경은 제외하고 이번 차단 작업만 부분 스테이징한다. 이전 임시 테스트 성공 기록은 과거 검증 근거이며 사람 항목은 대기다.
- Wiki lint critical·warning·suggestion 0, diff --check 통과. Wiki와 Workflow 뷰어를 갱신했다.

### ASC 공통화 최종 검증 — 2026-09-25

- 빌드 로그 `Saved/Logs/BuildDoctor/build_2026-09-25_231328_105_36192.log`: Result Succeeded, BUILD_DOCTOR_EXIT_CODE=0.
- GAS 회귀 로그 `Saved/Logs/ExclusiveAbilityBlocking-AscHook.log`와 JSON 결과 `Saved/Tests/ExclusiveAbilityBlocking-AscHook/index.json`: AssetDefaults·HookRules·Lifecycle 성공 3, 실패·경고 0. 프로세스 exit 0.
- 기존 GA 재로드 로그 `Saved/Logs/ExclusiveAbilityAssets-AscHook.log`와 계산·선언 목록 `Saved/Tests/ExclusiveAbilityAssets-AscHook.json`: GA 40개, 쌍 1,600건, 점프 40건, 도플갱어 네 효과 참조 확인. 프로세스 exit 0.
- 공통 규칙은 에셋/CDO를 수정하지 않는다. 명시 차단은 GA_Shared_Death의 Ability만 남고 나머지는 공통 계산으로 적용된다. GA 재저장 없이 같은 관계가 유지됨을 확인했다.
- 재사용할 계약과 확인 한계를 전투 어빌리티에 반영했다. 생성자 방식 원자료는 보존하고 새 ASC 공통화 원자료를 추가했다.

### 이전 생성자 선언 방식 검증

- 기존 GA 수정 필요 여부 후속 확인(2026-09-25): 사용자 질문에 따라 생성 목록을 다시 내보내고 기존 파서로 GA 40개의 저장 프로퍼티를 검사했다. 모두 네이티브 클래스를 직접 상속하며 `BlockAbilitiesWithTag`·`CancelAbilitiesWithTag`·`ActivationGroup` 저장 오버라이드는 각각 0개다. 앞선 새 프로세스 CDO 검증과 함께 새 C++ 차단 기본값의 적용을 확인했으므로 GA 재저장은 필요하지 않다. 클래스 참조가 바뀐 `ABS_Doppelganger`만 이관했다. 근거: `Saved/Tests/ExclusiveAbilitySerializedOverrides.json`.

- 빌드: 최종 로그 `Saved/Logs/BuildDoctor/build_2026-09-25_223726_639_47172.log`, `Result: Succeeded`, `BUILD_DOCTOR_EXIT_CODE=0`.
- 에셋 이관: 이관 로그 `Saved/Logs/ExclusiveTagMigration.log`, 리다이렉트 없는 검증 로그 `Saved/Logs/ExclusiveTagValidation.log`.
- GAS 회귀: 성공 로그 `Saved/Logs/ExclusiveAbilityBlocking-Retry.log`, JSON 보고서 `Saved/Tests/ExclusiveAbilityBlocking/index.json`. `AssetDefaults`·`Lifecycle` 모두 Success, 실패·경고 0.
- 생성 목록: `Export-AbilitySystemLists.ps1` 실행, GA 40·세트 9·캐릭터 7·몽타주 34·C++ 효과 26·GE 6·피해 표 1. `character-list`·`effect-list`에 새 효과 참조를 반영했다.
- 지식: .wiki 전투 문서와 AI 문서에 구현 계약과 미확인 범위를 반영했다.

## 근거

- `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`
- `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp`
- `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`
- `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxInputBufferComponent.cpp`
- `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp` 및 `Public/WxBTTask_MirrorAbility.h`
- `Source/WxGame/Character/WxCharacterBase.cpp`
- `.wiki/wiki/references/ability-list.md`·`character-list.md`·`effect-list.md`(이관 뒤 생성 스크립트로 캐릭터·이펙트 목록 갱신)
- 로컬 UE 5.8 `GameplayAbility.cpp`의 태그 검사·PreActivate·EndAbility·SetShouldBlockOtherAbilities, `AbilitySystemComponent_Abilities.cpp`의 발동·차단 카운트·예측 거절 처리

## 헤드리스 테스트로 옮김 · 2026-09-28

- 계기: 작업 절차의 헤드리스 규칙에 따라 사람 항목 다섯 개(콤보와 선입력, 반응 중첩·강공격 취소, 도플갱어 미러링, 예측·복제와 거절 뒤 정리, UI·상호작용 가용성)를 AI 항목으로 옮기고 AI가 끝까지 테스트했다(사용자 결정: `workflow-recheck.md` Q2 "지금 한 건씩 차례로"). 화면에 그려지는 모양만 사람 항목 「화면 표시 모양」으로 남겼다. 코드·에셋은 바꾸지 않아 코드 리뷰 항목은 그대로다.
- 단독 방법: `LV_DevCombat`을 `-game -nullrhi`로 띄우고 임시 자동화 테스트를 돌렸다. 입력은 캐릭터의 어빌리티 입력 처리와 같은 진입점(선입력 컴포넌트 → ASC)으로 누르고 뗐다. 공격자는 행동 트리 없는 기본 AI 컨트롤러로 빙의한 템플릿 적이다. 피격은 적을 원인으로 `UWxCombatLibrary::ApplyDamage`, 그로기는 `UWxEffect_AddGP`, 사망은 `UWxEffect_AddIncomingDamage`로 일으켰다. 가드는 입력 누름 상태로 발동했고, 일반 가드는 퍼펙트 창이 열렸다 닫힌 뒤에 맞았다. 처형은 그로기 적 앞에서 상호작용 스캐너의 `TryInteractSelected`로 실행했다. 도플갱어는 HGTest를 빙의시켜 궁극기 1이 실제로 소환한 분신이다. 분신 차단은 컷신 GE(`UWxEffect_SkillCutscene`) 적용과 `BlockAbilitiesWithTags(Ability)`로 걸었다.
- 네트워크 방법: 에디터를 `-nullrhi`로 띄우고 임시 에디터 자동화 테스트가 PIE를 리슨 서버 + 클라이언트 1명(한 프로세스)으로 시작했다. 원격 클라이언트 캐릭터를 조작하고 서버 쪽 같은 캐릭터와 비교했다. 서버만 거절하게 하려고 서버 ASC에만 `Movement.InAir` 느슨한 태그, 또는 `BlockAbilitiesWithTags(Ability.Attack.Light)`를 걸었다.
- 실패 원인: 서버만 콤보 재발동을 거절하면 클라이언트는 예측한 다음 단 몽타주가 거절로 끊기고, 인터럽트 종료(취소)로 콤보 단계를 비운다. 서버는 앞 단이 클라이언트가 복제한 재발동 종료(취소 아님)로 이미 끝나 단계가 남는다. 다음 단 발동은 CanActivate에서 거절되어 서버에는 단계를 비울 경로가 없다. 그래서 다음 공격에서 서버만 다음 단(LL)으로 시작한다. 로그에서 서버가 `InternalServerTryActivateAbility`를 `Ability.Attack.Light`로 거절했고, 다음 입력의 추적은 client L / server LL이다.
- 실제 게임에서 생기는 경우: 서버만 아는 차단이 클라이언트의 콤보 재발동과 겹칠 때다(아직 복제되지 않은 GE 차단, 서버만 공중으로 판정한 순간 등). 그 콤보 동안 서버는 클라이언트와 다른 몽타주(피해 행·판정 시점)로 공격하고, 그 몽타주가 끝나면 다시 맞는다.
- 검증한 수정안(적용하지 않음): `UWxAbilitySystemComponent::NotifyAbilityFailed`를 재정의해 발동이 거절된 어빌리티가 콤보이고 활성 상태가 아니면 단계를 비운다(`UWxAbility_Combo`에 단계 초기화 함수 추가). 엔진은 CanActivate 거절 때 이 통지를 인스턴스로 부른다. 활성 중인 거절(본동작에서 누른 재입력 등)은 제외하므로 창 선입력은 그대로다. 임시로 적용했을 때 네트워크 테스트(다음 입력이 양쪽 L)와 단독 테스트가 모두 통과했고, 적용분은 되돌렸다. 작업 절차에 따라 AI 실패 항목의 방향은 추가 요청으로 정한다.
- 확인: 단독 테스트는 Result=Success, ensure 0이다. 네트워크 테스트는 콤보·후딜 전환과 첫 발동 거절이 통과했고, 콤보 재발동 거절 뒤 다음 입력만 실패했다. 임시 테스트 두 파일을 지우고 수정안도 되돌린 상태에서 WxEditor Development 빌드가 성공했다.
