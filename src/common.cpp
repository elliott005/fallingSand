#include "common.hpp"

void InitializeAssetLoader()
{
	BasePath = SDL_GetBasePath();
}

SDL_GPUComputePipeline* CreateComputePipelineFromShader(
	SDL_GPUDevice* device,
	const char* shaderFilename,
	SDL_GPUComputePipelineCreateInfo *createInfo
) {
	char fullPath[256];
	SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(device);
	SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
	const char *entrypoint;

	if (backendFormats & SDL_GPU_SHADERFORMAT_SPIRV) {
		SDL_snprintf(fullPath, sizeof(fullPath), "%s/shaders/compiled/SPIRV/%s.spv", BasePath, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_SPIRV;
		entrypoint = "main";
	} else if (backendFormats & SDL_GPU_SHADERFORMAT_MSL) {
		SDL_snprintf(fullPath, sizeof(fullPath), "%s/shaders/compiled/MSL/%s.msl", BasePath, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_MSL;
		entrypoint = "main0";
	} else if (backendFormats & SDL_GPU_SHADERFORMAT_DXIL) {
		SDL_snprintf(fullPath, sizeof(fullPath), "%s/shaders/compiled/DXIL/%s.dxil", BasePath, shaderFilename);
		format = SDL_GPU_SHADERFORMAT_DXIL;
		entrypoint = "main";
	} else {
		SDL_Log("%s", "Unrecognized backend shader format!");
		return NULL;
	}

	size_t codeSize;
	Uint8* code = (Uint8*)SDL_LoadFile(fullPath, &codeSize);
	if (code == NULL)
	{
		SDL_Log("Failed to load compute shader from disk! %s", fullPath);
		return NULL;
	}

	// Make a copy of the create data, then overwrite the parts we need
	SDL_GPUComputePipelineCreateInfo newCreateInfo = *createInfo;
	newCreateInfo.code = code;
	newCreateInfo.code_size = codeSize;
	newCreateInfo.entrypoint = entrypoint;
	newCreateInfo.format = format;

	SDL_GPUComputePipeline* pipeline = SDL_CreateGPUComputePipeline(device, &newCreateInfo);
	if (pipeline == NULL)
	{
		SDL_Log("Failed to create compute pipeline!");
		SDL_free(code);
		return NULL;
	}

	SDL_free(code);
	return pipeline;
}