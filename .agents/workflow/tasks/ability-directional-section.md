# 어빌리티 방향별 몽타주 섹션 공용화

상태: 확인 대기 · 테스트 코드 제거·주석 정정·Editor 빌드 통과
다음 행동: 공용 방향 재생·콤보 배열 코드를 리뷰하고, 단독·서버/소유 클라이언트 PIE에서 콤보 연결과 방향별 연출을 확인한다.

- 사용자 요청·구현 승인(2026-09-25): "UWxAbility_Dodge::SelectDodgeSection , EWxDodgeDirection 의 방향 선택이 Dodge 어빌리티 전용으로 개발이 되어있습니다. 몽타주 섹션 규칙만 잘 지키면 모든 WxAbilityBase에서 쓸 수 있도록 개선해주실 수 있을까요?"
- 결정: 8방향 열거형과 방향 판정·섹션 검색을 UWxAbilityBase로 옮긴다. C++ 함수로 제공하고 UFUNCTION 지정자는 사용하지 않는다. 공용 무입력 기본값은 Forward이며 호출자가 지정할 수 있다. Dodge는 Back을 지정해 기존 동작을 보존한다.
- 현재 범위: PlayMontage에서 공통 입력 수집·방향 동기화·방향 섹션 자동 선택을 처리한다. 콤보 단계는 ComboMontages 배열로 관리하고 단계 번호 섹션은 사용하지 않는다. 기존 Dodge의 명시 선택과 보정은 유지한다.
- 사용자 변경 지시(2026-09-25): "방향과 콤보 모두 다 섹션으로 관리하면 너무 어려울 것 같아요. 콤보는 예전처럼 여러개의 몽타주로 쪼개는게 낫겠습니다." 이전 번호 섹션 방식에서 몽타주 배열 방식으로 변경했다. 아래 이력의 번호 섹션 검증은 이전 구현의 결과다.
- 사용자 정리·제출 지시(2026-09-25): "테스트 코드 제거하고, 주석 오류 정정해주세요.", "끝나면 제출하세요." 이번 작업의 테스트 구현을 제거하며 기존 실행 결과는 검증 이력으로 유지한다. 다른 작업의 테스트는 제거 대상이 아니다.

## 테스트 체크리스트

방향·콤보 자동화 결과는 테스트 소스 제거 전 실행 이력이다. 제거 후에는 아래 빌드와 잔여 참조 검사를 다시 수행한다.

| 항목 | 확인 방법 | 담당 | 결과 | 근거 |
| --- | --- | --- | --- | --- |
| Editor Development 빌드 | 테스트 소스 제거 후 build-doctor 실행 및 로그 확인 | AI | 통과 | build_2026-09-25_234230_097_45824.log; UHT·29개 작업 완료, Succeeded·종료 코드 0 |
| 테스트 제거·주석 정정 | 이번 작업 테스트 파일·잔여 참조가 없고 주석 외 런타임 코드가 동일한지 확인 | AI | 통과 | 방향 테스트 소스 3개·임시 재로드 검증 스크립트 제거, 소스 참조 없음, 담당 16개 파일의 주석 제외 코드 해시 일치 |
| 8방향·경계·무입력 | UE 자동화 테스트 | AI | 통과 | AbilityDirection.Resolve |
| 공용 섹션 검색·폴백·접두사 분리 | UE 자동화 테스트 | AI | 통과 | AbilityDirection.Sections |
| 실제 Skill 활성화·방향 콤보 | 선택 함수를 호출하지 않는 Skill 발동·재발동으로 배열 단계·방향 선택·리셋 확인 | AI | 통과 | SkillPlayback·SkillCombo; 서로 다른 몽타주의 Right→Left, 순환·창 닫힘·완료·취소·빈 배열·null 단계 |
| 공격·AI 패턴의 몽타주 배열 | GAS 발동과 재발동·블렌드아웃·종료 콜백으로 다음 몽타주 확인 | AI | 통과 | AttackCombo·PatternCombo; 공격 재발동·방향 없는 단계·패턴 자동 진행·최종 종료·취소 후 첫 단 |
| 원격 방향 수신 수명 | 수신 지연·선도착 캐시·취소·요청 교체·실패 종료를 GAS로 검사 | AI | 통과 | AbilityDirection.RemotePlayback; GAS 수신 진입점 재현이며 실제 네트워크 전송·PIE와 별도 |
| 기존 회피 에셋 호환 | GA_Shared_Dodge 및 AM_Shared_Dodge 로드·섹션 선택 검사 | AI | 통과 | AbilityDirection.DodgeAsset |
| 기존 태그 차단 회귀 | Wx.Combat.AbilityBlocking 실행 | AI | 통과 | AssetDefaults·HookRules·Lifecycle 3개 |
| 콤보 에셋 이관·저장 후 재로드 | 단계별 몽타주 복원, GA 22개 배열 설정·컴파일·저장 후 별도 UE 프로세스에서 재검사 | AI | 통과 | ComboMontageMigration.json·ComboMontageVerification.json; 29개 원본 복원·총 길이/노티파이 종류와 수/블렌드 설정 일치·기타 GA 설정 유지 |
| 생성 목록의 콤보 표시 | Export-AbilitySystemLists.ps1 실행 및 배열 순서·참조 목록 확인 | AI | 통과 | 어빌리티 40개·사용 몽타주 54개, 콤보를 배열 순서의 화살표로 표시 |
| BP 함수 노출 제거 | UHT 생성 코드에 새 공용 함수 등록이 없는지 확인 | AI | 통과 | ResolveDirection·SelectDirectionalSection·HasMontageSection·PlayMontageInternal 등록 없음 |
| 코드 리뷰 | WxAbilityBase·Combo·Attack·Skill·Pattern·Dodge·Groggy·HitReact, Export-AbilitySystemLists.ps1: 배열 단계·방향 선택 책임·동기화 수명·기존 회피 기본값·주석 정정 확인 | 사람 | 대기 | |
| 콤보 에셋·연결 연출 | 에디터에서 GA의 ComboMontages 순서를 확인하고 HGTest·Template 공격 및 Soldier·Template 패턴의 단계 연결·블렌딩·콤보 창 종료·취소 후 첫 단 복귀를 플레이 | 사람 | 대기 | |
| 방향별 어빌리티 연출·네트워크 회귀 | 단독·서버/소유 클라이언트 PIE에서 방향 섹션을 둔 스킬·회피·반응과 무입력·극한 회피를 확인 | 사람 | 대기 | |

