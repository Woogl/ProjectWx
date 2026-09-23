# Damage 파이프라인 구조 검토

- 상태: 1단계 사용자 플레이 확인 · 2단계 자체 검증/피니셔 수정 플레이 확인 · 3단계 단순화 회귀 통과 · 4단계 Hit 함수 분리 통과 · 네 인자 ApplyDamage 복원 빌드/회귀 통과
- 기준: 2026-09-23, HEAD `47b7f8bd7` 및 현재 작업 트리
- 범위: 결과 API, DamageRequest/호출자 출처 명시, 피해 행 1회 조회와 추가 효과 복사. Hit 내부 함수 책임 분리까지 진행한다. 이벤트 순서 재설계는 별도 범위다.

## 3단계 피해 행 조회 정리

- 사용자 판단(2026-09-23): “네, 고쳐진 것 확인했습니다. 다음 단계 작업도 이어서 진행해주세요.” 피니셔 피해 행 수정의 플레이 수용과 후속 진행 승인이다. 전체 멀티플레이 시나리오 수용으로 확대하지 않는다.
- 한 번의 타격에서 3회 조회하던 피해 행을 진입점에서 1회 조회한다. MakeHitSpec은 계수·공격 태그를 기존 Spec에 설정하고 추가 효과 목록만 Context에 복사한다. 별도 DamageDefinition과 유효 상태 플래그는 사용자 검토 후 제거했다.
- Context의 테이블/행 이름은 기존 네트워크 형식과 진단용 식별자로 유지하며 실행 중 조회하지 않는다. 추가 효과 목록은 로컬 전용이다. Duplicate는 목록을 보존하고 결과만 지우며 NetSerialize 수신은 목록과 결과를 지운다.
- 능력치 캡처, 추가 GE Spec 생성 시점, 방어 판정, 자원·이벤트 순서, BP 및 DataTable 저작 형식은 유지한다. 다음 요청은 최신 행을 다시 해석한다.
- 회귀 범위: 기존 GAS 자동화 + Spec 생성 후 원본 행 삭제에도 계수·추가 효과 유지, Context 복사, 다음 요청의 계수/가드/추가 효과 변경 반영. 빌드·실행 검증 통과.

## 2단계 판단과 구현

- 사용자 판단(2026-09-23): “테스트했는데 잘 됩니다. 다음 단계도 진행합시다”. 1단계 플레이 통과 보고와 다음 단계 진행 승인으로 기록한다. 구체 시나리오·네트워크 구성은 명시되지 않았으므로 모든 멀티플레이 검증을 수용한 것으로 확대하지 않는다.
- 이번 범위: 동기 C++ 입력 `FWxDamageRequest`와 `ApplyDamageRequest` 도입. 요청에는 출처 ASC/Instigator, Causer, Target, Ability, DamageLevel, 피해 행, HitResult를 담는다. raw UObject 포인터의 수명은 소유하지 않으며 요청을 보관하거나 지연 실행하지 않는다.
- 무기/범위 공격/피니셔는 기존의 적용 직전 AnimatingAbility 선택 시점을 유지하면서 요청을 만든다. 투사체는 현재 출처와 발사 레벨, Ability=nullptr을 직접 전달한다. 출처 ASC와 Instigator의 ASC가 어긋난 요청은 InvalidSource로 거부한다.
- 기존 BP 함수 이름/인자/반환값은 유지한다. 출처 추론과 구체 투사체 캐스트는 `Private/Damage/WxDamageCompatibility.cpp`의 호환 어댑터에만 남긴다. 새 처리 함수는 Causer의 Owner, 현재 몽타주, 구체 투사체 클래스를 조회하지 않는다.
- ATK/DEF 캡처 시점과 피해 행 저작 방식, 기존 이벤트 순서는 유지한다. 실행 정의 스냅샷과 DataTable 재조회 제거는 이 입력 API 단계에 포함하지 않는다.
- 검증: 기존 GAS 자동화에 명시 출처/Ability/레벨/HitResult, 추가 GE 레벨 전파, 요청 작성 후 변경한 ATK 사용, 출처 불일치 거부, 출처 교체와 저장 레벨 유지를 추가하고 통과했다.

## 2단계 검증

- WxEditor / Win64 / Development 빌드 성공(exit 0). [빌드 로그](../../../Saved/Logs/BuildDoctor/build_2026-09-23_114034_133_36840.log). 다른 빌드가 끝난 뒤 기존 실행기 잠금에 따라 실행했으며, 다른 작업의 코드는 수정하지 않았다.
- `Wx.Combat.Damage.Result`: 1건 성공, 오류 0, 경고 0, 프로세스 exit 0. [결과](../../../Saved/Automation/DamageRequest/index.json) · [실행 로그](../../../Saved/Logs/DamageRequestAutomation.log). 1단계 회귀와 위 요청 계약을 같은 실제 GAS 경로로 검증했다.
- NullRHI의 별도 UnrealEditor-Cmd 테스트다. 실제 무기/범위 공격/피니셔 몽타주, 투사체 궤적 및 멀티플레이 연출을 직접 플레이한 결과로 확대하지 않는다. 다음 플레이 확인은 네 가지 호출부의 적중과 투사체 반사 후 피해 출처/레벨이다.
- Wiki 순정 lint: 0 critical / 0 warnings / 0 suggestions. 변경 범위 diff 공백 검사 통과. 명시적 요청 계약은 기존 combat-damage 기사에 새 불변 원자료를 근거로 통합했다.

## 1단계 확정 범위와 구현

- 사용자 판단(2026-09-23): “네. 일단 그 부분 먼저 진행해주세요.” 직전 제안인 기존 계산·이벤트 순서를 유지한 결과 타입 도입과 투사체 중복 판정 제거를 승인했다. 전체 파이프라인 재설계나 이벤트 발행 시점 변경을 승인한 것으로 확대하지 않는다.
- `FWxDamageResult`와 BlueprintCallable `ApplyDamageWithResult`를 추가했다. 기존 `ApplyDamage`의 이름·인자·bool 반환은 호환 래퍼로 보존했다.
- 결과에는 적용 여부, 방어 종류, 실패 단계, 계산 피해/반사량, 반사 실행 여부, 크리티컬/가드 브레이크를 담는다. 실제 자원 순변화량은 포함하지 않는다.
- Hit 컴포넌트는 이벤트 전 방어 결과를 확보하고 추가 효과가 끝나도 원래 타격 결과를 반환한다. Context 내부 브리지는 유지하며 Duplicate/역직렬화에서는 결과를 초기화한다.
- 투사체의 서버 회피/퍼펙트 가드 재판정을 없앴다. 클라이언트 FX 사전 조회와 투사체 자체 반사 설정은 유지한다. 서버 Overlap ImpactFX만 확정 결과가 나오는 피해 호출 뒤로 이동한다. Hit 내부 계산·자원·이벤트·추가 효과 순서는 그대로다.
- 1단계 당시 후속 범위였던 요청 타입은 위 2단계에서 반영했다. 실제 자원 변화 수집, Hit 책임 분리, 상태 전이 시점 변경은 남아 있다.

