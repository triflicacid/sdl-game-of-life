#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <chrono>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include "bit_array.c"

#define COLOR_ALIVE 255, 255, 255, 255
#define COLOR_DEAD 0, 0, 0, 255

struct vec {
    int x;
    int y;
};

/** Given string "x,y", return vector */
vec str_to_vec(char *str) {
    char *p = str, comma = 0;
    while (*p != ',' && *p != '\0') ++p;
    if (*p == ',') {
        comma = 1;
        *p++ = '\0';
    }

    vec v;
    v.x = strtoul(str, 0, 10);
    v.y = comma ? strtoul(p, 0, 10) : v.x;
    return v;
}

struct grid {
    char *data;
    vec dim;
};

grid *create_grid(vec dim) {
    grid *g = (grid *) malloc(sizeof(grid));
    g->data = (char *) calloc(dim.x * dim.y, sizeof(char));
    g->dim = dim;
    return g;
}

grid *create_grid_with_data(char *data, vec dim) {
    grid *g = (grid *) malloc(sizeof(grid));
    g->data = (char *) malloc(dim.x * dim.y);
    memcpy(g->data, data, dim.x * dim.y);
    g->dim = dim;
    return g;
}

/** Clone a grid */
grid *clone_grid(grid *g) {
    grid *g_new = create_grid(g->dim);
    memcpy(g_new->data, g->data, g->dim.x * g->dim.y);
    return g_new;
}

void destroy_grid(grid *g) {
    free(g->data);
    free(g);
}

inline int index(vec pos, vec bounds) {
    return pos.y * bounds.x + pos.x;
}

/** Return whether the current cell is alive */
inline char is_alive(grid *g, vec pos) {
    return pos.x < 0 || pos.y < 0 || pos.x >= g->dim.x || pos.y >= g->dim.y ? 0 : g->data[index(pos, g->dim)];
}

/** Set position to alive or dead */
inline void set_alive(grid *g, vec pos, char alive) {
    g->data[index(pos, g->dim)] = alive;
}

/** Count number of neighbors on a 2D grid */
int count_neighbors(grid *g, vec pos) {
    return is_alive(g, {pos.x - 1, pos.y - 1})
        + is_alive(g, {pos.x, pos.y - 1})
        + is_alive(g, {pos.x + 1, pos.y - 1})
        + is_alive(g, {pos.x - 1, pos.y})
        + is_alive(g, {pos.x + 1, pos.y})
        + is_alive(g, {pos.x - 1, pos.y + 1})
        + is_alive(g, {pos.x, pos.y + 1})
        + is_alive(g, {pos.x + 1, pos.y + 1});
}

/** Blit `src` onto `dst` at the given `pos` (top-left). Assume that pos is valid and that the grid fits. */
void grid_blit(grid *src, grid *dst, vec pos) {
    for (int y = 0, i = 0; y < src->dim.y; ++y) {
        for (int x = 0; x < src->dim.x; ++x, ++i) {
            dst->data[index({ pos.x + x, pos.y + y }, dst->dim)] = src->data[i];
        }
    }
}

/** Fill the grid with a value */
inline void grid_fill(grid *g, char data) {
    memset(g->data, data, g->dim.x * g->dim.y);
}

/** Randomly populate the grid */
void grid_fill_random(grid *g) {
    int n = g->dim.x * g->dim.y;
    for (int i = 0; i < n; ++i) g->data[i] = rand() % 2;
}

/** Save grid to file */
void grid_fwrite(grid *g, const char *filename) {
    FILE *f = fopen(filename, "wb");

    bit_array *arr = bit_array_new(g->dim.x * g->dim.y);
    for (size_t i = 0; i < arr->size; i++) bit_array_set(arr, i, g->data[i]);

    fwrite(arr->data, arr->bytes, 1, f);
    fclose(f);
}

/** Read binary file. Load data into grid. */
void grid_fread(grid *g, const char *filename) {
    FILE *f = fopen(filename, "rb");

    bit_array *arr = bit_array_new(g->dim.x * g->dim.y);
    fread(arr->data, 1, arr->bytes, f);

    for (size_t i = 0; i < arr->size; ++i) {
        g->data[i] = bit_array_get(arr, i);
    }

    fclose(f);
}

