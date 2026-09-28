# GetAbilityBlockTags 제거

상태: 완료 · 체크리스트 9/9 통과
다음 행동: 변경 시 기록된 테스트 범위와 제약을 참고한다.

## 요청

사용자(2026-09-28): "UWxAbilityBase::GetAbilityBlockTags() 라는 함수를 없앱시다. 이게 있으니까 코드에서 Exclusive 어빌리티를 관리해야되는 문제점이 있어요"
사용자(2026-09-28): "우리의 코드 베이스를 분석해서 가장 관리하기 좋은 방식을 찾아볼래요?"

## 질문

| ID | 질문 | 선택지 | 추천 | 답변 |
| --- | --- | --- | --- | --- |
| Q1 | 차단 목록을 어디에 선언할까? | 타입 클래스 생성자의 BlockAbilitiesWithTag / GA 에셋마다 BlockAbilitiesWithTag / 부모 태그 신설로 한 줄 차단 | 타입 클래스 생성자. 에셋 태그·발동 조건·취소 태그가 이미 생성자에 있고, 순정 필드라 에디터에 실제 값이 보이며 GA별로 덮어쓸 수 있다. 에셋 기입은 GA 33개 중복·누락 시 조용히 배타 상실, 부모 태그는 대규모 태그 개명과 약공격 중 강공격 취소 불가 | 사용자(2026-09-28): "네, 그럽게 합시다! 문제 없을지도 꼼꼼하게 검토해주세요" → 타입 클래스 생성자 |
| Q2 | 피격 반응이 공격·스킬만 끊을까, Ability.Action 전체를 끊을까? | 공격·스킬만 / Ability.Action 전체 | 공격·스킬만. 액션 중 적 패턴만 피격에 끊기지 않는 것이 의도된 설계이고, 회피·아이템 사용·상호작용은 이미 피격 반응 몽타주가 밀어내 끝나며 가드·궁극기는 피격 반응 자체가 막힌다. 전체로 바꾸면 적 패턴이 평타 피격마다 끊긴다 | 사용자(2026-09-29): "네 일단 그렇게 하죠" → 공격·스킬만 유지. 패턴·피격 반응의 가상 함수로 패턴만 빼는 안은 취소 경로에 요청자를 아는 훅이 없어 기각 |

## 구현 계획 (Ability.Action 부모 태그 + ActivationGroup 제거 · 2026-09-28)

사용자: "코드 품질이 중요해요", "EWxAbilityActivationGroup 개념도 없앨 수 있나요?" → AI 제안(액션을 태그 위치로 표현하고 그룹을 없앰)에 "네".

어빌리티 규칙을 순정 필드와 태그 위치로만 표현한다. 액션인지는 에셋 태그 위치, 무엇을 막는지는 `BlockAbilitiesWithTag`, 무엇을 끊는지는 `CancelAbilitiesWithTag`가 정한다.

1. 태그
   - 액션 9종과 자식 26개를 `Ability.Action.*`으로 옮기고 `Ability.Action`을 추가한다: Attack(.Light·.Heavy·.Air·.DodgeCounter)·Skill(.1~.4)·Pattern(.1~.9)·Ultimate·Dodge·Guard·UseItem·Interact·Jump.
   - C++ 상수 이름을 문자열에 맞춘다(`Ability_Action_Attack_Light` 등, 약 70줄·20여 파일 일괄 변경). Sprint·LockOn·Passive·HitReact·GuardReact·Groggy·Death·Finisher·PlayMontageOnce는 그대로 둔다.
   - `WxGameplayTags.h`의 Ability 태그 설명에 "액션은 Ability.Action 아래"를 적는다.
2. 차단
   - 액션 타입(Attack·Skill·Pattern·Ultimate·Guard·Dodge·UseItem·Interact)과 반응 타입(HitReact·GuardReact·Groggy·Finisher·PlayMontageOnce) 생성자는 `BlockAbilitiesWithTag.AddTag(Ability_Action)` 한 줄.
   - 약공격만 Heavy를 뺀 목록(Attack.Light·Air·DodgeCounter와 나머지 액션 8종)을 풀어 적는다. 본동작 중 강공격 취소를 유지한다.
   - Death는 기존 Ability 차단 그대로.