## 1단계 검증

- UHT와 WxCombat(새 자동화 테스트 포함) 컴파일·DLL 링크 통과.
- 전체 WxEditor / Win64 / Development 빌드 성공(exit 0). [빌드 로그](../../../Saved/Logs/BuildDoctor/build_2026-09-23_112649_606_19708.log). 첫 빌드는 다른 작업의 인벤토리 헤더가 빌드 도중 변경되어 생성 코드 불일치로 실패했으며, 현재 상태로 재빌드해 통과했다. 그 코드는 이 작업에서 수정하지 않았다.
- `Wx.Combat.Damage.Result` 자동화 1건 성공(exit 0). 실제 GAS 경로의 일반 피해/가드/회피/퍼펙트 가드, 이벤트의 방어 태그 제거, 0 반사, Context 복사, 기존 bool 계약을 확인했다. [테스트 결과](../../../Saved/Automation/DamageResult/index.json) · [실행 로그](../../../Saved/Logs/DamageResultAutomation.log).
- 테스트는 별도 UnrealEditor-Cmd / NullRHI / Entry 맵에서 수행했다. 시작 로그의 기존 SourceControl 경로 경고와 기존 에디터가 점유한 HTTP 8000 포트 메시지는 테스트 결과와 구분한다. 테스트 이벤트에는 오류·경고가 없으며, 실제 투사체 궤적·멀티플레이 연출·사망/그로기 Ability 조합 전체 검증은 아니다.
- 순정 Wiki lint: 0 critical / 0 warnings / 0 suggestions. 문서 링크 41개 검사 오류 0, 뷰어 갱신 및 변경 범위 diff 공백 검사 통과.
- 플레이 확인: 회피 시 통과, 퍼펙트 가드 반사/반사 불가 시 파괴, 일반 가드·가드 불가 공격, 서버 ImpactFX가 피해 호출 뒤로 이동한 연출, 리슨 서버/원격 클라이언트 동작.

빌드 재실행: `powershell -NoProfile -ExecutionPolicy Bypass -File .agents/skills/build-doctor/scripts/Invoke-WxEditorBuild.ps1 -ProjectRoot C:/Wx`.

## 현재 흐름

무기/투사체/AreaDamage/Finisher의 네 인자 호출 → `UWxCombatLibrary::ApplyDamage`의 출처/레벨 선택 → DamageRow의 Hit Spec 생성 → Hit GE의 권한·적대·사망 검사 → Hit 컴포넌트의 회피·방어 판정 → Damage GE/ExecCalc → SP·IncomingDamage(HP)·GP 반영 → DamageResponse의 실행 기록 수집 → Hit 컴포넌트의 Cue·반응·반사·추가 GE → 호출자의 히트스톱·투사체 반사/파괴. Blueprint도 같은 네 인자의 ApplyDamage를 사용한다. 요청 구조체와 별도 어댑터는 제거했다.

AttributeSet은 자원 반영 도중 사망·그로기 이벤트를 발행한다. DamageFloater는 DamageResponse에서, Hit/PerfectGuard Cue는 Hit 컴포넌트에서 발행한다.

## 개선 후보와 근거

### 1. 1단계 반영: 적용 결과 계약을 명시한다

- 근거: `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp:90`, `Public/Damage/WxHitEffectContext.h:37`(이하 동일 모듈), `Private/Weapon/WxProjectileBase.cpp:134`, `:175`.
- 기존 문제: bool은 자식 Damage GE 적용 성공만 뜻해 투사체가 회피·퍼펙트 가드를 재판정했다.
- 반영: 결과 타입과 서버 소비 경로를 추가했다. 기존 bool 의미는 유지한다. 실제 자원 변화량 수집은 이벤트·추가 효과의 영향을 분리해야 하므로 후속 단계로 남긴다.
- 의미를 보존할 조건: 현재 히트스톱은 양수 피해가 아니라 GE 적용 성공에 종속된다. 0 피해 때의 정책을 임의 변경하지 않는다.

### 2. 높은 우선순위: 공유 Context의 입력·중간 상태·반환값을 분리한다

- 근거: `Public/Damage/WxHitEffectContext.h:14`, `Private/Damage/WxHitEffectContext.cpp:19`, `Private/AbilitySystem/Effect/WxEffectComponent_Hit.cpp:58`, `Private/AbilitySystem/Effect/WxEffectComponent_DamageResponse.cpp:21`.
- Context는 방어 판정, 추가 효과 목록, 실행 태그와 동기 반환 Result를 담는다. 미사용 DataTable 참조와 중복 수치는 후속 정리에서 제거했다. Duplicate/복제에서는 결과를 지우지만 LinkedSpec에서는 같은 Context를 의도적으로 공유한다. 호출 순서와 복사 방식이 결과 전달 계약의 일부가 된다.
- 제안: 요청/판정/결과의 타입과 수명을 구분한다. GAS Context는 출처·HitResult·필요한 복제 정보에 집중시키고, 로컬 실행 결과 전달은 좁은 경계로 캡슐화한다. 우선 기존 브리지를 유지하되 후속 이벤트 전에 불변 결과를 추출하는 점진적 변경이 가능하다.
- 현재 재진입 오염이 재현됐다는 결론은 아니다. 현재의 로컬 값 복사와 Reset 처리도 방어 장치다.

### 3. 높은 우선순위: 자원 반영과 상태 전이의 순서를 명시한다

- 근거: `Private/AbilitySystem/Effect/WxEffect_Damage.cpp:191`, `:204`, `Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp:98`, `:110`, `:126`, `Private/AbilitySystem/Effect/WxEffectComponent_Hit.cpp:134`.
- SP→HP→GP 모디파이어 순서가 이벤트 순서를 결정한다. HP 반영 중 사망 Ability가 활성화되고, GP 반영 중 그로기가 가드를 취소할 수 있어 후속 반응이 현재 Ability.Guard를 다시 확인한다. 전투 우선순위가 여러 콜백에 분산되어 있다.
- 제안: 먼저 현재 순서를 명시적인 단계로 묶고 전이 우선순위를 한곳에서 정의한다. 이후 자원 반영 후 상태 전이/반응을 확정하는 구조를 검토한다. Attribute delegate 등 외부 관찰자까지 자동으로 원자화되는 것은 아니므로 단순 이벤트 지연만으로 해결됐다고 보지 않는다.
- 변경 위험: 사망 Ability의 취소 효과와 태그 부여 시점이 바뀐다. Death/Groggy/GuardBreak 동시 조건을 실행 검증해야 한다. 직접 AddGP 및 치트 IncomingDamage 경로도 전이 처리를 유지해야 한다.

