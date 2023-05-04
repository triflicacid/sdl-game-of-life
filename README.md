# Game of Life

Conway's Game of Life, programmed in C++ using SDL.

## Execution

`./main <window dimensions> <grid dimensions> [<simulation delay>]`

- `<window dimensions>` : dimensions of window in pixels. May be `x,y` or `d` to set `x=y=d`.
- `<grid dimensions>` : dimensions of the grid. **Must** be less than or equal to the window dimensions. May be `x,y` or `d` to set `x=y=d`.
- `<simulation delay>` : calculate the next iteration of the every `x` milliseconds.

## Controls

Keyboard

- `c` : clear the grid (all dead)
- `f` : fully populate the grid (all live)
- `l` : populate the grid from saved data in `grid-save.bin`
- `q` : quit application
- `r` : populate the grid randomly (50% alive, 50% dead)
- `s` : save the current grid to `grid-save.bin`
- `Space` : pauses/resumes the simulation
- `+` : zoom in (around viewport centre)
- `-` : zoom out (around viewport centre)
- Arrow keys: move the viewport in the indicated direction, enlarging the grid if necessary.

Mouse

- `Click` : toggle the state of the target cell. The mouse may be dragged whilst pressed.


## Ideas

- Pass in save file name by argument.
- Debug mode: show information in window, such as position in a clear color (such as magenta).
- Save to file: only save what is in the viewport. First two bytes represent width/height of grid. When loading from a file, spawn the shape in in the centre of the viewport.
- Spawn in various shapes using function keys, stored in `shapes/`.
- Calculate FPS using last x frames
- Parallelise computation and/or rendering using CUDA C.
