# WxDevice 상태 동기화

## 책임과 관측

`AWxDevice`는 상호작용 발행·프롬프트·장치 연결을 맡는다. `UWxDeviceStateTreeComponent`는 실제 상태 진입 관측, 복제, 초기 수렴과 완료 후 복구를 소유한다.

- `FWxDeviceExecutionExtension::OnBeginApplyTransition`에서 기존 활성 상태를 기록하고 틱 종료 후 다시 기록한다. 한 틱에 진입한 뒤 완료되는 상태도 다음 전이의 ExitState 전에 확보한다.
- 태그 상태의 프레임/인스턴스 ID가 바뀌거나 그 상태·조상을 직접 재선택한 실제 전이가 적용될 때 진입을 센다. 조건 실패·지연 대기는 세지 않으며, 미태그 자식 사이 이동도 부모 진입으로 세지 않는다.
- 순정 Start/Restart 및 틱 스케줄링을 사용한다. 시작 직후 실행 확장을 설치하며, 확장 생성에는 파생 구조체 타입을 명시해야 한다.
- TickComponent는 부모를 항상 호출한다. 복제 수신에서는 동기화를 먼저 수행하고 복구가 필요한 경우 Restart/RequestTransition이 틱을 예약한다. 수신만을 이유로 정지된 컴포넌트의 틱을 강제로 켜지 않는다. 적용 대기 중인 전이는 다음 틱에서 확인하므로 완료 상태가 추가 수신되어도 요청을 중복 발행하지 않는다.
- 종료 여부는 순정 `bIsRunning`만 보지 않고 실제 `GetStateTreeRunStatus()`와 함께 판정한다. 완료된 장치는 상호작용 후보에서 제외한다.

## 복제 계약

`FWxDeviceStateSnapshot`은 상태 태그 이름, 진입 번호, 실행 상태, 상호작용자와 참조 존재 여부를 담는 일반 USTRUCT이다. 컴포넌트의 `ReplicatedUsing`과 `DOREPLIFETIME`을 통한 엔진 기본 프로퍼티 복제를 사용한다. 커스텀 `NetSerialize`와 구조체 traits는 사용하지 않는다.

구조체 하나라는 이유로 여러 네트워크 업데이트에 걸친 원자성을 보장하지 않는다. 엔진이 변경 멤버와 UObject 참조 해소를 처리하고, 장치 코드는 진입 번호·당사자 준비 여부를 검사한 뒤 실제 상태를 적용한다. 이전의 별도 당사자 복제와 재진입 멀티캐스트도 사용하지 않는다.

- 진입 번호 0은 미수신이다. 최초 수신 시 이미 같은 상태라면 과거 진입을 재생하지 않는다.
- 이후 새 번호의 동일 태그는 자기 전이로 적용한다. 직접 상호작용과 연결 장치 이벤트가 같은 관측 경로를 사용한다.
- 당사자의 NetGUID가 아직 해소되지 않았으면 진입을 보류하고 실행 중인 상태의 당사자를 유지한다. 기본 객체 프로퍼티의 참조 매핑 완료 후 RepNotify에서 적용을 재개한다. 당사자가 없다고 명시된 상태는 남아 있는 포인터를 사용하지 않는다.
- 같은 번호의 실행 상태만 완료로 갱신되면 진행 중인 진입 요청을 유지한다. 마지막 태그 진입을 적용한 뒤 트리를 정지한다.
- **최신 상태 복제**이다. 복제 사이의 여러 진입은 최신 상태로 합쳐진다. 지나간 모든 사운드·몽타주·이벤트를 빠짐없이 재생하는 계약은 아니다.
- 상태 태그는 루트 StateTree 에셋 안에서 유일해야 한다. 연결된 별도 에셋에만 존재하는 태그를 복구 대상으로 삼는 경로는 지원하지 않는다.

## 초기 상태와 복구

`InitialState` 프로퍼티 및 `Root` 예약값을 유지한다. 유효한 목표가 있으면 기본 상태의 태그가 없어도 전이를 요청한다. 초기 목표를 적용하기 전에는 기본 상태를 권위 스냅샷으로 발행하지 않는다.

클라이언트가 먼저 완료했으면 순정 Restart로 활성 프레임을 복구한 뒤 목표 전이를 요청한다. 상태 진입 확인 없이 요청만 보냈다는 이유로 적용 완료 처리하지 않는다. 같은 권위 진입의 복구·전이 요청은 총 3회로 제한한다. 조건 불일치나 반복 자동 완료로 수렴하지 못하면 오류를 한 번 기록하고 해당 목표의 재시도를 중단한다. 새 권위 진입·실행 상태 또는 명시적 Start/Restart는 시도 한도를 초기화한다.

새 권위 진입 없이 진행하는 미태그 시퀀스는 마지막 태그로 계속 되감지 않는다. 의도적인 종료는 상태의 진입 태스크까지 적용한 다음 정지하므로 태스크의 ExitState 정리도 실행된다.