### 4. 부분 반영: Hit 컴포넌트의 판정·준비·반응 함수 분리

- 근거: `Private/AbilitySystem/Effect/WxEffectComponent_Hit.cpp:24`, `:112`, `:161`, `Private/AbilitySystem/Effect/WxEffectComponent_DamageResponse.cpp:31`.
- Hit 컴포넌트가 방어 판정, 자식 Spec 구성, 추가 효과 캡처, 가드 취소, 피해 이벤트, 반사 GP, 패리, Cue까지 조율한다. DamageResponse 역시 결과 수집과 플로터 연출을 겸한다.
- 제안: 동일 모듈 안에서 방어 판정, 계산 입력 수집/계산, 자원 반영, 결과 기반 반응 발행의 경계를 만든다. DamageResponse는 GAS 결과 수집 어댑터로 좁히고 Cue/이벤트는 확정 결과를 소비하게 한다. 단계별 UObject나 플러그인을 반드시 만들 필요는 없다.
- 계산은 현재도 작은 정적 함수로 나뉘어 있다. 이를 입력 값과 주입된 크리티컬 판정으로 평가 가능한 함수로 묶으면 GAS 없이 수치 조합을 검증하기 쉬워진다.

### 5. 2·3단계 반영: 피해 출처 명시와 공격 정의 해석

- 근거: `Private/WxCombatLibrary.cpp:36`, `:47`, `:60`, `Private/Damage/WxDamageTableRow.cpp:9`, `Private/AbilitySystem/Effect/WxEffectComponent_Hit.cpp:19`, `:46`, `:65`.
- 반영: 네이티브 호출자는 출처·레벨·피해 행을 명시한 요청을 구성한다. 새 처리 함수의 구체 투사체 캐스트와 AnimatingAbility 추론은 제거했으며 기존 BP 호환 어댑터에만 옛 추론을 남겼다.
- 3단계 반영: MakeHitSpec에서 Spec의 태그/SetByCaller를 설정하고 추가 효과 목록을 Context에 복사한다. Hit 내부 DataTable 재조회는 제거했다. 캐릭터 없는 환경 피해 등은 필요가 확정될 때 명시 정책을 추가한다.
- 발사 레벨 고정과 ATK/DEF 캡처 시점은 별개다. 요청 구조 도입이 발사 시점 전체 능력치 스냅샷을 의미하지 않는다.

## 권장 순서와 수용 조건

1. 결과 타입 도입 → 투사체의 서버 재판정 제거. 기존 bool API는 필요하면 호환 래퍼로 유지.
2. 요청 타입/호출자 출처 명시와 실행 정의 스냅샷, DataTable 재조회 제거까지 반영.
3. Hit의 방어 판정·Spec 준비·결과 반응을 기존 클래스 내부 함수로 분리 완료. DamageResponse의 플로터와 상태 전이 시점은 유지.
4. 마지막으로 전이 발행 시점 개선. 다음 동작을 회귀 검증한 뒤 채택.

- 회피: DodgeSuccess만 발생하고 피해/추가 효과/히트스톱은 발생하지 않는다.
- 퍼펙트 가드: 반사량 0도 결과가 유지된다. CanParry는 GP 반사가 아닌 반응만 제어한다.
- 동시 임계: 치명타격의 사망 우선, 그로기로 가드가 끊긴 경우의 반응 선택을 보존한다.
- 추가 효과: 현재는 0 피해라도 Damage GE 적용 성공이고 퍼펙트 가드가 아니면 적용한다. Source 태그는 피해/반응 전에 캡처한다.
- 표시량: 계산 피해량과 남은 HP에 제한된 실제 HP 감소량을 혼동하지 않는다.
- 출처: 투사체 발사 레벨, 반사 후 원인 교체, 치트/직접 자원 GE의 기존 처리를 검증한다.

반영한 결과 API·투사체 계약은 `.wiki/wiki/concepts/combat-damage.md`에 새 코드 출처와 함께 통합했다. 나머지 미확정 구조 제안은 이 Task에만 유지한다.

## 2026-09-23 피니셔 피해 누락 수정

- 사용자 증상: 몽타주 재생, HP 감소 없음. 런타임 로그에서 DT_Damage.AM_Finisher 조회 실패를 확인했다.
- GA_Shared_Finisher의 FinisherVariant/BackstabVariant가 모두 없는 행을 참조했다. 두 DamageDataRow를 실제 AM_Shared_Finisher로 수정했다. C++ 변경 없음.
- Blueprint 컴파일·저장 및 새 프로세스 재조회로 두 참조와 행 존재 확인. 저장 명령은 기존 SourceControl 경로 오류로 exit 1이지만 저장 성공 후 재조회는 exit 0이다. Saved/Logs/FinisherFix.log, FinisherVerification.log 참고.
- 사용자 플레이로 HP 감소 수정 확인 완료. 합성 DataTable 기반 GAS 자동화는 저작 에셋 참조 오류를 검출하지 못했다.

## 3단계 검증 결과

- WxEditor / Win64 / Development 빌드 성공(exit 0). [빌드 로그](../../../Saved/Logs/BuildDoctor/build_2026-09-23_121803_352_24728.log). 기존 빌드 잠금 해제 후 UHT, C++ 컴파일, DLL 링크 통과.
- Wx.Combat.Damage.Result: 1건 성공, 오류 0, 경고 0, 프로세스 exit 0. [결과](../../../Saved/Automation/DamageDefinition/index.json) · [로그](../../../Saved/Logs/DamageDefinitionAutomation.log). 기존 GAS 회귀와 행 삭제/변경 시 스냅샷 계약을 확인했다.
- Wiki lint 0 findings, 문서 링크 오류 0, diff 공백 검사 통과. 실제 플레이·멀티플레이 연출 검증은 별도이며 이번 변경의 사용자 플레이 수용은 대기한다.

## 3단계 사용자 검토 후 단순화

