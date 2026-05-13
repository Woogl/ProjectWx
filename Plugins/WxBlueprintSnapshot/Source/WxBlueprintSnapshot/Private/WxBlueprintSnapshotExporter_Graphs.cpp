// Copyright Woogle. All Rights Reserved.

//
// BP 그래프 → UE C++ 의사 코드 렌더러.
//
// 접근 방식:
//   - Entry 노드(Event / CustomEvent / FunctionEntry)에서 시작해 함수 시그니처와 본문 블록을 출력.
//   - 각 exec 노드는 RenderExecNode 안의 TryRender* 디스패처가 노드 타입에 맞는 C++ 문장을 찍는다.
//   - 데이터 입력 핀은 RenderDataInput → RenderExpression 재귀로 역방향 평가.
//   - UK2Node_Knot은 데이터/실행 양쪽 모두 SkipKnots로 pass-through.
//   - exec cycle은 Visited set으로 감지해 goto 라인으로 처리. 데이터 핀 재귀는 깊이 제한(MaxExpressionDepth)으로 차단.
//   - 알려지지 않은 노드 타입은 RenderFallbackNode가 NodeTitle 한 줄을 주석으로 기록 — 회귀는 조용히 발생할 수 있음.
//

#include "WxBlueprintSnapshotExporter.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "K2Node.h"
#include "K2Node_Event.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_Knot.h"
#include "K2Node_Self.h"
#include "K2Node_Literal.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_CallParentFunction.h"
#include "K2Node_BaseAsyncTask.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "K2Node_Switch.h"
#include "K2Node_SwitchEnum.h"
#include "K2Node_BreakStruct.h"
#include "K2Node_MakeStruct.h"
#include "K2Node_StructMemberGet.h"
#include "K2Node_StructMemberSet.h"
#include "K2Node_Tunnel.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

namespace
{
	// 데이터 핀 사이클(드물지만 매크로/커스텀 노드에서 가능)에서의 무한 재귀 방지.
	constexpr int32 MaxExpressionDepth = 32;

	// 본문 라인 한 줄당 들여쓰기 칸 수. UE 코드 스타일 4-space.
	constexpr int32 IndentSpaces = 4;

	struct FPseudoLine
	{
		int32 Indent = 0;
		FString Text;
	};

	struct FPseudoEntry
	{
		FString Name;                         // 이벤트/함수 이름. eventGraph object 키로 사용.
		FString Signature;                    // "void HandleBeginPlay()"
		TArray<FString> UFunctionSpecifiers;  // UFUNCTION(...) 매크로 인자. 비어있으면 매크로 라인 생략.
		TArray<FPseudoLine> Body;             // 함수 본문 — 시리얼라이즈 단계에서 { } 로 감싼다.
	};

	struct FRenderCtx
	{
		TArray<FPseudoLine>& Body;
		TSet<UEdGraphNode*>& Visited;
		int32 ExprDepth = 0; // 데이터 핀 표현식 재귀 깊이; TGuardValue로 자동 증감
		UEdGraphPin* ArrivedExecPin = nullptr; // RenderExecNode 가 호출된 입력 exec 핀 — 매크로 exit 분기에서 어느 출구로 나가는지 식별
	};

	UEdGraphPin* SkipKnots(UEdGraphPin* Pin);
	FString RenderExpression(UEdGraphPin* OutputPin, FRenderCtx& Ctx);
	FString RenderDataInput(UEdGraphPin* InputPin, FRenderCtx& Ctx);
	void RenderExecChain(UEdGraphPin* ExecOutPin, int32 Indent, FRenderCtx& Ctx);
	void RenderExecNode(UEdGraphNode* Node, int32 Indent, FRenderCtx& Ctx);
	FString DeriveAsyncTaskVarName(UK2Node_BaseAsyncTask* Async);

	// UE 5.7에서 UK2Node_BaseAsyncTask의 ProxyClass/ProxyFactoryClass/ProxyActivateFunctionName 가
	// protected 로 좁혀져 외부 코드에서 직접 접근 불가. 의사코드 렌더링에 read-only 로만 필요하므로
	// using 선언으로 derived 접근자 타입에 노출하고 static_cast 로 접근한다.
	struct FAsyncTaskAccessor : public UK2Node_BaseAsyncTask
	{
		using UK2Node_BaseAsyncTask::ProxyClass;
		using UK2Node_BaseAsyncTask::ProxyFactoryClass;
		using UK2Node_BaseAsyncTask::ProxyActivateFunctionName;
	};

	UClass* GetAsyncProxyClass(UK2Node_BaseAsyncTask* Async)
	{
		return Async ? static_cast<FAsyncTaskAccessor*>(Async)->ProxyClass.Get() : nullptr;
	}

	UClass* GetAsyncProxyFactoryClass(UK2Node_BaseAsyncTask* Async)
	{
		return Async ? static_cast<FAsyncTaskAccessor*>(Async)->ProxyFactoryClass.Get() : nullptr;
	}

	FName GetAsyncProxyActivateFunctionName(UK2Node_BaseAsyncTask* Async)
	{
		return Async ? static_cast<FAsyncTaskAccessor*>(Async)->ProxyActivateFunctionName : NAME_None;
	}

	void Emit(FRenderCtx& Ctx, int32 Indent, FString Text)
	{
		Ctx.Body.Add({ Indent, MoveTemp(Text) });
	}

	bool IsExecPin(const UEdGraphPin* Pin)
	{
		return Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec;
	}

	bool IsObjectLikePinCategory(const FName& Cat)
	{
		return Cat == UEdGraphSchema_K2::PC_Object
			|| Cat == UEdGraphSchema_K2::PC_Class
			|| Cat == UEdGraphSchema_K2::PC_Interface
			|| Cat == UEdGraphSchema_K2::PC_SoftObject
			|| Cat == UEdGraphSchema_K2::PC_SoftClass;
	}

	// 알려진 루프 매크로 — 본문에서 출력 데이터 핀을 로컬 변수처럼 참조하도록 RenderExpression 분기 키.
	bool IsKnownLoopMacro(const FString& MacroName)
	{
		return MacroName == TEXT("ForEachLoop")
			|| MacroName == TEXT("ForEachLoopWithBreak")
			|| MacroName == TEXT("ReverseForEachLoop")
			|| MacroName == TEXT("ForLoop")
			|| MacroName == TEXT("ForLoopWithBreak");
	}

	// 매크로 출력 데이터 핀명을 C++ 식별자로 (예: "Array Element" → "ArrayElement").
	FString MacroPinToLocalVar(const FName& PinName)
	{
		FString Result = PinName.ToString();
		Result.ReplaceInline(TEXT(" "), TEXT(""));
		return Result;
	}

