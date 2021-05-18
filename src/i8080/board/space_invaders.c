#include <stdbool.h>
#include <time.h>

#ifdef __APPLE__
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include "space_invaders.h"

#include "../ram.h"

#define SPACE_INVADERS_SCREEN_WIDTH  256
#define SPACE_INVADERS_SCREEN_HEIGHT 224

#define SPACE_INVADERS_BIT_INPUT_CREDIT   8
#define SPACE_INVADERS_BIT_INPUT_2P_START 9
#define SPACE_INVADERS_BIT_INPUT_1P_START 10
#define SPACE_INVADERS_BIT_INPUT_P1_SHOT  12
#define SPACE_INVADERS_BIT_INPUT_P1_LEFT  13
#define SPACE_INVADERS_BIT_INPUT_P1_RIGHT 14
#define SPACE_INVADERS_BIT_INPUT_P2_SHOT  20
#define SPACE_INVADERS_BIT_INPUT_P2_LEFT  21
#define SPACE_INVADERS_BIT_INPUT_P2_RIGHT 22

#define SPACE_INVADERS_MASK_INPUT_DEFAULT  0x080E
#define SPACE_INVADERS_MASK_INPUT_CREDIT   (1 << SPACE_INVADERS_BIT_INPUT_CREDIT)
#define SPACE_INVADERS_MASK_INPUT_2P_START (1 << SPACE_INVADERS_BIT_INPUT_2P_START)
#define SPACE_INVADERS_MASK_INPUT_1P_START (1 << SPACE_INVADERS_BIT_INPUT_1P_START)
#define SPACE_INVADERS_MASK_INPUT_P1_SHOT  (1 << SPACE_INVADERS_BIT_INPUT_P1_SHOT)
#define SPACE_INVADERS_MASK_INPUT_P1_LEFT  (1 << SPACE_INVADERS_BIT_INPUT_P1_LEFT)
#define SPACE_INVADERS_MASK_INPUT_P1_RIGHT (1 << SPACE_INVADERS_BIT_INPUT_P1_RIGHT)
#define SPACE_INVADERS_MASK_INPUT_P2_SHOT  (1 << SPACE_INVADERS_BIT_INPUT_P2_SHOT)
#define SPACE_INVADERS_MASK_INPUT_P2_LEFT  (1 << SPACE_INVADERS_BIT_INPUT_P2_LEFT)
#define SPACE_INVADERS_MASK_INPUT_P2_RIGHT (1 << SPACE_INVADERS_BIT_INPUT_P2_RIGHT)

typedef uint64_t nanoseconds_t;

static struct {
	/* Board management */
	bool isonline;
	uint64_t inputs;
	uint64_t interrupt_frame;

	/* Dedicated shift HW */
	uint16_t shift_register;
	unsigned shift_amount;

	/* Time management */
	nanoseconds_t start;
	nanoseconds_t cycle_duration;
	nanoseconds_t vblank_duration;

	/* SDL2 */
	Uint32 sdl_initialized;
	const Uint8 *sdl_keyboard_state;
	SDL_Window *sdl_window;
	SDL_Renderer *sdl_renderer;
	SDL_Texture *sdl_texture;
} space_invaders;

static inline nanoseconds_t
space_invaders_now(void) {
	struct timespec res;

	if(clock_gettime(CLOCK_REALTIME, &res) != 0) {
		perror("clock_gettime");
		exit(EXIT_FAILURE);
	}

	return (nanoseconds_t)res.tv_sec * 1000000000 + res.tv_nsec;
}

static inline void
space_invaders_sleep(nanoseconds_t duration) {
	struct timespec req = {
		.tv_sec = duration / 1000000000,
		.tv_nsec = duration % 1000000000,
	}, rem;

	while(nanosleep(&req, &rem) != 0) {
		req = rem;
	}
}

static inline nanoseconds_t
space_invaders_frequency_period(uint64_t frequency) {
	return 1000000000 / frequency;
}

static void
space_invaders_input(struct i8080_cpu *cpu, uint8_t device) {
	switch(device) {
	case 0:
	case 1:
	case 2:
		cpu->registers.a = space_invaders.inputs >> device * 8;
		break;
	case 3:
		cpu->registers.a = space_invaders.shift_register >> space_invaders.shift_amount;
		break;
	}
}

