
// Tetris by Juha Ulkoniemi 2016
z, x  = rotate block left/right
down  = drop block
ESC   = quit

requires SDL1.2, SDL_image and SDL_mixer.

please compile with 
g++ tetris.cpp -o tetris `sdl-config --cflags --libs` -lSDL -lSDL_image -lSDL_mixer