	UEdGraphPin* FindThenPin(UEdGraphNode* Node)
	{
		if (!Node)
		{
			return nullptr;
		}
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin && Pin->Direction == EGPD_Output && IsExecPin(Pin))
			{
				return Pin;
			}
		}
		return nullptr;
	}

	UEdGraphPin* SkipKnots(UEdGraphPin* Pin)
	{
		if (!Pin)
		{
			return nullptr;
		}
		// knot은 input/output 핀이 하나씩. 현재 Pin의 반대쪽 핀의 LinkedTo[0] 을 따라가야 knot을 건너뛴다.
		// - Pin 이 knot 의 output 핀(=상류 데이터 추적 중) → input 핀의 LinkedTo[0] 이 실제 source output 핀
		// - Pin 이 knot 의 input 핀(=하류 exec 추적 중) → output 핀의 LinkedTo[0] 이 실제 destination input 핀
		while (Pin && Cast<UK2Node_Knot>(Pin->GetOwningNode()))
		{
			UK2Node_Knot* Knot = Cast<UK2Node_Knot>(Pin->GetOwningNode());
			UEdGraphPin* Other = Pin->Direction == EGPD_Output ? Knot->GetInputPin() : Knot->GetOutputPin();
			if (!Other || Other->LinkedTo.Num() == 0)
			{
				return nullptr;
			}
			Pin = Other->LinkedTo[0];
		}
		return Pin;
	}

	// UStruct 의 C++ 식별자 ("ACharacter", "FVector"). UClass::GetPrefixCPP 는 "A"/"U", UScriptStruct 는 기본 "F".
	FString FormatStructCPP(const UStruct* S)
	{
		if (!S)
		{
			return TEXT("UObject");
		}
		return FString::Printf(TEXT("%s%s"), S->GetPrefixCPP(), *S->GetName());
	}

	// 단일 (Category, SubObj) 조합을 UE C++ 식별자로 변환. 컨테이너 래핑은 호출자 책임.
	FString FormatTerminalCPP(const FName& Cat, UObject* SubObj)
	{
		if (UClass* AsClass = Cast<UClass>(SubObj))
		{
			return FString::Printf(TEXT("%s%s*"), AsClass->GetPrefixCPP(), *AsClass->GetName());
		}
		if (UScriptStruct* AsStruct = Cast<UScriptStruct>(SubObj))
		{
			return FString::Printf(TEXT("F%s"), *AsStruct->GetName());
		}
		if (UEnum* AsEnum = Cast<UEnum>(SubObj))
		{
			return AsEnum->GetName(); // 소스 이름이 이미 'E' 접두사 포함
		}

		if (Cat == UEdGraphSchema_K2::PC_Boolean)
		{
			return TEXT("bool");
		}
		if (Cat == UEdGraphSchema_K2::PC_Int)
		{
			return TEXT("int32");
		}
		if (Cat == UEdGraphSchema_K2::PC_Int64)
		{
			return TEXT("int64");
		}
		if (Cat == UEdGraphSchema_K2::PC_Byte)
		{
			return TEXT("uint8");
		}
		if (Cat == UEdGraphSchema_K2::PC_Float)
		{
			return TEXT("float");
		}
		if (Cat == UEdGraphSchema_K2::PC_Double || Cat == UEdGraphSchema_K2::PC_Real)
		{
			return TEXT("double");
		}
		if (Cat == UEdGraphSchema_K2::PC_String)
		{
			return TEXT("FString");
		}
		if (Cat == UEdGraphSchema_K2::PC_Name)
		{
			return TEXT("FName");
		}
		if (Cat == UEdGraphSchema_K2::PC_Text)
		{
			return TEXT("FText");
		}
		// SubObj 가 비어 있는 객체-카테고리는 UObject* 로 폴백.
		if (IsObjectLikePinCategory(Cat))
		{
			return TEXT("UObject*");
		}
		return Cat.ToString();
	}

	// 컨테이너(TArray/TSet/TMap) 까지 포함한 핀 타입 → C++ 표기.
	FString FormatPinTypeCPP(const FEdGraphPinType& PinType)
	{
		const FString Terminal = FormatTerminalCPP(PinType.PinCategory, PinType.PinSubCategoryObject.Get());
		if (PinType.IsArray())
		{
			return FString::Printf(TEXT("TArray<%s>"), *Terminal);
		}
		if (PinType.IsSet())
		{
			return FString::Printf(TEXT("TSet<%s>"), *Terminal);
		}
		if (PinType.IsMap())
		{
			const FString Value = FormatTerminalCPP(PinType.PinValueType.TerminalCategory, PinType.PinValueType.TerminalSubCategoryObject.Get());
			return FString::Printf(TEXT("TMap<%s, %s>"), *Terminal, *Value);
		}
		return Terminal;
	}

	FString TypeDefaultLiteral(const FEdGraphPinType& PinType)
	{
		const FName Cat = PinType.PinCategory;
		if (Cat == UEdGraphSchema_K2::PC_Boolean)
		{
			return TEXT("false");
		}
		if (Cat == UEdGraphSchema_K2::PC_Int
			|| Cat == UEdGraphSchema_K2::PC_Int64
			|| Cat == UEdGraphSchema_K2::PC_Byte)
		{
			return TEXT("0");
		}
		if (Cat == UEdGraphSchema_K2::PC_Float
			|| Cat == UEdGraphSchema_K2::PC_Double
			|| Cat == UEdGraphSchema_K2::PC_Real)
		{
			return TEXT("0.0");
		}
		if (Cat == UEdGraphSchema_K2::PC_String)
		{
			return TEXT("TEXT(\"\")");
		}
		if (Cat == UEdGraphSchema_K2::PC_Name)
		{
			return TEXT("NAME_None");
		}
		if (Cat == UEdGraphSchema_K2::PC_Text)
		{
			return TEXT("FText::GetEmpty()");
		}
		if (IsObjectLikePinCategory(Cat))
		{
			return TEXT("nullptr");
		}
		if (Cat == UEdGraphSchema_K2::PC_Struct)
		{
			if (UScriptStruct* AsStruct = Cast<UScriptStruct>(PinType.PinSubCategoryObject.Get()))
			{
				return FString::Printf(TEXT("F%s()"), *AsStruct->GetName());
			}
			return TEXT("{}");
		}
		return TEXT("");
	}

	FString FormatLiteralPin(const UEdGraphPin* Pin)
	{
		if (!Pin)
		{
			return TEXT("?");
		}
		if (Pin->DefaultObject)
		{
			return Pin->DefaultObject->GetName();
		}
		const FString Val = Pin->DefaultValue.IsEmpty() ? Pin->AutogeneratedDefaultValue : Pin->DefaultValue;
		if (Val.IsEmpty())
		{
			return TypeDefaultLiteral(Pin->PinType);
		}
		const FName Cat = Pin->PinType.PinCategory;
		if (Cat == UEdGraphSchema_K2::PC_String || Cat == UEdGraphSchema_K2::PC_Text)
		{
			return FString::Printf(TEXT("TEXT(\"%s\")"), *Val);
		}
		if (Cat == UEdGraphSchema_K2::PC_Name)
		{
			return FString::Printf(TEXT("FName(TEXT(\"%s\"))"), *Val);
		}
		// Enum 리터럴은 BP 가 값 이름만 저장 — C++ 식별자로 만들려면 EnumType:: 접두사 필요.
		if (UEnum* AsEnum = Cast<UEnum>(Pin->PinType.PinSubCategoryObject.Get()))
		{
			return FString::Printf(TEXT("%s::%s"), *AsEnum->GetName(), *Val);
		}
		return Val;
	}

	FString RenderDataInput(UEdGraphPin* InputPin, FRenderCtx& Ctx)
	{
		if (!InputPin)
		{
			return TEXT("?");
		}
		if (InputPin->LinkedTo.Num() == 0)
		{
			return FormatLiteralPin(InputPin);
		}
		UEdGraphPin* Src = SkipKnots(InputPin->LinkedTo[0]);
		if (!Src)
		{
			return FormatLiteralPin(InputPin);
		}
		return RenderExpression(Src, Ctx);
	}

	// 멤버 접근 연산자: 객체 포인터는 ->, 구조체/원시는 .
	const TCHAR* MemberAccessor(const UEdGraphPin* Pin)
	{
		if (Pin && IsObjectLikePinCategory(Pin->PinType.PinCategory))
		{
			return TEXT("->");
		}
		return TEXT(".");
	}

	// 함수 호출 인자 (positional). C++ 스타일 일관성 우선 — 인자명은 정의 쪽 시그니처에서 확인.
	FString RenderCallArgs(UK2Node_CallFunction* Call, FRenderCtx& Ctx)
	{
		TArray<FString> Args;
		for (UEdGraphPin* Pin : Call->Pins)
		{
			if (!Pin || Pin->Direction != EGPD_Input || IsExecPin(Pin))
			{
				continue;
			}
			if (Pin->PinName == UEdGraphSchema_K2::PN_Self)
			{
				continue;
			}
			// 숨김 핀(WorldContextObject 등)은 사용자가 건드리지 않으면 스킵
			if (Pin->bHidden && Pin->LinkedTo.Num() == 0)
			{
				continue;
			}
			// 연결 없이 autogenerated default 그대로인 핀은 스킵 (Duration, Key, bPrintToScreen 등 노이즈 제거).
			if (Pin->LinkedTo.Num() == 0 && Pin->DefaultValue == Pin->AutogeneratedDefaultValue)
			{
				continue;
			}
			Args.Add(RenderDataInput(Pin, Ctx));
		}
		return FString::Join(Args, TEXT(", "));
	}

	// "Super::" / "Target->" / "Class::" / "" 중 하나를 반환.
	FString RenderCallTargetWithAccessor(UK2Node_CallFunction* Call, FRenderCtx& Ctx)
	{
		// 부모 함수 호출 노드는 UK2Node_CallFunction 의 서브클래스로, FunctionReference 가 부모 클래스를 가리킨다.
		if (Cast<UK2Node_CallParentFunction>(Call))
		{
			return TEXT("Super::");
		}
		UEdGraphPin* SelfPin = Call->FindPin(UEdGraphSchema_K2::PN_Self);
		if (SelfPin && SelfPin->LinkedTo.Num() > 0)
		{
			return RenderDataInput(SelfPin, Ctx) + TEXT("->");
		}
		if (Call->FunctionReference.IsSelfContext())
		{
			return FString();
		}
		if (UClass* MemberClass = Call->FunctionReference.GetMemberParentClass())
		{
			return FormatStructCPP(MemberClass) + TEXT("::");
		}
		return FString();
	}

	// UKismetMathLibrary 의 연산자성 함수(BooleanAND, EqualEqual_*, Add_* 등)를 C++ 연산자로 환원.
	// 매칭 실패 시 false 반환 → 호출자가 일반 함수 호출로 폴백.
	bool TryRenderAsOperator(UK2Node_CallFunction* Call, FRenderCtx& Ctx, FString& OutExpr)
	{
		UClass* Parent = Call->FunctionReference.GetMemberParentClass();
		if (!Parent || Parent->GetName() != TEXT("KismetMathLibrary"))
		{
			return false;
		}
		const FString FnName = Call->GetFunctionName().ToString();

		auto CollectOperands = [&](int32 Expected) -> TArray<FString>
		{
			TArray<FString> Operands;
			for (UEdGraphPin* Pin : Call->Pins)
			{
				if (!Pin || Pin->Direction != EGPD_Input || IsExecPin(Pin))
				{
					continue;
				}
				if (Pin->PinName == UEdGraphSchema_K2::PN_Self)
				{
					continue;
				}
				Operands.Add(RenderDataInput(Pin, Ctx));
				if (Operands.Num() >= Expected)
				{
					break;
				}
			}
			return Operands;
		};

		auto EmitBinary = [&](const TCHAR* Op) -> bool
		{
			TArray<FString> Operands = CollectOperands(2);
			if (Operands.Num() != 2)
			{
				return false;
			}
			OutExpr = FString::Printf(TEXT("(%s) %s (%s)"), *Operands[0], Op, *Operands[1]);
			return true;
		};

		// 단항 부정
		if (FnName == TEXT("Not_PreBool"))
		{
			TArray<FString> Operands = CollectOperands(1);
			if (Operands.Num() == 1)
			{
				OutExpr = FString::Printf(TEXT("!(%s)"), *Operands[0]);
				return true;
			}
			return false;
		}

		// 정확 일치 (boolean)
		if (FnName == TEXT("BooleanAND"))
		{
			return EmitBinary(TEXT("&&"));
		}
		if (FnName == TEXT("BooleanOR"))
		{
			return EmitBinary(TEXT("||"));
		}

		// 접두사 매칭 — LessEqual/GreaterEqual 을 Less/Greater 보다 먼저 체크해야 함.
		if (FnName.StartsWith(TEXT("LessEqual_")))
		{
			return EmitBinary(TEXT("<="));
		}
		if (FnName.StartsWith(TEXT("GreaterEqual_")))
		{
			return EmitBinary(TEXT(">="));
		}
		if (FnName.StartsWith(TEXT("EqualEqual_")))
		{
			return EmitBinary(TEXT("=="));
		}
		if (FnName.StartsWith(TEXT("NotEqual_")))
		{
			return EmitBinary(TEXT("!="));
		}
		if (FnName.StartsWith(TEXT("Less_")))
		{
			return EmitBinary(TEXT("<"));
		}
		if (FnName.StartsWith(TEXT("Greater_")))
		{
			return EmitBinary(TEXT(">"));
		}
		if (FnName.StartsWith(TEXT("Add_")))
		{
			return EmitBinary(TEXT("+"));
		}
		if (FnName.StartsWith(TEXT("Subtract_")))
		{
			return EmitBinary(TEXT("-"));
		}
		if (FnName.StartsWith(TEXT("Multiply_")))
		{
			return EmitBinary(TEXT("*"));
		}
		if (FnName.StartsWith(TEXT("Divide_")))
		{
			return EmitBinary(TEXT("/"));
		}
		return false;
	}

	FString RenderExpression(UEdGraphPin* OutputPin, FRenderCtx& Ctx)
	{
		if (!OutputPin)
		{
			return TEXT("?");
		}
		if (Ctx.ExprDepth >= MaxExpressionDepth)
		{
			return TEXT("...");
		}
		TGuardValue<int32> DepthGuard(Ctx.ExprDepth, Ctx.ExprDepth + 1);

		// SplitStructPin: sub-pin 을 만나면 부모 핀까지 거슬러 올라가 ".Member" 경로 누적 후 root pin 으로 정규화한다.
		// UE 5 split 핀명 규칙: `ParentPinName + "_" + MemberName` (EdGraphSchema_K2::SplitPin 참조).
		FString MemberSuffix;
		while (OutputPin->ParentPin)
		{
			FString Member = OutputPin->PinName.ToString();
			const FString ParentPrefix = OutputPin->ParentPin->PinName.ToString() + TEXT("_");
			Member.RemoveFromStart(ParentPrefix);
			MemberSuffix = TEXT(".") + Member + MemberSuffix;
			OutputPin = OutputPin->ParentPin;
		}

		auto ComputeBase = [&]() -> FString
		{
		UEdGraphNode* Node = OutputPin->GetOwningNode();
		if (!Node)
		{
			return TEXT("?");
		}

		if (Cast<UK2Node_Self>(Node))
		{
			return TEXT("this");
		}
		// Event / CustomEvent / FunctionEntry 의 출력 데이터 핀은 함수 파라미터에 해당 — 핀 이름으로 직접 참조.
		if (Cast<UK2Node_Event>(Node) || Cast<UK2Node_CustomEvent>(Node) || Cast<UK2Node_FunctionEntry>(Node))
		{
			if (!OutputPin->PinName.IsNone())
			{
				return OutputPin->PinName.ToString();
			}
		}
		if (UK2Node_VariableGet* VG = Cast<UK2Node_VariableGet>(Node))
		{
			const FString VarName = VG->GetVarNameString();
			// target 컨텍스트(Self 핀)가 연결돼 있으면 `Target->Var` 또는 `Target.Var` 로 표기.
			UEdGraphPin* SelfPin = VG->FindPin(UEdGraphSchema_K2::PN_Self);
			if (SelfPin && SelfPin->LinkedTo.Num() > 0)
			{
				return FString::Printf(TEXT("%s%s%s"),
					*RenderDataInput(SelfPin, Ctx), MemberAccessor(SelfPin), *VarName);
			}
			return VarName;
		}
		// VariableSet의 Output_Get pass-through 핀 — 방금 세팅한 값과 동치이므로 변수명만 반환.
		if (UK2Node_VariableSet* VS = Cast<UK2Node_VariableSet>(Node))
		{
			return VS->GetVarNameString();
		}
		if (UK2Node_Literal* Lit = Cast<UK2Node_Literal>(Node))
		{
			if (UObject* Obj = Lit->GetObjectRef())
			{
				return Obj->GetName();
			}
			FString Title = Lit->GetNodeTitle(ENodeTitleType::ListView).ToString();
			Title.ReplaceInline(TEXT("\n"), TEXT(" "));
			return Title;
		}
		if (UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Node))
		{
			const FString FnName = Call->GetFunctionName().ToString();
			// 참고: BP 자동 변환(Conv_*) 은 일반 함수 호출로 그대로 흘려보낸다. C-style 캐스트로 압축하면 Conv_VectorToString 같은
			// 비-numeric 변환에서 컴파일 불가 표현(`(FString)(Vector)`) 이 나오기 때문. numeric ↔ numeric 도 함수 호출 표기로 충분.
			// KismetMathLibrary 연산자성 함수(BooleanAND/EqualEqual_*/Add_* 등)는 연산자로 환원.
			{
				FString OperatorExpr;
				if (TryRenderAsOperator(Call, Ctx, OperatorExpr))
				{
					return OperatorExpr;
				}
			}
			// UKismetSystemLibrary::IsValid 는 UE 의 글로벌 IsValid() 와 동치 — 네임스페이스 생략.
			if (UClass* P = Call->FunctionReference.GetMemberParentClass())
			{
				if (P->GetName() == TEXT("KismetSystemLibrary") && FnName == TEXT("IsValid"))
				{
					return FString::Printf(TEXT("IsValid(%s)"), *RenderCallArgs(Call, Ctx));
				}
			}
			const FString Target = RenderCallTargetWithAccessor(Call, Ctx);
			const FString Args = RenderCallArgs(Call, Ctx);
			FString Expr = FString::Printf(TEXT("%s%s(%s)"), *Target, *FnName, *Args);
			// 다중 출력 함수(예: 구조체 분해)에서 ReturnValue 외 출력 핀을 참조한 경우 핀 이름으로 멤버 접근.
			if (OutputPin->PinName != UEdGraphSchema_K2::PN_ReturnValue && !OutputPin->PinName.IsNone())
			{
				Expr += FString::Printf(TEXT(".%s"), *OutputPin->PinName.ToString());
			}
			return Expr;
		}
		if (UK2Node_DynamicCast* DCast = Cast<UK2Node_DynamicCast>(Node))
		{
			const FString RawName = DCast->TargetType ? DCast->TargetType->GetName() : TEXT("Object");
			// exec 체인에서 이미 렌더돼 As<RawName> 지역변수가 선언된 경우, 데이터 참조는 그 변수로.
			if (Ctx.Visited.Contains(DCast))
			{
				return FString::Printf(TEXT("As%s"), *RawName);
			}
			UEdGraphPin* ObjIn = DCast->GetCastSourcePin();
			const FString TypeName = DCast->TargetType ? FormatStructCPP(DCast->TargetType) : TEXT("UObject");
			return FString::Printf(TEXT("Cast<%s>(%s)"), *TypeName, *RenderDataInput(ObjIn, Ctx));
		}
		// BreakStruct: 입력 struct 식 + ".MemberName" 으로 멤버 접근.
		if (UK2Node_BreakStruct* BS = Cast<UK2Node_BreakStruct>(Node))
		{
			UEdGraphPin* StructIn = nullptr;
			for (UEdGraphPin* P : BS->Pins)
			{
				if (P && P->Direction == EGPD_Input && !IsExecPin(P) && P->PinName != UEdGraphSchema_K2::PN_Self)
				{
					StructIn = P;
					break;
				}
			}
			return FString::Printf(TEXT("%s.%s"), *RenderDataInput(StructIn, Ctx), *OutputPin->PinName.ToString());
		}
		// MakeStruct: 출력 struct 핀 → positional 함수 캐스트 `FStruct(value1, value2, ...)`.
		// `variables` 필드의 struct 표기와 통일. BP 의 MakeStruct 핀 순서 = struct 멤버 선언 순서.
		if (UK2Node_MakeStruct* MS = Cast<UK2Node_MakeStruct>(Node))
		{
			TArray<FString> Args;
			for (UEdGraphPin* P : MS->Pins)
			{
				if (!P || P->Direction != EGPD_Input || IsExecPin(P) || P->bHidden)
				{
					continue;
				}
				Args.Add(RenderDataInput(P, Ctx));
			}
			const FString StructName = MS->StructType ? FormatStructCPP(MS->StructType) : TEXT("FStruct");
			return FString::Printf(TEXT("%s(%s)"), *StructName, *FString::Join(Args, TEXT(", ")));
		}
		// StructMemberGet: self 의 struct 변수에서 일부 멤버만 추출. self 컨텍스트 표기 + ".VarName.MemberName".
		if (UK2Node_StructMemberGet* SMG = Cast<UK2Node_StructMemberGet>(Node))
		{
			const FString VarName = SMG->GetVarNameString();
			UEdGraphPin* SelfPin = SMG->FindPin(UEdGraphSchema_K2::PN_Self);
			if (SelfPin && SelfPin->LinkedTo.Num() > 0)
			{
				return FString::Printf(TEXT("%s%s%s.%s"),
					*RenderDataInput(SelfPin, Ctx), MemberAccessor(SelfPin),
					*VarName, *OutputPin->PinName.ToString());
			}
			return FString::Printf(TEXT("%s.%s"), *VarName, *OutputPin->PinName.ToString());
		}
		// 비동기 task 출력 데이터 핀: exec 체인의 TryRenderAsyncTask 와 같은 변수명으로 `Task->Pin` 참조.
		// 콜백 본문에서 OutputPin 을 자연스럽게 가리키게 한다.
		if (UK2Node_BaseAsyncTask* Async = Cast<UK2Node_BaseAsyncTask>(Node))
		{
			const FString VarName = DeriveAsyncTaskVarName(Async);
			if (OutputPin->PinName.IsNone() || OutputPin->PinName == UEdGraphSchema_K2::PN_ReturnValue)
			{
				return VarName;
			}
			return FString::Printf(TEXT("%s->%s"), *VarName, *OutputPin->PinName.ToString());
		}

		// 매크로 인스턴스 데이터 핀 참조. exec 체인의 TryRenderMacro 와 이름 표기 통일(MacroGraph->GetName()).
		if (UK2Node_MacroInstance* MacroInst = Cast<UK2Node_MacroInstance>(Node))
		{
			UEdGraph* MacroGraph = MacroInst->GetMacroGraph();
			const FString MacroName = MacroGraph ? MacroGraph->GetName() : TEXT("Macro");
			if (!OutputPin->PinName.IsNone())
			{
				// 알려진 루프 매크로의 출력 데이터 핀(Array Element/Array Index/Index)은 본문에서 로컬 변수처럼 참조.
				if (IsKnownLoopMacro(MacroName))
				{
					return MacroPinToLocalVar(OutputPin->PinName);
				}
				return FString::Printf(TEXT("%s.%s"), *MacroName, *OutputPin->PinName.ToString());
			}
			return MacroName;
		}

		// 일반 K2Node fallback: NodeTitle + 필요시 출력 핀명
		FString Title = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
		Title.ReplaceInline(TEXT("\n"), TEXT(" "));
		if (!OutputPin->PinName.IsNone() && OutputPin->PinName != UEdGraphSchema_K2::PN_ReturnValue)
		{
			return FString::Printf(TEXT("%s.%s"), *Title, *OutputPin->PinName.ToString());
		}
		return Title;
		};
		return ComputeBase() + MemberSuffix;
	}

	void RenderExecChain(UEdGraphPin* ExecOutPin, int32 Indent, FRenderCtx& Ctx)
	{
		if (!ExecOutPin || ExecOutPin->LinkedTo.Num() == 0)
		{
			return;
		}
		UEdGraphPin* NextPin = SkipKnots(ExecOutPin->LinkedTo[0]);
		if (!NextPin)
		{
			return;
		}
		UEdGraphNode* NextNode = NextPin->GetOwningNode();
		if (!NextNode)
		{
			return;
		}
		// ForEachLoopWithBreak / ForLoopWithBreak 의 "Break" input exec 핀으로 흐름이 들어오면 C++ break 로 종결.
		if (Cast<UK2Node_MacroInstance>(NextNode) && NextPin->Direction == EGPD_Input && NextPin->PinName == TEXT("Break"))
		{
			Emit(Ctx, Indent, TEXT("break;"));
			return;
		}
		// 분기 합류/사이클: 같은 노드를 다시 만나면 합류 지점을 주석으로 남긴다.
		// 정확한 표현은 post-dominator 분석으로 합류점을 if-else 뒤로 빼내야 하지만, 본 도구 범위 초과.
		// 조용히 스킵하면 "이 분기는 합류 호출이 사라진 것처럼" 오해를 줘서 주석이 차라리 명확.
		if (Ctx.Visited.Contains(NextNode))
		{
			FString Title = NextNode->GetNodeTitle(ENodeTitleType::ListView).ToString();
			Title.ReplaceInline(TEXT("\n"), TEXT(" "));
			Emit(Ctx, Indent, FString::Printf(TEXT("// merges into: %s"), *Title));
			return;
		}
		Ctx.Visited.Add(NextNode);
		TGuardValue<UEdGraphPin*> ArrivedGuard(Ctx.ArrivedExecPin, NextPin);
		RenderExecNode(NextNode, Indent, Ctx);
	}

	bool TryRenderBranch(UEdGraphNode* Node, int32 Indent, FRenderCtx& Ctx)
	{
		UK2Node_IfThenElse* Branch = Cast<UK2Node_IfThenElse>(Node);
		if (!Branch)
		{
			return false;
		}
		Emit(Ctx, Indent, FString::Printf(TEXT("if (%s)"), *RenderDataInput(Branch->GetConditionPin(), Ctx)));
		Emit(Ctx, Indent, TEXT("{"));
		RenderExecChain(Branch->GetThenPin(), Indent + 1, Ctx);
		Emit(Ctx, Indent, TEXT("}"));
		UEdGraphPin* ElsePin = Branch->GetElsePin();
		if (ElsePin && ElsePin->LinkedTo.Num() > 0)
		{
			Emit(Ctx, Indent, TEXT("else"));
			Emit(Ctx, Indent, TEXT("{"));
			RenderExecChain(ElsePin, Indent + 1, Ctx);
			Emit(Ctx, Indent, TEXT("}"));
		}
		return true;
	}

	bool TryRenderSequence(UEdGraphNode* Node, int32 Indent, FRenderCtx& Ctx)
	{
		UK2Node_ExecutionSequence* Seq = Cast<UK2Node_ExecutionSequence>(Node);
		if (!Seq)
		{
			return false;
		}
		int32 Idx = 0;
		for (UEdGraphPin* Pin : Seq->Pins)
		{
			if (Pin && Pin->Direction == EGPD_Output && IsExecPin(Pin))
			{
				Emit(Ctx, Indent, FString::Printf(TEXT("// sequence[%d]"), Idx++));
				Emit(Ctx, Indent, TEXT("{"));
				RenderExecChain(Pin, Indent + 1, Ctx);
				Emit(Ctx, Indent, TEXT("}"));
			}
		}
		return true;
	}

	bool TryRenderVariableSet(UEdGraphNode* Node, int32 Indent, FRenderCtx& Ctx)
	{
		UK2Node_VariableSet* VS = Cast<UK2Node_VariableSet>(Node);
		if (!VS)
		{
			return false;
		}
		UEdGraphPin* ValPin = VS->FindPin(VS->GetVarName());
		// target.Var 가 아닌 인스턴스 멤버 세트는 좌변에 Self 표기 생략.
		UEdGraphPin* SelfPin = VS->FindPin(UEdGraphSchema_K2::PN_Self);
		FString Lhs;
		if (SelfPin && SelfPin->LinkedTo.Num() > 0)
		{
			Lhs = FString::Printf(TEXT("%s%s%s"),
				*RenderDataInput(SelfPin, Ctx), MemberAccessor(SelfPin), *VS->GetVarNameString());
		}
		else
		{
			Lhs = VS->GetVarNameString();
		}
		Emit(Ctx, Indent, FString::Printf(TEXT("%s = %s;"), *Lhs, *RenderDataInput(ValPin, Ctx)));
		RenderExecChain(FindThenPin(VS), Indent, Ctx);
		return true;
	}

	// StructMemberSet: 변수 struct 의 일부 멤버만 대입. 멤버 별로 한 줄씩 `Var.Member = Value;`.
	bool TryRenderStructMemberSet(UEdGraphNode* Node, int32 Indent, FRenderCtx& Ctx)
	{
		UK2Node_StructMemberSet* SMS = Cast<UK2Node_StructMemberSet>(Node);
		if (!SMS)
		{
			return false;
		}
		UEdGraphPin* SelfPin = SMS->FindPin(UEdGraphSchema_K2::PN_Self);
		const FString VarName = SMS->GetVarNameString();
		FString Lhs;
		if (SelfPin && SelfPin->LinkedTo.Num() > 0)
		{
			Lhs = FString::Printf(TEXT("%s%s%s"),
				*RenderDataInput(SelfPin, Ctx), MemberAccessor(SelfPin), *VarName);
		}
		else
		{
			Lhs = VarName;
		}
		for (UEdGraphPin* Pin : SMS->Pins)
		{
			if (!Pin || Pin->Direction != EGPD_Input || IsExecPin(Pin))
			{
				continue;
			}
			if (Pin->PinName == UEdGraphSchema_K2::PN_Self || Pin->bHidden)
			{
				continue;
			}
			Emit(Ctx, Indent, FString::Printf(TEXT("%s.%s = %s;"),
				*Lhs, *Pin->PinName.ToString(), *RenderDataInput(Pin, Ctx)));
		}
		RenderExecChain(FindThenPin(SMS), Indent, Ctx);
		return true;
	}

	bool TryRenderCallFunction(UEdGraphNode* Node, int32 Indent, FRenderCtx& Ctx)
	{
		UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Node);
		if (!Call)
		{
			return false;
		}
		Emit(Ctx, Indent, FString::Printf(TEXT("%s%s(%s);"),
			*RenderCallTargetWithAccessor(Call, Ctx), *Call->GetFunctionName().ToString(), *RenderCallArgs(Call, Ctx)));
		RenderExecChain(FindThenPin(Call), Indent, Ctx);
		return true;
	}

	bool TryRenderDynamicCast(UEdGraphNode* Node, int32 Indent, FRenderCtx& Ctx)
	{
		UK2Node_DynamicCast* DCast = Cast<UK2Node_DynamicCast>(Node);
		if (!DCast)
		{
			return false;
		}
		const FString TypeName = DCast->TargetType ? FormatStructCPP(DCast->TargetType) : TEXT("UObject");
		const FString RawName = DCast->TargetType ? DCast->TargetType->GetName() : TEXT("Object");
		Emit(Ctx, Indent, FString::Printf(TEXT("%s* As%s = Cast<%s>(%s);"),
			*TypeName, *RawName, *TypeName, *RenderDataInput(DCast->GetCastSourcePin(), Ctx)));
		UEdGraphPin* Success = DCast->GetValidCastPin();
		UEdGraphPin* Failed = DCast->GetInvalidCastPin();
		if (Success && Success->LinkedTo.Num() > 0)
		{
			Emit(Ctx, Indent, FString::Printf(TEXT("if (As%s)"), *RawName));
			Emit(Ctx, Indent, TEXT("{"));
			RenderExecChain(Success, Indent + 1, Ctx);
			Emit(Ctx, Indent, TEXT("}"));
		}
		if (Failed && Failed->LinkedTo.Num() > 0)
		{
			Emit(Ctx, Indent, TEXT("else"));
			Emit(Ctx, Indent, TEXT("{"));
			RenderExecChain(Failed, Indent + 1, Ctx);
			Emit(Ctx, Indent, TEXT("}"));
		}
		return true;
	}

	// 케이스 핀의 PinName 을 C++ switch 케이스 라벨로 변환. SwitchEnum 은 EnumName:: 접두사, String/Name 은 따옴표.
	FString FormatSwitchCaseLabel(UK2Node_Switch* Switch, const UEdGraphPin* Pin)
	{
		if (!Switch || !Pin)
		{
			return TEXT("?");
		}
		const FString PinNameStr = Pin->PinName.ToString();
		if (UK2Node_SwitchEnum* SwitchEnum = Cast<UK2Node_SwitchEnum>(Switch))
		{
			if (UEnum* E = SwitchEnum->Enum)
			{
				return FString::Printf(TEXT("%s::%s"), *E->GetName(), *PinNameStr);
			}
			return PinNameStr;
		}
		const UEdGraphPin* SelectionPin = Switch->GetSelectionPin();
		const FName Cat = SelectionPin ? SelectionPin->PinType.PinCategory : NAME_None;
		if (Cat == UEdGraphSchema_K2::PC_String)
		{
			return FString::Printf(TEXT("TEXT(\"%s\")"), *PinNameStr);
		}
		if (Cat == UEdGraphSchema_K2::PC_Name)
		{
			return FString::Printf(TEXT("FName(TEXT(\"%s\"))"), *PinNameStr);
		}
		return PinNameStr;
	}

	bool TryRenderSwitch(UEdGraphNode* Node, int32 Indent, FRenderCtx& Ctx)
	{
		UK2Node_Switch* Switch = Cast<UK2Node_Switch>(Node);
		if (!Switch)
		{
			return false;
		}
		UEdGraphPin* SelectionPin = Switch->GetSelectionPin();
		UEdGraphPin* DefaultPin = Switch->bHasDefaultPin ? Switch->GetDefaultPin() : nullptr;

		Emit(Ctx, Indent, FString::Printf(TEXT("switch (%s)"), *RenderDataInput(SelectionPin, Ctx)));
		Emit(Ctx, Indent, TEXT("{"));
		for (UEdGraphPin* Pin : Switch->Pins)
		{
			if (!Pin || Pin->Direction != EGPD_Output || !IsExecPin(Pin))
			{
				continue;
			}
			if (Pin == DefaultPin)
			{
				continue;
			}
			Emit(Ctx, Indent, FString::Printf(TEXT("case %s:"), *FormatSwitchCaseLabel(Switch, Pin)));
			if (Pin->LinkedTo.Num() > 0)
			{
				RenderExecChain(Pin, Indent + 1, Ctx);
			}
			Emit(Ctx, Indent + 1, TEXT("break;"));
		}
		if (DefaultPin)
		{
			Emit(Ctx, Indent, TEXT("default:"));
			if (DefaultPin->LinkedTo.Num() > 0)
			{
				RenderExecChain(DefaultPin, Indent + 1, Ctx);
			}
			Emit(Ctx, Indent + 1, TEXT("break;"));
		}
		Emit(Ctx, Indent, TEXT("}"));
		return true;
	}

	// Async task 의 proxy 객체 변수명. 두 호출자(TryRenderAsyncTask, RenderExpression) 에서 동일하게 도출되어야
	// 콜백 본문에서 `ProxyVar->OutputPin` 형태로 일관되게 참조 가능.
	FString DeriveAsyncTaskVarName(UK2Node_BaseAsyncTask* Async)
	{
		UClass* ProxyClass = GetAsyncProxyClass(Async);
		if (!ProxyClass)
		{
			return TEXT("Task");
		}
		// `AbilityTask_PlayMontageAndWait` → `PlayMontageAndWait`. AbilityTask/AsyncAction 접두사도 추가로 트리밍.
		FString Name = ProxyClass->GetName();
		int32 LastUnder = INDEX_NONE;
		if (Name.FindLastChar(TEXT('_'), LastUnder))
		{
			Name = Name.Mid(LastUnder + 1);
		}
		else if (Name.StartsWith(TEXT("AsyncAction"), ESearchCase::CaseSensitive))
		{
			Name = Name.RightChop(11); // strlen("AsyncAction")
		}
		else if (Name.StartsWith(TEXT("AbilityTask"), ESearchCase::CaseSensitive))
		{
			Name = Name.RightChop(11); // strlen("AbilityTask")
		}
		// 트리밍 후 빈 문자열이거나 타입명과 동일한 경우 폴백.
		if (Name.IsEmpty() || Name == ProxyClass->GetName())
		{
			return TEXT("Task");
		}
		return Name;
	}

	// Latent async task 노드 (PlayMontageAndWait, NewPlayAnimationProxyObject 등). UE 의 표준 패턴은
	// `auto* Task = Class::Factory(...); Task->Delegate.AddDynamic(this, ...); Task->Activate();` 이므로
	// 그 형태를 따라 의사코드를 만든다.
	bool TryRenderAsyncTask(UEdGraphNode* Node, int32 Indent, FRenderCtx& Ctx)
	{
		UK2Node_BaseAsyncTask* Async = Cast<UK2Node_BaseAsyncTask>(Node);
		if (!Async)
		{
			return false;
		}

		// 입력 데이터 핀들을 인자로 모은다 (CallFunction 과 동일한 필터링).
		TArray<FString> Args;
		for (UEdGraphPin* Pin : Async->Pins)
		{
			if (!Pin || Pin->Direction != EGPD_Input || IsExecPin(Pin))
			{
				continue;
			}
			if (Pin->PinName == UEdGraphSchema_K2::PN_Self)
			{
				continue;
			}
			if (Pin->bHidden && Pin->LinkedTo.Num() == 0)
			{
				continue;
			}
			if (Pin->LinkedTo.Num() == 0 && Pin->DefaultValue == Pin->AutogeneratedDefaultValue)
			{
				continue;
			}
			Args.Add(RenderDataInput(Pin, Ctx));
		}

		// `Class::FactoryFunc(args)` 호출식. ProxyFactory 메타가 비어있으면 NodeTitle 폴백.
		FString FactoryCall;
		if (UFunction* FactoryFn = Async->GetFactoryFunction())
		{
			UClass* ProxyFactoryClass = GetAsyncProxyFactoryClass(Async);
			UClass* OwnerClass = ProxyFactoryClass ? ProxyFactoryClass : FactoryFn->GetOuterUClass();
			if (OwnerClass)
			{
				FactoryCall = FString::Printf(TEXT("%s::%s(%s)"),
					*FormatStructCPP(OwnerClass), *FactoryFn->GetName(), *FString::Join(Args, TEXT(", ")));
			}
			else
			{
				FactoryCall = FString::Printf(TEXT("%s(%s)"), *FactoryFn->GetName(), *FString::Join(Args, TEXT(", ")));
			}
		}
		else
		{
			FString Title = Async->GetNodeTitle(ENodeTitleType::ListView).ToString();
			Title.ReplaceInline(TEXT("\n"), TEXT(" "));
			FactoryCall = FString::Printf(TEXT("%s(%s)"), *Title, *FString::Join(Args, TEXT(", ")));
		}

		const FString VarName = DeriveAsyncTaskVarName(Async);
		UClass* ProxyClass = GetAsyncProxyClass(Async);
		const FString ProxyTypeCpp = ProxyClass ? FormatStructCPP(ProxyClass) : TEXT("auto*");
		Emit(Ctx, Indent, FString::Printf(TEXT("%s* %s = %s;  // [latent]"), *ProxyTypeCpp, *VarName, *FactoryCall));

		// 출력 exec 핀들: `then` 은 즉시 연속 흐름, 그 외는 multicast delegate 바인딩.
		// BP 의 multicast delegate 는 dynamic 이지만, 의사코드에선 람다 가독성을 위해 AddLambda 표기.
		UEdGraphPin* ThenPin = nullptr;
		for (UEdGraphPin* Pin : Async->Pins)
		{
			if (!Pin || Pin->Direction != EGPD_Output || !IsExecPin(Pin))
			{
				continue;
			}
			if (Pin->PinName == UEdGraphSchema_K2::PN_Then)
			{
				ThenPin = Pin;
				continue;
			}
			Emit(Ctx, Indent, FString::Printf(TEXT("%s->%s.AddLambda([this]()"),
				*VarName, *Pin->PinName.ToString()));
			Emit(Ctx, Indent, TEXT("{"));
			RenderExecChain(Pin, Indent + 1, Ctx);
			Emit(Ctx, Indent, TEXT("});"));
		}

		// Activate() — UBlueprintAsyncActionBase 파생은 BP 컴파일러가 자동 호출하므로 emit 생략.
		// GameplayTask(예: AbilityTask) 등은 명시적 호출이 정석 패턴.
		const FName ProxyActivateFunctionName = GetAsyncProxyActivateFunctionName(Async);
		const bool bIsBlueprintAsyncAction = ProxyClass && ProxyClass->IsChildOf(UBlueprintAsyncActionBase::StaticClass());
		if (!ProxyActivateFunctionName.IsNone() && !bIsBlueprintAsyncAction)
		{
			Emit(Ctx, Indent, FString::Printf(TEXT("%s->%s();"), *VarName, *ProxyActivateFunctionName.ToString()));
		}

		if (ThenPin)
		{
			RenderExecChain(ThenPin, Indent, Ctx);
		}
		return true;
	}

	// ForEachLoop / ForEachLoopWithBreak / ReverseForEachLoop → C++ for 루프.
	bool RenderForEachMacro(UK2Node_MacroInstance* Macro, const FString& MacroName, int32 Indent, FRenderCtx& Ctx)
	{
		const bool bIsForEach = MacroName == TEXT("ForEachLoop") || MacroName == TEXT("ForEachLoopWithBreak");
		const bool bIsReverse = MacroName == TEXT("ReverseForEachLoop");
		if (!bIsForEach && !bIsReverse)
		{
			return false;
		}
		UEdGraphPin* ArrayPin = Macro->FindPin(TEXT("Array"));
		UEdGraphPin* LoopBody = Macro->FindPin(TEXT("LoopBody"));
		UEdGraphPin* Completed = Macro->FindPin(TEXT("Completed"));
		const FString ArrayExpr = ArrayPin ? RenderDataInput(ArrayPin, Ctx) : TEXT("?");

		if (bIsReverse)
		{
			// 역순 순회는 인덱스 기반 — ArrayElement / ArrayIndex 둘 다 본문에서 사용 가능.
			Emit(Ctx, Indent, FString::Printf(TEXT("for (int32 ArrayIndex = %s.Num() - 1; ArrayIndex >= 0; --ArrayIndex)"), *ArrayExpr));
			Emit(Ctx, Indent, TEXT("{"));
			Emit(Ctx, Indent + 1, FString::Printf(TEXT("auto& ArrayElement = %s[ArrayIndex];"), *ArrayExpr));
			if (LoopBody)
			{
				RenderExecChain(LoopBody, Indent + 1, Ctx);
			}
			Emit(Ctx, Indent, TEXT("}"));
		}
		else
		{
			Emit(Ctx, Indent, FString::Printf(TEXT("for (auto& ArrayElement : %s)"), *ArrayExpr));
			Emit(Ctx, Indent, TEXT("{"));
			if (LoopBody)
			{
				RenderExecChain(LoopBody, Indent + 1, Ctx);
			}
			Emit(Ctx, Indent, TEXT("}"));
		}
		if (Completed)
		{
			RenderExecChain(Completed, Indent, Ctx);
		}
		return true;
	}

	// ForLoop / ForLoopWithBreak → for (int32 Index = First; Index <= Last; ++Index).
	bool RenderForLoopMacro(UK2Node_MacroInstance* Macro, const FString& MacroName, int32 Indent, FRenderCtx& Ctx)
	{
		if (MacroName != TEXT("ForLoop") && MacroName != TEXT("ForLoopWithBreak"))
		{
			return false;
		}
		UEdGraphPin* FirstPin = Macro->FindPin(TEXT("First Index"));
		UEdGraphPin* LastPin = Macro->FindPin(TEXT("Last Index"));
		UEdGraphPin* LoopBody = Macro->FindPin(TEXT("LoopBody"));
		UEdGraphPin* Completed = Macro->FindPin(TEXT("Completed"));
		const FString FirstExpr = FirstPin ? RenderDataInput(FirstPin, Ctx) : TEXT("0");
		const FString LastExpr = LastPin ? RenderDataInput(LastPin, Ctx) : TEXT("0");
		Emit(Ctx, Indent, FString::Printf(TEXT("for (int32 Index = %s; Index <= %s; ++Index)"), *FirstExpr, *LastExpr));
		Emit(Ctx, Indent, TEXT("{"));
		if (LoopBody)
		{
			RenderExecChain(LoopBody, Indent + 1, Ctx);
		}
		Emit(Ctx, Indent, TEXT("}"));
		if (Completed)
		{
			RenderExecChain(Completed, Indent, Ctx);
		}
		return true;
	}

	// WhileLoop → while (Condition).
	bool RenderWhileLoopMacro(UK2Node_MacroInstance* Macro, const FString& MacroName, int32 Indent, FRenderCtx& Ctx)
	{
		if (MacroName != TEXT("WhileLoop"))
		{
			return false;
		}
		UEdGraphPin* CondPin = Macro->FindPin(TEXT("Condition"));
		UEdGraphPin* LoopBody = Macro->FindPin(TEXT("LoopBody"));
		UEdGraphPin* Completed = Macro->FindPin(TEXT("Completed"));
		const FString CondExpr = CondPin ? RenderDataInput(CondPin, Ctx) : TEXT("true");
		Emit(Ctx, Indent, FString::Printf(TEXT("while (%s)"), *CondExpr));
		Emit(Ctx, Indent, TEXT("{"));
		if (LoopBody)
		{
			RenderExecChain(LoopBody, Indent + 1, Ctx);
		}
		Emit(Ctx, Indent, TEXT("}"));
		if (Completed)
		{
			RenderExecChain(Completed, Indent, Ctx);
		}
		return true;
	}

	bool TryRenderMacro(UEdGraphNode* Node, int32 Indent, FRenderCtx& Ctx)
	{
		UK2Node_MacroInstance* Macro = Cast<UK2Node_MacroInstance>(Node);
		if (!Macro)
		{
			return false;
		}
		UEdGraph* MacroGraph = Macro->GetMacroGraph();
		const FString MacroName = MacroGraph ? MacroGraph->GetName() : TEXT("Macro");

		// 알려진 루프 매크로는 C++ 루프 키워드(for / while)로 변환.
		if (RenderForEachMacro(Macro, MacroName, Indent, Ctx))
		{
			return true;
		}
		if (RenderForLoopMacro(Macro, MacroName, Indent, Ctx))
		{
			return true;
		}
		if (RenderWhileLoopMacro(Macro, MacroName, Indent, Ctx))
		{
			return true;
		}

		// 그 외 매크로: LoopBody/Completed 가 있으면 호출-블록 형태, 없으면 단순 호출.
		TArray<FString> Args;
		for (UEdGraphPin* Pin : Macro->Pins)
		{
			if (!Pin || Pin->Direction != EGPD_Input || IsExecPin(Pin))
			{
				continue;
			}
			if (Pin->bHidden && Pin->LinkedTo.Num() == 0)
			{
				continue;
			}
			Args.Add(RenderDataInput(Pin, Ctx));
		}

		// `// [macro]` 마커: BP 매크로는 C++ 함수와 의미가 다르므로(인라인 확장, latent 노드 가능) 호출 라인에 명시.
		UEdGraphPin* LoopBody = Macro->FindPin(TEXT("LoopBody"));
		UEdGraphPin* Completed = Macro->FindPin(TEXT("Completed"));
		if (LoopBody || Completed)
		{
			Emit(Ctx, Indent, FString::Printf(TEXT("%s(%s)  // [macro]"), *MacroName, *FString::Join(Args, TEXT(", "))));
			Emit(Ctx, Indent, TEXT("{"));
			if (LoopBody)
			{
				RenderExecChain(LoopBody, Indent + 1, Ctx);
			}
			Emit(Ctx, Indent, TEXT("}"));
			if (Completed)
			{
				RenderExecChain(Completed, Indent, Ctx);
			}
			return true;
		}

		Emit(Ctx, Indent, FString::Printf(TEXT("%s(%s);  // [macro]"), *MacroName, *FString::Join(Args, TEXT(", "))));
		RenderExecChain(FindThenPin(Macro), Indent, Ctx);
		return true;
	}

	// 매크로 그래프의 exit 터널: 입력 exec 핀 하나에 도달했을 때 macro 의 해당 output exec 로 흐름이 빠져나간다.
	// `// exit -> PinName(OutData = Expr, ...)` 형태로 표기. ArrivedExecPin 으로 어느 출구인지 식별.
	bool TryRenderTunnelExit(UEdGraphNode* Node, int32 Indent, FRenderCtx& Ctx)
	{
		UK2Node_Tunnel* Tunnel = Cast<UK2Node_Tunnel>(Node);
		if (!Tunnel)
		{
			return false;
		}
		// exit 터널은 output 을 가질 수 없음 (bCanHaveOutputs == false). entry 터널은 여기서 처리하지 않는다.
		if (Tunnel->bCanHaveOutputs)
		{
			return false;
		}

		FString ExitLabel;
		if (Ctx.ArrivedExecPin && Ctx.ArrivedExecPin->GetOwningNode() == Tunnel)
		{
			ExitLabel = Ctx.ArrivedExecPin->PinName.ToString();
		}

		TArray<FString> Assigns;
		for (UEdGraphPin* Pin : Tunnel->Pins)
		{
			if (!Pin || Pin->Direction != EGPD_Input || IsExecPin(Pin) || Pin->bHidden)
			{
				continue;
			}
			Assigns.Add(FString::Printf(TEXT("%s = %s"), *Pin->PinName.ToString(), *RenderDataInput(Pin, Ctx)));
		}

		const FString JoinedAssigns = FString::Join(Assigns, TEXT(", "));
		if (!ExitLabel.IsEmpty() && !JoinedAssigns.IsEmpty())
		{
			Emit(Ctx, Indent, FString::Printf(TEXT("// exit -> %s(%s)"), *ExitLabel, *JoinedAssigns));
		}
		else if (!ExitLabel.IsEmpty())
		{
			Emit(Ctx, Indent, FString::Printf(TEXT("// exit -> %s"), *ExitLabel));
		}
		else if (!JoinedAssigns.IsEmpty())
		{
			Emit(Ctx, Indent, FString::Printf(TEXT("// exit(%s)"), *JoinedAssigns));
		}
		else
		{
			Emit(Ctx, Indent, TEXT("// exit"));
		}
		return true;
	}

	bool TryRenderReturn(UEdGraphNode* Node, int32 Indent, FRenderCtx& Ctx)
	{
		UK2Node_FunctionResult* Ret = Cast<UK2Node_FunctionResult>(Node);
		if (!Ret)
		{
			return false;
		}
		TArray<UEdGraphPin*> ReturnPins;
		for (UEdGraphPin* Pin : Ret->Pins)
		{
			if (Pin && Pin->Direction == EGPD_Input && !IsExecPin(Pin))
			{
				ReturnPins.Add(Pin);
			}
		}
		if (ReturnPins.Num() == 0)
		{
			Emit(Ctx, Indent, TEXT("return;"));
		}
		else if (ReturnPins.Num() == 1)
		{
			Emit(Ctx, Indent, FString::Printf(TEXT("return %s;"), *RenderDataInput(ReturnPins[0], Ctx)));
		}
		else
		{
			// 다중 반환은 C++ 에 직접 대응 없음 — braced-init-list 형태로 묶어 표기.
			TArray<FString> Parts;
			for (UEdGraphPin* Pin : ReturnPins)
			{
				Parts.Add(RenderDataInput(Pin, Ctx));
			}
			Emit(Ctx, Indent, FString::Printf(TEXT("return { %s };"), *FString::Join(Parts, TEXT(", "))));
		}
		return true;
	}

	void RenderFallbackNode(UEdGraphNode* Node, int32 Indent, FRenderCtx& Ctx)
	{
		FString Title = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
		Title.ReplaceInline(TEXT("\n"), TEXT(" "));
		Emit(Ctx, Indent, FString::Printf(TEXT("// %s"), *Title));
		RenderExecChain(FindThenPin(Node), Indent, Ctx);
	}

	void RenderExecNode(UEdGraphNode* Node, int32 Indent, FRenderCtx& Ctx)
	{
		if (TryRenderBranch(Node, Indent, Ctx))
		{
			return;
		}
		if (TryRenderSequence(Node, Indent, Ctx))
		{
			return;
		}
		if (TryRenderSwitch(Node, Indent, Ctx))
		{
			return;
		}
		if (TryRenderVariableSet(Node, Indent, Ctx))
		{
			return;
		}
		if (TryRenderStructMemberSet(Node, Indent, Ctx))
		{
			return;
		}
		if (TryRenderCallFunction(Node, Indent, Ctx))
		{
			return;
		}
		if (TryRenderDynamicCast(Node, Indent, Ctx))
		{
			return;
		}
		if (TryRenderAsyncTask(Node, Indent, Ctx))
		{
			return;
		}
		if (TryRenderMacro(Node, Indent, Ctx))
		{
			return;
		}
		if (TryRenderReturn(Node, Indent, Ctx))
		{
			return;
		}
		if (TryRenderTunnelExit(Node, Indent, Ctx))
		{
			return;
		}
		RenderFallbackNode(Node, Indent, Ctx);
	}

	// 함수 본문 그래프에서 결과 노드를 찾아 단일 반환 타입을 추론. 없으면 void.
	FString InferReturnTypeCPP(UEdGraph* Graph)
	{
		if (!Graph)
		{
			return TEXT("void");
		}
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			UK2Node_FunctionResult* Result = Cast<UK2Node_FunctionResult>(Node);
			if (!Result)
			{
				continue;
			}
			TArray<UEdGraphPin*> ReturnPins;
			for (UEdGraphPin* Pin : Result->Pins)
			{
				if (Pin && Pin->Direction == EGPD_Input && !IsExecPin(Pin))
				{
					ReturnPins.Add(Pin);
				}
			}
			if (ReturnPins.Num() == 1)
			{
				return FormatPinTypeCPP(ReturnPins[0]->PinType);
			}
			return TEXT("void"); // 0 또는 다중 반환 → void 로 표기 (다중은 본문에서 묶음 반환으로 표시).
		}
		return TEXT("void");
	}

	// 이벤트/함수 시그니처에 노출되지 않는 핀: exec, hidden, delegate 출력핀(BP 내부 OutputDelegate 등).
	bool IsEntryParamPin(const UEdGraphPin* Pin)
	{
		if (!Pin || Pin->Direction != EGPD_Output || IsExecPin(Pin))
		{
			return false;
		}
		if (Pin->bHidden)
		{
			return false;
		}
		const FName Cat = Pin->PinType.PinCategory;
		if (Cat == UEdGraphSchema_K2::PC_Delegate || Cat == UEdGraphSchema_K2::PC_MCDelegate)
		{
			return false;
		}
		return true;
	}

	// 네트워킹 specifier (NetMulticast/Server/Client + Reliable/Unreliable). 멀티플레이어 거동에 영향.
	void CollectNetworkingSpecifiers(int32 FuncFlags, TArray<FString>& OutSpecifiers)
	{
		const bool bHasNet = (FuncFlags & (FUNC_NetMulticast | FUNC_NetServer | FUNC_NetClient)) != 0;
		if (FuncFlags & FUNC_NetMulticast)
		{
			OutSpecifiers.Add(TEXT("NetMulticast"));
		}
		if (FuncFlags & FUNC_NetServer)
		{
			OutSpecifiers.Add(TEXT("Server"));
		}
		if (FuncFlags & FUNC_NetClient)
		{
			OutSpecifiers.Add(TEXT("Client"));
		}
		// Net specifier 가 있으면 Reliable/Unreliable 명시 (UE C++ 컨벤션상 둘 중 하나 필수).
		if (bHasNet)
		{
			OutSpecifiers.Add((FuncFlags & FUNC_NetReliable) ? TEXT("Reliable") : TEXT("Unreliable"));
		}
	}

	bool BuildEntryHeader(UEdGraphNode* Entry, FPseudoEntry& OutEntry)
	{
		if (UK2Node_CustomEvent* Ce = Cast<UK2Node_CustomEvent>(Entry))
		{
			TArray<FString> Params;
			for (UEdGraphPin* Pin : Ce->Pins)
			{
				if (IsEntryParamPin(Pin))
				{
					Params.Add(FString::Printf(TEXT("%s %s"),
						*FormatPinTypeCPP(Pin->PinType), *Pin->PinName.ToString()));
				}
			}
			CollectNetworkingSpecifiers(static_cast<int32>(Ce->GetNetFlags()), OutEntry.UFunctionSpecifiers);
			OutEntry.Name = Ce->CustomFunctionName.ToString();
			OutEntry.Signature = FString::Printf(TEXT("void %s(%s)"),
				*OutEntry.Name, *FString::Join(Params, TEXT(", ")));
			return true;
		}
		if (UK2Node_Event* Ev = Cast<UK2Node_Event>(Entry))
		{
			// 부모 함수 오버라이드 — specifier 는 부모 선언이 정하므로 여기선 추가하지 않음.
			TArray<FString> Params;
			for (UEdGraphPin* Pin : Ev->Pins)
			{
				if (IsEntryParamPin(Pin))
				{
					Params.Add(FString::Printf(TEXT("%s %s"),
						*FormatPinTypeCPP(Pin->PinType), *Pin->PinName.ToString()));
				}
			}
			OutEntry.Name = Ev->EventReference.GetMemberName().ToString();
			OutEntry.Signature = FString::Printf(TEXT("void %s(%s)"),
				*OutEntry.Name, *FString::Join(Params, TEXT(", ")));
			return true;
		}
		// 매크로/composite entry 터널: 출력 데이터 핀 = 매크로 입력 파라미터.
		// UK2Node_FunctionEntry 는 Tunnel 의 서브클래스가 아니므로 (둘 다 UK2Node_EditablePinBase 형제) 캐스트가 섞이지 않는다.
		if (UK2Node_Tunnel* Tunnel = Cast<UK2Node_Tunnel>(Entry))
		{
			// 안전 차원: bCanHaveInputs == true 인 터널은 exit 터널이므로 entry 로 다루지 않는다.
			if (Tunnel->bCanHaveInputs)
			{
				return false;
			}
			TArray<FString> Params;
			for (UEdGraphPin* Pin : Tunnel->Pins)
			{
				if (IsEntryParamPin(Pin))
				{
					Params.Add(FString::Printf(TEXT("%s %s"),
						*FormatPinTypeCPP(Pin->PinType), *Pin->PinName.ToString()));
				}
			}
			const FString GraphName = Tunnel->GetGraph() ? Tunnel->GetGraph()->GetName() : TEXT("Macro");
			OutEntry.Name = GraphName;
			OutEntry.Signature = FString::Printf(TEXT("void %s(%s)  // [macro]"),
				*OutEntry.Name, *FString::Join(Params, TEXT(", ")));
			return true;
		}
		if (UK2Node_FunctionEntry* Fe = Cast<UK2Node_FunctionEntry>(Entry))
		{
			TArray<FString> Params;
			for (UEdGraphPin* Pin : Fe->Pins)
			{
				if (IsEntryParamPin(Pin))
				{
					Params.Add(FString::Printf(TEXT("%s %s"),
						*FormatPinTypeCPP(Pin->PinType), *Pin->PinName.ToString()));
				}
			}
			const FName FnName = Fe->CustomGeneratedFunctionName.IsNone()
				? Fe->FunctionReference.GetMemberName()
				: Fe->CustomGeneratedFunctionName;
			const FString ReturnType = InferReturnTypeCPP(Fe->GetGraph());
			// FUNC_Const → C++ 한정자로 시그니처에 직접. FUNC_BlueprintPure / 네트워킹 / CallInEditor 는 UFUNCTION 매크로.
			const int32 FuncFlags = Fe->GetFunctionFlags();
			if (FuncFlags & FUNC_BlueprintPure)
			{
				OutEntry.UFunctionSpecifiers.Add(TEXT("BlueprintPure"));
			}
			CollectNetworkingSpecifiers(FuncFlags, OutEntry.UFunctionSpecifiers);
			if (Fe->MetaData.bCallInEditor)
			{
				OutEntry.UFunctionSpecifiers.Add(TEXT("CallInEditor"));
			}
			const TCHAR* ConstSuffix = (FuncFlags & FUNC_Const) ? TEXT(" const") : TEXT("");
			OutEntry.Name = FnName.ToString();
			OutEntry.Signature = FString::Printf(TEXT("%s %s(%s)%s"),
				*ReturnType, *OutEntry.Name, *FString::Join(Params, TEXT(", ")), ConstSuffix);
			return true;
		}
		return false;
	}

	bool RenderEntryNode(UEdGraphNode* Entry, FPseudoEntry& OutEntry)
	{
		if (!BuildEntryHeader(Entry, OutEntry))
		{
			return false;
		}
		TSet<UEdGraphNode*> Visited;
		Visited.Add(Entry);
		FRenderCtx Ctx{ OutEntry.Body, Visited };

		// 매크로 entry 터널은 출력 exec 핀 여러 개를 가질 수 있다 (매크로 인스턴스의 입력 exec 핀 개수와 매칭).
		// 각각 별개의 시작점이므로 라벨 + 본문으로 렌더한다. 단일 exec 면 함수와 동일하게 표기.
		if (UK2Node_Tunnel* Tunnel = Cast<UK2Node_Tunnel>(Entry))
		{
			TArray<UEdGraphPin*> ExecOuts;
			for (UEdGraphPin* Pin : Tunnel->Pins)
			{
				if (Pin && Pin->Direction == EGPD_Output && IsExecPin(Pin))
				{
					ExecOuts.Add(Pin);
				}
			}
			if (ExecOuts.Num() <= 1)
			{
				if (ExecOuts.Num() == 1)
				{
					RenderExecChain(ExecOuts[0], 1, Ctx);
				}
			}
			else
			{
				for (UEdGraphPin* Pin : ExecOuts)
				{
					Emit(Ctx, 1, FString::Printf(TEXT("// entry: %s"), *Pin->PinName.ToString()));
					RenderExecChain(Pin, 1, Ctx);
				}
			}
			return true;
		}

		RenderExecChain(FindThenPin(Entry), 1, Ctx);
		return true;
	}

	TArray<FPseudoEntry> RenderGraphPseudoCode(UEdGraph* Graph)
	{
		TArray<FPseudoEntry> Entries;
		if (!Graph)
		{
			return Entries;
		}

		TArray<UEdGraphNode*> EntryNodes;
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (!Node)
			{
				continue;
			}
			if (Cast<UK2Node_Event>(Node) || Cast<UK2Node_CustomEvent>(Node) || Cast<UK2Node_FunctionEntry>(Node))
			{
				EntryNodes.Add(Node);
				continue;
			}
			// 매크로/composite 그래프 entry 터널 (출력 전용 터널) 도 entry 로 취급.
			if (UK2Node_Tunnel* Tunnel = Cast<UK2Node_Tunnel>(Node))
			{
				if (!Tunnel->bCanHaveInputs)
				{
					EntryNodes.Add(Node);
				}
			}
		}

		EntryNodes.Sort([](UEdGraphNode& A, UEdGraphNode& B)
		{
			return A.GetNodeTitle(ENodeTitleType::ListView).ToString() < B.GetNodeTitle(ENodeTitleType::ListView).ToString();
		});

		for (UEdGraphNode* Node : EntryNodes)
		{
			FPseudoEntry Entry;
			if (RenderEntryNode(Node, Entry) && Entry.Body.Num() > 0)
			{
				Entries.Add(MoveTemp(Entry));
			}
		}
		return Entries;
	}

	// 엔트리들을 라인 배열로 직렬화. 각 함수는 시그니처/{ 본문 }/공백 라인으로 구분.
	void AppendEntriesToLines(TArray<TSharedPtr<FJsonValue>>& OutLines, const TArray<FPseudoEntry>& Entries)
	{
		for (const FPseudoEntry& Entry : Entries)
		{
			if (OutLines.Num() > 0)
			{
				OutLines.Add(MakeShared<FJsonValueString>(FString()));
			}
			if (Entry.UFunctionSpecifiers.Num() > 0)
			{
				OutLines.Add(MakeShared<FJsonValueString>(FString::Printf(TEXT("UFUNCTION(%s)"),
					*FString::Join(Entry.UFunctionSpecifiers, TEXT(", ")))));
			}
			OutLines.Add(MakeShared<FJsonValueString>(Entry.Signature));
			OutLines.Add(MakeShared<FJsonValueString>(TEXT("{")));
			for (const FPseudoLine& Line : Entry.Body)
			{
				const FString Pad = FString::ChrN(Line.Indent * IndentSpaces, TEXT(' '));
				OutLines.Add(MakeShared<FJsonValueString>(Pad + Line.Text));
			}
			OutLines.Add(MakeShared<FJsonValueString>(TEXT("}")));
		}
	}
}