- 사용자 우려: DamageDefinition으로 구조가 복잡해지고 있다. 제안한 제거 방향에 “네”로 승인했다.
- 별도 타입과 bHasDefinition을 제거했다. 계수·태그의 중복 보관 없이 기존 Spec을 사용하고 Context에는 추가 효과 목록만 복사한다. 행 1회 조회와 기존 전투 동작은 유지한다.
- 기존 자동화에서 별도 정의 플래그 검사를 제거하고 실제 피해·추가 효과·Context 복사 검사는 유지했다. WxCombat 컴파일·링크와 GAS 회귀는 통과했다. 전체 Editor 빌드는 아래 외부 모듈 오류로 실패했다.
- 단순화 검증: [빌드 로그](../../../Saved/Logs/BuildDoctor/build_2026-09-23_122954_647_37660.log)에서 WxCombat 컴파일/DLL 링크 통과. 전체 빌드는 WxDataTableRowRename의 FPropertyBindingBindableStructDescriptor 소멸자 LNK2019로 exit 6. 해당 모듈은 이번 변경 범위 밖이며 수정하지 않았다.
- 생성된 DLL로 Wx.Combat.Damage.Result 재실행 성공(exit 0), 1건 성공/오류 0/경고 0. [결과](../../../Saved/Automation/DamageSimplification/index.json) · [로그](../../../Saved/Logs/DamageSimplificationAutomation.log).
- Wiki lint와 링크 검사, 변경 범위 공백 검사 통과. 실제 플레이는 별도 확인 대상이다.

## 4단계 Hit 함수 책임 분리

- 사용자 요청: “다음 단계도 진행해주세요”, “끝까지 자동으로 작업 진행하세요.” 기존 동작을 유지하는 함수 분리와 자체 검증·기록까지 진행한다. 이전의 구조 단순화 방향을 유지한다.
- EvaluateDefense는 방어 판정만, PrepareDamageSpecs는 피해·추가 효과 Spec 준비, ProcessHitReactions는 확정 결과 기반 Cue/반응을 담당한다. OnGameplayEffectApplied는 실행 순서와 결과 보존을 조율한다. 새 타입·클래스·파일은 추가하지 않는다.
- 자원·이벤트 순서와 DamageResponse의 플로터 시점은 유지한다. HP → 피격 → 가해 → 추가 효과 순서, 추가 효과의 반응 전 출처 태그 캡처, 0 피해의 추가 효과, 회피/퍼펙트 가드/피해 거부의 추가 효과 생략을 기존 GAS 회귀에 추가했다.
- 첫 빌드는 다른 작업의 Quest/QuestObjective 헤더 이동 중 동명 헤더 충돌로 UHT 실패. 해당 파일을 수정하지 않고 현재 상태를 확인해 재시도한다.

## 4단계 검증 결과

- 전체 WxEditor / Win64 / Development 빌드 성공(exit 0). [로그](../../../Saved/Logs/BuildDoctor/build_2026-09-23_123627_200_29544.log). 다른 작업의 헤더 이동 완료 후 재실행했으며 해당 파일을 수정하지 않았다.
- Wx.Combat.Damage.Result: 1건 성공, 테스트 오류 0/경고 0, 프로세스 exit 0. [결과](../../../Saved/Automation/HitResponsibilities/index.json) · [로그](../../../Saved/Logs/HitResponsibilitiesAutomation.log). 기존 피해·방어·출처·행 복사 회귀와 추가한 이벤트 순서·태그 캡처·추가 효과 분기 검사를 통과했다.
- 테스트 시작 중 기존 SourceControl 경로 경고와 HTTP 8000 포트 점유 오류는 테스트 실패와 구분한다. NullRHI / Entry 맵에서 실제 GAS로 검사했으며 실제 몽타주·멀티플레이 연출 수용으로 확대하지 않는다.
- Wiki lint 0 findings, 링크 오류 0, 변경 범위 diff 공백 검사 통과. 이번 함수 분리의 실제 플레이 수용은 아직 없으며, DamageResponse 연출 이동과 사망/그로기 이벤트 지연은 이번 범위에 포함하지 않았다.

## 불필요한 Context 데이터 정리

- 사용자 요청: “불필요한 기계장치가 있으면 덜어주세요.” Source/Plugins 사용처를 확인해 미사용 저장·복제 데이터와 중복 수치를 제거한다.
- Context의 DamageTable/DamageRowName, 생성자 행 인자, 테이블 객체 매핑·행 이름 직렬화 제거. 실제 피해 행 조회/오류 진단은 ApplyDamageRequest에 유지한다.
- DamageMagnitude/ReflectMagnitude/bHasReflect의 중간 필드를 제거하고 DamageResponse가 Result에 직접 기록한다. Hit는 성공 후 지역 값으로 보존하며 실패 및 추가 효과 종료 결과 복원은 유지한다.
- 네트워크 형식 변경으로 이전 빌드와 비트 호환되지 않는다. 같은 빌드의 서버/클라이언트를 사용해야 한다. 기본 Context·방어 비트 왕복과 수신 시 로컬 결과 초기화 검사를 추가했다.
- 방어 플래그, 추가 효과 목록, 실행 태그 스냅샷은 실제 소비/수명 차이가 있으므로 유지한다. 빌드/회귀 검증 통과.
- 정리 검증: 전체 WxEditor Development 빌드 성공(exit 0). [로그](../../../Saved/Logs/BuildDoctor/build_2026-09-23_125525_742_43328.log).
- Wx.Combat.Damage.Result 자동화 성공: 1건, 오류 0/경고 0, exit 0. 기존 피해·방어·추가 효과·처리 순서 회귀와 Context 직렬화 왕복 검사를 통과했다. [결과](../../../Saved/Automation/DamageContextCleanup/index.json) · [로그](../../../Saved/Logs/DamageContextCleanupAutomation.log).
- Wiki lint 0 findings, 링크 오류 0, diff 공백 검사 통과. 직렬화 검사는 객체 매핑 없는 기본 Context 원점과 방어 비트를 대상으로 하며 실제 네트워크 세션 테스트는 아니다.

## 결과 필드 평탄화

- 사용자 요청: EWxDamageDefense/EWxDamageRejection을 제거하고 FWxDamageResult를 평탄화. bApplied/bEvaded/bGuarded/bPerfectGuard 직접 필드 전환에 동의했다.
- 두 enum 및 Defense/Rejection 제거. 상세 거부 원인 반환은 제거하되 모든 기존 입력·권한·출처 검증은 유지한다. 일반 실패는 bApplied=false, 회피는 추가로 bEvaded=true다. 일반 가드와 퍼펙트 가드는 배타적이다.
- Hit 판정, 투사체 소비, 기존 자동화를 새 필드로 전환했다. 기존 피해/반사 수치와 추가 효과 순서, bool ApplyDamage 반환은 유지한다.
- Content uasset 문자열 검색에서 제거된 enum/ApplyDamageWithResult 참조는 없었다. 외부 BP 사용처의 제거된 핀은 직접 전환이 필요하며 전체 BP 실행 검증으로 확대하지 않는다.
- 전체 WxEditor Development 빌드 성공(exit 0). [로그](../../../Saved/Logs/BuildDoctor/build_2026-09-23_130456_182_44324.log).
- Wx.Combat.Damage.Result: 1건 성공, 오류 0/경고 0, exit 0. 평탄한 방어 필드의 배타성 및 기존 피해·추가 효과·순서·직렬화 회귀를 통과했다. [결과](../../../Saved/Automation/DamageResultFlat/index.json) · [로그](../../../Saved/Logs/DamageResultFlatAutomation.log).
- Wiki lint/link 및 diff 공백 검사 통과. 실제 플레이·멀티플레이 연출 확인은 별도다.

