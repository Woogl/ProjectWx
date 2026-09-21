// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WxFrontEndLibrary.generated.h"

class APawn;
class UUserWidget;
class UVerticalBox;
class UWidget;
class UWxButtonBase;

DECLARE_DYNAMIC_DELEGATE_OneParam(FWxFrontEndOptionSelected, int32, Index);

/** WBP 목록 항목. */
USTRUCT(BlueprintType)
struct WXGAME_API FWxFrontEndOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wx|FrontEnd")
	TSoftClassPtr<APawn> PawnClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wx|FrontEnd")
	TSoftObjectPtr<UWorld> Level;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wx|FrontEnd")
	FText DisplayName;
};

UCLASS()
class WXGAME_API UWxFrontEndLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Wx|FrontEnd")
	static TArray<FWxFrontEndOption> GetCharacterOptions();

	UFUNCTION(BlueprintPure, Category = "Wx|FrontEnd")
	static TArray<FWxFrontEndOption> GetLevelOptions();

	UFUNCTION(BlueprintPure, Category = "Wx|FrontEnd")
	static bool HasSelectableOptions();

	/** 설정 순서대로 버튼을 만들고 첫 번째 선택 가능한 버튼을 포커스 대상으로 반환한다. */
	UFUNCTION(BlueprintCallable, Category = "Wx|FrontEnd", meta = (AutoCreateRefTerm = "OnSelected"))
	static UWidget* BuildOptionButtons(UUserWidget* Owner, UVerticalBox* Container, TSubclassOf<UWxButtonBase> ButtonClass,
		const TArray<FWxFrontEndOption>& Options, const FWxFrontEndOptionSelected& OnSelected, float ButtonSpacing = 4.f);

	UFUNCTION(BlueprintPure, Category = "Wx|FrontEnd", meta = (WorldContext = "WorldContextObject"))
	static void GetTravelStatus(const UObject* WorldContextObject, bool& bBusy, FText& Message);

	/** true는 접수이며 도착 완료가 아니다. */
	UFUNCTION(BlueprintCallable, Category = "Wx|FrontEnd", meta = (WorldContext = "WorldContextObject"))
	static bool RequestNewGame(const UObject* WorldContextObject, TSoftClassPtr<APawn> PawnClass, TSoftObjectPtr<UWorld> Level);
};
