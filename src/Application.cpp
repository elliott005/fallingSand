#include "application.hpp"

Application::Application(const char* title)
{
	//Initialize SDL
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
	}
	else
	{
		//Create window
		bool success = SDL_CreateWindowAndRenderer(title, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_TRANSPARENT, &m_window, &m_renderer);
		if (!success)
		{
			printf("Window and renderer could not be created! SDL Error: %s\n", SDL_GetError());
		}
		else
		{
			//Get window surface
			m_window_surface = SDL_GetWindowSurface(m_window);
			if (m_window_surface == NULL)
			{
				printf("Window surface could not be created! SDL Error: %s\n", SDL_GetError());
			}
		}
	}
}

Application::~Application()
{
	SDL_DestroySurface(m_window_surface);
	SDL_DestroyWindow(m_window);

	//Quit SDL subsystems
	SDL_Quit();
}

void Application::updateScreen() {
	//Update the surface
	SDL_RenderPresent(m_renderer);
	SDL_SetRenderDrawColor(m_renderer, 0xFF, 0xFF, 0xFF, 0xFF);
	SDL_RenderClear(m_renderer);
}