## ApplyDamage API 통합

- 사용자 요청: ApplyDamage와 ApplyDamageWithResult 통합. FWxDamageResult를 반환하는 ApplyDamage 하나로 합쳤다. bool 래퍼와 WithResult 함수는 제거했다.
- 네이티브 명시 입력 함수 ApplyDamageRequest와 출처 추론/피해 동작은 유지한다. 기존 테스트의 bool 사용은 .bApplied로 전환했다.
- C++ 호출부는 테스트뿐이었고 Content uasset 문자열 검색에서 두 함수 이름 참조는 없었다. 외부 BP에서는 bool 핀을 결과의 bApplied로 전환해야 한다. 검색이 전체 BP 실행 검증은 아니다.
- 중간 형태는 아래 최종 단일 요청 진입점으로 대체했다.

## 단일 요청 진입점으로 최종 통합

- 사용자 추가 지시: ApplyDamageRequest도 없애고 진입점을 하나로. ApplyDamage(const FWxDamageRequest&) 하나만 제공하며 결과는 FWxDamageResult다. 바로 위 두 함수 통합의 중간 구현을 대체한다.
- 요청을 BlueprintType으로 노출하고 UPROPERTY/TObjectPtr로 Blueprint 입력을 지원한다. Target/SourceAbility의 const는 유지한다. 네이티브 4개 호출자와 테스트 모두 같은 함수를 호출한다.
- 출처 추론 어댑터 파일과 기존 다중 인자 함수 제거. 피해 계산과 호출자의 출처·레벨 선택은 유지한다. 기존 Blueprint 노드 사용처가 있다면 Request 구성으로 전환해야 한다.
- 중간 구현 빌드 성공: build_2026-09-23_131928_394_32040.log. 최종 단일 진입점은 아래 별도 재빌드/회귀 검증을 통과했다.
- 최종 WxEditor Development 빌드 성공(exit 0). [로그](../../../Saved/Logs/BuildDoctor/build_2026-09-23_132548_218_29320.log).
- 초기 테스트는 다른 작업의 DataTableRowFixup 모듈 누락으로 엔진 시작 중 종료했다. 프로젝트 설정 변경 없이 재실행 인자 -DisablePlugins=DataTableRowFixup으로 해당 편집기 플러그인만 제외했다.
- 재실행에서 Wx.Combat.Damage.Result 1건 성공, 오류 0/경고 0, exit 0. Python 반영 API로 요청 생성/ApplyDamage 호출 및 옛 두 함수 부재 확인(WX_SINGLE_DAMAGE_API_VERIFIED). [결과](../../../Saved/Automation/DamageSingleEntry/index.json) · [로그](../../../Saved/Logs/DamageSingleEntryRetryAutomation.log).
- Wiki lint 0 findings, 링크 오류 0, diff 공백 검사 통과. 실제 Blueprint 그래프·멀티플레이 플레이 검증은 별도다.

## 근본 구조 재설계안 (승인·구현 완료)

- 사용자 요청(2026-09-23): “근본적인 대미지 파이프라인 전체를 점검하고 구조 개선 방향을 설계해주세요.” 코드 변경 없이 설계만 제시했다.
- 진단: 복잡도의 근원은 반응 처리가 결과를 모르는 위치(Hit GE의 OnGameplayEffectApplied)에 있는 것이다. 그래서 결과가 Damage GE에서 Context를 거쳐 거꾸로 올라와야 하고, 이 역방향 통로가 Context 서브클래스·Result 수치·DamageResultTags·Reset/Duplicate/NetSerialize 규칙을 만든다. 메모리의 “중첩·Context 경유는 필연”은 바깥 층이 GE이고 반응이 Hit에 있다는 전제에서만 성립한다.
- 제안 흐름: ApplyDamage(판정·조율) → Damage GE 1개(ExecCalc) → DamageResponse(OnGameplayEffectExecuted: 실행 기록을 가진 자리에서 Cue·반응·반사) → ApplyDamage가 추가 효과 적용·결과 반환. 방어 판정은 ApplyDamage가 한 번 내리고 Damage Spec의 동적 태그로 앞으로만 전달한다.
- 제거 대상: UWxEffect_Hit, UWxEffectComponent_Hit, FWxHitEffectContext(NetSerialize·Duplicate·TStructOpsTypeTraits 포함), Result의 DamageMagnitude/ReflectMagnitude/bHasReflect/bCritical/bGuardBreak, DamageTargetTags(GC_Hit은 데이터 전용 BP이고 WxCueNotify_Hit은 AggregatedTargetTags를 읽지 않음).
- 유지: ApplyDamage(Causer, Target, 행, HitResult) 시그니처와 출처·레벨 추론(사용자가 FWxDamageRequest를 제거함, 2026-09-23), DataTable 행, IncomingDamage/IncomingReflect 메타 속성, AttributeSet의 Death/Groggy 발행(치트·AddGP 경로 공유), 호출부의 히트스톱·투사체 반사.
- 확인한 엔진 사실(UE 5.8): ApplyGameplayEffectSpecToSelf가 함수 전체에 FScopedActiveGameplayEffectLock을 잡고, ExecuteActiveEffectsFrom은 모디파이어·조건부 GE·Cue 다음에 OnExecuted를 호출한다(GameplayEffect.cpp:3369). OnApplied와 적용 델리게이트는 그 뒤다(AbilitySystemComponent.cpp:1165~1175). 프로젝트에 적용 델리게이트/WaitGameplayEffectApplied 구독은 없다. UWxEffect_Hit는 MakeHitSpec에서만 생성된다. 무적 Immunity는 Damage GE를 대상으로 해 영향이 없다.
- 기각: ExecutionDefinition.ConditionalGameplayEffects로 추가 효과 대체(행별 목록 불가, 반응보다 먼저 적용되어 순서 변경). ExecCalc가 방어 판정(ApplyDamage가 결과를 되돌려 받아야 해 역방향 통로 재발생). 사망/그로기 전이를 DamageResponse로 이동(직접 자원 경로와 갈라짐).
- 순서 보존: HP→(사망/그로기)→플로터→Hit Cue→가드 취소→피격→가해→퍼펙트 가드→추가 효과→히트스톱. 추가 효과 Spec은 피해 전에 생성한다.
- 변화점: 추가 효과와 DodgeSuccess가 대상 AGE 잠금 밖에서 실행된다. 반응이 Damage GE의 OnApplied·적용 델리게이트보다 먼저 실행된다(현재 구독자 없음). 신규 태그 2개(방어 판정) 필요 또는 기존 태그 재사용 결정 필요. 테스트의 Context 직렬화/Duplicate 검사는 대상 자체가 사라진다.
- 단계안: A) 반응을 DamageResponse로 이동해 역방향 통로 제거(Hit GE 유지). B) Hit GE 제거·ApplyDamage 조율·판정 태그 전달·FWxHitEffectContext 삭제. 각 단계 빌드와 기존 GAS 회귀, 가드·퍼펙트 가드·회피·그로기 동시 임계 플레이 확인.

