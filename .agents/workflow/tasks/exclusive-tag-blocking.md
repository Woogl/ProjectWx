# Exclusive 어빌리티 태그 차단 전환

상태: 확인 대기 · 임시 테스트 제거·차단 주석 정정·Editor Development 빌드 통과
다음 행동: 코드 리뷰와 콤보·선입력·도플갱어·예측/복제·화면 표시 체크리스트를 확인한다.

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
| 콤보와 선입력 | 게임: 콤보 창 안/밖에서 재입력, 회피 선입력, 후딜에서 다른 액션·점프 | 사람 | 대기 | 자기 다음 단과 회피 대기, 새 액션의 후딜 취소 확인 |
| 반응 중첩·강공격 취소 | 게임: 약공격→강공격, 가드→가드 반응, 피격·그로기·처형·사망 | 사람 | 대기 | 기존 진입 관계와 차단 잔류 여부 확인 |
| 도플갱어 미러링 | 게임: 상태별 GA·콤보 선입력·컷신/사망 상태에서 따라 쓰기 | 사람 | 대기 | 0.4초 재시도 타이밍과 외부 차단 유지 확인 |
| 예측·복제와 거절 뒤 정리 | 서버/클라이언트: 콤보·후딜 전환 및 서버 발동 거절 | 사람 | 대기 | 태그 차단 잔류와 입력 재시도 확인 |
| UI·상호작용 가용성 | 게임: 본동작·콤보·후딜 전환 중 표시 확인 | 사람 | 대기 | 빈번한 CanActivateAbility 조회가 실행 상태에 영향을 주지 않는지 확인 |

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

AI 검증은 위 체크리스트와 아래 로그에 기록했다. 렌더링·몽타주를 생략한 ServerOnly 테스트이므로 실제 노티파이와 입력 버퍼 타이밍, 도플갱어 BT 재시도, 네트워크 예측/복제, 화면 표시는 별도 확인이 필요하다. 사람 항목은 아직 미실행이다.

## 검증 로그

### 제출 전 정리 — 2026-09-25

- [테스트 제거 후 빌드](../../../Saved/Logs/BuildDoctor/build_2026-09-25_234334_925_50992.log): Result Succeeded, BUILD_DOCTOR_EXIT_CODE=0. 선행 빌드 대기 후 Target is up to date로 검증을 마쳤다.
- C++ 테스트·타입 파일 2개, 테스트 전용 friend, Saved의 GA 검증 Python 2개를 제거했다. Source·Plugins에서 해당 타입/테스트 참조가 남지 않았음을 검색했다.
- 주석 대상 12개 중 11개 파일 수정, 정정 21건·압축 4건·보류 0. 테스트 friend 제거 직후와 비교해 주석 외 코드 해시가 12개 모두 동일하다.
- 기존 변경과 정리 diff가 겹친 파일: `WxAbilityBase.h/.cpp`, `WxAbilitySystemComponent.h`, `WxAbility_Finisher.cpp`, `WxAbility_GuardReact.cpp`, `WxAbility_Guard.cpp/.h`, `WxAbility_Death.h`, `WxAbility_UseItem.cpp`, `WxAbility_Interact.cpp`, `WxCharacterBase.cpp`. `WxAbilitySystemComponent.cpp`는 점검만 했고 추가 주석 수정은 없다.
- 다른 작업의 방향 몽타주·콤보 구성·에셋·Workflow 변경은 제외하고 이번 차단 작업만 부분 스테이징한다. 이전 임시 테스트 성공 기록은 과거 검증 근거이며 사람 항목은 대기다.
- Wiki lint critical·warning·suggestion 0, diff --check 통과. Wiki와 Workflow 뷰어를 갱신했다.

### ASC 공통화 최종 검증 — 2026-09-25

