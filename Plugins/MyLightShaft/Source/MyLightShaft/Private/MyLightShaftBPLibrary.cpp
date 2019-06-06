// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "MyLightShaftBPLibrary.h"
#include "MyLightShaft.h"

UMyLightShaftBPLibrary::UMyLightShaftBPLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

static void DrawMyLightShaft_RenderThread(const UObject*, UTextureRenderTarget2D*);
void UMyLightShaftBPLibrary::MyLightShaftSampleFunction(const UObject* WorldContextObject,
	UTextureRenderTarget2D* OutputRenderTarget)
{
	DrawMyLightShaft_RenderThread(WorldContextObject, OutputRenderTarget);
}