3. ActivationGroup 제거
   - `EWxAbilityActivationGroup`·`ActivationGroup` 프로퍼티·생성자 13곳의 그룹 선언을 지운다. GA 에셋은 이 값을 저장하지 않았다(사전 검토).
   - Exclusive 판정 자리(액션 단계 전환·후딜 차단 해제·콤보 창·콤보 자기 재발동·후딜 액션 취소 대상·입력 버퍼)는 `GetAssetTags().HasTag(Ability_Action)`으로 바꾼다.
   - Override 취소 면역(`CanBeCanceled` 재정의)을 지운다. 대신 그로기·사망의 `CancelAbilitiesWithTag`를 `Ability` 전체에서 `Ability.Action`·Sprint·LockOn·Passive로 좁힌다. 지금 끊기는 대상과 같다.
   - 발동할 때 후딜 중인 액션을 취소하는 쪽을 "그룹이 Independent가 아닌 어빌리티"에서 "액션"으로 바꾼다.
4. 동작이 달라지는 곳(후딜 취소만)
   - 반응(피격·가드 반응·그로기·사망·처형·몽타주 재생)이 발동해도 후딜 중인 액션을 따로 취소하지 않는다.
   - 피격은 공격·스킬을 원래 끊는다. 그로기·사망은 액션 전체를 원래 끊는다. 그래서 차이는 다음 세 가지다.
     - 피격 → 후딜 중인 패턴: 몽타주 밀어냄으로 끝난다.
     - 가드 반응 → 후딜 중인 가드: 끊지 않는다.
     - 처형·몽타주 재생 → 후딜 중인 액션: 겹쳐 재생된다.
5. 컷신 효과는 태그 이름만 따라 바꾸고 막는 대상은 그대로 둔다.
6. 에셋 19개는 임시 GameplayTagRedirects ini로 하나씩 재저장한 뒤 ini를 지우고, 리다이렉트 없이 한 번 더 재저장해 왕복을 확인한다.
7. 앞선 변경(`GetAbilityBlockTags`·ASC 재정의 삭제, 콤보 조회의 `BlockAbilitiesWithTag` 사용)은 그대로 쓴다.

검증
- 빌드.
- 임시 대조 테스트(GA 40개 CDO): 새 태그 대응표로 옮긴 옛 규칙과 비교해 다음 세 가지가 같은지 본다.
  - 모든 Ability 태그에 대한 차단 여부
  - 액션 판정 = 옛 Exclusive
  - 그로기·사망이 끊는 대상 = 옛 "Override가 아닌 어빌리티"
- 에셋 안 옛 태그 문자열 0개.
- 헤드리스 게임 임시 테스트:
  - 약공격 콤보·선입력, 약공격 중 강공격 취소, 후딜 회피·스킬
  - 가드 중 피격·가드 반응, 공격 중 피격, 패턴 중 피격·그로기·사망
  - 처형, 적 BT의 패턴 번호 발동, 플레이어 스킬 슬롯 발동
  - 각 동작 뒤 차단 태그가 남지 않는지
- 임시 테스트는 확인 뒤 지우고 다시 빌드한다.

테스트 체크리스트 초안
- AI: 빌드 · GA 규칙 대조 · 에셋 태그 이관 · 헤드리스 전투 흐름
- 사람: 코드 리뷰 · 화면 표시(스킬 아이콘·상호작용 프롬프트 사용 가능 표시)

태그 이름은 사용자가 `Ability.Exclusive.*`·`Ability.Trait.Exclusive` 마커를 물었고, AI가 식별 태그로서의 뜻과 슬롯 GA의 에셋 태그 덮어쓰기(마커 누락 위험)를 근거로 `Ability.Action`을 유지하자고 답했다.

구현 승인: 사용자 2026-09-28 ("네 승인합니다. 구현하세요")

## 구현 계획 (추가: 콤보 창 취소 진입 · 2026-09-29)

사용자: "UWxAbility_Attack_Light::UWxAbility_Attack_Light() 생성자가 여전히 좀 복잡한데", "콤보 윈도우일 때에만 예외, 약공격일 때에만 예외를 추가하면 되지 않을까요? 아니면 캔슬 규칙만 약간 커스텀한다던지요", "그럼 좋네요. 이 방향으로 진행해도 괜찮은지 검토해주세요"