- [빌드 로그](../../../Saved/Logs/BuildDoctor/build_2026-09-25_231328_105_36192.log): Result Succeeded, BUILD_DOCTOR_EXIT_CODE=0.
- [GAS 회귀 로그](../../../Saved/Logs/ExclusiveAbilityBlocking-AscHook.log)와 [JSON 결과](../../../Saved/Tests/ExclusiveAbilityBlocking-AscHook/index.json): AssetDefaults·HookRules·Lifecycle 성공 3, 실패·경고 0. 프로세스 exit 0.
- [기존 GA 재로드 로그](../../../Saved/Logs/ExclusiveAbilityAssets-AscHook.log)와 [계산·선언 목록](../../../Saved/Tests/ExclusiveAbilityAssets-AscHook.json): GA 40개, 쌍 1,600건, 점프 40건, 도플갱어 네 효과 참조 확인. 프로세스 exit 0.
- 공통 규칙은 에셋/CDO를 수정하지 않는다. 명시 차단은 GA_Shared_Death의 Ability만 남고 나머지는 공통 계산으로 적용된다. GA 재저장 없이 같은 관계가 유지됨을 확인했다.
- 재사용할 계약과 확인 한계를 [전투 어빌리티](../../../.wiki/wiki/concepts/combat-abilities.md)에 반영했다. 생성자 방식 원자료는 보존하고 새 [ASC 공통화 원자료](../../../.wiki/raw/notes/2026-09-25-ability-block-policy-centralization.md)를 추가했다.

### 이전 생성자 선언 방식 검증

- 기존 GA 수정 필요 여부 후속 확인(2026-09-25): 사용자 질문에 따라 생성 목록을 다시 내보내고 기존 파서로 GA 40개의 저장 프로퍼티를 검사했다. 모두 네이티브 클래스를 직접 상속하며 `BlockAbilitiesWithTag`·`CancelAbilitiesWithTag`·`ActivationGroup` 저장 오버라이드는 각각 0개다. 앞선 새 프로세스 CDO 검증과 함께 새 C++ 차단 기본값의 적용을 확인했으므로 GA 재저장은 필요하지 않다. 클래스 참조가 바뀐 `ABS_Doppelganger`만 이관했다. 근거: `Saved/Tests/ExclusiveAbilitySerializedOverrides.json`.

- 빌드: [최종 로그](../../../Saved/Logs/BuildDoctor/build_2026-09-25_223726_639_47172.log), `Result: Succeeded`, `BUILD_DOCTOR_EXIT_CODE=0`.
- 에셋 이관: [이관 로그](../../../Saved/Logs/ExclusiveTagMigration.log), [리다이렉트 없는 검증 로그](../../../Saved/Logs/ExclusiveTagValidation.log).
- GAS 회귀: [성공 로그](../../../Saved/Logs/ExclusiveAbilityBlocking-Retry.log), [JSON 보고서](../../../Saved/Tests/ExclusiveAbilityBlocking/index.json). `AssetDefaults`·`Lifecycle` 모두 Success, 실패·경고 0.
- 생성 목록: `Export-AbilitySystemLists.ps1` 실행, GA 40·세트 9·캐릭터 7·몽타주 34·C++ 효과 26·GE 6·피해 표 1. `character-list`·`effect-list`에 새 효과 참조를 반영했다.
- 지식: [.wiki 전투 문서](../../../.wiki/wiki/concepts/combat-abilities.md)와 [AI 문서](../../../.wiki/wiki/topics/ai.md)에 구현 계약과 미확인 범위를 반영했다.

## 근거

- `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`
- `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp`
- `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`
- `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxInputBufferComponent.cpp`
- `Plugins/WxAI/Source/WxAI/Private/WxBTTask_MirrorAbility.cpp` 및 `Public/WxBTTask_MirrorAbility.h`
- `Source/WxGame/Character/WxCharacterBase.cpp`
- `.wiki/wiki/references/ability-list.md`·`character-list.md`·`effect-list.md`(이관 뒤 생성 스크립트로 캐릭터·이펙트 목록 갱신)
- 로컬 UE 5.8 `GameplayAbility.cpp`의 태그 검사·PreActivate·EndAbility·SetShouldBlockOtherAbilities, `AbilitySystemComponent_Abilities.cpp`의 발동·차단 카운트·예측 거절 처리
