#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "tr/tr.hpp"

#include <SDL.h>

bool initSdl()
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0)
	{
		std::cerr << "InitSdl() in src/main.cpp: Failed to initialise SDL." << std::endl;
		return false;
	}
	return true;
}

bool initWindow(SDL_Window** window, SDL_Renderer** renderer, const int screenWidth, const int screenHeight, const bool fullscreen)
{
	*window = SDL_CreateWindow("Space Raster", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screenWidth, screenHeight, SDL_WINDOW_SHOWN | (fullscreen ? SDL_WINDOW_FULLSCREEN : 0));

	if (!window)
	{
		std::cerr << "InitWindow() in src/main.cpp: Failed to initialise SDL window." << std::endl;
		return false;
	}

	*renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);

	if (!renderer)
	{
		std::cerr << "InitWindow() in src/main.cpp: Failed to initialise SDL renderer." << std::endl;
	}

	return true;
}

Matrix4 createPerspectiveProjectionMatrix(const float left, const float right, const float bottom, const float top, const float near, const float far)
{
	Matrix4 mat;

	mat[0]  = (2.0f * near) / (right - left);
	mat[5]  = (2.0f * near) / (top - bottom);
	mat[8]  = (right + left) / (right - left);
	mat[9]  = (top + bottom) / (top - bottom);
	mat[10] = -(far + near) / (far - near);
	mat[11] = -1.0f;
	mat[14] = -(2.0f * far * near) / (far - near);

	return mat;
}

Matrix4 createOrthographicProjectionMatrix(const float left, const float right, const float bottom, const float top, const float near, const float far)
{
	Matrix4 mat;

	mat[0]  = 2.0f / (right - left);
	mat[5]  = 2.0f / (top - bottom);
	mat[10] = -2.0f / (far - near);
	mat[12] = -(right + left) / (right - left);
	mat[13] = -(top + bottom) / (top - bottom);
	mat[14] = -(far + near) / (far - near);

	return mat;
}

std::vector<tr::Vertex> defineVertices()
{
	return {
		
		// Front
		{ Vector4( 2.0f,  2.0f, -50.0f, 1.0f), Vector3(0.0f, 0.0f, 0.0f), Vector2(1.0f, 0.0f) },
		{ Vector4( 2.0f, -2.0f, -50.0f, 1.0f), Vector3(0.0f, 0.0f, 0.0f), Vector2(1.0f, 1.0f) },
		{ Vector4(-2.0f, -2.0f, -50.0f, 1.0f), Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 1.0f) },

		{ Vector4(-2.0f,  2.0f, -50.0f, 1.0f), Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 0.0f) },
		{ Vector4( 2.0f,  2.0f, -50.0f, 1.0f), Vector3(0.0f, 0.0f, 0.0f), Vector2(1.0f, 0.0f) },
		{ Vector4(-2.0f, -2.0f, -50.0f, 1.0f), Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 1.0f) },

		// Top
		{ Vector4(-2.0f,  2.0f, -50.0f, 1.0f), Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 0.0f) },
		{ Vector4(-2.0f,  2.0f, -54.0f, 1.0f), Vector3(0.0f, 0.0f, 0.0f), Vector2(1.0f, 0.0f) },
		{ Vector4( 2.0f,  2.0f, -50.0f, 1.0f), Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 1.0f) },

		{ Vector4(-2.0f,  2.0f, -54.0f, 1.0f), Vector3(0.0f, 0.0f, 0.0f), Vector2(1.0f, 0.0f) },
		{ Vector4( 2.0f,  2.0f, -54.0f, 1.0f), Vector3(0.0f, 0.0f, 0.0f), Vector2(1.0f, 1.0f) },
		{ Vector4( 2.0f,  2.0f, -50.0f, 1.0f), Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 1.0f) },

		// Right
		{ Vector4( 2.0f,  2.0f, -54.0f, 1.0f), Vector3(0.0f, 0.0f, 0.0f), Vector2(1.0f, 0.0f) },
		{ Vector4( 2.0f, -2.0f, -54.0f, 1.0f), Vector3(0.0f, 0.0f, 0.0f), Vector2(1.0f, 1.0f) },
		{ Vector4( 2.0f,  2.0f, -50.0f, 1.0f), Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 0.0f) },

		{ Vector4( 2.0f,  2.0f, -50.0f, 1.0f), Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 0.0f) },
		{ Vector4( 2.0f, -2.0f, -54.0f, 1.0f), Vector3(0.0f, 0.0f, 0.0f), Vector2(1.0f, 1.0f) },
		{ Vector4( 2.0f, -2.0f, -50.0f, 1.0f), Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 1.0f) },

		// Back
		{ Vector4(-2.0f, -2.0f, -54.0f, 1.0f), Vector3(0.0f, 0.0f, 0.0f), Vector2(1.0f, 1.0f) },
		{ Vector4( 2.0f, -2.0f, -54.0f, 1.0f), Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 1.0f) },
		{ Vector4( 2.0f,  2.0f, -54.0f, 1.0f), Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 0.0f) },
		
		{ Vector4(-2.0f, -2.0f, -54.0f, 1.0f), Vector3(0.0f, 0.0f, 0.0f), Vector2(1.0f, 1.0f) },
		{ Vector4( 2.0f,  2.0f, -54.0f, 1.0f), Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 0.0f) },
		{ Vector4(-2.0f,  2.0f, -54.0f, 1.0f), Vector3(0.0f, 0.0f, 0.0f), Vector2(1.0f, 0.0f) },
	};
}