장치 Start/Restart, 초기 목표 수렴, 최초 스냅샷 적용 및 같은 진입의 재시도는 복구로 구분한다. `FWxDeviceExecutionPolicy`를 통해 사운드·몽타주·시퀀스·GE·리스폰·장치 이벤트의 일회성 실행을 억제한다. 사운드의 명시적인 `bPlayOnRestore` 설정은 유지한다. 장치의 ComponentMove/SplineMove는 복구 중 목표 위치를 즉시 적용하고, 실제 발동에서는 기존 시간 기반 이동을 사용한다.

`상태 상호작용` 태스크는 해당 상태가 활성인 동안만 설정을 소유한다. 종료하면 자기 바인딩을 제거하고 활성 부모 설정을 복원한다. 활성 설정이 없으면 프롬프트와 발행 자리도 비운다. 기존 에셋 중 이전 상태의 상호작용 설정을 계속 사용하는 구성은 상호작용이 필요한 상태 또는 공통 부모에 태스크를 배치해야 한다.

전용 Schema, 태그 계층 제한, 저작 검증 및 템플릿은 기획 확정 전에는 도입하지 않는다.

## 디버깅

콘솔에서 `Log LogWxWorld Verbose`를 설정하면 발행·수신·전이 요청을 볼 수 있다. 개별 로컬 진입은 `Log LogWxWorld VeryVerbose`로 확인한다. 종료 후 `Log LogWxWorld Log`로 복구한다.

`DescribeSynchronization()`과 Gameplay Debugger의 기존 StateTree 디버그 문자열에 다음 값을 함께 노출한다.

| 필드 | 의미 |
| --- | --- |
| role | 권위 / 클라이언트 |
| local | 마지막 실제 진입 태그 / 로컬 진입 번호 |
| target, applied | 받은 태그·번호 / 적용 완료한 번호 |
| run, authorityRun | 실제 로컬 / 권위 실행 상태 |
| attempts | 현재 목표의 복구·요청 횟수 |
| interactor, waitingInteractor | 당사자 / 참조 해소 대기 여부 |
| initial | 아직 적용하지 못한 초기 목표 |
| error | 현재 목표의 수렴 중단 원인 |

오류 로그가 있으면 먼저 태그 존재 여부, 로컬 전이 조건, 진입 태스크의 즉시 완료/실패를 확인한다. `waitingInteractor=1`이면 해당 액터의 네트워크 관련성과 참조 해소를 확인한다. 콘솔 로그를 활성화하지 않아도 수렴 중단 원인은 Error로 한 번 출력한다.

## 검증

`Private/Device/Tests/WxDeviceTests.cpp`에서 임시 StateTree 에셋을 컴파일하고 실제 장치와 컴포넌트를 실행한다. 서버/클라이언트 역할과 스냅샷 수신은 테스트가 제어한다.

- `Wx.Device.DelegateReentry`: 조건 실패·지연·성공한 직접 상호작용 자기 전이.
- `Wx.Device.EventReentry`: 연결 장치 이벤트를 통한 반복 자기 전이.
- `Wx.Device.UntaggedInitialAndLateJoin`: 미태그 기본 상태에서 권위 초기화·클라이언트 최초 수렴.
- `Wx.Device.SameTickCompletionAndRecovery`: 같은 틱의 마지막 진입·완료 보존 및 완료한 클라이언트 복구.
- `Wx.Device.SnapshotBaselineAndReentry`: 최초 수신, 새 번호, 중복 방지, 진입 요청 중 완료 수신.
- `Wx.Device.RecoveryAttemptLimit`: 반복 자동 완료를 3회로 제한하고 한 번 진단.
- `Wx.Device.ChildTransitionPreservesParentEntry`: 미태그 자식 이동은 부모 진입을 증가시키지 않음.
- `Wx.Device.WaitForInteractor`: 미해소 당사자 참조 대기와 해소 후 진입.
- `Wx.Device.NativeSnapshotSerialization`: 엔진 FRepLayout 구조체 직렬화 왕복, 미해소 참조 보고, 개별 객체 프로퍼티 해소 시 최신 태그·진입 번호·완료 상태 보존. 패키지 맵은 참조 해소 상태만 제어하는 테스트 대역이다.

실행 명령:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'C:\Wx\Wx.uproject' -unattended -nop4 -NullRHI -nosound -nosplash '-ExecCmds=Automation RunTests Wx.Device' '-TestExit=Automation Test Queue Empty' '-ReportExportPath=C:\Wx\Saved\Automation\WxDevice' '-abslog=C:\Wx\Saved\Logs\WxDeviceAutomation.log'
```

테스트 보고서의 성공/실패를 확인해야 한다. 실행기 종료 코드만으로 테스트 성공을 판정하지 않는다.

실제 BP/StateTree 콘텐츠, 다중 프로세스 네트워크 패킷 전송, 실제 NetGUID 캐시의 지연·관련성 변화, 사운드·몽타주 연출은 이 자동화 테스트의 검증 범위 밖이다. 프로퍼티 직렬화 및 적용 단계 테스트를 실제 네트워크 검증과 동일시하지 않는다.
