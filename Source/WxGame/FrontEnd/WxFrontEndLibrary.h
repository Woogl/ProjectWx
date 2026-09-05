// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WxFrontEndLibrary.generated.h"

class APawn;

/** WBP 목록 항목. 표시 정보와 입장에 전달할 참조만 보관한다. */
USTRUCT(BlueprintType)
struct WXGAME_API FWxFrontEndOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wx|FrontEnd")
	TSoftClassPtr<APawn> PawnClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wx|FrontEnd")
	TSoftObjectPtr<UWorld> Level;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wx|FrontEnd")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wx|FrontEnd")
	FText Description;
};

UCLASS()
class WXGAME_API UWxFrontEndLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Wx|FrontEnd", meta = (WorldContext = "WorldContextObject"))
	static void GetTravelStatus(const UObject* WorldContextObject, bool& bBusy, FText& Message);

	/** 커스텀 WBP도 동일한 검증·전환 경로를 사용한다. true는 접수이며 도착 완료가 아니다. */
	UFUNCTION(BlueprintCallable, Category = "Wx|FrontEnd", meta = (WorldContext = "WorldContextObject"))
	static bool RequestNewGame(const UObject* WorldContextObject, TSoftClassPtr<APawn> PawnClass, TSoftObjectPtr<UWorld> Level);
};