- 규칙: 콤보 창에서는 자기 재발동과, 콤보 창 주인을 `CancelAbilitiesWithTag`로 지목한 어빌리티만 들어올 수 있다.
- 베이스 `DoesAbilitySatisfyTagRequirements`: 점유자를 `GetAnimatingAbility()`로 찾는다(콤보 창 노티파이와 같은 방식). 콤보 창이면 점유자의 차단 몫 1건만 빼고 본다. 소유자 발동 조건은 자기 재발동만 면제하고, 끼어드는 어빌리티는 그대로 검사한다. ASC 함수는 그대로 쓴다.
- 약공격 생성자: `BlockAbilitiesWithTag`를 `Ability.Action` 한 줄로.
- 조작감 변경: 강공격은 약공격 본동작 전체 → 콤보 창·후딜부터. 본동작 중에 누른 강공격은 선입력으로 남았다가 콤보 창이 열릴 때 나간다.
- 검토 결과
  - `GetAnimatingAbility()`는 const이고 콤보 창도 이 어빌리티에 열린다. 약공격 몽타주 17개(템플릿·HGTest) 모두 콤보 창과 후딜 노티파이가 있다.
  - GA 에셋 중 `CancelAbilitiesWithTag`를 덮어쓴 것은 없다. 액션을 지목하는 취소는 강공격 → 약공격뿐이고, 피격·그로기·사망은 Ability.Action 밖이라 이 예외와 무관하다.
  - 위험: 강공격 발동이 콤보 창 타이밍에 달리게 되어, 창 경계에서 서버만 거절할 수 있다. 이때 소유 클라이언트는 예측으로 약공격을 이미 취소했고 서버의 약공격은 계속된다(취소는 예측 롤백되지 않음). 지금은 강공격이 약공격에 막히지 않아 이 경우가 없다. 콤보 재발동의 같은 종류 문제는 `combo-stage-desync-after-rejection.md`에 있다.
  - UI: 약공격 본동작 동안 강공격 아이콘이 사용 불가로 표시된다.
- 검증: 빌드, 헤드리스 게임(약공격 본동작 중 강공격 거절·선입력으로 콤보 창 진입, 콤보 창에서 강공격 취소 진입·약공격 재발동, 콤보 창에서 회피 거절, 강공격 소유자 조건), 헤드리스 에디터 PIE(리슨 서버 + 원격 클라이언트)로 콤보 창 강공격의 양쪽 일치.

추가 변경 구현 승인: 사용자 2026-09-29 ("네 진행하세요")

