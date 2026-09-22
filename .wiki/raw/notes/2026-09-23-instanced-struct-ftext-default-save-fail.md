---
title: "인스턴스 구조체 FText 기본값과 에셋 저장 실패 조사"
source: "Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceTriggerRule.h"
type: notes
ingested: 2026-09-23
tags: [wx, static-review, world, statetree]
summary: "StateTree 노드 안의 수락 규칙(인스턴스 구조체)에 C++ 기본값이 있는 FText를 두자, 값이 기본값과 같을 때 ST_Elevator 저장이 FortniteMain 커스텀 버전 불일치로 실패했다. 커밋 5212bbe3a의 원문 발췌, UE 5.8 엔진 근거, 2026-09-23 에디터 저장 재현 기록."
revision: 5212bbe3aa4b6a0b935293a9a6a5f3973ae846f5
sha: ea125db73182d0e6824d04fa3e20bcbaedfabefb37949598f58cab3fa27b4e10
---

# 인스턴스 구조체 FText 기본값과 에셋 저장 실패 조사

조사일: 2026-09-23. 대상 커밋은 `5212bbe3aa4b6a0b935293a9a6a5f3973ae846f5`("엘리베이터 정차 지점 규칙을 기획서의 탑승칸 동작에 맞게 변경", 2026-09-23 01:48 +0900)이다. 엔진은 설치형 UE 5.8(`5.8.2-56702186`)이다. SHA-256은 파일 전체 바이트의 식별값이며 파일 전체의 결함 검토를 뜻하지 않는다. 코드 원문은 해당 경로가 정본이다. 저장 재현은 DebugGame 에디터에서 MCP로 한 결과이고, 게임 실행 검증이 아니다.

## 경위와 재현 결과 (확인)

`FWxDeviceTriggerRule_SplineStops`에 층 문구 필드 `FText StopPrompt = NSLOCTEXT("WxDeviceTriggerRule", "StopPrompt", "Floor {0}")`를 추가했다. 이 규칙은 `FWxStateTreeTask_WaitForTrigger`의 `TInstancedStruct<FWxDeviceTriggerRule> Rule` 안에 들어가고, 그 태스크는 다시 StateTree 에디터 노드의 인스턴스 구조체 안에 들어간다. 그 뒤 `ST_Elevator` 저장이 다음 오류로 중단됐다.

```text
LogSavePackage: Error: Unexpected custom version "FortniteMain" found when saving /Game/WorldObject/Gimmick/ST_Elevator. This usually happens when export tagging and final serialization paths differ. Package will not be saved.
```

직전 경고의 호출 스택은 `WriteGatherableText()`(SavePackage2.cpp:2330) → `WritePackageHeader()` → `UPackage::Save()` 순이었다.

| 시각(+0900) | 조건 | 결과 |
|---|---|---|
| 01:34 | 두 노드 모두 C++ 기본값 그대로 | 저장 실패 |
| 01:38 | MCP `set_properties`로 두 노드에 `"Floor {0}"` 입력(기본값과 같은 문자열) | 저장 실패 |
| 01:39 | 두 노드에 `"Stop {0}"` 입력(기본값과 다른 문자열) | 저장 성공 |
| 01:43, 01:44 | C++ 기본값을 제거한 빌드에서 두 노드에 `"Floor {0}"` 입력 | 저장 성공 |

대조군: 작동 대기 노드의 `Prompt`는 C++ 기본값이 없는 FText다. 이 문구를 수정한 `ST_Button`·`ST_CheckPoint`·`ST_TreasureChest`는 커밋 `da21a7844`("기믹 문구 수정", 2026-09-22)에서 정상 저장됐다. 01:38의 실패는 MCP가 넣은 텍스트가 기본값과 같다고 판정됐기 때문으로 보이지만, 그 판정 경로는 확인하지 않았다.

## 조치 (커밋 5212bbe3a)

### Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceTriggerRule.h
- [저장소 원문](<../../../Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceTriggerRule.h>)
- SHA-256: `ea125db73182d0e6824d04fa3e20bcbaedfabefb37949598f58cab3fa27b4e10`
```text
47:	/**
48:	 * 비활성 장치를 깨우는 상태에 켠다. 밖 호출만 받고 탑승칸 버튼은 잠근다.
49:	 * 탑승칸이 이미 있는 정차 지점의 호출도 받으므로, 같은 층에서 깨우면 제자리에서 문만 열리고 다른 층에서 깨우면 그 층으로 온다.
50:	 */
51:	UPROPERTY(EditAnywhere, Category = "Wx")
52:	bool bWakeOnCall = false;
53:
54:	/**
55:	 * 탑승칸 버튼이 내놓는 정차 지점 선택지의 문구. {0} 은 정차 지점 번호(1부터)다. 밖 버튼은 대기 노드의 Prompt 를 쓴다.
56:	 * C++ 기본값을 두지 않는다 — 인스턴스 구조체 안의 FText 가 기본값과 같으면 에셋 저장이 실패한다(FortniteMain 커스텀 버전 불일치).
57:	 */
58:	UPROPERTY(EditAnywhere, Category = "Wx")
59:	FText StopPrompt;
```

### Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceTriggerRule.cpp
- [저장소 원문](<../../../Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceTriggerRule.cpp>)
- SHA-256: `024c332b942dd9789833c5958d34faec73567c22d5bd6e1df935c953064b03f4`
```text
33:	if (Sender->GetRootComponent()->IsAttachedTo(PlatformComponent))
34:	{
35:		if (bWakeOnCall)
36:		{
37:			return;
38:		}
39:
40:		for (int32 Stop = 0; Stop < NumStops; ++Stop)
41:		{
42:			if (Stop != CurrentStop)
43:			{
44:				OutOptions.Add({FText::Format(StopPrompt, FText::AsNumber(Stop + 1)), Stop});
45:			}
46:		}
47:
48:		return;
49:	}
```

`ST_Elevator`의 Inactive·Idle 작동 대기 노드에는 `StopPrompt = "Floor {0}"`을 StateTree 값으로 입력해 저장했다(01:44 저장본에서 `Stop {0}` 없음, `Floor {0}` 있음 확인).

## 엔진 근거 (UE 5.8 설치본)

### Engine/Source/Runtime/CoreUObject/Private/StructUtils/InstancedStruct.cpp
- SHA-256: `35ea07b7935c4d5a806139b18fcbff3e92f67c84675b63581c9edbfb675b4a3c`
- 저장·로드 시 인스턴스 구조체 내용을 기본값(부모 기본값 또는 새로 초기화한 임시 구조체)과 대조해 직렬화한다. 기본값과 같은 프로퍼티는 태그 직렬화에서 빠진다.
```text
133:			const bool bUseDefaults = !bDuplicate
134:				&& bSavingLoading
135:				&& !Ar.IsCooking()
136:				&& !Ar.WantBinaryPropertySerialization();
...
148:					// Create a temporary struct on the stack. The struct has default values.
...
154:					ScriptStruct->InitializeStruct(TempMemory);
155:					ScriptStruct->SerializeItem(Ar, Self->GetMutableMemory(), TempMemory);
```

### Engine/Source/Runtime/Core/Private/Internationalization/GatherableTextData.cpp
- SHA-256: `8b8e391518ad512034b0632917d8f92172162abf93b830774656c7974b172b2e`
- 번역 수집 데이터를 쓸 때 `FortniteMain` 커스텀 버전을 등록한다.
```text
26:#if WITH_EDITORONLY_DATA
27:	FArchive& UnderlyingArchive = Slot.GetUnderlyingArchive();
28:	UnderlyingArchive.UsingCustomVersion(FFortniteMainBranchObjectVersion::GUID);
```

### Engine/Source/Runtime/Core/Private/Internationalization/TextHistory.cpp
- SHA-256: `aa1d950f2d870e5ca1711f529adb39ee9d5e1685f5f3fcf66a0757994a423df2`
- FText 본문 직렬화도 같은 커스텀 버전을 등록한다. 본문에서 FText가 한 번이라도 직렬화되면 요약 헤더 전에 등록된다.
```text
915:#if WITH_EDITORONLY_DATA || IS_PROGRAM
916:	BaseArchive.UsingCustomVersion(FFortniteMainBranchObjectVersion::GUID);
```

### Engine/Source/Runtime/CoreUObject/Private/UObject/SavePackage2.cpp
- SHA-256: `c6281235e3ef7e2cf704114800a5e49934136c0dbba9322307b8f8716ff7cb8e`
- 수집된 번역 텍스트는 패키지 요약 뒤에 헤더로 기록된다. 주석대로 여기서 새 커스텀 버전을 쓰려면 요약 전에 등록돼 있어야 한다.
```text
2324:		// The Editor version is used as part of the check to see if a package is too old to use the gather cache, so we always have to add it if we have gathered loc for this asset
2325:		// Note that using custom version here only works because we already added it to the export tagger before the package summary was serialized
2326:		Linker->UsingCustomVersion(FEditorObjectVersion::GUID);
...
2330:		for (FGatherableTextData& GatherableTextData : Linker->GatherableTextDataMap)
2331:		{
2332:			Stream.EnterElement() << GatherableTextData;
```

## 해석 (추론)

기본값과 같은 FText는 인스턴스 구조체 본문 직렬화에서 빠지므로, 다른 FText가 없는 에셋이면 본문 단계에서 `FortniteMain`이 등록되지 않는다. 그런데 번역 수집은 그 텍스트를 잡아 요약 뒤 헤더(`WriteGatherableText`)에서 `FortniteMain`을 처음 쓰고, 저장이 중단된다. 번역 수집기가 기본값과 같은 인스턴스 구조체 텍스트를 왜 잡는지는 엔진 수집 경로를 추적하지 않아 확인하지 않았다. 재현 결과는 "C++ 기본값이 있는 FText + 값이 기본값과 같음"일 때만 실패한다는 점과 일치한다.