## 사용 규칙

- 기본 재생은 `Forward`를 두면 이동 입력에 맞춰 `ForwardRight`, `Right`, `BackRight`, `Back`, `BackLeft`, `Left`, `ForwardLeft`를 자동 선택한다. 무입력·누락 방향은 Forward다.
- 공격·스킬·패턴은 GA의 `Wx > Combo > ComboMontages` 배열에 몽타주를 단계 순서대로 지정한다. 한 단계도 배열에 하나를 넣는다. 각 몽타주 안의 방향 섹션은 `Forward`, `Right` 등으로 두며 번호 접두사는 쓰지 않는다. 콤보 타입의 단일 `AbilityMontage` 편집 항목은 숨긴다.
- `UWxAbility_Combo`가 배열과 단계 인덱스를 공유한다. 공격·스킬은 기존 콤보 창 재발동으로, 패턴은 블렌드아웃에서 다음 몽타주로 진행한다. `GetMontage()`와 인스턴스 방향 선택 함수는 현재 단계의 몽타주를 사용한다.
- 반응은 `GuardHitForward`, `NormalRight`처럼 기존 요청 섹션 이름을 접두사로 사용한다. 정확히 일치하는 기존 섹션이 있으면 그 섹션을 우선하므로 방향 변형을 쓰려면 기존 일반 섹션을 방향 이름으로 바꾼다.
- 방향별 섹션을 독립 재생하려면 다음 섹션 링크를 끊는다. 방향 섹션이 없는 기존 몽타주는 기존 시작 위치를 유지한다.
- 입력 수집·로컬 좌표 변환·첫 방향 재생의 TargetData 동기화는 공통 PlayMontage가 처리한다. 한 활성화 안에서는 같은 입력 방향을 사용하고, 콤보 재발동은 새 입력을 받는다. 임의의 몸 회전 보정은 하지 않는다.
- 명시적으로 방향을 고르는 Dodge는 기존 Backstep·후방 기본값·루트모션 보정·극한 회피를 유지한다. 새 함수는 C++ 전용이다.

## 조사·구현 이력

### 2026-09-25

