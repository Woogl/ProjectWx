// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ToolsetRegistry/ToolsetDefinition.h"
#include "WxAnimMontageToolset.generated.h"

class UAnimMontage;
class UAnimNotify;

/**
 * 몽타주의 섹션과 세그먼트는 Blueprint 노출이 없어 Python·기존 MCP 표면이 닿지 못한다 — 그 지점만 뚫는다.
 */
UCLASS(BlueprintType, Hidden)
class UWxAnimMontageToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/**
	 * 슬롯 세그먼트·섹션·노티파이를 JSON으로 돌려준다.
	 * 저작 전 원본 파악과 저작 후 검증에 모두 쓴다.
	 */
	UFUNCTION(meta = (AICallable), Category = "Wx")
	static FString DescribeMontage(UAnimMontage* Montage);

	/**
	 * 원본의 슬롯 세그먼트와 섹션 구성을 대상에 옮기고, 각 섹션의 다음 섹션 링크를 비워 한 방향만 재생되게 한다.
	 * 노티파이와 블렌드·재생 속도 같은 대상 고유 설정은 그대로 둔다.
	 */
	UFUNCTION(meta = (AICallable), Category = "Wx")
	static bool MirrorMontageStructure(UAnimMontage* Source, UAnimMontage* Target);

	/**
	 * 기준 섹션의 노티파이를 나머지 섹션에 같은 상대 위치로 복제하고 복제한 개수를 돌려준다.
	 * 노티파이 오브젝트를 통째로 복제하므로 설정된 프로퍼티가 그대로 따라간다.
	 * 이미 노티파이가 있는 섹션은 건너뛰므로 다시 돌려도 중복되지 않는다.
	 */
	UFUNCTION(meta = (AICallable), Category = "Wx")
	static int32 ReplicateNotifiesToSections(UAnimMontage* Montage, FName SourceSectionName);

	/**
	 * 섹션 시작 기준 오프셋(초)에 단발 노티파이를 하나 추가하고 성공 여부를 돌려준다.
	 * 트랙은 인덱스로 지목하며 그 인덱스까지 없는 트랙은 만든다.
	 */
	UFUNCTION(meta = (AICallable), Category = "Wx")
	static bool AddNotify(UAnimMontage* Montage, TSubclassOf<UAnimNotify> NotifyClass, FName SectionName, float OffsetInSection, int32 TrackIndex);

	/** 저작 결과를 디스크에 쓴다. */
	UFUNCTION(meta = (AICallable), Category = "Wx")
	static bool SaveMontage(UAnimMontage* Montage);
};