## 사용자 선호에 따른 네 인자 복원

- 사용자 승인: 이전 ApplyDamage 인자로 되돌리되 Result 반환과 단일 진입점을 유지한다. 출처/Ability/레벨 추론 복원을 승인했다.
- ApplyDamage(Causer, Target, DamageTableRow, HitResult) 하나로 통합하고 FWxDamageRequest를 삭제했다. 호출부 요청 조립과 중복 조회를 제거했다.
- 출처는 Causer ASC 우선, 없으면 직접 Owner ASC. 일반 공격은 현재 AnimatingAbility/레벨, 투사체는 Ability 없음/발사 레벨을 사용한다. 실제 템플릿 투사체로 Owner 교체 뒤 피해 출처/레벨 보존 검사를 추가했다.
- 빌드/회귀 검증 통과. 위 요청 구조체 기반 최종 형태는 이 사용자 지시로 대체한다.
- 검증: 전체 WxEditor Development 빌드 성공(exit 0). [로그](../../../Saved/Logs/BuildDoctor/build_2026-09-23_133538_467_43284.log).
- Wx.Combat.Damage.Result 1건 성공, 오류 0/경고 0, exit 0. 실제 템플릿 투사체의 Owner 출처, 누락 Owner 거부, 발사 레벨 7과 추가 효과 레벨, Owner 교체 뒤 새 출처/기존 레벨 유지 및 기존 피해 회귀 통과. 실제 반사 궤적/멀티플레이 플레이 검증은 아니다.
- Python 반영 API에서 네 인자 호출 성공 및 요청 타입/옛 함수 부재 확인(WX_FOUR_ARGUMENT_DAMAGE_API_VERIFIED). [결과](../../../Saved/Automation/DamageFourArguments/index.json) · [로그](../../../Saved/Logs/DamageFourArgumentsAutomation.log). 이번 실행은 플러그인 제외 없이 통과했다.
- Wiki lint/link, diff 공백 검사 통과.

## 정방향 흐름 구현

- 사용자 판단(2026-09-23): “네. 제안 구조: 결과가 앞으로만 흐르게로 합시다. 구조가 단순화되고 직관적이 되는 것을 저는 선호합니다.” A/B 단계를 한 번에 적용했다. 판정 태그는 제안대로 신규 `Damage.Guarded`/`Damage.PerfectGuarded`(WxCore)로 정했다.
- 구현: ApplyDamage가 권위·적대·사망 확인, 회피 이벤트, 방어 판정·Spec 태그 부착, 추가 효과 Spec 선생성, Damage GE 적용, 추가 효과 적용을 조율한다. `FWxDamageTableRow::MakeHitSpec` → `MakeDamageSpec`(Damage GE Spec 직접 생성). ExecCalc는 Spec 태그로 판정을 읽는다. DamageResponse가 플로터·Hit Cue·피격/가해·퍼펙트 가드 반응을 처리한다.
- 삭제: UWxEffect_Hit, UWxEffectComponent_Hit, FWxHitEffectContext(6개 파일). FWxDamageResult는 4개 bool. Hit Cue의 AggregatedTargetTags 전달 제거. 관련 주석(Guard·PerfectGuard Cue·IncomingReflect·Invincible) 정정.
- 테스트: Context 복사/직렬화 검사를 제거하고 수치 검사는 HP 변화량으로 전환, 판정 태그 전달·퍼펙트 가드 이벤트(반사 0 포함) 검사를 추가했다. 처리 순서(HP→Hit→Dealt→Extra)·출처 태그 캡처·추가 효과 분기 검사는 유지했다.
- 전체 WxEditor Development 빌드 성공(exit 0, BUILD_DOCTOR_RESULT=success).
- Wx.Combat.Damage.Result 1건 Success, 프로세스 exit 0. [결과](../../../Saved/Automation/DamageForwardFlow/index.json) · [로그](../../../Saved/Logs/DamageForwardFlowAutomation.log). NullRHI 별도 프로세스이며 -DisablePlugins=DataTableRowFixup 사용.
- Wiki: 원자료 2026-09-23-damage-forward-flow 추가, combat-damage 재편찬, combat 토픽 클래스 표 갱신. 링크 검사 이상 없음.
- 미검증: 실제 플레이(가드·퍼펙트 가드 반사·회피·그로기 동시 임계·투사체 반사), 리슨 서버/원격 클라 연출.

## 외부 참고 조사 (2026-09-23)

- 사용자 요청: ApplyDamage가 커져 “Effect 내부에서 순차 처리”를 검토하며 가드·회피가 있는 게임 기준 참고자료 조사를 요청했다.
- Lyra 5.7 원본(VaultCache): 팀 판정은 ExecCalc 배율(0/1), DamageImmunity는 HealthSet::PreGameplayEffectExecute에서 차단, 사망은 OnOutOfHealth 델리게이트→HealthComponent, 피격 알림은 GameplayMessage. 호출부는 결과를 받지 않는다. 가드/패리/회피 없음.
- GASDocumentation(tranek): 메타 속성 Damage를 PostGameplayEffectExecute에서 Health로 옮기며 같은 자리에서 피격 반응·플로터·현상금 처리.
- Vitor Cantão(RoR2식): 계산 GE(공격자) → Event.BeforeDamage로 보정 → 실행 GE(대상) → Event.Hit 반응. 다단 GE를 이벤트로 연결.
- Ninja Combat(상용): Defense 컴포넌트(블록·무적)와 Damage Manager 컴포넌트(피격 반응·사망 트리거) 분리. 문서 도메인 접속 불가로 세부 순서 미확인.
- 엔진 확인: 무적 Immunity가 GE를 막으면 ASC::OnImmunityBlockGameplayEffectDelegate가 방송된다(ImmunityGameplayEffectComponent.cpp:67).
- 공통점: 적용은 결과를 돌려받지 않는 fire-and-forget이고 후속은 GAS 내부 반응이다. 우리 ApplyDamage가 큰 주원인은 호출부(히트스톱·투사체 통과/반사)의 동기 결과 소비와 추가 효과 선캡처다. 결정 대기.