- 프로젝트 작업 절차와 Wiki 색인을 읽고 Export-AbilitySystemLists.ps1을 실행했다. 생성 목록 기준 회피는 GA_Shared_Dodge의 AM_Shared_Dodge를 사용한다.
- EWxDodgeDirection은 BlueprintType이지만 Content의 직렬화 참조와 Config 참조는 검색되지 않았다. 항목명과 순서를 보존하고 열거형 이름만 공용으로 변경한다.
- 기존 UWxAbilityBase·Dodge의 미커밋 태그 차단 변경을 보존하고 방향 선택 부분만 수정한다.
- 회피 무입력의 Backstep 우선 사용, Back 방향 대체, Success 접두사 범위 내 Forward 폴백을 보존한다.
- 최초 구현에서 Editor Development 빌드와 Wx.Combat.AbilityDirection 자동화 테스트 3개(Resolve·Sections·DodgeAsset)가 통과했다. 이후 UFUNCTION 제거 뒤 재빌드·같은 3개 테스트도 모두 통과했다.
- 사용자 추가 지시(2026-09-25): "새로 추가한 함수를 BP에서 쓸 일이 없어서 UFUNCTION 지정자 빼주세요." ResolveDirection·SelectDirectionalSection의 지정자를 제거했다.
- 사용자 수정 요청(2026-09-25): "코드 위치는 잘 옮기셨는데, 정작 Dodge 어빌리티에서만 쓸 수 있는 점은 똑같은거 같아요". 기존 검증은 함수 직접 호출까지만 확인했고 실제 다른 어빌리티의 자동 재생을 확인하지 못했다. PlayMontage 통합과 실제 Skill 활성화 테스트를 추가한다.
- 공통 PlayMontage에 자동 섹션 선택·입력 수집·발동 키별 TargetData 수신 대기를 연결했다. 콤보 단계 검사와 HitReact의 발동 전 섹션 검사도 방향별 접두사를 인식한다.
- 그로기는 공통 선택 경로를 사용하되 PlayMontageInternal에서 기존 ASC 직접 재생과 GP·폴링 수명을 유지한다. 방향을 기다리는 동안 다른 반응이 시작됐으면 폴링으로 재시도한다.
- 통합 테스트에서 누락된 필수 속성 세트를 보완했고, 원격 소유자 판정을 PlayerState 대신 ActorInfo의 PlayerController로 수정했다. 최종 빌드와 테스트 6개를 다시 실행해 모두 통과했다.
- 사용자 요청으로 번호 섹션 단계 검사 두 함수를 제거하고 공용 Combo 부모에 몽타주 배열을 복원했다. 기존 단일 몽타주 어빌리티의 데이터 방식과 새 공용 함수의 C++ 전용 정책은 유지했다.
- 통합 몽타주 9개가 생성 커밋 `a12ca75a0` 뒤에 바뀌지 않았음을 확인하고, 직전 커밋에서 원본 단계별 몽타주 29개를 작업 폴더로 복원했다. GA 자체는 과거 버전으로 되돌리지 않고 현재 22개 GA의 몽타주 속성만 이관했다.
- 자동 승인 검토가 통합 몽타주 삭제를 명시적 승인 없는 삭제로 거부했다. 삭제 코드를 제거하고 더 안전한 이관만 실행했다. 통합 에셋 9개는 참조가 없는 상태로 남아 있고, 런타임은 단계별 에셋을 사용한다.
- 최초 샌드박스 내 UE 실행은 Zen 캐시 경로 접근 문제로 스크립트 시작 전에 중단했다. 사용자 권한 실행으로 이관·재로드 검증을 완료했다. 두 명령 모두 오류 0이며 공통 MCP 플러그인의 EULA 안내 경고 1건만 있다.
- 사용자 정리 요청에 따라 `WxAbilityDirectionTests.cpp`, `WxAbilityDirectionPlaybackTests.cpp`, `WxAbilityDirectionTestTypes.h`와 임시 `VerifyComboMontages.py`를 제거했다. 기존 실행 로그·JSON 결과는 검증 이력으로 남겼다.
- 주석 16개 파일을 점검해 9개 파일의 사실 오류 14건을 정정하고 중복 1줄을 제거했다. 선택 함수와 자동 재생의 책임, 방향 수신 대기, Dodge 섹션 누락·폴백, 콤보 단계 조회·완료, 방향별 피격 발동 조건을 맞췄다.
- 주석 정정 전후의 코드 해시를 비교해 런타임 변경이 없음을 확인했다. 동시에 다른 작업이 제거한 `FWxExclusiveAbilityBlockingTest` friend 선언은 메모리에서만 보정해 비교했으며, 그 변경은 이번 제출에 포함하지 않는다.
- 번호 태그를 GA가 설정한다는 기존 주석은 이번 이관 보고서의 에셋·소유 태그 일치로 확인했다. HitReact의 슬롯 그룹·착지 섹션에 따른 에셋 분리 제약 설명은 이번 코드 점검만으로 확정할 수 없어 유지했다.
- 제출은 사용자 지시에 따르되 사람 코드 리뷰·PIE 확인을 통과로 간주하지 않는다. 공유 WxAbilityBase·이펙트 생성 목록은 부분 스테이징해 다른 작업의 태그 차단 변경을 제외한다.
- 테스트 제거 후 [BuildDoctor 로그](../../../Saved/Logs/BuildDoctor/build_2026-09-25_234230_097_45824.log)에서 현재 작업 폴더의 Editor Development 빌드 성공을 확인했다. 제거한 테스트의 소스 참조·UHT 등록은 없으며, 테스트 재실행은 하지 않았다.