추가 변경 검증 결과(2026-09-29)
- 단독 헤드리스(LV_DevCombat -game): 17항목 통과. 약공격 차단 한 줄, 본동작 중 강공격 거절 → 선입력으로 콤보 창이 열리는 0.341초에 발동해 약공격 취소, 콤보 창에서 회피 거절·약공격 재발동, 공중이면 콤보 창에서도 강공격 거절, 본동작·콤보 창 점프 불가, 후딜 점프 가능, 차단 잔류 없음.
- 리슨 서버 PIE(원격 클라이언트): 실패. 본동작 중 누른 강공격이 선입력으로 클라 콤보 창이 열리는 프레임(0.225초)에 나가면, 그때 서버 약공격은 아직 본동작이라 서버가 거절한다(8회 중 8회, `InternalServerTryActivateAbility: Rejecting ClientActivation of GA_Template_Attack_Heavy`). 클라가 예측으로 끊은 약공격 종료가 서버로 전해져 양쪽 모두 공격이 없는 상태로 끝난다. 창이 열린 뒤 테스트가 직접 누른 강공격은 3회 모두 수락됐다. 선입력 LL 비교는 테스트 설계 문제로 판정하지 못했다.
- 사용자(2026-09-29): "강공격 이슈는 WxAbility_Attack_Light의 적절한 가상함수를 override해서 해결할 수 있지 않을까요?" → 약공격의 차단 등록은 엔진이 멤버를 직접 넘겨 가상 함수가 없으므로, 끼어드는 쪽 발동 판정(`DoesAbilitySatisfyTagRequirements`)에서 콤보 창 조건을 뺀 규칙을 제안: "재생 중인 액션을 CancelAbilitiesWithTag로 지목한 어빌리티는 그 액션의 차단 몫을 빼고 본다(단계 무관), 자기 재발동은 콤보 창에서만". 조작감은 1번과 같고(본동작 중 언제든 강공격), 서버는 약공격 활성을 먼저 받으므로 타이밍 거절이 없다. 사용자: "내! 그렇게 해주세여"(2026-09-29, 수정 구현 승인).
- 사용자(2026-09-29): "WxAbility 관련 코드를 확인해서 더 품질을 올릴 수 있는 방법을 제안해주세요" → AI 제안 1~3, 사용자: "네. 그렇게 해주세요", "1,2,3번 모두 여기서 진행합시다".
  - 1. 공격·스킬·패턴에 복사돼 있던 콤보 진행(`ActivateAbility`·`EndAbility`·`HandleMontageCompleted`)을 `UWxAbility_Combo`로 올렸다. 창 닫힘 초기화(`OnComboWindowClosed`)는 입력 콤보인 공격·스킬에만 둔다(병사 패턴 몽타주에 콤보 창이 있어 패턴은 초기화하면 안 됨).
  - 2. 재생 속도 기본값을 1.0으로, 공격 속도(ASPD)는 콤보만 따르게 했다. `return 1.f` 오버라이드 7개 삭제. 궁극기·아이템 사용은 이제 ASPD를 따르지 않는다(동작 변경, AI 추천대로 진행).
  - 3. 베이스 `PlayMontage`의 방향 섹션 선택을 가상 함수 `SelectInputDirectionSection`으로 열고, 회피는 이 훅에서 Backstep·Back 기본값·보정 회전만 처리한다. 회피 자체의 방향 TargetData 송수신 코드(약 90줄)를 지웠다.
  - 이번 변경으로 낡은 주석 3곳(그로기 Override, 피격 "그룹", 가드 "Ability 태그")을 고쳤다.
- 사용자(2026-09-29): "WxAbility_Groggy에서 캔슬 태그로 Sprint와 LockOn은 굳이 캔슬하지 않아도 될거 같아요" → 그로기는 적 세트(ABS_Shared_Enemy·ABS_Sandbag)에만 있고 질주·락온은 플레이어 세트에만 있어 동작 차이가 없으므로 뺐다. 그로기 취소 대상은 Ability.Action 하나. 사망은 플레이어도 가져 질주·락온 취소를 유지한다.
- 사용자(2026-09-29): "WxEffect_SkillCutscene에서 일일히 어빌리티 다 나열하는 부분도 Ability.Action 으로 단순화할 수 있지 않을까요?" → 차단을 Ability.Action·Sprint·LockOn 세 줄로 줄였다. 플레이어에만 걸리므로 패턴 추가 차단은 영향이 없고, 컷신 중 점프가 새로 막힌다(입력이 컷신 뒤로 이어지지 않게 한다는 효과 의도와 맞음). 단독 테스트에서 컷신 중 액션·질주·락온·점프 차단, 피격·가드 반응 비차단, 약공격 입력 거절, 해제 뒤 잔류 없음 확인(22항목 통과).
- 사용자(2026-09-29 작업 중): "Ability.Action.Jump 태그는 없애도 되지 않을까요?" → 태그를 지우고 점프 검사를 `Ability.Action` 차단으로 바꿨다(단독 테스트의 점프 항목 통과).

## 구현 계획 (생성자 목록 · 대체됨)

- `UWxAbilityBase::GetAbilityBlockTags`를 지운다.
- `UWxAbilitySystemComponent::ApplyAbilityBlockAndCancelTags` 재정의를 지운다(순정 그대로).
- 콤보 자기 기여 제외 조회는 `BlockAbilitiesWithTag`를 넘긴다.
- 지금 계산 결과와 같은 목록을 생성자에 선언한다. Exclusive: Attack(Light는 Heavy 대신 Light·Air·DodgeCounter)·Skill·Pattern·Ultimate·Guard·Dodge·UseItem·Interact. Override: HitReact·GuardReact·Groggy·Finisher·PlayMontageOnce. Death는 기존 Ability 차단 그대로.
- 그룹이 차단을 정한다는 주석을 고친다.
- 검증: 빌드, 임시 자동화 테스트로 GA 40개 CDO의 새 `BlockAbilitiesWithTag`가 옛 계산 규칙과 모든 Ability 태그에 대해 같은 차단을 내는지 대조.
- 테스트 체크리스트 초안: 빌드(AI) · GA 차단 관계 전후 동일(AI) · 코드 리뷰(사람).