## 회피를 Immunity 통지로 전환

- 사용자 지시(2026-09-23): “OnImmunityBlockGameplayEffectDelegate 를 쓰는 방식으로 재구현합시다.”
- ApplyDamage의 무적 분기·Event.DodgeSuccess 발행 제거, Event.DodgeSuccess 태그 삭제(C++·Content 참조 없음). Dodge 어빌리티가 활성 동안 Immunity 차단 델리게이트를 구독해 Damage.Attack 차단을 극한 회피로 처리, 클라 확정은 기존 NetSync 유지.
- FWxDamageResult.bEvaded 제거. 투사체는 적용 전 무적 태그 조회로 통과를 정한다(클라 FX 조건과 공유).
- 테스트: 느슨한 태그 대신 실제 UWxEffect_Invincible GE를 적용해 Immunity 차단 통지 1회·HP 보존·반응 중 무적 제거를 검사.
- 빌드 성공(exit 0) [로그](../../../Saved/Logs/BuildDoctor/build_2026-09-23_140452_356_1544.log). Wx.Combat.Damage.Result Success, exit 0 [결과](../../../Saved/Automation/DamageImmunityDodge/index.json) · [로그](../../../Saved/Logs/DamageImmunityDodgeAutomation.log).
- 미검증: 실제 극한 회피 플레이(리슨 서버·원격 클라 NetSync 전환 포함).

## 결과 반환 제거와 Damage GE 컴포넌트 분할

- 사용자 판단(2026-09-23): ApplyDamage 인자 추가 거부, “Damage Effect 내부에서 Internal하게 작동하는 파이프라인용 Effect들로 쪼개기” 제안 → 컴포넌트 분할 + 히트스톱 원인 액터 조회(B) 승인 “네. B로 진행하세요.”
- DamageResponse 삭제 → UWxEffectComponent_DamageReaction / _PerfectGuard / _HitStop (Damage GE 생성자 추가 순서가 실행 순서). _PerfectGuard가 원인 투사체를 Reflect(bCanReflect 확인을 Reflect 안으로, public 전환), _HitStop이 무기/투사체 캐스트로 기존 값 적용. 투사체 히트스톱 프로퍼티 public 전환.
- FWxDamageResult 삭제, ApplyDamage void. 무기 ProcessHit는 호출만, 투사체는 무적 사전 조회 + 적용 후 Owner 확인으로 파괴 결정.
- 테스트: Damage GE 적용 관찰(OnGameplayEffectAppliedDelegateToSelf)로 적용·판정 태그 검사, 투사체 원인의 피격자 히트스톱·공격자 히트스톱 0 검사 추가. 투사체 되돌림은 테스트 액터가 Pawn이 아니라 자동화로 검증하지 못했다.
- 빌드 성공 [로그](../../../Saved/Logs/BuildDoctor/build_2026-09-23_142226_603_41948.log). Wx.Combat.Damage.Result Success, exit 0 [결과](../../../Saved/Automation/DamageEffectComponents/index.json) · [로그](../../../Saved/Logs/DamageEffectComponentsAutomation.log).
- 미검증 플레이: 무기/투사체 히트스톱 체감, 퍼펙트 가드 투사체 되돌림, 반사 불가 투사체 파괴.

## 파이프라인 리뷰 후속 수정 (2026-09-23)

- code-review(high) 10건 중 확인된 결함 3건·경미 4건. 사용자 승인: 1·6·7 수정, 2는 “테스트가 잘못된 것” 확인, 3은 Owner 기준 판정.
- Dodge: ListenForDodgeSuccess가 IsActive()일 때만 Immunity 차단을 구독(원격 회피 시작 실패 후 구독 잔류·다른 무적 차단에 극한 회피 오작동 방지).
- 테스트: Immunity 차단 통지 안에서 무적 GE를 제거하던 코드 제거. 엔진이 GameplayEffectApplicationQueries를 range-for로 도는 중 OnEffectRemoved가 즉시 발행돼 실행 중인 쿼리를 지우는 UB 경로였다. 실제 무적 제거는 ANS NotifyEnd(애님 업데이트)다.
- 투사체: 호출 전 Owner를 지역 변수로 두고 바뀌지 않았으면 파괴(충돌 액터와 ASC 아바타가 다를 때 되돌린 투사체 파괴 방지). 무적 사전 조회 주석 정정.
- MakeDamageSpec의 행 변경 대비 지역 복사·주석 제거. 테스트 메시지의 Applied flag 표현 정정.
- 빌드 성공 [로그](../../../Saved/Logs/BuildDoctor/build_2026-09-23_144915_549_34652.log), Wx.Combat.Damage.Result Success exit 0 [결과](../../../Saved/Automation/DamageReviewFixes/index.json).
- 대기: 행 삭제 스냅샷 테스트(로컬 Row 사용) 처리, 테스트 관찰 람다 수명 정리 방식.

## 리뷰 후속 2차 (2026-09-23)

- 사용자 판단: 행 삭제 스냅샷 테스트는 삭제(b), 테스트 관찰 람다는 “반환값으로 처리”, 테스트 세부는 위임. `_DamageReaction`→`_DamageResult` 개명은 결과 데이터로 읽혀 유지 권고.
- ApplyDamage가 Damage GE 적용 여부(bool) 반환. 테스트: 적용 관찰 람다(CountDamage/LastDamageTags) 제거, 판정은 SP·GP 변화로, 퍼펙트 가드 태그는 호출 뒤 테스트가 제거, 반사 0 이벤트 관찰만 남기고 사용 직후 해제.
- WxCombat 컴파일·DLL 링크 성공, Wx.Combat.Damage.Result Success exit 0 [결과](../../../Saved/Automation/DamageBoolReturn/index.json). 전체 빌드는 다른 작업의 WxGame(WxViewModel_BossDisplay·WxEngagedCharacterWidget) 컴파일 오류로 실패 [로그](../../../Saved/Logs/BuildDoctor/build_2026-09-23_151224_711_38936.log) — 해당 파일 미수정.

## 사망 확인을 Effect 요건으로 일원화 (2026-09-23)