## 콤보 배열 복원 검증 근거

- [BuildDoctor 로그](../../../Saved/Logs/BuildDoctor/build_2026-09-25_231816_093_53240.log): `Result: Succeeded`, 종료 코드 0. 이후 C++ 주석의 조사 오타만 수정했다.
- [자동화 JSON](../../../Saved/Automation/ComboMontageArray/index.json), [실행 로그](../../../Saved/Logs/ComboMontageArrayTests.log): Wx.Combat 11개 성공·실패 0·미실행 0. 테스트별 오류·경고 0.
- [이관 보고서](../../../Saved/Tests/ComboMontageMigration.json), [재로드 보고서](../../../Saved/Tests/ComboMontageVerification.json): GA 22개 배열·참조·기타 설정 보존, 원본 몽타주 29개 확인. 통합본 9개는 삭제하지 않았다.
- [이관 로그](../../../Saved/Logs/ComboMontageMigration.log), [재로드 로그](../../../Saved/Logs/ComboMontageVerification.log): `COMBO_MIGRATION_OK`, `COMBO_RELOAD_VERIFIED`.
- 재생 요청·GAS 수명·저장 데이터 검증이며 실제 애니메이션 연출·루트모션·네트워크 RPC 왕복은 사람 PIE 확인 항목이다. 커밋하지 않았다.

## 자동 재생 통합 검증 근거

- [최종 BuildDoctor 로그](../../../Saved/Logs/BuildDoctor/build_2026-09-25_230119_720_50688.log): `Result: Succeeded`, `BUILD_DOCTOR_EXIT_CODE=0`.
- [최종 자동화 JSON](../../../Saved/Automation/AbilityDirectionPlaybackVerified/index.json), [실행 로그](../../../Saved/Logs/AbilityDirectionPlaybackVerifiedTests.log): 성공 6·실패 0·미실행 0, 각 테스트 오류·경고 0.
- 제거 전 `WxAbilityDirectionPlaybackTests.cpp`는 실제 Skill의 GAS 활성화·재발동을 실행하고 최종 재생 요청만 기록했다. 렌더링·루트모션·실제 RPC 왕복은 검증 범위가 아니다.
- 신규 API는 UFUNCTION 없이 C++로만 제공한다. 에셋을 바꾸거나 커밋하지 않았다.

## 이전 구현 검증 근거

- 최종 빌드: [BuildDoctor 로그](../../../Saved/Logs/BuildDoctor/build_2026-09-25_223052_302_12720.log), `Result: Succeeded`, `BUILD_DOCTOR_EXIT_CODE=0`. UHT와 30개 빌드 작업을 완료했다.
- 빌드 명령: `& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" WxEditor Win64 Development "-Project=C:\Wx\Wx.uproject" -WaitMutex -NoHotReloadFromIDE` (build-doctor 실행기가 호출).
- 당시 테스트 코드: `WxAbilityDirectionTests.cpp` (사용자 요청으로 제거).
- 최종 자동화 결과: [JSON 보고서](../../../Saved/Automation/AbilityDirectionCppOnly/index.json), [실행 로그](../../../Saved/Logs/AbilityDirectionCppOnlyTests.log). 성공 3·실패 0·미실행 0, 테스트별 오류·경고 0, 프로세스 종료 코드 0. NullRHI·unattended로 실행했다.
- 최종 소스·작업 현황의 `git diff --check`가 통과했다. 기존 미커밋 변경은 그대로 유지했다.
- 실제 회피 몽타주 재생·이동·극한 회피 판정·네트워크 동기화는 자동화 테스트의 확인 범위가 아니며 사람 플레이 확인이 필요하다.
