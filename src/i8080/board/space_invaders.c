#include <stdbool.h>
#include <time.h>

#include <SDL2/SDL.h>

#include "space_invaders.h"

#include "../ram.h"

#define SPACE_INVADERS_SCREEN_WIDTH  256
#define SPACE_INVADERS_SCREEN_HEIGHT 224

#define SPACE_INVADERS_BIT_INPUT_CREDIT   8
#define SPACE_INVADERS_BIT_INPUT_P2_START 9
#define SPACE_INVADERS_BIT_INPUT_P1_START 10
#define SPACE_INVADERS_BIT_INPUT_P1_SHOT  12
#define SPACE_INVADERS_BIT_INPUT_P1_LEFT  13
#define SPACE_INVADERS_BIT_INPUT_P1_RIGHT 14
#define SPACE_INVADERS_BIT_INPUT_P2_SHOT  20
#define SPACE_INVADERS_BIT_INPUT_P2_LEFT  21
#define SPACE_INVADERS_BIT_INPUT_P2_RIGHT 22

#define SPACE_INVADERS_MASK_INPUT_DEFAULT  0x080E
#define SPACE_INVADERS_MASK_INPUT_CREDIT   (1 << SPACE_INVADERS_BIT_INPUT_CREDIT)
#define SPACE_INVADERS_MASK_INPUT_P2_START (1 << SPACE_INVADERS_BIT_INPUT_P2_START)
#define SPACE_INVADERS_MASK_INPUT_P1_START (1 << SPACE_INVADERS_BIT_INPUT_P1_START)
#define SPACE_INVADERS_MASK_INPUT_P1_SHOT  (1 << SPACE_INVADERS_BIT_INPUT_P1_SHOT)
#define SPACE_INVADERS_MASK_INPUT_P1_LEFT  (1 << SPACE_INVADERS_BIT_INPUT_P1_LEFT)
#define SPACE_INVADERS_MASK_INPUT_P1_RIGHT (1 << SPACE_INVADERS_BIT_INPUT_P1_RIGHT)
#define SPACE_INVADERS_MASK_INPUT_P2_SHOT  (1 << SPACE_INVADERS_BIT_INPUT_P2_SHOT)
#define SPACE_INVADERS_MASK_INPUT_P2_LEFT  (1 << SPACE_INVADERS_BIT_INPUT_P2_LEFT)
#define SPACE_INVADERS_MASK_INPUT_P2_RIGHT (1 << SPACE_INVADERS_BIT_INPUT_P2_RIGHT)

static struct {
	/* Board management */
	bool isonline;
	uint64_t inputs;
	uint64_t vsync_frame;

	/* Dedicated shift HW */
	uint16_t shift_register;
	unsigned shift_amount;

	/* Time management */
	double cycle_duration;
	double vsync_duration;
	double start_time;

	/* SDL2 */
	Uint32 sdl_initialized;
	const Uint8 *sdl_keyboard_state;
	SDL_Window *sdl_window;
	SDL_Renderer *sdl_renderer;
	SDL_Texture *sdl_texture;
} space_invaders;

static inline double
space_invaders_now(void) {
	struct timespec res;

	if(clock_gettime(CLOCK_REALTIME, &res) != 0) {
		perror("clock_gettime");
		exit(EXIT_FAILURE);
	}

	return (double)res.tv_sec + res.tv_nsec / 1000000000.0;
}

static inline void
space_invaders_sleep(double duration) {
	struct timespec req = {
		.tv_sec = duration,
		.tv_nsec = (duration - req.tv_sec) * 1000000000,
	}, rem;

	while(nanosleep(&req, &rem) != 0) {
		req = rem;
	}
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

	space_invaders.cycle_duration = 1.0 / 2000000.0;
	space_invaders.vsync_duration = 1.0 / 120.0;
	space_invaders.start_time = space_invaders_now();

	const Uint32 required_initialized = SDL_INIT_VIDEO;
	space_invaders.sdl_initialized = SDL_WasInit(required_initialized) ^ required_initialized;
	if(SDL_InitSubSystem(space_invaders.sdl_initialized) != 0) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
		goto space_invaders_board_setup_err0;
	}

	space_invaders.sdl_keyboard_state = SDL_GetKeyboardState(NULL);

	if(SDL_CreateWindowAndRenderer(SPACE_INVADERS_SCREEN_WIDTH, SPACE_INVADERS_SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE,
		&space_invaders.sdl_window, &space_invaders.sdl_renderer) != 0) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer SDL: %s", SDL_GetError());
		goto space_invaders_board_setup_err1;
	}

	SDL_SetWindowSize(space_invaders.sdl_window, SPACE_INVADERS_SCREEN_WIDTH, SPACE_INVADERS_SCREEN_WIDTH);

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