이 계획은 사용자가 2026-09-28 "네, 그럽게 합시다!"로 승인해 구현·대조 테스트까지 했으나, 같은 날 방향이 위 계획으로 바뀌어 승인을 다시 받는다.

## 테스트 체크리스트

| 항목 | 확인 방법 | 담당 | 결과 | 근거 |
| --- | --- | --- | --- | --- |
| Editor Development 빌드 | 임시 테스트 삭제 후 build-doctor 실행 | AI | 통과 | `build_2026-09-29_013052_505_5704.log`: Result Succeeded, exit 0(품질 개선 1~3 반영, 임시 테스트 삭제 뒤) |
| GA 규칙 대조 | 임시 에디터 자동화(GA 40개 CDO)로 옛 규칙과 비교: Ability 태그별 차단 여부, 액션 판정 = 옛 Exclusive, 그로기·사망 취소 대상 = 옛 Override 아닌 어빌리티 | AI | 통과 | Result=Success. 차이는 새 부모 태그 `Ability.Action` 자체의 차단뿐(그 태그를 식별 태그로 가진 어빌리티 없음). 이후 공격 생성자 정리·Passive 제외 뒤 공격 4종 CDO(차단 목록·소유 태그)를 게임 테스트에서 다시 확인 |
| 에셋 태그 이관 | 임시 리다이렉트 ini로 19개 재저장 → ini 삭제 → 리다이렉트 없이 재저장 → Content 문자열 검색 | AI | 통과 | 옛 태그 문자열 0개, 새 태그 유지, 재저장 로그의 태그 경고 0(오류는 없는 폴더 `Content/Character/Player/` 소스 컨트롤 경고뿐) |
| 헤드리스 전투 흐름 | LV_DevCombat `-game -nullrhi` 임시 자동화, 입력은 선입력 컴포넌트 → ASC, 피격은 `ApplyDamage`, 적은 기본 AI 컨트롤러로 빙의한 템플릿 적 | AI | 통과 | 흐름 테스트 44항목 성공: 약공격 중 회피·점프 차단, 강공격이 약공격 취소, 콤보 창 재발동, 후딜 회피 캔슬, 스킬 입력, 가드 중 피격 → 가드 반응(가드 유지), 공격 중 피격 → 약공격 취소, 패턴 번호 발동, 그로기·사망이 패턴 취소, 사망 뒤 발동·피격 거절, 본동작 패턴은 피격에 유지, 후딜 아이템 사용 중 피격 → 반응 발동·아이템 사용 0.3초 안 종료, 매 단계 뒤 차단 잔류 없음. 겹침 테스트 27항목 성공: 공격 4종 CDO, 약공격 후딜 중 몽타주 재생 → 액션 차단·약공격 종료, 그로기 적 처형(상호작용 처리 경로) → 액션 차단·피해자 몽타주·그로기 유지. ensure 0 |
| 취소 진입·점프·컷신·그로기(추가 변경) | LV_DevCombat `-game -nullrhi` 임시 자동화 | AI | 통과 | 25항목 성공: 약공격 차단 = Ability.Action 한 줄, 본동작 중 강공격 판정 통과·즉시 발동·약공격 취소, 본동작·콤보 창의 회피 판정 거절, 콤보 창 약공격 재발동, 공중이면 강공격 거절, 본동작·콤보 창 점프 불가·후딜 점프 가능, 컷신 효과 중 액션·질주·락온·점프 차단·피격/가드 반응 비차단, 적 패턴 중 그로기가 패턴 취소, 차단 잔류 없음. ensure 0 |
| 리슨 서버 강공격 일치 | 헤드리스 에디터 PIE(리슨 서버 + 원격 클라이언트, 한 프로세스): 약공격 뒤 0.02·0.05·0.1·0.2·0.3초에 강공격 | AI | 통과 | 5회 모두 서버·클라 모두 강공격 활성·약공격 종료, 서버 거절 0, 양쪽 차단 잔류 없음. (앞선 콤보 창 한정 규칙은 선입력 강공격이 8회 모두 거절되어 폐기) |
| 품질 개선(콤보 진행 공통화·재생 속도·회피 방향) | LV_DevCombat `-game -nullrhi` 임시 자동화 + 리슨 서버 PIE | AI | 통과 | 단독 25항목: ASPD 1.5에서 약공격·스킬만 1.5배, 회피·가드·피격·가드 반응·사망·처형·궁극기·아이템 사용 1.0, 콤보 L→LL 재발동과 종료 뒤 첫 단, 무입력 회피 Backstep, 60도 입력 ForwardRight·보정 회전 14.4도, 템플릿·병사 패턴.1이 4단을 순서대로 한 번씩 재생(병사는 콤보 창 있는 몽타주 포함), 패턴 재발동은 첫 단. 리슨 서버: 원격 클라 회피 Backstep·Forward·ForwardRight·BackRight 섹션이 서버와 일치, 약공격 중 강공격 일치, 거절 0. ensure 0 |
| 코드 리뷰 | 변경 파일과 볼 점: `WxGameplayTags`(Ability.Action 계층), `WxAbilityBase`(그룹 제거·액션 판정·취소 진입 규칙·재생 속도 기본값·방향 섹션 훅), `WxAbility_Combo`(콤보 진행 공통화), `WxAbility_Dodge`(방향 동기화를 베이스로), `WxAbilitySystemComponent`·`WxInputBufferComponent`(액션 판정), 공격·반응 타입 생성자(차단·취소 선언), 에셋 19개 태그 이관 | 사람 | 통과 | woogle 2026-09-28 |
| 화면 표시 | 에디터 플레이: 스킬·회피 아이콘의 사용 가능 표시와 상호작용 프롬프트가 본동작·콤보 창·후딜에서 자연스럽게 바뀌는지, 스킬 슬롯 UI(WBP_PlayerSkills)·아이템 퀵슬롯이 이관한 태그로 정상 표시되는지 | 사람 | 통과 | woogle 2026-09-28 |

