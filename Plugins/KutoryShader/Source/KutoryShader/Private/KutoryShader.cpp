// Copyright Epic Games, Inc. All Rights Reserved.

#include "KutoryShader.h"
#include "KuShaderTest.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GlobalShader.h"
#include "PipelineStateCache.h"
#include "ProfilingDebugging/RealtimeGPUProfiler.h"
#include "RHIStaticStates.h"
#include "SceneInterface.h"
#include "ShaderParameterUtils.h"
#include "Logging/MessageLog.h"
#include "TextureResource.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "RenderingThread.h"
#include "Interfaces/IPluginManager.h"


#define LOCTEXT_NAMESPACE "FKutoryShaderModule"

static const uint32 kGridSubdivisionX = 32;
static const uint32 kGridSubdivisionY = 16;


//BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FKuTestShaderParameters, )
//		SHADER_PARAMETER(FLinearColor, MyColor)
//END_GLOBAL_SHADER_PARAMETER_STRUCT()

class KuTestShaderVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(KuTestShaderVS, Global);
public:

	/** Default constructor. */
	KuTestShaderVS() {}

	/** Initialization constructor. */
	KuTestShaderVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
	}

};

class KuTestShaderPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(KuTestShaderPS, Global);
public:

	/** Default constructor. */
	KuTestShaderPS() {}

	// 使用初始化对象的构造函数我们将在此处绑定参数，使 C++ 代码
// 能与 USF 进行交互，使用户能从代码设置着色器参数。
	KuTestShaderPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		:FGlobalShader(Initializer)
	{
		MyColor.Bind(Initializer.ParameterMap, TEXT("MyColor"));
	}

	// 此函数是一个范例，说明如何基于
	// 特定于着色器的数据预计算着色器参数。因此，着色器需要数个可从几个参数进行计算的
	// 矩阵，而这在着色器自身中进行计算则效率不高。注意
	// 此函数并非为覆盖，它针对该类而定制，并在
	// 此功能的特定实现需要时进行调用。
	void SetParameters(
		FRHIBatchedShaderParameters& BatchedParameters,
		const FLinearColor& Color)
	{
		SetShaderValue(BatchedParameters, MyColor, Color);
	}
private:
	LAYOUT_FIELD(FShaderParameter, MyColor);
};



// 此宏将把着色器公开到引擎。注意绝对的虚拟源文件路径。
IMPLEMENT_SHADER_TYPE(, KuTestShaderVS, TEXT("/Plugins/KutoryShader/Private/KuShader.usf"), TEXT("MainVS"), SF_Vertex)
IMPLEMENT_SHADER_TYPE(, KuTestShaderPS, TEXT("/Plugins/KutoryShader/Private/KuShader.usf"), TEXT("MainPS"), SF_Pixel)

static void DrawToRenderTarget_RenderThread(FRHICommandListImmediate& RHICmdList, FTextureRenderTargetResource* OutRenderTargetResource, const FLinearColor& InColor, ERHIFeatureLevel::Type FeatureLevel)
{
	check(IsInRenderingThread());

	FRHITexture2D* RenderTargetTexture = OutRenderTargetResource->GetRenderTargetTexture();

	RHICmdList.Transition(FRHITransitionInfo(RenderTargetTexture, ERHIAccess::SRVMask, ERHIAccess::RTV));

	FRHIRenderPassInfo RPInfo(RenderTargetTexture, ERenderTargetActions::DontLoad_Store);
	RHICmdList.BeginRenderPass(RPInfo, TEXT("Test Draw to Render Target Global Shader"));
	{
		// Get Shader
		FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(FeatureLevel);
		TShaderMapRef<KuTestShaderVS> VertexShader(GlobalShaderMap);
		TShaderMapRef<KuTestShaderPS> PixelShader(GlobalShaderMap);

		// Set the Graphic pipeline state
		FGraphicsPipelineStateInitializer GraphicsPSOInit;
		RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
		GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
		GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
		GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
		GraphicsPSOInit.PrimitiveType = PT_TriangleStrip;
		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GetVertexDeclarationFVector4();
		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
		SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);

		//update viewport z for depth
		RHICmdList.SetViewport(0, 0, 0.f, OutRenderTargetResource->GetSizeX(), OutRenderTargetResource->GetSizeY(), 1.f);

		FRHIBatchedShaderParameters BatchedParameters = RHICmdList.GetScratchShaderParameters();
		PixelShader->SetParameters(BatchedParameters, InColor);
		RHICmdList.SetBatchedShaderParameters(PixelShader.GetPixelShader(), BatchedParameters);

		// 第一个参数0：起始顶点索引。
		// 第二个参数2：绘制的原语数量。对于三角形条带（PT_TriangleStrip），2个原语表示4个顶点（0 - 3）的三角形条带。
		// 第三个参数1：实例数量，这里只绘制一个实例。
		RHICmdList.DrawPrimitive(0, 2, 1);
	}
	RHICmdList.EndRenderPass();
	RHICmdList.Transition(FRHITransitionInfo(RenderTargetTexture, ERHIAccess::RTV, ERHIAccess::SRVMask));
}


void UKutoryShaderBlueprintLibrary::DrawTestShaderRenderTarget(
	UTextureRenderTarget2D* OutputRenderTarget,   
	const UObject* WorldContextObject,  
	FLinearColor MyColor  
)  
{  
	check(IsInGameThread());  
 
	if (!OutputRenderTarget)  
	{  
		return;  
	}  
 
	FTextureRenderTargetResource* TextureRenderTargetResource = OutputRenderTarget->GameThread_GetRenderTargetResource();  
	ERHIFeatureLevel::Type FeatureLevel = WorldContextObject->GetWorld()->Scene->GetFeatureLevel();  
	FName TextureRenderTargetName = OutputRenderTarget->GetFName();  
	ENQUEUE_RENDER_COMMAND(CaptureCommand)(  
		[TextureRenderTargetResource, FeatureLevel, MyColor, TextureRenderTargetName](FRHICommandListImmediate& RHICmdList)  
		{  
			DrawToRenderTarget_RenderThread(RHICmdList, TextureRenderTargetResource, MyColor, FeatureLevel);  
		}  
	);  
 
}  

void FKutoryShaderModule::StartupModule()
{
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("KutoryShader"))->GetBaseDir(),TEXT("Shaders"));

	//真实路径映射到虚拟路径
	AddShaderSourceDirectoryMapping(TEXT("/Plugins/KutoryShader"),PluginShaderDir);
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FKutoryShaderModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}







#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FKutoryShaderModule, KutoryShader)