void renderColorBufferToWindow(tr::ColorBuffer& colorBuffer, SDL_Renderer* renderer)
{
	SDL_Surface* sdlSurface = SDL_CreateRGBSurfaceFrom((void*)colorBuffer.getData(), int(colorBuffer.getWidth()), int(colorBuffer.getHeight()), 32, int(sizeof(tr::Color) * colorBuffer.getWidth()), 0, 0, 0, 0);
	SDL_Texture* sdlTexture = SDL_CreateTextureFromSurface(renderer, sdlSurface);

	SDL_FreeSurface(sdlSurface);
	SDL_RenderCopy(renderer, sdlTexture, nullptr, nullptr);
	SDL_RenderPresent(renderer);
	SDL_DestroyTexture(sdlTexture);
}

void updateInputs(bool& running, Vector4& position, Vector4& rotation)
{
	SDL_Event  event;

	while (SDL_PollEvent(&event))
	{
		if (event.type == SDL_QUIT)
		{
			running = false;
		}
		else if (event.type == SDL_KEYDOWN)
		{
			constexpr float translationIncrement = 5.0f;
			constexpr float rotationIncrement    = 2.0f;

			if      (event.key.keysym.sym == SDLK_ESCAPE) { running = false;                    }
			else if (event.key.keysym.sym == SDLK_w)      { position.z -= translationIncrement; }
			else if (event.key.keysym.sym == SDLK_a)      { position.x -= translationIncrement; }
			else if (event.key.keysym.sym == SDLK_s)      { position.z += translationIncrement; }
			else if (event.key.keysym.sym == SDLK_d)      { position.x += translationIncrement; }
			else if (event.key.keysym.sym == SDLK_SPACE)  { position.y += translationIncrement; }
			else if (event.key.keysym.sym == SDLK_LCTRL)  { position.y -= translationIncrement; }
			else if (event.key.keysym.sym == SDLK_LEFT)   { rotation.y += rotationIncrement;    }
			else if (event.key.keysym.sym == SDLK_RIGHT)  { rotation.y -= rotationIncrement;    }
			else if (event.key.keysym.sym == SDLK_UP)     { rotation.x += rotationIncrement;    }
			else if (event.key.keysym.sym == SDLK_DOWN)   { rotation.x -= rotationIncrement;    }
		}
	}
}

int main(int argc, char* argv[])
{
	constexpr int                 screenWidth      = 2560;
	constexpr int                 screenHeight     = 1440;
	bool                          running          = true;
	SDL_Window*                   window           = nullptr;
	SDL_Renderer*                 renderer         = nullptr;
	const std::vector<tr::Vertex> vertices         = defineVertices();
	const tr::ColorBuffer         texture          = tr::loadTexture("data/udon.png");
	const Matrix4                 projectionMatrix = createPerspectiveProjectionMatrix(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 100.0f);
	tr::ColorBuffer               colorBuffer(screenWidth, screenHeight);
	tr::DepthBuffer               depthBuffer(screenWidth, screenHeight);

	Vector4                       cameraRotation(0.0f, 0.0f, 0.0f, 1.0f);
	Vector4                       cameraPosition(10.0f, 0.0f, -45.0f, 1.0f);

	Vector4                       modelRotation(0.0f, 0.0f, 0.0f, 1.0f);
	Vector4                       modelPosition(10.0f, 0.0f, 0.0f, 1.0f);

	tr::Rasterizer                rasterizer;

	if (screenWidth <= 0 || screenHeight <= 0)
	{
		return 0;
	}

	if (!initSdl())
	{
		SDL_Quit();
		return 0;
	}

	if (!initWindow(&window, &renderer, screenWidth, screenHeight, false))
	{
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();

		return 0;
	}

	rasterizer.setPrimitive(tr::Primitive::TRIANGLES);

	while (running)
	{
		const auto start = std::chrono::high_resolution_clock::now();
		Matrix4    modelMatrix;
		Matrix4    viewMatrix;
		
		updateInputs(running, cameraPosition, cameraRotation);

		colorBuffer.fill(tr::Color{ 0, 0, 0, 0 });
		depthBuffer.fill(1.0f);

		modelMatrix.identity();
		modelMatrix.translate(modelPosition.x, modelPosition.y, modelPosition.z);
		modelMatrix.rotateX(modelRotation.x);
		modelMatrix.rotateY(modelRotation.y);
		modelMatrix.rotateZ(modelRotation.z);
		
		viewMatrix.identity();
		viewMatrix.translate(-cameraPosition.x, -cameraPosition.y, -cameraPosition.z);
		viewMatrix.rotateY(-cameraRotation.y);
		viewMatrix.rotateX(-cameraRotation.x);
		
		rasterizer.setMatrix(projectionMatrix * viewMatrix * modelMatrix);
		rasterizer.draw(vertices, texture, colorBuffer, depthBuffer);

		renderColorBufferToWindow(colorBuffer, renderer);

		const auto frameTime = std::chrono::high_resolution_clock::now() - start;
		std::cout << std::chrono::duration<double, std::milli>(frameTime).count() << " ms" << std::endl;
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