- 사용자 판단: “사망 확인 같은 경우는 각 Effect 내부에서 조회하는게 더 바람직”. ApplyDamage의 Ability.Death 조기 반환 제거, Damage GE의 TargetTagRequirements(Ability.Death)만 남김. 근거: 사망 어빌리티가 Ability.* 전체를 Cancel/Block하므로 사망 대상에서 Dodge가 Immunity 통지를 받지 못한다.
- 적대 판정의 CanApply 컴포넌트 이관은 시도 후 되돌림: 엔진이 Immunity 쿼리를 CanApply보다 먼저 돌려, 아군 범위 공격·피니셔가 회피 무적 동료에게 극한 회피를 일으킨다(무기·투사체만 호출 전 적대를 거름). ApplyDamage에 사유 주석.
- 전체 WxEditor 빌드 성공, Wx.Combat.Damage.Result Success exit 0 [결과](../../../Saved/Automation/DamageDeathInEffect/index.json).
- 대기: B안(방어 판정 ExecCalc 이관 + 추가 효과 전용 입력 Context·_AdditionalEffects 컴포넌트) 결정.

## B안: 방어 판정·추가 효과를 Effect로 (2026-09-23)

- 사용자 판단: “네 B안 전체로 진행하세요.”
- ExecCalc가 Damage.CanGuard와 대상 태그(Effect.PerfectGuard/GuardReduction)로 방어 판정 후 Damage.Guarded/PerfectGuarded 결과 태그 부착. ApplyDamage 판정 코드 제거.
- 추가 효과: FWxDamageEffectContext(입력 전용, 서버 로컬, Duplicate 시 비움) 신설, MakeDamageSpec이 피해 전 Spec 생성(Context 사본 사용 → 순환 참조 없음), UWxEffectComponent_AdditionalEffects가 마지막에 퍼펙트 가드 아니면 적용(적용 후 목록 비움). ApplyDamage는 입력 조립 + 권위·적대 + 적용 반환(약 50줄).
- 전체 WxEditor 빌드 성공 [로그](../../../Saved/Logs/BuildDoctor/build_2026-09-23_154653_083_20156.log), Wx.Combat.Damage.Result Success exit 0 [결과](../../../Saved/Automation/DamageRulesInEffect/index.json). 기존 추가 효과 순서(HP→Hit→Dealt→Extra)·출처 태그 선캡처·퍼펙트 가드/회피/사망 생략 회귀 통과.
- 엔진 UAdditionalEffectsGameplayEffectComponent 대체 검토: GE 클래스 단위 정적 목록, RequiredSourceTags 조건만, OnGameplayEffectApplied에서 Spec 생성 → 행별 목록·퍼펙트 가드 생략·선캡처 불가. 사용자 결정 대기.

## 추가 효과 Spec 생성 시점을 반응 뒤로 (2026-09-23)

- 사용자 지시: “반응이 끝난 뒤에 만들어야해요.” 이전의 “피해 전 출처 태그 캡처” 불변식을 폐기.
- FWxDamageEffectContext는 추가 효과 GE 클래스 목록만 보관, _AdditionalEffects가 실행 시 같은 Context·Damage 레벨로 Spec 생성 후 적용. 사본 Context·순환 참조 대책 제거.
- 테스트 “Extra source tags captured after hit reaction”으로 전환. 전체 빌드 성공 [로그](../../../Saved/Logs/BuildDoctor/build_2026-09-23_155626_778_3916.log), Wx.Combat.Damage.Result Success exit 0 [결과](../../../Saved/Automation/DamageExtrasAfterReaction/index.json).
- 엔진 UAdditionalEffectsGameplayEffectComponent 대체의 남은 장애: 행별 목록(GE 클래스 단위 정적 목록), 퍼펙트 가드 생략(RequiredSourceTags만).

## 정리: Damage.Attack 제거·투사체 중복 판정·테스트 삭제 (2026-09-23)

- 사용자 지시: “1,2,3 정리하고 테스트 코드도 없애주세요.”
- Damage.Attack 태그 삭제: Damage GE는 MakeDamageSpec으로만 만들어져 항상 부착되던 무의미 조건(치트 피해는 별도 GE). _DamageReaction은 피해>0만, Dodge 필터는 막힌 Spec의 Def가 UWxEffect_Damage인지로 판정(쿨다운 면제 등 다른 Immunity 통지 배제).
- 투사체 회피 조건에서 중복 적대 판정·SourceASC 제거(핸들러 앞단이 이미 적대를 거름).
- Private/Tests/WxDamageResultTest.cpp(Wx.Combat.Damage.Result) 삭제 → 이후 자동 회귀 없음, 검증은 빌드와 플레이로.
- 전체 WxEditor 빌드 성공 [로그](../../../Saved/Logs/BuildDoctor/build_2026-09-23_161254_241_18748.log).
- 남은 과제: 플레이 검증, 상태 전이 순서(AttributeSet 모디파이어별 사망·그로기) 정리.

## 타 GAS 프로젝트 비교 후속: 0 피해 히트스톱·낡은 주석 (2026-09-23)

- 사용자 지시: “1, 2 적용해주세요” (Lyra·Action RPG·GASDocumentation·Aura·Ninja Combat 등과 비교한 개선 제안 중 1 주석 정정, 2 0 피해 히트스톱).
- `_HitStop`: 실행 기록 피해 > 0 또는 `Damage.PerfectGuarded`일 때만 건다(Hit Cue와 같은 조건). 이전에는 반올림 0·완전 경감 가드처럼 ExecCalc가 출력 없이 끝난 타격에도 연출 없이 히트스톱만 걸렸다. `_AdditionalEffects`는 피해 없는 디버프 행을 위해 0 피해에도 적용을 유지한다.
- 주석 정정: `WxCueNotify_Hit.h`·`WxCueNotify_DamageFloater.h`의 Hit Cue 예측 서술(실제는 `_DamageReaction`이 서버에서 빈 예측 키로 발행), `WxEffect_Invincible.cpp`의 ApplyDamage 회피 판정 전제(Damage GE만 막고 Dodge가 차단 통지를 받음), `WxWeaponBase.cpp` 적대 사전 검사의 낡은 이유 삭제(검사 자체는 ApplyDamage와 중복이며 유지).
- 전체 WxEditor 빌드 성공(경고 0) [로그](../../../Saved/Logs/BuildDoctor/build_2026-09-23_204153_209_33376.log). 플레이 미검증.
- 대기(기획 확인): 가드 방향(도입 시 `_DamageReaction`의 가드 취소 조건 `!CanGuard`도 함께 수정 — 반응 라우팅이 `Ability.Guard` 기준이라 등 뒤 피격에 흡수 몽타주가 나감), 공격별 그로기 파워, 그로기 중 피해 증가, 플로터 표시 대상·위치, 일반 가드의 추가 효과 통과, 커스텀 Context 할당 경로(`AllocGameplayEffectContext` 순정 경로 여부).