/** Draw grid to renderer. Note, clears renderer and forces re-render. */
void draw_grid(SDL_Renderer *r, grid *g, vec *top_left, vec *bottom_right, vec *CDIM) {
    SDL_SetRenderDrawColor(r, COLOR_DEAD);
    SDL_RenderClear(r);
    SDL_SetRenderDrawColor(r, COLOR_ALIVE);
    for (int y = top_left->y, sy = 0; y < bottom_right->y; ++y, ++sy) {
        for (int x = top_left->x, sx = 0; x < bottom_right->x; ++x, ++sx) {
            size_t i = index({ x, y }, g->dim);
            if (g->data[i] != 0) {
                const SDL_Rect rect = { sx * CDIM->x, sy * CDIM->y, CDIM->x, CDIM->y };
                SDL_RenderFillRect(r, &rect);
            }
        }
    }
    SDL_RenderPresent(r);
}

/** Update state of grid */
void grid_update(grid *g) {
    size_t size = g->dim.x * g->dim.y;
    char *new_data = (char *) malloc(size);
    memcpy(new_data, g->data, size);

    for (vec v = { 0, 0 }; v.y < g->dim.y; ++v.y) {
        for (v.x = 0; v.x < g->dim.x; ++v.x) {
            int n = count_neighbors(g, v);
            int i = index(v, g->dim);

            if (is_alive(g, v)) {
                if (n < 2 || n > 3) {
                    new_data[i] = 0;
                }
            } else {
                if (n == 3) {
                    new_data[i] = 1;
                }
            }
        }
    }

    free(g->data);
    g->data = new_data;
}

/** Increase grid in X direction */
void grid_expand_right(grid *g, size_t amount) {
    char *new_data = (char *)calloc((g->dim.x + amount) * g->dim.y, sizeof(char));

    for (size_t y = 0; y < g->dim.y; ++y) {
        size_t src = y * g->dim.x;
        size_t dst = y * (g->dim.x + amount);
        memcpy(new_data + dst, g->data + src, g->dim.x);
    }

    free(g->data);
    g->dim.x += amount;
    g->data = new_data;
}

/** Increase grid in Y direction */
void grid_expand_up(grid *g, size_t amount) {
    char *new_data = (char *)calloc(g->dim.x * (g->dim.y + amount), sizeof(char));
    memcpy(new_data + amount * g->dim.x, g->data, g->dim.x * g->dim.y);

    free(g->data);
    g->dim.y += amount;
    g->data = new_data;
}


/** Increase grid in -Y direction */
void grid_expand_down(grid *g, size_t amount) {
    char *new_data = (char *)calloc(g->dim.x * (g->dim.y + amount), sizeof(char));
    memcpy(new_data, g->data, g->dim.x * g->dim.y);

    free(g->data);
    g->dim.y += amount;
    g->data = new_data;
}

/** Increase grid in -X direction */
void grid_expand_left(grid *g, size_t amount) {
    char *new_data = (char *)calloc((g->dim.x + amount) * g->dim.y, sizeof(char));

    for (size_t y = 0; y < g->dim.y; ++y) {
        size_t src = y * g->dim.x;
        size_t dst = y * (g->dim.x + amount) + amount;
        memcpy(new_data + dst, g->data + src, g->dim.x);
    }

    free(g->data);
    g->dim.x += amount;
    g->data = new_data;
}

/** grid: move viewport to the right `n` places */
void viewport_mv_right(grid *g, vec *tl, vec *br, size_t n) {
    // needs expanding?
    if (br->x + n > g->dim.x) {
        grid_expand_right(g, br->x + n - g->dim.x);
    }

    // move viewport vectors
    tl->x += n;
    br->x += n;
}

/** Viewport: move viewport to the left `n` places */
void viewport_mv_left(grid *g, vec *tl, vec *br, size_t n) {
    // needs expanding?
    if (n > tl->x) {
        size_t m = n - tl->x;
        grid_expand_left(g, m);
        tl->x += m;
        br->x += m;
    }

    // move viewport vectors
    tl->x -= n;
    br->x -= n;
}

