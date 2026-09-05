// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayEffectUIData.h"
#include "GameplayModMagnitudeCalculation.h"
#include "WxUIData.h"
#include "WxEffectComponent_Table.generated.h"

struct FWxEffectTableRow;

/**
 * GE가 참조할 FWxEffectTableRow 행을 지목한다.
 * 값을 스펙에 실어 보내지는 않는다 — 컴포넌트 콜백은 적용이 끝난 뒤에 오거나 스펙이 const라 주입할 자리가 없다.
 * 대신 아래 MMC들이 계산 시점에 GE 정의에서 이 컴포넌트를 찾아 행을 읽는다.
 *
 * 베이스가 UGameplayEffectUIData 인 것은 표시 데이터 때문이다 — WxUI 는 WxCombat 을 참조할 수 없고 GE 의 컴포넌트 배열도 클래스로만 뒤질 수 있어, 양쪽이 아는 이 엔진 클래스가 유일한 조회 앵커다.
 */
UCLASS()
class WXCOMBAT_API UWxEffectComponent_Table : public UGameplayEffectUIData, public IWxUIData
{
	GENERATED_BODY()

public:
	UWxEffectComponent_Table();

	//~ Begin IWxUIData
	virtual FText GetTitle() const override;
	virtual FText GetDescription() const override;
	virtual TSoftObjectPtr<UObject> GetIcon() const override;
	//~ End IWxUIData

	UPROPERTY(EditDefaultsOnly, meta = (RowType = "/Script/WxCombat.WxEffectTableRow", WxPreviewRow = "true"))
	FDataTableRowHandle EffectDataRow;

	const FWxEffectTableRow* GetRow() const;

	static const FWxEffectTableRow* FindRow(const UGameplayEffect* Def);
};

/**
 * 모디파이어가 지목하는 것이 클래스라 인스턴스별 설정이 없어, 크기용과 지속시간용을 따로 둔다(UWxMMC_Cost가 자원별로 나뉜 것과 같은 제약).
 * 어트리뷰트를 캡처하지 않으니 엔진이 적용 도중 여러 번 재계산해도 같은 값이 나온다.
 */
UCLASS()
class WXCOMBAT_API UWxMMC_EffectMagnitude : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};

UCLASS()
class WXCOMBAT_API UWxMMC_EffectDuration : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