static unsigned debug;

static void
space_invaders_board_poll(struct i8080_cpu *cpu) {
	SDL_Event event;

	while(SDL_PollEvent(&event) != 0) {
		switch(event.type) {
		case SDL_QUIT:
			space_invaders.isonline = false;
			break;
		}
	}

	space_invaders.inputs |= -(uint64_t)space_invaders.sdl_keyboard_state[SDL_SCANCODE_LEFT]  & SPACE_INVADERS_MASK_INPUT_P1_LEFT;
	space_invaders.inputs |= -(uint64_t)space_invaders.sdl_keyboard_state[SDL_SCANCODE_RIGHT] & SPACE_INVADERS_MASK_INPUT_P1_RIGHT;
	space_invaders.inputs |= -(uint64_t)space_invaders.sdl_keyboard_state[SDL_SCANCODE_UP]    & SPACE_INVADERS_MASK_INPUT_P1_SHOT;
	space_invaders.inputs |= -(uint64_t)space_invaders.sdl_keyboard_state[SDL_SCANCODE_RCTRL] & SPACE_INVADERS_MASK_INPUT_P1_START;
	space_invaders.inputs |= -(uint64_t)space_invaders.sdl_keyboard_state[SDL_SCANCODE_Q]     & SPACE_INVADERS_MASK_INPUT_P1_LEFT;
	space_invaders.inputs |= -(uint64_t)space_invaders.sdl_keyboard_state[SDL_SCANCODE_D]     & SPACE_INVADERS_MASK_INPUT_P1_RIGHT;
	space_invaders.inputs |= -(uint64_t)space_invaders.sdl_keyboard_state[SDL_SCANCODE_Z]     & SPACE_INVADERS_MASK_INPUT_P1_SHOT;
	space_invaders.inputs |= -(uint64_t)space_invaders.sdl_keyboard_state[SDL_SCANCODE_LCTRL] & SPACE_INVADERS_MASK_INPUT_P1_START;
	space_invaders.inputs |= -(uint64_t)space_invaders.sdl_keyboard_state[SDL_SCANCODE_SPACE] & SPACE_INVADERS_MASK_INPUT_CREDIT;
}

static void
space_invaders_blit(const uint8_t *vram, bool vblank) {
	uint8_t pixels[SPACE_INVADERS_SCREEN_WIDTH * SPACE_INVADERS_SCREEN_HEIGHT];

	for(unsigned x = 0; x < SPACE_INVADERS_SCREEN_WIDTH; x++) {
		for(unsigned y = 0; y < SPACE_INVADERS_SCREEN_HEIGHT; y++) {
			const unsigned index = x + (y * SPACE_INVADERS_SCREEN_WIDTH);
			const uint8_t pixel = -(vram[index / 8] >> (x & 7) & 1);
			pixels[index] = pixel;
		}
	}

	SDL_UpdateTexture(space_invaders.sdl_texture, NULL, pixels, SPACE_INVADERS_SCREEN_WIDTH);
	SDL_RenderCopyEx(space_invaders.sdl_renderer, space_invaders.sdl_texture, NULL, NULL, -90.0, NULL, SDL_FLIP_NONE);
	SDL_RenderPresent(space_invaders.sdl_renderer);
}

static void
space_invaders_board_sync(struct i8080_cpu *cpu) {
	const double elapsed = space_invaders_now() - space_invaders.start_time,
		uptime = cpu->uptime_cycles * space_invaders.cycle_duration;

	if(uptime > elapsed) {
		space_invaders_sleep(uptime - elapsed);
	}

	const uint64_t vsync_frame = uptime / space_invaders.vsync_duration;
	while(space_invaders.vsync_frame != vsync_frame) {
		const uint8_t * const vram = cpu->memory + 0x2400;
		const union i8080_imm imm = { };

		if(space_invaders.vsync_frame & 1) { /* VBLANK (high) */
			i8080_cpu_interrupt(cpu, 0xD7, imm); /* RST 10 */
			space_invaders_blit(vram, true);
		} else { /* (low) */
			i8080_cpu_interrupt(cpu, 0xCF, imm); /* RST 8 */
			space_invaders_blit(vram, false);
		}

		space_invaders.vsync_frame++;
	}

	space_invaders.inputs = SPACE_INVADERS_MASK_INPUT_DEFAULT;
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

