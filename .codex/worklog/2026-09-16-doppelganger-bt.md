# 도플갱어 BT 전환

## 계획

- BP_Minion과 동일한 기존 캐릭터 및 AIController를 사용하고 BT_Doppelganger로 제어한다. 추가했던 DoppelgangerCharacter 및 DoppelgangerComponent를 제거한다.
- 이번 작업에서 추가한 기존 어빌리티·소환 노티파이 코드 변경을 원복한다. 기존 어빌리티 구현은 수정하지 않는다.
- BT_Minion의 Blackboard Master 계약·GAS 델리게이트 구독·BT 수명에 따른 해제 방식을 참고해 기존 MirrorMovement와 MirrorAbility를 재구현할지 판단한다.
- 판단: 이동 노드의 정지 대상 추종 입력 0, 공중 추가 점프 누락과 어빌리티 노드의 태그 하나 선택·동일 태그 재발동 미감지 때문에 두 노드를 재구현한다.
- 이동은 BT Service가 자체 CMC 입력으로 우측 1m에 접근시키고 지상 추종이 1초 이상 걸리면 텔레포트한다. 어빌리티는 BT Task가 성공 발동·종료와 재생 중 몽타주를 관찰해 데이터로 대응시킨 분신 어빌리티를 실행한다.
- 분신용 어빌리티·몽타주 에셋으로 비용·쿨다운·컷신·재귀 소환을 제외한다. 콤보는 Master가 실제 재생하는 단계에 대응하는 단일 단계 에셋으로 실행해 기존 어빌리티 내부 상태를 수정하지 않는다.
- 기존 자동 검증을 BT 구동 검증으로 교체하고 UE 5.8 WxEditor Development 빌드 및 실제 에셋을 통한 추종·점프·콤보·취소·1초 텔레포트·소환 정리를 확인한다.

## 완료

- 2026-09-17 완료. `BP_Doppelganger`는 기존 `WxEnemyCharacter`/`WxAIController`와 `BT_Doppelganger`를 사용한다. 추가했던 `DoppelgangerCharacter`와 `DoppelgangerComponent` 소스는 제거했다. 처형 피해 처리는 이미 존재하는 `WxFinisherDamageComponent`를 BP에 배치해 재사용한다.
- `WxAbility` 계열과 `WxAnimNotify_SpawnMinion`의 이번 작업 변경을 전부 원복했다. `git diff HEAD`로 WxCombat 소스·기존 Character 소스·모듈 의존성·BP_Minion/BT_Minion에 변경이 없음을 확인했다.
- 기존 Mirror 노드는 재구현했다. 이동 서비스는 인스턴스별 상태를 사용하며 CMC 입력으로 우측 100cm에 접근한다. 도달 반경 15cm, 연속 지상 추종 1초 후 충돌 검사 텔레포트이며 공중에서는 타이머를 초기화한다. 회전·앉기·점프·공중 추가 점프와 이동 속도를 복제하고 BT 해제 시 설정·틱 의존성을 복원한다.
- 어빌리티 태스크는 Master 커밋·종료를 구독해 스펙별로 추적한다. 커밋 직후가 아닌 다음 BT 틱에 실제 몽타주를 읽어 동일 태그 콤보와 회피 변형을 구분한다. 패시브는 성공한 동기 종료, 처형은 기존 GameplayEvent 문맥을 이용한다. 이벤트 문맥은 반영된 UPROPERTY로 보관하며 BT 중단 시 구독·부여 스펙을 정리한다.
- 분신용 어빌리티 20개 대응을 구성했다. 공격·스킬 콤보는 단일 단계 에셋, 회피/백스텝/완벽회피는 몽타주별 에셋을 사용한다. 분신 에셋에서 비용·쿨다운·입력 조건·자체 이벤트 트리거를 제거했다. 궁극기 컷신, 재귀 소환 노티파이, 처형 피해자 몽타주 중복도 분신 데이터에서 제외했다. 원본 HGTest 어빌리티 코드는 수정하지 않았다.
- 궁극기 몽타주의 기존 `WxAnimNotify_SpawnMinion`은 BP_Doppelganger와 (0, 100, 0) 오프셋에 연결돼 있다. 메시·애니메이션·Minion 기본 어트리뷰트 데이터도 BP_Minion과 동일함을 새 프로세스에서 검사했다. Blueprint 재컴파일 및 20개 매핑/비용/쿨다운/노티파이 검사 성공: `Saved/DoppelgangerBTValidation.json`, `Saved/Logs/VerifyDoppelgangerBT.log`.
- UE 5.8 WxEditor Win64 Development 최종 빌드 성공: `Saved/Logs/BuildDoctor/build_2026-09-17_002309_652_33892.log`.
- 자동 테스트 `Wx.Combat.Doppelganger.MasterReplay` 성공: 실제 궁극기 소환, BT 구동, 우측 추종/이동/정지, 점프/공중 추가 점프/착지/앉기, 자원 0의 4단 콤보, 가드/질주/회피/Skill.2, 패시브 1회 전달, 처형 이벤트/정상 종료, 취소, 벽 충돌과 1초 텔레포트, BT 중단 후 스펙 제거/구독 해제, 재소환 및 Master 제거 정리를 검증했다. `Saved/Logs/DoppelgangerBTTest.log`, `Saved/Automation/DoppelgangerBT/index.json`.
- 기존 데이터 결함은 변경하지 않았다: 강공격 2의 쿨다운 GE 누락, DT_Effect의 `GE_Passive_AddUP` 누락, DT_Damage의 `AM_Finisher` 누락. 테스트에서 정확한 발생 횟수의 예상 진단으로 분리했다. 패시브 보상 및 처형 피해 수치의 정상성은 이 누락 행을 복구해야 검증할 수 있다.
- 카메라 없는 독립 서버 게임 월드에서 궁극기 컷신을 생략해 검증했다. 멀티플레이 연결/화면 연출은 별도로 실행하지 않았다. 새 스킬 추가 시 BT 매핑도 추가해야 하며, 이벤트 수신 전에 이미 진행 중인 처형에 뒤늦게 붙는 경우에는 대상 문맥을 추측하지 않고 다음 발동을 기다린다.
