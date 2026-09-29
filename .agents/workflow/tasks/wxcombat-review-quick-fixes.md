# WxCombat 리뷰 지적 두 건 수정

상태: 완료 · 체크리스트 4/4 통과
다음 행동: 변경 시 기록된 테스트 범위와 제약을 참고한다.

- 계기: `module_review_WxCombat.md`(기준 커밋 `e09ed19d1`, 2026-09-30)의 지적 1(몽타주 동기 재생 실패를 성공으로 반환)과 지적 3(권위 없는 머신의 피해 판정 쿼리).
- 범위: 지적 2(겹친 슬로모션이 전역 배율을 먼저 되돌림)는 월드 단위 배율 요청 관리처와 겹침 정책이 필요해 AI가 보류를 권했고, 이번 범위에 넣지 않았다.

## 요청

- 요청 · 이우성 2026-09-30

> 손쉽게 해결 가능할까요?

AI가 1·3은 몇 줄 수정, 2는 보류 권장으로 답하고 "1번과 3번을 지금 고치고 빌드까지 확인해 드릴까요?"라고 물었다.

> 네

> 해결방법 적용했을 때 사이드이펙트 생길까요?

## 구현 계획

구현 승인: 이우성 2026-09-30 (대화: "네")

추가 질문 없음.

1. 지적 1: `UWxAbilityBase::PlayMontageInternal`이 태스크 활성화 뒤 `true` 대신 `IsActive()`를 반환한다. 엔진 `UAbilityTask_PlayMontageAndWait::Activate`는 재생 실패 시 같은 호출 안에서 `OnCancelled`를 방송하고 베이스가 어빌리티를 끝내므로, 호출자가 실패를 `false`로 받아 띄우기·회전·후속 태스크를 건너뛴다.
2. 지적 3: `AWxWeaponBase::BeginAttack`과 `UWxAnimNotify_AreaDamage::Notify` 입구에서 공격자 캐릭터에 권위가 없으면 판정을 시작하지 않는다. 무기 자신이 아니라 소유 캐릭터로 판단한다(복제하지 않는 차일드 액터 무기는 각 머신이 로컬로 만들어 클라이언트에서도 자신이 권위다).
3. 사이드 이펙트 검토: 지적 1은 엔진이 재생을 동기로 거부할 때만 반환이 바뀌고, 실패 분기의 `EndAbility`는 이미 끝난 어빌리티라 무시된다. 취소 처리에서 끝내지 않는 `UWxAbility_Death`는 여전히 `true`라 동작이 같고, `UWxAbility_Groggy`는 자체 재정의라 영향이 없으며, `PlayMontage`는 BP에 노출되지 않는다. 지적 3은 무기 판정 상태를 무기 밖에서 읽는 곳이 없고 판정 결과의 유일한 소비처 `ApplyDamage`가 이미 비권위 출처를 거절하므로, 단독·리슨 호스트·데디 서버 동작은 같고 클라이언트의 버려지던 쿼리만 사라진다. 클라이언트에서 범위 피해 타게팅의 `ts.debug` 드로잉이 더는 보이지 않는다.
4. 검증 방법: WxEditor Development 빌드. `LV_DevCombat`을 `-game -nullrhi`로 띄운 임시 자동화 테스트로 (가) 적 HitReact의 KnockUp이 정상 재생 때는 띄우고 AnimInstance가 없어 동기 실패할 때는 띄우지 않고 어빌리티가 끝나 있는지, (나) 적의 로컬 역할을 비권위로 바꿨을 때 WeaponAttack 구간이 판정 형상·틱을 켜지 않고 AreaDamage가 타게팅 요청을 만들지 않으며, 권위 역할에서는 둘 다 지금처럼 동작하는지 확인한다. 수정 전 대조 실행으로 테스트가 결함을 잡는지 먼저 본다. 끝나면 리뷰 문서에서 해결된 지적을 정리한다.

## 테스트 체크리스트

| 항목 | 확인 방법 | 담당 | 결과 | 근거 |
| --- | --- | --- | --- | --- |
| Editor Development 빌드 | 임시 테스트를 지우고 WxGame.Build.cs 임시 의존(TargetingSystem)을 되돌린 뒤 build-doctor 실행 | AI | 통과 | build_2026-09-30_035647_225_34440.log: Succeeded(주석 정정 뒤 마지막 빌드). Source/WxGame/Tests 없음, WxGame.Build.cs는 HEAD와 차이 없음 |
| 몽타주 동기 재생 실패 시 HitReact | 헤드리스 게임(LV_DevCombat -game -nullrhi) 임시 자동화 테스트: BP_Template 적을 행동 트리 없는 기본 AI 컨트롤러로 스폰해 Event.Hit + HitReact.KnockUp을 보낸다. 정상 재생을 대조한 뒤 ASC 메시의 AnimInstance를 비워 재생을 동기로 실패시킨다. 정상이면 띄우고, 실패하면 어빌리티가 끝나 있고 띄우지 않는다. | AI | 통과 | 임시 Wx.Temp.ReviewFixes(확인 뒤 삭제), ReviewFixes_PostFix.log. 정상: 발동 1·종료 0·활성·AM_Shared_HitReact_Knockup 재생·띄우기 Z 640. 실패: 발동 1·종료 1·비활성·띄우기 Z 0. 수정 전 대조 실행(ReviewFixes_PreFix2.log)은 실패 경우에 끝난 어빌리티가 Z 640으로 띄워 이 항목만 실패 |
| 권위 없는 머신의 판정 생략 | 같은 테스트: 적의 로컬 역할을 SimulatedProxy로 바꿔 WeaponAttack 구간 시작·끝과 AreaDamage 노티파이를 직접 부르고, Authority로 되돌려 같은 호출을 반복한다. 비권위는 판정 형상·무기 틱·타게팅 요청이 없고, 권위는 지금처럼 켜졌다 구간 끝에 꺼지고 요청을 한 번 만든다. | AI | 통과 | 비권위: 무기 틱 0·형상 충돌 0·타게팅 요청 0. 권위: 무기 틱·형상 충돌 켜짐 뒤 구간 끝에 꺼짐, 타게팅 요청 1. 수정 전 대조는 비권위에서도 틱 1·충돌 1·요청 1로 실패. 실제 리슨 서버·클라이언트 PIE가 아니라 로컬 역할 전환으로 비권위를 만들었다 |
| 코드 리뷰 | 변경 파일: Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp(PlayMontageInternal 반환값), Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp(BeginAttack 권위 확인, ProcessHit 주석 정정), Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_AreaDamage.cpp(Notify 권위 확인, 주석 정정). 볼 점: 무기 자신이 아니라 소유 캐릭터의 권위로 가르는 기준, 실패 분기에서 이미 끝난 어빌리티에 다시 부르는 EndAbility | 사람 | 통과 | woogle 2026-09-29 |


## 사용자 테스트 결과 · 2026-09-29T19:05:18.180Z

<!-- test-feedback:request-e65021ac-83ad-4cea-a605-2cb6edea151e:submitted -->
- 전달한 사람: woogle

> 통과 · 코드 리뷰
