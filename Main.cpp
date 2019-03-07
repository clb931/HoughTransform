#include <SDL.h>
#include <algorithm>


struct WindowStuff {
	SDL_Window* window;
	SDL_Renderer* renderer;
	int width, height;
};
struct Img {
	SDL_Rect rect;
	SDL_Texture* texture;
	uint32_t* data;
};

struct Hough {
	Img img;

	int max_rho;
	int max_theta;
	int half_max_rho;
	int half_max_theta;
	double* rho;
	double* theta;
	double* cos;
	double* sin;
};

struct EventStuff {
	bool quit;
	int mouse_x, mouse_y;
};

void RunHough(Hough& hough, Img& source_img, int theta_inc) {
	std::memset(hough.img.data, 0, hough.img.rect.w * hough.img.rect.h * 4);
	for (int y = 0; y < source_img.rect.h; ++y) {
		for (int x = 0; x < source_img.rect.w; ++x) {
			int i = x + y * source_img.rect.w;
			if (source_img.data[i] != 0) {
				for (int ti = 0; ti < hough.max_theta; ++ti) {
					double theta = hough.theta[ti];
					int rho = std::round((x * hough.cos[ti]) + (y * hough.sin[ti])) + hough.half_max_rho;
					int j = std::round(ti + rho * hough.max_theta);
					//int j = std::round(rho + ti * hough.max_rho);
					hough.img.data[j] += 0x040404;
				}
			}
		}
	}
}

int Init(WindowStuff& window, Img& source_img, Hough& hough);
EventStuff HandleEvent(SDL_Event& e);

int main(int argc, char **argv) {
	WindowStuff window;
	Hough hough;
	Img source_img;
	int result = Init(window, source_img, hough);
	if (result != 0) {
		return result;
	}

	SDL_Event evt;
	while (1) {
		SDL_PollEvent(&evt);
		EventStuff event_stuff = HandleEvent(evt);
		if (event_stuff.quit) {
			break;
		}

		RunHough(hough, source_img, 1);
		SDL_UpdateTexture(source_img.texture, NULL, &source_img.data[0], source_img.rect.w * 4);
		SDL_UpdateTexture(hough.img.texture, NULL, &hough.img.data[0], hough.img.rect.w * 4);

		SDL_SetRenderDrawColor(window.renderer, 0x00, 0x00, 0xFF, 0x00);
		SDL_RenderClear(window.renderer);
		SDL_RenderCopy(window.renderer, source_img.texture, NULL, &source_img.rect);
		SDL_RenderCopy(window.renderer, hough.img.texture, NULL, &hough.img.rect);
		SDL_SetRenderDrawColor(window.renderer, 0x00, 0x00, 0xFF, 0x00);
		SDL_RenderDrawLine(window.renderer, source_img.rect.w, 0, source_img.rect.w, window.height);

		double rho = hough.rho[event_stuff.mouse_y];
		double theta = hough.theta[(event_stuff.mouse_x - hough.img.rect.x)];	
		double a = cos(theta), b = sin(theta);
		double x0 = a * rho, y0 = b * rho;
		SDL_SetRenderDrawColor(window.renderer, 0x00, 0xFF, 0x00, 0x00);
		SDL_RenderDrawLine(window.renderer,
			std::round(x0 + hough.half_max_rho * (-b)),
			std::round(y0 + hough.half_max_rho * (a)),
			std::round(x0 - hough.half_max_rho * (-b)),
			std::round(y0 - hough.half_max_rho * (a)));

		SDL_RenderPresent(window.renderer);
	}

	SDL_DestroyTexture(hough.img.texture);
	SDL_DestroyTexture(source_img.texture);
	SDL_DestroyRenderer(window.renderer);
	SDL_DestroyWindow(window.window);
	delete[] source_img.data;
	delete[] hough.img.data;
	delete[] hough.rho;
	delete[] hough.theta;
	delete[] hough.cos;
	delete[] hough.sin;

	SDL_Quit();
	return 0;
}

int Init(WindowStuff& window, Img& source_img, Hough& hough) {
	source_img.rect.w = 180;
	source_img.rect.h = 180;
	source_img.rect.x = source_img.rect.y = 0;
	source_img.data = new uint32_t[source_img.rect.w * source_img.rect.h];
	std::memset(source_img.data, 0, source_img.rect.w * source_img.rect.h * 4);

	hough.half_max_theta = 90;
	hough.half_max_rho = std::ceil(std::sqrt((source_img.rect.w * source_img.rect.w) + (source_img.rect.h * source_img.rect.h)));
	hough.max_theta = (hough.half_max_theta * 2) + 1;
	hough.max_rho = (hough.half_max_rho * 2) + 1;
	hough.img.rect.h = hough.max_rho;
	hough.img.rect.w = hough.max_theta;
	hough.img.rect.x = source_img.rect.w;
	hough.img.rect.y = 0;

	hough.rho = new double[hough.max_rho];
	for (int i = 0; i < hough.img.rect.h; ++i) {
		hough.rho[i] = i - hough.half_max_rho;
	}

	hough.cos = new double[hough.max_theta];
	hough.sin = new double[hough.max_theta];
	hough.theta = new double[hough.max_theta];
	for (int i = 0; i < hough.max_theta; ++i) {
		hough.theta[i] = (i - hough.half_max_theta) * (M_PI / 180);
		hough.cos[i] = cos(hough.theta[i]);
		hough.sin[i] = sin(hough.theta[i]);
	}

	hough.img.data = new uint32_t[hough.img.rect.w * hough.img.rect.h];
	std::memset(hough.img.data, 0, hough.img.rect.w * hough.img.rect.h * 4);

	window.width = source_img.rect.w + hough.img.rect.w;
	window.height = std::max(source_img.rect.h, hough.img.rect.h);

	if (SDL_CreateWindowAndRenderer(window.width, window.height, SDL_WINDOW_SHOWN, &window.window, &window.renderer)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not create window and renderer: %s", SDL_GetError());
		return 3;
	}


	hough.img.texture = SDL_CreateTexture(window.renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, hough.img.rect.w, hough.img.rect.h);
	source_img.texture = SDL_CreateTexture(window.renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, source_img.rect.w, source_img.rect.h);
	if (!source_img.texture || !hough.img.texture) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not create texture from surface: %s", SDL_GetError());
		return 3;
	}

	for (int i = 0; i < source_img.rect.w; ++i) {
		source_img.data[i + ((source_img.rect.h / 2) * source_img.rect.w)] = 0x00FFFFFF;
		if (i < source_img.rect.h) {
			source_img.data[i + (i * source_img.rect.w)] = 0x00FFFFFF;
			source_img.data[(source_img.rect.w / 2) + (i * source_img.rect.w)] = 0x00FFFFFF;
		}
	}
	source_img.data[(source_img.rect.w / 2) + ((source_img.rect.h / 2) * source_img.rect.w)] = 0x00FFFFFF;

	return 0;
}

EventStuff HandleEvent(SDL_Event& e) {
	EventStuff event_stuff;
	event_stuff.quit = (e.type == SDL_QUIT);

	SDL_GetMouseState(&event_stuff.mouse_x, &event_stuff.mouse_y);
	return event_stuff;
}