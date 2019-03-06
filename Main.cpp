#include <SDL.h>
#include <algorithm>


SDL_Renderer *renderer;
uint32_t hough_pixels[500 * 180];
uint32_t source_pixels[400 * 300];

bool HandleEvent(SDL_Event& e);

void RenderHoughTransform() {
	uint32_t max = 0;
	SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 0x00);
	for (int a = 0; a < 180; ++a) {
		for (int p = 0; p < 500; ++p) {
			int j = p + a * 500;
			hough_pixels[j] = 0;
		}
	}
	for (int y = 0; y < 400; ++y) {
		for (int x = 0; x < 300; ++x) {
			int i = x + y * 300;
			if (source_pixels[i] != 0) {
				for (int a = 0; a < 180; ++a) {
					int p = (x * cosf(a-90)) + (y * sinf(a-90));
					int j = p + a * 500;
					if (j > 0) {
						int val = (hough_pixels[j] & 0xFF) + 255;
						hough_pixels[j] = (val << 16) | (val << 8) | val;
					}
				}
			}
		}
	}
}

int main(int argc, char **argv) {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not initialize SDL: %s", SDL_GetError());
		return 3;
	}

	SDL_Window *window;
	if (SDL_CreateWindowAndRenderer(500, 480, SDL_WINDOW_SHOWN, &window, &renderer)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not create window and renderer: %s", SDL_GetError());
		return 3;
	}

	SDL_Texture* hough = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 500, 180);
	SDL_Texture* source = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, 400, 300);
	if (!source || !hough) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not create texture from surface: %s", SDL_GetError());
		return 3;
	}

	SDL_Rect surface_rect;
	surface_rect.w = 400;
	surface_rect.h = 300;
	surface_rect.x = surface_rect.y = 0;

	SDL_Rect hough_rect;
	hough_rect.w = 500;
	hough_rect.h = 180;
	hough_rect.x = 0;
	hough_rect.y = 300;

	for (int i = 0; i < 100; ++i) {
		source_pixels[13000 + i] = 0x00FFFFFF;
	}

	SDL_Event evt;
	while (1) {
		SDL_PollEvent(&evt);
		if (!HandleEvent(evt)) {
			break;
		}

		RenderHoughTransform();
		SDL_UpdateTexture(source, NULL, &source_pixels[0], 400 * 4);
		SDL_UpdateTexture(hough, NULL, &hough_pixels[0], 500 * 4);

		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, source, NULL, &surface_rect);
		SDL_RenderCopy(renderer, hough, NULL, &hough_rect);
		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0xFF, 0x00);
		SDL_RenderDrawLine(renderer, 0, 300, 500, 300);
		SDL_RenderPresent(renderer);
	}

	SDL_DestroyTexture(hough);
	SDL_DestroyTexture(source);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();
	return 0;
}

bool HandleEvent(SDL_Event& e) {
	if (e.type == SDL_QUIT) {
		return false;
	}

	switch (e.type) {
		case SDL_KEYDOWN: {
			if (e.key.keysym.scancode == SDL_SCANCODE_SPACE) {
				RenderHoughTransform();
			}
		} break;
	}
}