static void
space_invaders_output(struct i8080_cpu *cpu, uint8_t device) {
	switch(device) {
	case 2:
		space_invaders.shift_amount = cpu->registers.a & 0x3;
		return;
	case 3:
		return;
	case 4:
		space_invaders.shift_register = cpu->registers.a << 8 | space_invaders.shift_register >> 8;
		return;
	case 5:
		return;
	case 6:
		return;
	}
}

static void
space_invaders_board_setup(struct i8080_cpu *cpu, const char *filename) {
	static const struct i8080_rom_section space_invaders_rom_map[] = {
		{ .begin = 0x1000, .end = 0x2000 },
		{ },
	};

	i8080_ram_load_file(cpu, filename, 0x0000);
	cpu->rom_map = space_invaders_rom_map;

	space_invaders.isonline = true;
	space_invaders.inputs = SPACE_INVADERS_MASK_INPUT_DEFAULT;

	space_invaders.cycle_duration = space_invaders_frequency_period(2000000);
	space_invaders.vblank_duration = space_invaders_frequency_period(60);
	space_invaders.start = space_invaders_now();

	const Uint32 required_initialized = SDL_INIT_VIDEO;
	space_invaders.sdl_initialized = SDL_WasInit(required_initialized) ^ required_initialized;
	if(SDL_InitSubSystem(space_invaders.sdl_initialized) != 0) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
		goto space_invaders_board_setup_err0;
	}

	space_invaders.sdl_keyboard_state = SDL_GetKeyboardState(NULL);

	if(SDL_CreateWindowAndRenderer(SPACE_INVADERS_SCREEN_WIDTH * 2, SPACE_INVADERS_SCREEN_WIDTH * 2, SDL_WINDOW_RESIZABLE,
		&space_invaders.sdl_window, &space_invaders.sdl_renderer) != 0) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer SDL: %s", SDL_GetError());
		goto space_invaders_board_setup_err1;
	}

	/* SDL is weird when rotating its renderer, to simplify, we make the screen appear square and ensure aspect ratio when blitting */
	SDL_RenderSetLogicalSize(space_invaders.sdl_renderer, SPACE_INVADERS_SCREEN_WIDTH, SPACE_INVADERS_SCREEN_WIDTH);

	space_invaders.sdl_texture = SDL_CreateTexture(space_invaders.sdl_renderer, SDL_PIXELFORMAT_RGB332,
		SDL_TEXTUREACCESS_STREAMING, SPACE_INVADERS_SCREEN_WIDTH, SPACE_INVADERS_SCREEN_HEIGHT);
	if(space_invaders.sdl_texture == NULL) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer SDL: %s", SDL_GetError());
		goto space_invaders_board_setup_err1;
	}

	return;
space_invaders_board_setup_err1:
	SDL_QuitSubSystem(space_invaders.sdl_initialized);
space_invaders_board_setup_err0:
	exit(EXIT_FAILURE);
}

static void
space_invaders_board_teardown(struct i8080_cpu *cpu) {
	SDL_DestroyTexture(space_invaders.sdl_texture);
	SDL_DestroyRenderer(space_invaders.sdl_renderer);
	SDL_DestroyWindow(space_invaders.sdl_window);

	SDL_QuitSubSystem(space_invaders.sdl_initialized);
}

static bool
space_invaders_board_isonline(struct i8080_cpu *cpu) {
	return space_invaders.isonline;
}

static inline uint64_t
space_invaders_sdl_key_mask(SDL_Keycode keycode, uint64_t mask) {
	return -(uint64_t)space_invaders.sdl_keyboard_state[SDL_GetScancodeFromKey(keycode)] & mask;
}