/** Viewport: move viewport up `n` places */
void viewport_mv_up(grid *g, vec *tl, vec *br, size_t n) {
    // needs expanding?
    if (n > tl->y) {
        size_t m = n - tl->y;
        grid_expand_up(g, m);
        tl->y += m;
        br->y += m;
    }

    // move viewport vectors
    tl->y -= n;
    br->y -= n;
}

/** Viewport: move viewport down `n` places */
void viewport_mv_down(grid *g, vec *tl, vec *br, size_t n) {
    // needs expanding?
    if (br->y + n > g->dim.y) {
        grid_expand_down(g, br->y + n - g->dim.y);
    }

    // move viewport vectors
    tl->y += n;
    br->y += n;
}

/** Viewport: zoom in */
void viewport_zoom_in(grid *g, vec *tl, vec *br, vec *cell_dim, vec *win_dim) {
    if (cell_dim->x >= win_dim->x || cell_dim->y >= win_dim->y) return; // Fully zoomed in

    int w = br->x - tl->x, h = br->y - tl->y;
    tl->x += w / 4;
    tl->y += h / 4;
    br->x -= w / 4;
    br->y -= h / 4;
    cell_dim->x *= 2;
    cell_dim->y *= 2;
}

/** grid: zoom out */
void viewport_zoom_out(grid *g, vec *tl, vec *br, vec *cell_dim, vec *win_dim) {
    if (cell_dim->x < 2 || cell_dim->y < 2) return; // Fully zoomed out

    int w = br->x - tl->x, h = br->y - tl->y;
    br->x *= 2;
    br->y *= 2;
    cell_dim->x /= 2;
    cell_dim->y /= 2;

    char *new_data = (char *) calloc(g->dim.x * g->dim.y * 4, sizeof(char));

    for (int y = 0, i = 0; y < g->dim.y; ++y) {
        for (int x = 0; x < g->dim.x; ++x, ++i) {
            new_data[(tl->y + y + h / 2) * g->dim.x * 2 + tl->x + x + w / 2] = g->data[i];
        }
    }

    free(g->data);
    g->data = new_data;
    g->dim.x *= 2;
}

/** Given the co-ordinates on the screen, return grid co-ordinates. The second argument is the dimensions (in px) of each cell. */
vec screen_to_grid_pos(vec screen, vec cell) {
    return { screen.x / cell.x, screen.y / cell.y };
}