## 구현

- `Ability.Action` 아래로 액션 태그 26개를 옮기고 C++ 상수 이름도 맞췄다. 에셋 19개는 임시 리다이렉트로 재저장 뒤 ini를 지웠다.
- `GetAbilityBlockTags`·ASC `ApplyAbilityBlockAndCancelTags` 재정의·`EWxAbilityActivationGroup`·`ActivationGroup`·`CanBeCanceled` 재정의를 지웠다. 액션 판정은 `GetAssetTags().HasTag(Ability.Action)`.
- 차단은 각 타입 생성자의 `BlockAbilitiesWithTag`(`Ability.Action` 한 줄, 약공격만 Heavy 제외 목록, 사망은 Ability). 그로기·사망의 취소 대상은 `Ability.Action`·Sprint·LockOn.
- 사용자(2026-09-29 작업 중): "WxAbility_Attack 계열의 생성자가 좀 지저분하네요" → 베이스는 재발동만 두고, 공격 4종이 식별 태그·소유 태그·차단을 직접 선언하게 했다(`SetAttackTag` 헬퍼와 덮어쓰기·RemoveTag 제거).
- 사용자(2026-09-29 작업 중): "Passive를 막는 생성자는 없어야하지 않을까요?" → 패시브는 발동한 프레임에 끝나 취소할 일이 없으므로 그로기·사망의 취소 대상에서 뺐다.
- 사망의 `BlockAbilitiesWithTag = Ability`는 기존대로 모든 발동(패시브 포함)을 막는다.

## 조사