static void
space_invaders_board_poll(struct i8080_cpu *cpu) {

	if((cpu->uptime_cycles * space_invaders.cycle_duration & 0xFFF) == 0) {
		SDL_Event event;

		while(SDL_PollEvent(&event) != 0) {
			switch(event.type) {
			case SDL_QUIT:
				space_invaders.isonline = false;
				break;
			}
		}

		space_invaders.inputs = SPACE_INVADERS_MASK_INPUT_DEFAULT
			| space_invaders_sdl_key_mask(SDLK_SPACE, SPACE_INVADERS_MASK_INPUT_CREDIT)
			| space_invaders_sdl_key_mask(SDLK_1, SPACE_INVADERS_MASK_INPUT_1P_START)
			| space_invaders_sdl_key_mask(SDLK_2, SPACE_INVADERS_MASK_INPUT_2P_START)
			| space_invaders_sdl_key_mask(SDLK_LEFT, SPACE_INVADERS_MASK_INPUT_P1_LEFT)
			| space_invaders_sdl_key_mask(SDLK_RIGHT, SPACE_INVADERS_MASK_INPUT_P1_RIGHT)
			| space_invaders_sdl_key_mask(SDLK_UP, SPACE_INVADERS_MASK_INPUT_P1_SHOT)
			| space_invaders_sdl_key_mask(SDLK_q, SPACE_INVADERS_MASK_INPUT_P2_LEFT)
			| space_invaders_sdl_key_mask(SDLK_d, SPACE_INVADERS_MASK_INPUT_P2_RIGHT)
			| space_invaders_sdl_key_mask(SDLK_z, SPACE_INVADERS_MASK_INPUT_P2_SHOT)
		;
	}
}

static void
space_invaders_blit(const uint8_t *vram, bool vblank) {
	const SDL_Point center = { .x = SPACE_INVADERS_SCREEN_WIDTH / 2, .y = SPACE_INVADERS_SCREEN_WIDTH / 2 };
	SDL_Rect src = { .x = 0, .y = 0, .w = SPACE_INVADERS_SCREEN_WIDTH, .h = SPACE_INVADERS_SCREEN_HEIGHT / 2, };
	SDL_Rect dest = { .x = src.x + (SPACE_INVADERS_SCREEN_WIDTH - SPACE_INVADERS_SCREEN_HEIGHT) / 2, .y = src.y, .w = src.w, .h = src.h };
	uint8_t pixels[SPACE_INVADERS_SCREEN_WIDTH * SPACE_INVADERS_SCREEN_HEIGHT / 2];

	if(!vblank) {
		src.y += src.h;
		dest.x += dest.h;
		vram += 0xE00;
	}

	for(unsigned i = 0; i < sizeof(pixels); i++) {
		pixels[i] = -(vram[i / 8] >> (i & 7) & 1);
	}

	SDL_UpdateTexture(space_invaders.sdl_texture, &src, pixels, SPACE_INVADERS_SCREEN_WIDTH);
	SDL_RenderCopyEx(space_invaders.sdl_renderer, space_invaders.sdl_texture, &src, &dest, -90.0, &center, SDL_FLIP_NONE);
	SDL_RenderPresent(space_invaders.sdl_renderer);
}

static void
space_invaders_board_sync(struct i8080_cpu *cpu) {
	const nanoseconds_t elapsed = space_invaders_now() - space_invaders.start,
		uptime = cpu->uptime_cycles * space_invaders.cycle_duration;

	if(uptime > elapsed) {
		space_invaders_sleep(uptime - elapsed);
	}

	const uint64_t interrupt_frame = uptime / (space_invaders.vblank_duration / 2) ;
	while(space_invaders.interrupt_frame != interrupt_frame) {
		const uint8_t * const vram = cpu->memory + 0x2400;

		if(space_invaders.interrupt_frame & 1) { /* VBLANK (high) */
			i8080_cpu_interrupt_restart(cpu, 2); /* RST 10 */
			space_invaders_blit(vram, true);
		} else { /* (low) */
			i8080_cpu_interrupt_restart(cpu, 1); /* RST 8 */
			space_invaders_blit(vram, false);
		}

		space_invaders.interrupt_frame++;
	}
}

static const struct i8080_io space_invaders_io = {
	.input = space_invaders_input, .output = space_invaders_output,
};

const struct i8080_board space_invaders_board = {
	.io = &space_invaders_io,
	.setup = space_invaders_board_setup,
	.teardown = space_invaders_board_teardown,
	.isonline = space_invaders_board_isonline,
	.poll = space_invaders_board_poll,
	.sync = space_invaders_board_sync,
};