int main(int argc, char **argv) {
    vec WIN = str_to_vec(argv[1]); // Window dimensions (pixels)
    vec CELLS = str_to_vec(argv[2]); // Number of cells in viewport

    if (CELLS.x > WIN.x) {
        printf("CELLS.x must be less than or equal to WIN.x (%i)\n", WIN.x);
        return -1;
    }
    if (CELLS.y > WIN.y) {
        printf("CELLS.y must be less than or equal to WIN.y (%i)\n", WIN.y);
        return -1;
    }

    const int SIM_EVERY = (argc > 3 ? strtoul(argv[3], 0, 10) : 200) * 1e6; // Update simulation every x ms

    vec CDIM = { WIN.x / CELLS.x, WIN.y / CELLS.y }; // Cell dimensions (pixels)
    grid *g = create_grid(CELLS);
    vec top_left = { 0, 0 }, bottom_right = { CELLS.x, CELLS.y }; // These two vectors are used to define the viewport, co-ordinates are in grid space. Grid data starts at <0,0>

    // Seed random
    srand(time(0));

    // char small_data[] = { 1, 0, 0, 1 };
    // grid *small = create_grid_with_data(small_data, { 2, 2 });
    // grid_blit(small, g, { 0, 0 });
    // destroy_grid(small);

    // Create window
    SDL_Init(SDL_INIT_EVERYTHING);

    char paused = 0, update = 1, is_mouse_pressed = 0;
    auto last_sim = std::chrono::high_resolution_clock::now(); // Time at which the last simulation update took place

    SDL_Window *window = SDL_CreateWindow("Game of Life", 20, 20, WIN.x, WIN.y, SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Event event;

    while (1) {
        while (SDL_PollEvent(&event)) { // Check every event which occured since last check
            if (SDL_QUIT == event.type) {
                goto end; // Exit simulation
            } else if (SDL_KEYUP == event.type) {
                switch (event.key.keysym.sym) {
                    case SDLK_q: // Quit
                        goto end;
                    case SDLK_d: // Debug
                        printf("=== [ DEBUG ] ===\n");
                        printf("> Screen Dimensions: screen=<%i,%i>px, cell=<%i,%i>px\n", WIN.x, WIN.y, CDIM.x, CDIM.y);
                        printf("> Grid Dimensions: <%i,%i>\n", g->dim.x, g->dim.y);
                        printf("> Viewport: <%i,%i> to <%i,%i>\n", top_left.x, top_left.y, bottom_right.x, bottom_right.y);
                        printf("> Cells in viewport: <%i,%i> (total %i)\n", CELLS.x, CELLS.y, CELLS.x * CELLS.y);
                        printf("> Paused: %s\n", paused ? "true" : "false");
                        printf("=================\n");
                        break;
                    case SDLK_SPACE: // Pause/resume
                        paused = !paused;
                        break;
                    case SDLK_c: // Clear
                        grid_fill(g, 0);
                        update = 1;
                        break;
                    case SDLK_f: // Fill
                        grid_fill(g, 1);
                        update = 1;
                        break;
                    case SDLK_o: // Move to origin
                        top_left.x = top_left.y = 0;
                        bottom_right.x = CELLS.x;
                        bottom_right.y = CELLS.y;
                        update = 1;
                        break;
                    case SDLK_r: // Random
                        grid_fill_random(g);
                        update = 1;
                        break;
                    case SDLK_s: // Save to file
                        grid_fwrite(g, "grid-save.bin");
                        break;
                    case SDLK_l: // Load from file
                        grid_fread(g, "grid-save.bin");
                        update = 1;
                        break;
                    case SDLK_RIGHT: // Move to the right
                        viewport_mv_right(g, &top_left, &bottom_right, 1);
                        update = 1;
                        break;
                    case SDLK_LEFT: // Move to the left
                        viewport_mv_left(g, &top_left, &bottom_right, 1);
                        update = 1;
                        break;
                    case SDLK_UP: // Expand upwards
                        viewport_mv_up(g, &top_left, &bottom_right, 1);
                        update = 1;
                        break;
                    case SDLK_DOWN: // Move downwards
                        viewport_mv_down(g, &top_left, &bottom_right, 1);
                        update = 1;
                        break;
                    case SDLK_EQUALS:
                        if (event.key.keysym.mod & KMOD_SHIFT != 0) { // Plus: zoom in
                            viewport_zoom_in(g, &top_left, &bottom_right, &CDIM, &WIN);
                            update = 1;
                        }
                        break;
                    case SDLK_MINUS: // Zoom out
                        viewport_zoom_out(g, &top_left, &bottom_right, &CDIM, &WIN);
                        update = 1;
                        break;
                }
            } else if (SDL_MOUSEBUTTONUP == event.type) {
                is_mouse_pressed = 0;
            } else if (SDL_MOUSEBUTTONDOWN == event.type) {
                is_mouse_pressed = 1;
            }
        }

        // Is the mouse button pressed?
        if (is_mouse_pressed) {
            vec mouse;
            SDL_GetMouseState(&mouse.x, &mouse.y);
            vec pos = screen_to_grid_pos(mouse, CDIM); // Screen -> grid space
            // Align to viewport
            pos.x += top_left.x;
            pos.y += top_left.y;
            set_alive(g, pos, !is_alive(g, pos)); // Invert cell at this position
            update = 1;
        }

        // Update the screen render if necessary
        if (update) {
            draw_grid(renderer, g, &top_left, &bottom_right, &CDIM);
            update = 0;
        }

        // If the simulation is not paused, check if it needs updating
        if (!paused) {
            auto current_time = std::chrono::high_resolution_clock::now();
            if ((current_time - last_sim).count() >= SIM_EVERY) {
                grid_update(g);
                last_sim = current_time;
                update = 1;
            }
        }
    }

end:
    SDL_DestroyWindow(window);
    SDL_Quit();

    destroy_grid(g);

    return 0;
}