TSharedPtr<FJsonObject> FWxBlueprintSnapshotExporter::BuildEventGraphJson(UBlueprint* Blueprint)
{
	if (!Blueprint)
	{
		return nullptr;
	}

	// 모든 ubergraph page 의 entry 를 entry 이름 키로 한 object 로 통합 — functions 와 동일 구조.
	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		TArray<FPseudoEntry> Entries = RenderGraphPseudoCode(Graph);
		for (const FPseudoEntry& Entry : Entries)
		{
			if (Entry.Name.IsEmpty())
			{
				continue;
			}
			TArray<TSharedPtr<FJsonValue>> Lines;
			AppendEntriesToLines(Lines, { Entry });
			Root->SetArrayField(Entry.Name, Lines);
		}
	}
	return Root;
}

TSharedPtr<FJsonObject> FWxBlueprintSnapshotExporter::BuildFunctionsJson(UBlueprint* Blueprint)
{
	if (!Blueprint)
	{
		return nullptr;
	}

	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		TArray<FPseudoEntry> Entries = RenderGraphPseudoCode(Graph);
		if (Entries.Num() == 0)
		{
			continue;
		}
		TArray<TSharedPtr<FJsonValue>> Lines;
		AppendEntriesToLines(Lines, Entries);
		Root->SetArrayField(Graph->GetName(), Lines);
	}
	return Root;
}

TSharedPtr<FJsonObject> FWxBlueprintSnapshotExporter::BuildMacrosJson(UBlueprint* Blueprint)
{
	if (!Blueprint)
	{
		return nullptr;
	}

	// 사용자 정의 매크로 — 일반 BP 의 로컬 매크로와 BPTYPE_MacroLibrary 의 매크로 모두 MacroGraphs 에 들어간다.
	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	for (UEdGraph* Graph : Blueprint->MacroGraphs)
	{
		TArray<FPseudoEntry> Entries = RenderGraphPseudoCode(Graph);
		if (Entries.Num() == 0)
		{
			continue;
		}
		TArray<TSharedPtr<FJsonValue>> Lines;
		AppendEntriesToLines(Lines, Entries);
		Root->SetArrayField(Graph->GetName(), Lines);
	}
	return Root;
}
