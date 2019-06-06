#include "MyLightShaft.h"

#include "Classes/Engine/TextureRenderTarget2D.h"
#include "Classes/Engine/World.h"
#include "GlobalShader.h"
#include "PipelineStateCache.h"
#include "RHIStaticStates.h"
#include "SceneUtils.h"
#include "SceneInterface.h"
#include "ShaderParameterUtils.h"
#include "Logging/MessageLog.h"
#include "Internationalization/Internationalization.h"

#define LOCTEXT_NAMESPACE "MyLightShaftPlugin"

class FMyLightShaftShader : public FGlobalShader
{
public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}

	FMyLightShaftShader() {}

	FMyLightShaftShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{

	}

	template<typename TShaderRHIParamRef>
	void SetParameters(
		FRHICommandListImmediate& RHICmdList,
		const TShaderRHIParamRef ShaderRHI)
	{
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		return bShaderHasOutdatedParameters;
	}

private:

};

class FMyLightShaftShaderVS : public FMyLightShaftShader
{
	DECLARE_SHADER_TYPE(FMyLightShaftShaderVS, Global);

public:

	/** Default constructor. */
	FMyLightShaftShaderVS() {}

	/** Initialization constructor. */
	FMyLightShaftShaderVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FMyLightShaftShader(Initializer)
	{
	}
};


class FMyLightShaftShaderPS : public FMyLightShaftShader
{
	DECLARE_SHADER_TYPE(FMyLightShaftShaderPS, Global);

public:

	/** Default constructor. */
	FMyLightShaftShaderPS() {}

	/** Initialization constructor. */
	FMyLightShaftShaderPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FMyLightShaftShader(Initializer)
	{ }
};

IMPLEMENT_SHADER_TYPE(, FMyLightShaftShaderVS, TEXT("/Plugin/MyLightShaft/Private/MyLightShaft.usf"),
	TEXT("MainVertexShader"), SF_Vertex);
IMPLEMENT_SHADER_TYPE(, FMyLightShaftShaderPS, TEXT("/Plugin/MyLightShaft/Private/MyLightShaft.usf"),
	TEXT("MainPixelShader"), SF_Pixel);

static void DrawMyLightShaft_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	const FName& TextureRenderTargetName,
	FTextureRenderTargetResource* OutTextureRenderTargetResource,
	ERHIFeatureLevel::Type FeatureLevel)
{
	check(IsInRenderingThread());

#if WANTS_DRAW_MESH_EVENTS
	FString EventName;
	TextureRenderTargetName.ToString(EventName);
	SCOPED_DRAW_EVENTF(RHICmdList, SceneCapture, TEXT("DrawMyLightShaft %s"), *EventName);
#else
	SCOPED_DRAW_EVENT(RHICmdList, DrawMyLightShaft_RenderThread);
#endif

	FRHIRenderPassInfo RPInfo(OutTextureRenderTargetResource->GetRenderTargetTexture(), ERenderTargetActions::DontLoad_Store, OutTextureRenderTargetResource->TextureRHI);
	RHICmdList.BeginRenderPass(RPInfo, TEXT("DrawMyLightShaft"));
	{
		FIntPoint DisplacementMapResolution(OutTextureRenderTargetResource->GetSizeX(), OutTextureRenderTargetResource->GetSizeY());

		// Update viewport.
		RHICmdList.SetViewport(
			0, 0, 0.f,
			DisplacementMapResolution.X, DisplacementMapResolution.Y, 1.f);

		// Get shaders.
		TShaderMap<FGlobalShaderType>* GlobalShaderMap = GetGlobalShaderMap(FeatureLevel);
		TShaderMapRef< FMyLightShaftShaderVS > VertexShader(GlobalShaderMap);
		TShaderMapRef< FMyLightShaftShaderPS > PixelShader(GlobalShaderMap);

		// Set the graphic pipeline state.
		FGraphicsPipelineStateInitializer GraphicsPSOInit;
		RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
		GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
		GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
		GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
		GraphicsPSOInit.PrimitiveType = PT_TriangleStrip;
		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GetVertexDeclarationFVector4();
		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
		SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

		// Update viewport.
		RHICmdList.SetViewport(
			0, 0, 0.f,
			OutTextureRenderTargetResource->GetSizeX(), OutTextureRenderTargetResource->GetSizeY(), 1.f);

		// Update shader uniform parameters.

		// Draw grid.
		RHICmdList.DrawPrimitive(0, 2, 1);
	}
	RHICmdList.EndRenderPass();
}


static void DrawMyLightShaft_RenderThread(const UObject* WorldContextObject, UTextureRenderTarget2D* OutputRenderTarget)
{
	check(IsInGameThread());

	if (!OutputRenderTarget)
	{
		FMessageLog("Blueprint").Warning(
			LOCTEXT("LensDistortionCameraModel_DrawUVDisplacementToRenderTarget",
				"DrawUVDisplacementToRenderTarget: Output render target is required."));
		return;
	}

	const FName TextureRenderTargetName = OutputRenderTarget->GetFName();
	FTextureRenderTargetResource* TextureRenderTargetResource = OutputRenderTarget->GameThread_GetRenderTargetResource();

	ERHIFeatureLevel::Type FeatureLevel = WorldContextObject->GetWorld()->Scene->GetFeatureLevel();

	ENQUEUE_RENDER_COMMAND(CaptureCommand)(
		[TextureRenderTargetResource, TextureRenderTargetName, FeatureLevel](FRHICommandListImmediate& RHICmdList)
	{
		DrawMyLightShaft_RenderThread(
			RHICmdList,
			TextureRenderTargetName,
			TextureRenderTargetResource,
			FeatureLevel);
	}
	);
}

#undef LOCTEXT_NAMESPACE