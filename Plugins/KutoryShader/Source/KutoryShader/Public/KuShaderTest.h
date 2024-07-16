#pragma once
#include "CoreMinimal.h"
#include "KuShaderTest.generated.h"

UCLASS(MinimalAPI, meta = (ScriptName = "KutoryShaderLibaray"))
class UKutoryShaderBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, Category = "KutoryShader", meta = (WorldContext = "WorldContexObject"))
	static void DrawTestShaderRenderTarget(UTextureRenderTarget2D* OutputRenderTarget, const UObject* WorldContextObject, FLinearColor MyColor);
public:

};



