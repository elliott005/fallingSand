#include "src/Application.hpp"
#include "src/Map.hpp"
#include <SDL3/SDL_main.h>

int main(int argc, char* args[]) {
    Application game("falling sand");

    //Main loop flag
	bool quit = false;

	double frameRate = 1.0 / 25.0;

	Uint64 NOW = SDL_GetPerformanceCounter();
	Uint64 LAST = 0;
	double deltaTime = 0;

	SDL_GPUDevice* gpuDevice = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL, false, NULL);

	InitializeAssetLoader();	

	Map map(1000, 1000, gpuDevice);

    while (!quit) {
        while (SDL_PollEvent(&game.m_window_event) != 0)
		{
			//User requests quit
			if (game.m_window_event.type == SDL_EVENT_QUIT)
			{
				quit = true;
			}
        }
		
        LAST = NOW;
		NOW = SDL_GetPerformanceCounter();
		deltaTime = (double)((NOW - LAST) * 1000 / (double)SDL_GetPerformanceFrequency());
		deltaTime *= 0.001;
		//SDL_Delay(frameRate * 1000);
		//printf("FPS: %f\n", 1.0f / deltaTime);

		map.update(deltaTime, game.SCREEN_WIDTH, game.SCREEN_HEIGHT);
		
		SDL_SetRenderDrawColor(game.m_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(game.m_renderer);
		
        SDL_SetRenderDrawColor(game.m_renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
		float before = SDL_GetTicks();
		map.draw(game.m_renderer, game.SCREEN_WIDTH, game.SCREEN_HEIGHT);
        //printf("drawTime: %f\n", (SDL_GetTicks() - before) * 0.001f);

		SDL_RenderPresent(game.m_renderer);
	}

	map.quit();

	SDL_DestroyGPUDevice(gpuDevice);

    return 0;
}