#pragma once

#include <SDL3/SDL.h>

static const char* BasePath = NULL;
void InitializeAssetLoader();

SDL_GPUComputePipeline* CreateComputePipelineFromShader(
    SDL_GPUDevice* device,
	const char* shaderFilename,
	SDL_GPUComputePipelineCreateInfo *createInfo
);