- 네이티브 타입 클래스마다 에셋 태그가 하나(Ability.Attack.Light 등)이고 생성자가 `SetAssetTags`·`ActivationOwnedTags`·`ActivationBlockedTags`·`CancelAbilitiesWithTag`를 선언한다. `UWxAbilityBase` 클래스 주석도 "생성자에서 태그 관계와 발동 그룹 같은 규칙 기본값을 정한다"이다.
- GA 40개는 모두 네이티브를 직접 상속한 데이터 전용이고, Independent가 아닌 GA가 약 33개다. 캐릭터가 늘 때마다 공격·스킬·궁극기·패턴 GA가 늘어난다.
- `GetAbilityBlockTags`의 호출처는 ASC `ApplyAbilityBlockAndCancelTags` 재정의와 콤보 자기 기여 제외 조회 두 곳이다. 블루프린트 사용 없음.
- 현재 에디터의 BlockAbilitiesWithTag는 비어 보이고 실제 차단은 실행 중 계산되어, GA에서 보이지도 빼지도 못한다.

### 사전 검토 · 2026-09-28

- GA 40개 모두 `BlockAbilitiesWithTag`·`ActivationGroup`·공격 에셋 태그를 에셋에 저장하지 않았다(uasset 이름 표 검색). 생성자 기본값이 그대로 적용되어 GA 재저장이 필요 없다.
- 공격 GA는 모두 Light·Heavy·Air·DodgeCounter 네이티브 클래스를 직접 쓴다. 옛 규칙의 "Exclusive이고 Light 에셋 태그" 분기는 Light 클래스와 정확히 일치한다.
- 엔진(UE 5.8 `GameplayAbility.cpp`)은 PreActivate·SetShouldBlockOtherAbilities·EndAbility 세 곳 모두 `BlockAbilitiesWithTag`를 넘긴다. 적용·후딜 해제·종료 해제가 같은 목록을 쓰므로 카운트가 어긋나지 않는다.
- 옛 ASC 재정의는 넘어온 목록에 계산 목록을 합치기만 했다. 생성자 목록이 계산 결과와 같으면 엔진에 들어가는 차단이 같다.
- Death는 옛 계산에서 Ability와 공통 9개를 함께 등록했고, 새 선언은 Ability만 등록한다. 부모 태그 차단이 자식 전부를 막으므로(`AreAbilityTagsBlocked`는 요청 태그의 부모까지 본다) 막히는 대상은 같다. 콤보 자기 기여 제외 조회도 Death 활성 중 Ability 명시 카운트로 막는다.
- Skill·Pattern GA가 슬롯 태그(Ability.Skill.N 등)를 에셋 태그로 가져도, 차단 목록의 Ability.Skill·Ability.Pattern이 부모라 막히고 콤보 조회의 자기 기여(부모 태그 명시 카운트 1)도 전과 같다.
- 블루프린트·스크립트·도구에서 `GetAbilityBlockTags`를 쓰는 곳은 없다.

### 방향 재검토 · 2026-09-28

- 생성자 선언을 구현하고 빌드·임시 대조 테스트(`Wx.Temp.AbilityBlockTags`: GA 40개, Ability 태그 36개의 차단 여부가 옛 규칙과 모두 같음, 표기만 다른 것은 Death 1건)까지 마쳤다. 임시 테스트 파일은 아직 남아 있다.
- 사용자: "이렇게 되면 Exclusive 어빌리티 그룹 정책과 이중이 되지 않나요?", "State.Exclusive 태그를 만들어서 처리하는게 더 나을까요?", "그럼 아예 ActivationGroup를 없애는게 낫나요?", "뭔가 깔끔하지 않은 느낌이 계속 느껴져요" → AI가 9개 태그 목록 13곳 반복을 원인으로 보고 액션 부모 태그(`Ability.Action`)를 제안했다. 사용자: "조사해주세요. 태그가 어떻게 되나요?"
- 액션 태그 참조: C++ 태그 상수 약 70줄·20여 파일(생성자 차단 목록 제외), 설정·스크립트 0, 에셋 19개(Skill·Pattern 슬롯 번호를 쓰는 GA 12개, BT 4개, WBP_PlayerSkills·WBP_ItemQuickSlot).
- `WxEffect_SkillCutscene`은 따로 Attack·Skill·Ultimate·Dodge·Guard·UseItem·Interact·Sprint·LockOn을 막는다(Pattern·Jump 없음).


## 사용자 테스트 결과 · 2026-09-28T16:45:40.117Z

<!-- test-feedback:request-ef621cfc-f3f6-4fed-a0f0-9dadff282e33:submitted -->
- 전달한 사람: woogle

> 통과 · 코드 리뷰
> 통과 · 화면 표시
