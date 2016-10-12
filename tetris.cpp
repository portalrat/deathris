// Tetris by Juha Ulkoniemi 2016
// z, x  = rotate block left/right
// down  = drop block
// ESC   = quit

                 
#include "SDL/SDL.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_mixer.h"
#include "SDL_syswm.h"
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream> 
#include <queue>
#include <sstream>
#include <cctype> 

using namespace std;

int block_x = 0, block_y = 0;
int block_v = 0;

int dbg_cntr = 0;

const int SCREEN_WIDTH = 350;
const int SCREEN_HEIGHT = 330;
const int SCREEN_BPP = 32;
const int FRAMES_PER_SECOND = 25;

const int I_BLOCK = 0;
const int J_BLOCK = 2;
const int L_BLOCK = 6;
const int O_BLOCK = 10;
const int T_BLOCK = 11;
const int S_BLOCK = 15;
const int Z_BLOCK = 17;

int block_types[] = {I_BLOCK, J_BLOCK, L_BLOCK, O_BLOCK, T_BLOCK, S_BLOCK, Z_BLOCK};
//vector<int> block_types;
vector<int>::iterator vi;
map<int, int> sprite_coords;

typedef struct {
    int x;
    int y;
} piece;

typedef struct {
    int x;
    int y;
    int block_type;
    int position;
    bool dead;
    vector<piece*> pieces; 
} block;
block *block_ptr, *next_block_ptr;

block* get_new_block();

// Sisältää koordinaatit neljälle pikkupalikalle, jotka muodostavat ison palikan.
struct block_coords {
    int x[4];
    int y[4];

};

// Sisältää kaikkien palikoiden kaikki eri asennot, järjestyksessä I_BLOCK .. Z_BLOCK.
struct block_positions {
    block_coords bc_array[19];
    // I:lle 2 asentoa
    // J
    // L
    // O
    // T
    // S
    // Z
};
// sisältää jokaisen palikan asentojen lukumäärän.
// FIXME: olettaa, että indeksien erotus on 1, joten ei toimi enää,
// jos lisätään uusia palikoita, koska I_BLOCK = 0, J_BLOCK = 2, L_BLOCK = 6?
map<int, int> frame_count;

struct block_positions bp = 
// sisältää kaksi asentoa I-palikalle:
//
//  0)  #    1) 
//      #       ####
//      #
//      #     
{0, 0, 0, 0, 0, 1, 2, 3, -2, -1, 0, 1, 1, 1, 1, 1,     // I   2 asentoa
 0, 0, 0, -1, -1, 0, 1, 1, -1, -1, 0, 1, -1, 0, 0, 0,  // J   4
 1, 0, 0, 0, -1, -1, 0, 1, -1, 0, 1, 1, 0, 0, 0, 1,     
 0, 0, 0, 1, -1, 0, 1, 1, -1, -1, 0, 1, 1, 0, 0, 0,    // L   4
 -1, 0, 0, 0, -1, -1, 0, 1, -1, 0, 1, 1, 0, 0, 0, -1, 
 -1, 0, -1, 0, -1, 0, 0, -1,                           // 0   1
 -1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, -1, 1, 0,    // T   4
 -1, 0, 0, 1, 0, -1, 0, 0,   0, 0, 0, -1, -1, 0, 1, 0,
 -1, 0, 0, 1, 0, 0, -1, -1, -1, -1, 0, 0, -1, 0, 0, 1,  // S   2
 -1, 0, 0, 1, -1, -1, 0, 0, 0, 0, 1, 1, 1, 0, 0, -1 };// Z   2    

// palikoiden akselit
//  # I   # J #  L     S     Z     T
//  #     X   X     X#   #X    #X# 
//  X    ##   ##   ##     ##    #
//  #
// syntaksitesti
//block_coords bc[2] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1};
//block_pos.bc_array[0].x = {0, 0, 0, 0};
//block_pos.bc_array[0].y = {0, 1, 2, 3};

typedef struct {
	string name;
	int score;
} highscore_entry;
highscore_entry *hs_ptr;

vector<highscore_entry*> highscore_vector;

Mix_Chunk *block_rotate = NULL;
Mix_Chunk *block_drop = NULL;
Mix_Chunk *line_delete = NULL;
bool line_found = false;
bool line = false; // edellistä muuttujaa ei voi käyttää ääniefektien soittamiseen,
                   // koska check_for_lines nollaa sen arvon aina funktiokutsun alussa.

vector<block*> blockvector;
vector<block*>::iterator bi;

vector<int> deleted_lines;  		// sisältää poistettavien rivien numerot.
vector<SDL_Surface*> line_surfaces; // sisältää poistettavien rivien pintojen osoittimet.

// Table of free squares
int squareTable[10][20];


SDL_Surface *screen; 
SDL_Surface *block_sprite;
SDL_Surface *background; 
SDL_Surface *field; // pelikenttä
SDL_Surface *font; // bittikarttafontti
SDL_Surface *empty_square;
// debug
//SDL_Surface *dead_sprite;
//SDL_Surface *floodfill_sprite;


int line_counter = 0;
Uint32 wait_delay = 0;
int level = 1;
int num_lines = 0;
int multiplier = 0; // yhdellä kerralla poistettujen rivien lukumäärä = kerroin
int score = 0;
bool update_highscore_file = false;

SDL_Event event;

// prototyypit

SDL_Surface *load_image(string name)
{
    SDL_Surface *image = NULL;
    
    image = IMG_Load(name.c_str());
    return image;
}

void blit_surface(int x, int y, SDL_Surface *src, SDL_Surface *dst)
{
    SDL_Rect rect;

    rect.x = x;
    rect.y = y;    
    
    SDL_BlitSurface(src, NULL, dst, &rect);
}

// Ylikuormitettu versio edellisestä funktiosta fonttiengineä varten.
// src_rect-parametri sisältää osoittimen lähteenä käytettävän pinnan
// rectiin.
void blit_surface(int x, int y, SDL_Surface *src, SDL_Surface *dst, SDL_Rect *src_rect)
{
    SDL_Rect rect;
    
    rect.x = x;
    rect.y = y;    
    
    SDL_BlitSurface(src, src_rect, dst, &rect);
}

bool init()
{
	//ofstream myfile("init_debug.txt");    // debug
	srand(time(NULL));
    // FIXME
    block_sprite = load_image("blocks.png");
	
	// DEBUG
	//if (block_sprite == NULL)
	//	myfile << "block_sprite == NULL: " << IMG_GetError() << endl;
	
    //dead_sprite = load_image("deadblock.png");
    //floodfill_sprite = load_image("floodfill.png");
    //block_ptr = get_new_block();
    block_ptr = get_new_block();
    // seuraava rivi siirrettiin get_new_block2()-funktiosta tähän,
    // jotta next_block_ptr toimisi oikein.
    blockvector.push_back(block_ptr);
    next_block_ptr = get_new_block();
    
    for (int i = 0; i < 10; i++)
        for (int j = 0; j < 20; j++)
            squareTable[i][j] = 0;
    
    sprite_coords[I_BLOCK] = 0;
    sprite_coords[J_BLOCK] = 1;
    sprite_coords[L_BLOCK] = 2;
    sprite_coords[O_BLOCK] = 3;
    sprite_coords[T_BLOCK] = 4;
    sprite_coords[S_BLOCK] = 5;
    sprite_coords[Z_BLOCK] = 6;
    
    if (SDL_Init(SDL_INIT_EVERYTHING) == -1)
    {
		// DEBUG
		//myfile << "SDL_Init failed: " << SDL_GetError() << endl;
        return false;    
    }
    
    if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096) == -1)
    {
		//myfile << "Mix_OpenAudio failed: " << Mix_GetError() << endl;
        return false;
    }
    
    // lataa ääniefektit

    block_rotate = Mix_LoadWAV("block_rotate.wav");
    block_drop = Mix_LoadWAV("block_drop.wav");
    line_delete = Mix_LoadWAV("line_delete.wav");
    
    // debug 
    //block_rotate = Mix_LoadWAV("empty.wav");
    //block_drop = Mix_LoadWAV("empty.wav");
    //row_delete = Mix_LoadWAV("empty.wav");
	
	// DEBUG
	/*
	if (block_rotate == NULL)
		myfile << "block_rotate == NULL: " << Mix_GetError() << endl;
    
	if (block_drop == NULL)
		myfile << "block_drop == NULL: " << Mix_GetError() << endl;
		
	if (line_delete == NULL)
		myfile << "line_delete == NULL: " << Mix_GetError() << endl;
	*/
	
    if (block_rotate == NULL || block_drop == NULL || line_delete == NULL)
        return false;
    
    SDL_WM_SetCaption( "tetris", NULL );

    screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, SDL_HWSURFACE);
    // SDL_MaximizeWindow(
	
    // lataa grafiikat
	
	empty_square = load_image("empty.png");
	
    font = load_image("font3.png");
    
    background = load_image("background.png");
   
    field = load_image("field.png");
	
	
    if (screen == NULL || block_sprite == NULL ||  background == NULL || field == NULL 
        || font == NULL || empty_square == NULL)
        return false;
  
    return true;
}

void clean_up()
{
    vector<piece*>::iterator pi;
    // removed block 1.00 memory deallocation
        
    for (bi = blockvector.begin(); bi < blockvector.end(); bi++) {
        for (pi = (*bi)->pieces.begin(); pi != (*bi)->pieces.end(); pi++)
            delete *pi;
        delete *bi;
    }
	/*
	if (block_ptr != NULL) {
		//for (pi = block_ptr->pieces.begin(); pi != block_ptr->pieces.end(); pi++)
		//	delete *pi;
		
		delete block_ptr;
	}
	*/
	delete next_block_ptr;
	
	for (int i = 0; i < highscore_vector.size(); i++)
		delete highscore_vector[i];
	/*
	if (next_block_ptr != NULL) {
		for (pi = next_block_ptr->pieces.begin(); pi != next_block_ptr->pieces.end(); pi++)
			delete *pi;
		delete next_block_ptr;
	}
    */
	
    SDL_FreeSurface(block_sprite);
    //SDL_FreeSurface(dead_sprite);   // debug
    //SDL_FreeSurface(floodfill_sprite);
    SDL_FreeSurface(background);
    SDL_FreeSurface(field);
    SDL_FreeSurface(font);
	SDL_FreeSurface(empty_square);
    
    Mix_FreeChunk(block_rotate);
    Mix_FreeChunk(block_drop);
    Mix_FreeChunk(line_delete);
    Mix_CloseAudio();
    
    SDL_Quit();
}


int find_biggest_y(block *block_ptr = block_ptr)
{
    int bi = 0;
    int by = 0;
    
    for (int i = 0; i < block_ptr->pieces.size(); i++)
        if (block_ptr->pieces[i]->y > by) {
            by = block_ptr->pieces[i]->y;
            bi = i;
        }
            
    return bi; 
}

bool CheckSquares(block* block_ptr, int y = 0)
{
     for (int i = 0; i < block_ptr->pieces.size(); i++)
         // jos ruutu on varattu tai se on pelikentän rajojen ulkopuolella (< 0 tai > 9) niin 
         // palauta arvo false.
         if ((squareTable[(block_ptr->pieces[i]->x)][(block_ptr->pieces[i]->y)+y] == 1) 
              || ((block_ptr->pieces[i]->x) < 0)
              || ((block_ptr->pieces[i]->x) > 9))
             return false;
     
     return true;
}

void AllocateSquares(block *block_ptr)
{
    for (int i = 0; i < 4; i++)
        squareTable[(block_ptr->pieces[i]->x)][(block_ptr->pieces[i]->y)] = 1;
}       //                        tuossa oli kirjoitusvirhe ------------^
        // Siinä oli y:n sijasta x. (04/05 18:07)

void move_block()
{
    block_ptr->x += block_v;
    for (int i = 0; i < 4; i++)
        block_ptr->pieces[i]->x+=block_v;
      
    if (!CheckSquares(block_ptr)) {
        for (int i = 0; i < 4; i++) 
            block_ptr->pieces[i]->x-=block_v;
        block_ptr->x -= block_v;
    }
}

// FONTTIENGINE
void write_text(string myString, int x, int y)
{
    map<char, int> fontCoords;
    SDL_Rect font_rect;

    font_rect.w = 10;
    font_rect.h = 10;
    font_rect.y = 0;
    fontCoords['A'] = 0;
    fontCoords['B'] = 1;
    fontCoords['C'] = 2;
	fontCoords['D'] = 3;
    fontCoords['E'] = 4;
    fontCoords['F'] = 5;
    fontCoords['G'] = 6;
    fontCoords['H'] = 7;
    fontCoords['I'] = 8;
    fontCoords['J'] = 9;
    fontCoords['K'] = 10;
    fontCoords['L'] = 11;
    fontCoords['M'] = 12;
    fontCoords['N'] = 13;
    fontCoords['O'] = 14;
    fontCoords['P'] = 15;
    fontCoords['Q'] = 16;
    fontCoords['R'] = 17;
    fontCoords['S'] = 18;
    fontCoords['T'] = 19;
    fontCoords['U'] = 20;
    fontCoords['V'] = 21;
    fontCoords['W'] = 22;
    fontCoords['X'] = 23;
    fontCoords['Y'] = 24;
    fontCoords['Z'] = 25;
	fontCoords['0'] = 26;
    fontCoords['1'] = 27;
    fontCoords['2'] = 28;
    fontCoords['3'] = 29;
	fontCoords['4'] = 30;
    fontCoords['5'] = 31;
    fontCoords['6'] = 32;
    fontCoords['7'] = 33;
    fontCoords['8'] = 34;
    fontCoords['9'] = 35;
	fontCoords['_'] = 36;
	fontCoords['!'] = 37;
	fontCoords[':'] = 38;
    
    for (int i = 0; i < myString.length(); i++) {
        if (myString[i] == ' ') {
			SDL_Rect rect;
			rect.x = x+i*10;
			rect.y = y;
			rect.w = 10;
			rect.h = 10;
			SDL_FillRect(screen, &rect, SDL_MapRGB(screen->format, 0x00, 0x00, 0x00));
			//continue;
		} else {
			font_rect.x = fontCoords[myString[i]] * 10;
			blit_surface(x+i*10, y, font, screen, &font_rect);
		}
    }
}

block* get_new_block()
{
    block *block_ptr = new(block);
    piece *piece_ptr;
    
    block_ptr->block_type = block_types[rand()%7];
    //block_ptr->block_type = I_BLOCK;
    
    block_ptr->position = 0;
    block_ptr->x = 4;
    block_ptr->y = 1;
    block_ptr->dead = false;
    
    for (int i = 0; i < 4; i++) {
        piece_ptr = new(piece);
        block_ptr->pieces.push_back(piece_ptr);
    }
   
    for (int i = 0; i < 4; i++) {   
        block_ptr->pieces[i]->x = block_ptr->x + bp.bc_array[block_ptr->block_type].x[i];
        block_ptr->pieces[i]->y = block_ptr->y + bp.bc_array[block_ptr->block_type].y[i];
    }
    
    // Jos käytän next_block_ptr:iä, niin en voi automaattisesti työntää kaikkia palikoita
    // vektoriin, koska seuraava palikka ei ole vielä aktiivinen, joten
    // sitä ei saa vielä käsitellä.
    //blockvector.push_back(block_ptr);
    return block_ptr;
}

// Tämän funktion tarkoituksena on poistaa yksi pelikentän rivi, joka koostuu
// tetris-palikoiden palasista.
void delete_line(int y)
{
    
	vector<block*>::iterator bi;
    vector<piece*>::iterator pi;
    
    // Huomasin bugin: squareTable on muodossa [x][y] eikä [y][x].
    // Oletin, että se on muodossa [y][x].
    
    // Vapautetaan rivin käyttämät ruudut.
    
    for (int i = 0; i < 10; i++) {
        squareTable[i][y] = 0;
        
    }
    
    // Käydään läpi kaikki blockit. Jos block on kuollut, käydään läpi kaikki sen
    // palaset. Jos blockin palasen y-koordinaatti on sama kuin poistettavan rivin
    // y-koordinaatti, poistetaan se.
    //
    // Bugi oli siinä, että tuhosin elementin, johon iteraattori osoitti, ja samalla
    // myös iteraattori tuhoutui. Seuraavan kierroksen alussa käytin kuitenkin samaa,
    // väärän arvon sisältävää iteraattoria ja tämän takia ohjelma kaatui. Ratkaisu:
    // Alusta iteraattorin arvo uudelleen jokaisen tuhoamisoperaation jälkeen.
    
    int index = 0;
    int piece_num;
    int num_deleted;
    int block_num = 0;

    queue<int> piece_delq; // jono, joka sisältää poistettavien palasten indeksit.
    queue<int> block_delq; // jono, joka sisältää poistettavien palikoiden indeksit.
    
    for (bi = blockvector.begin(); bi < blockvector.end(); bi++) {
        piece_num = 0;   // 
        num_deleted = 0; //
        
        if ((*bi)->dead) {
            //myfile << "block " << block_num << " contains " << (*bi)->pieces.size() << " pieces." << endl;
            for (pi = (*bi)->pieces.begin(); pi != (*bi)->pieces.end(); pi++) {
                
                //myfile << "inspecting piece (x = " << (*pi)->x << ", y = " << (*pi)->y << ")" << endl;
                
                if ((*pi)->y == y) {
                    //myfile << "deleting piece (x = " << (*pi)->x << ", y = " << (*pi)->y << ")" << endl;
                    piece_delq.push(piece_num);

                }
                piece_num++;
            }
            
            //// V
            while (!piece_delq.empty()) {
                index = piece_delq.front(); // index = poistettavan palasen indeksi pieces-vektorissa.
                //myfile << "index = " << index << endl;
                
                delete (*bi)->pieces.at(index - num_deleted);                      // Vektorin koko pienenee jokaisen poiston jälkeen,                  
                (*bi)->pieces.erase((*bi)->pieces.begin() + (index - num_deleted));// joten indeksistä täytyy vähentää poistettujen
                piece_delq.pop();                                                  // alkioiden lukumäärä.
                num_deleted++;
            }
            //// ^
            
        } // if ((*bi)->dead...
    
        if ((*bi)->pieces.empty()) {    // Jos blockissa ei ole enää yhtään pieceä, poistetaan se.
            //myfile << "block " << block_num << " is empty." << endl;
            block_delq.push(block_num); 
        }
        
        block_num++;
    }
    
    num_deleted = 0; 
                     
    while (!block_delq.empty()) {
        index = block_delq.front();
        //myfile << "block_index = " << index << endl;
        delete blockvector.at(index - num_deleted); // kokoa pitää taas säätää.
        blockvector.erase(blockvector.begin() + (index - num_deleted));
        block_delq.pop();
        num_deleted++;
    }
    
    // Siirretään rivin yläpuolella olevia palikoita yhden rivin verran alaspäin.
    
    for (bi = blockvector.begin(); bi < blockvector.end(); bi++) {
        if ((*bi)->dead)
            for (int i = 0; i < (*bi)->pieces.size(); i++)
                    if ((*bi)->pieces[i]->y < y) {
                        squareTable[(*bi)->pieces[i]->x][(*bi)->pieces[i]->y] = 0;
                        (*bi)->pieces[i]->y++;
                        //squareTable[(*bi)->pieces[i]->x][(*bi)->pieces[i]->y] = 1;
                    }
    }
    
    for (bi = blockvector.begin(); bi < blockvector.end(); bi++) {
        if ((*bi)->dead)
            for (int i = 0; i < (*bi)->pieces.size(); i++)
                squareTable[(*bi)->pieces[i]->x][(*bi)->pieces[i]->y] = 1;
    }
    
    // debug
    //myfile.close();
}

// Etsii vaakasuorat pelikentän rivit ja poistaa ne.
void check_for_lines()
{
    //ofstream myfile("debug.txt");
	line_found = false;
    int num_squares;
    
	//myfile << "entering check_for_lines()..." << endl;
	
    for (int i = 19; i > 0; i--) {
        num_squares = 0;
        
        for (int j = 0; j < 10; j++)
            if (squareTable[j][i] == 1)
                num_squares++;
        
        // Seuraava ehto täyttyy, jos pelikentällä on vaakasuora rivi.
        if (num_squares == 10) {
            deleted_lines.push_back(i);
			line_counter++;
			multiplier++;
			num_lines++;
			//myfile << "line " << i << endl;
			//delete_line(i);
            
            //line_found = true;
            //line = true;
            //assert(1 == 2);
        }
    }
    
    //return line_found;    
	//myfile << "leaving check_for_lines()..." << endl;
}

// Piirtää rivin y paikalle tyhjistä ruuduista koostuvan rivin.
void empty_line(int y)
{
	SDL_Rect rect;
	
	rect.x = 0;
	rect.y = 0;
	rect.w = 15;
	rect.h = 15;
    
	for (int i = 0; i < 10; i++) {
		blit_surface(((SCREEN_WIDTH - field->w) /2) + i * 15,
					((SCREEN_HEIGHT - field->h) /2) + y * 15,
		   			empty_square, screen, &rect);
	}	
	
}

SDL_Surface* get_line_surface(int y)
{
	ofstream debugfile("debug4.txt");
	SDL_Surface *line;
	SDL_Rect rect;
	Uint32 rmask, gmask, bmask, amask;
	
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = 0xff000000;
	
	rect.x = (SCREEN_WIDTH - field->w) / 2;
	rect.y = y * 15 + 15;
	rect.w = 150;
	rect.h = 15;
	
	// tee tyhjä surface.
	line = SDL_CreateRGBSurface(SDL_SWSURFACE, 15*10, 15, 32, rmask, gmask, bmask, amask);
	//line = load_image("lol.png");
	blit_surface(0, 0, screen, line, &rect);
			
	
	if (line == NULL) {
		debugfile << SDL_GetError() << endl
		          << IMG_GetError() << endl;
	}
	
	//SDL_FillRect(line, &foo, SDL_MapRGB(screen->format, 0xff, 0x00, 0x00));
	return line;
}

void delete_lines()
{
	int offset = 0;
	//ofstream myfile("debug3.txt");
	
	// kopioi poistettavien rivien pinnat
	for (int i = 0; i < deleted_lines.size(); i++) {
		line_surfaces.push_back(get_line_surface(deleted_lines[i]));
	}
	
	for (int j = 0; j < 2; j++) {
		
	    // aluksi tyhjennetään rivit.
		for (int i = 0; i < deleted_lines.size(); i++) {
			empty_line(deleted_lines[i]);
		}

		SDL_Flip(screen);
		SDL_Delay(50);
		
		// sitten piirretään rivit uudestaan ruudulle.
		
		for (int i = 0; i < deleted_lines.size(); i++) {
			blit_surface(((SCREEN_WIDTH - field->w) /2) + 0,
						((SCREEN_HEIGHT - field->h) /2) + deleted_lines[i] * 15,
						line_surfaces[i], screen);
		}

		SDL_Flip(screen);
		SDL_Delay(50);
		
	}
	      
	// poista rivit
	for (int i = 0; i < deleted_lines.size(); i++) {
		delete_line(deleted_lines[i] + offset);
		offset++; // BUGFIX
		// delete_line olettaa, että parametri y on päivitetty edellisen
		// rivin poiston jälkeen.
	}
	// siirrä poistettujen rivien yläpuolella olevia palikoita alaspäin.
	//drop_blocks(offset);
				
	// vapauta pintojen osoittimet
	for (int i = 0; i < line_surfaces.size(); i++) {
		SDL_FreeSurface(line_surfaces[i]);
	}

	for (int i = 0; i < highscore_vector.size(); i++)
		delete highscore_vector[i];
		
	// tyhjennetään vektori, jottei synny ikuista silmukkaa.
	deleted_lines.clear();
	// 
	line_surfaces.clear();
}

int new_count = 0;
int delete_count = 0;

void* operator new(size_t sz) {
  new_count++;
  void* m = malloc(sz);
  return m;
}

void operator delete(void* m) {
  delete_count++;
  free(m);
}

void open_highscore_file()
{
	ifstream highscore_file("highscore.dat");
	string::size_type index;
	int i = 0;
	string line;
	
	while (!highscore_file.eof()) {
		stringstream scorestream;
		hs_ptr = new(highscore_entry);
		string score_s;
		
		getline(highscore_file, line);
		index = line.find_first_of('#');
		// kopioi nimi.
		hs_ptr->name = line.substr(0, index);
		// kopioi tulos.
		score_s = line.substr(index+1);
		scorestream << score_s;
		//string debug_s = hs_ptr->name;	
		hs_ptr->score = atoi(scorestream.str().c_str());
		highscore_vector.push_back(hs_ptr);
    }
	// poista viimeinen alkio, koska se on 0.
	// FIXME
	highscore_vector.pop_back();
	highscore_file.close();
}

void check_highscore_list(int score)
{
	int highest_slot = -1; // suurin mahdollinen score, joka on parametriä score pienempi.
	int k = 0;
	char inch;
	string name;
	bool quit = false;
	bool skip = false;
	SDL_Event event;
	SDL_Rect name_rect;
	
	for (int i = 0; i < highscore_vector.size(); i++) {
		if (highscore_vector[i]->score < score) {
			
			highest_slot = i;
			break;
		}
	}
	
	if (highest_slot != -1) {
		write_text("YOU MADE A NEW HIGH SCORE!", (screen->w - 25*10 ) / 2, (screen->h- 10) / 2 - 20);
		write_text("PLEASE ENTER YOUR NAME:", (screen->w - 22*10)/2, (screen->h- 10) / 2 - 10);
		write_text("_", (screen->w - 1*10)/2, (screen->h- 10) / 2);
		SDL_Flip(screen);
		while (quit == false) {
			while(SDL_PollEvent(&event)) {
				if (event.type == SDL_KEYDOWN) {
					if (event.key.keysym.sym == SDLK_RETURN) {
						quit = true;
						break;
					}
					else if (event.key.keysym.sym == SDLK_SPACE) {
						name += " ";
						skip = true;
						k++;
					}
					// FIXME: kun backspacea painetaan, niin
					else if (event.key.keysym.sym == SDLK_BACKSPACE) {
						if (!name.empty())
							name.erase(name.size()-1);
						k--;
						skip = true;
					}
					
					if (!skip) {
						if (k < 10) {
							inch = (char)event.key.keysym.sym;
							k++;
							if (isalpha(inch))
								inch = toupper(inch);
							name += inch;
						}
					}
					// nimen paikka ruudulla täytyy puhdistaa ennen kuin siihen voi kirjoittaa.
					name_rect.x = (screen->w -  (name.length()*10+1)) / 2;
					name_rect.y = (screen->h - 10) / 2;
					name_rect.w = name.length()*10;
					name_rect.h = 10;
					SDL_FillRect(screen, &name_rect, SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00));
					write_text(name+"_", (screen->w - (name.length()*10+1)) / 2, (screen->h - 10) / 2);
					SDL_Flip(screen);
					
					skip = false;
				}
			}
		}
		
		for (int i = highscore_vector.size()-1; i > highest_slot; i--) {
			highscore_vector[i]->score = highscore_vector[i-1]->score;
			highscore_vector[i]->name  = highscore_vector[i-1]->name;
		}
		
		highscore_vector[highest_slot]->name = name;
		highscore_vector[highest_slot]->score = score;
		update_highscore_file = true;
	}
}

void my_getch()
{
	// stupid hack: empty the event queue.
	while (SDL_PollEvent(&event));
	
	bool quit = false;
	while (quit == false) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_KEYDOWN)
				quit = true;
		}
	}
}

void show_highscore_list()
{
	
	write_text("HIGHSCORES", (screen->w - (10*10)) / 2 , ((screen->h - (5*10)) /2) - 20 );
	//open_highscore_file();
	for (int i = 0; i < highscore_vector.size(); i++) {
		stringstream ss;
		string entry;
		ss << highscore_vector[i]->score;
		string score(ss.str());
		//string debug_s = highscore_vector[i]->name;
		entry += highscore_vector[i]->name;
		
		// entryä täytyy muotoilla hieman.
		for (int j = 0; j < 10 - highscore_vector[i]->name.size(); j++)
			entry += " ";
		
		entry += score;
		
		for (int j = 0; j < 5 - score.size(); j++)
			entry += " ";
			
		write_text(entry, (screen->w - (entry.length()*10)) / 2 , ((screen->h - (5*10)) /2) +i*10);
		//write_text(score, (screen->w / 2) + (highscore_vector[i]->name.size()*10)+30 ,(screen->h/2) + 20+i*10);
	}
	SDL_Flip(screen);
	//my_getch();
}

void save_highscores()
{
	ofstream highscore_file("highscore.dat");
	string::size_type index;

	for (int i = 0; i < highscore_vector.size(); i++) 
		highscore_file << highscore_vector[i]->name << "#" << highscore_vector[i]->score << endl;
	
	highscore_file.close();
	update_highscore_file = false;
}

void game_over()
{
			
	//SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0x00, 0x00, 0x00));
    write_text("GAME OVER", (screen->w - 90)/ 2, (screen->h -10)/2);
	
	if (SDL_Flip(screen) == -1)
    {
    //        return 1;    
    }
	//my_getch();
}

int main(int argc, char *argv[])
{
    bool quit = false;
    bool drop_block = false;
    //bool move_block_down = false;
    bool rotate_right = false;
    bool rotate_left  = false;
    Uint32 start = 0;
    Uint32 timer = 0;
    Uint32 move_timer = 0;
    SDL_Rect sprite_rect;
    
    
    int block_type; // Indeksi bc_array-taulukkoon
    int frame;      // Palikan kehyksen järjestysnumero

    int block_num = 0;  // Lisätty debuggaamista varten.
    ofstream myfile;    // debug
    //int debug_counter = 0;
    
    vector<piece*>::iterator pi; // debug, korjaa tämä myöhemmin.
    
    int biggest_y = 0;
    int biggest_y_in_region = 0;

    //myfile.open("jutris_debug.txt"); // debug    
	//myfile << "calling init()" << endl;
    if (!init()) {
        //myfile << "init() failed" << endl;
		return 1;
    }
        
    
    
    sprite_rect.y = 0;
    sprite_rect.w = 15;
    sprite_rect.h = 15;
    
    frame = 0;
    frame_count[I_BLOCK] = 2;
    frame_count[J_BLOCK] = 4; // lisätty 17/3
    frame_count[L_BLOCK] = 4;
    frame_count[O_BLOCK] = 1;
    frame_count[T_BLOCK] = 4;
    frame_count[S_BLOCK] = 2;
    frame_count[Z_BLOCK] = 2;
    
    start = SDL_GetTicks();
    move_timer = SDL_GetTicks();
    wait_delay = 2000;
    
    while (quit == false) {
        timer = SDL_GetTicks(); 
    
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_LEFT)
                    block_v = -1;
                else if (event.key.keysym.sym == SDLK_RIGHT)
                    block_v = 1;
                else if (event.key.keysym.sym == SDLK_DOWN)
                    drop_block = true;
				// jos pelaaja painaa alas-nuolinäppäintä, niin palikka siirtyy yhden ruudun verran alaspäin.
				// move_block_downwards_by_one_square = true;
                else if (event.key.keysym.sym == SDLK_x)
                    rotate_right = true;
                else if (event.key.keysym.sym == SDLK_z)
                    rotate_left = true;
		else if (event.key.keysym.sym == SDLK_ESCAPE)
		    quit = true;    
                
            } else if (event.type == SDL_KEYUP) {
                if (event.key.keysym.sym == SDLK_LEFT)
                    block_v = 0;
                else if (event.key.keysym.sym == SDLK_RIGHT)
                    block_v = 0;
              }
                 
            else if (event.type == SDL_QUIT)
                quit = true;
        }
        
        // Jos uuden palikan kohdalla olevat ruudut ovat jo varattuja, GAME OVER.
        
        if ((block_ptr->y == 1) && !CheckSquares(block_ptr))
            quit = true;
        
		// Palikkaa liikutetaan vasta sitten, kun on kulunut 50 ms.
        if ((SDL_GetTicks() - move_timer) > 50) {
            move_block();
            move_timer = SDL_GetTicks();
        }
        
        // Seuraavan algoritmin pitäisi kääntää palikkaa, mutta se ei toimi kunnolla.
        // En kerta kaikkiaan tajua, että miten algoritmi toimii tai miten edes onnistuin
        // alun perin koodaamaan sen.
        //
        // 06/05 19:16 Vaikuttaa siltä, että sain algoritmin korjattua. Tein sen siten, että
        //             poistin osan edellisestä algoritmista ja vaihdoin algoritmin suorittamien
        //             operaatioiden järjestystä, jotta ymmärtäisin, miten algoritmi toimii.
        //             
        int prev_pos;        
        
        if (rotate_right) { 
            
            prev_pos = block_ptr->position;
            
            if (block_ptr->position+1 > (frame_count[block_ptr->block_type]-1))
                    block_ptr->position = 0;
            else
                block_ptr->position++;
            
            for (int i = 0; i < 4; i++) {
                    block_ptr->pieces[i]->x = block_ptr->x + bp.bc_array[block_ptr->block_type+block_ptr->position].x[i];
                    block_ptr->pieces[i]->y = block_ptr->y + bp.bc_array[block_ptr->block_type+block_ptr->position].y[i];
            }
           
            if (!CheckSquares(block_ptr)) {
                
                block_ptr->position = prev_pos;
                
                for (int i = 0; i < 4; i++) {
                    block_ptr->pieces[i]->x = block_ptr->x + bp.bc_array[block_ptr->block_type+block_ptr->position].x[i];
                    block_ptr->pieces[i]->y = block_ptr->y + bp.bc_array[block_ptr->block_type+block_ptr->position].y[i];
                }
            }
        } else if (rotate_left) {
            
            prev_pos = block_ptr->position;
            
            if (block_ptr->position-1 < 0)
                    block_ptr->position = frame_count[block_ptr->block_type]-1;
            else
                block_ptr->position--;
            
            for (int i = 0; i < 4; i++) {
                    block_ptr->pieces[i]->x = block_ptr->x + bp.bc_array[block_ptr->block_type+block_ptr->position].x[i];
                    block_ptr->pieces[i]->y = block_ptr->y + bp.bc_array[block_ptr->block_type+block_ptr->position].y[i];
            }
           
            if (!CheckSquares(block_ptr)) {
                
                block_ptr->position = prev_pos;
                
                for (int i = 0; i < 4; i++) {
                    block_ptr->pieces[i]->x = block_ptr->x + bp.bc_array[block_ptr->block_type+block_ptr->position].x[i];
                    block_ptr->pieces[i]->y = block_ptr->y + bp.bc_array[block_ptr->block_type+block_ptr->position].y[i];
                }
            }
        } // else
        
        if (drop_block) { 
            biggest_y = find_biggest_y();
            while ((block_ptr->pieces[biggest_y]->y < 19) && (CheckSquares(block_ptr, 1))) {
                for (int i = 0; i < 4; i++)
                    block_ptr->pieces[i]->y++;
                block_ptr->y++;
            }

            AllocateSquares(block_ptr);
            block_ptr->dead = true;
            
            // Uusin palikka täytyy ensin piirtää screen-pinnalle, jotta se näkyisi, kun riviä
			// väläytetään.
			sprite_rect.x = sprite_coords[block_ptr->block_type]*15;
				for (int i = 0; i < block_ptr->pieces.size(); i++) {
					blit_surface(((SCREEN_WIDTH - field->w) /2) + block_ptr->pieces[i]->x * 15,
								((SCREEN_HEIGHT - field->h) /2) + block_ptr->pieces[i]->y * 15,
								block_sprite, screen, &sprite_rect);
				}
            
            check_for_lines();
			
			if (!deleted_lines.empty()) {
				score += (num_lines * 100) * (multiplier > 1 ? multiplier : 1);
				num_lines = 0;
				multiplier = 0;
				
				delete_lines();
                
                if(Mix_PlayChannel(-1, line_delete, 0) == -1) 
                    return 1;
            } else
                if(Mix_PlayChannel(-1, block_drop, 0) == -1)
                    return 1;

            // vaihdetaan palikkaa ja varataan uusi palikka next_block_ptr:lle.
            block_ptr = next_block_ptr;
            blockvector.push_back(block_ptr);
            next_block_ptr = get_new_block();
                        
            drop_block = false;
            
        } /*else if (move_block_down) {
            
            biggest_y = find_biggest_y();
        
            if ((block2_ptr->pieces[biggest_y]->y < 19) && CheckSquares(block2_ptr, 1)) {
                for (int i = 0; i < block2_ptr->pieces.size(); i++)
                    block2_ptr->pieces[i]->y++; 
                block2_ptr->y++;
                //move_block_down = false;  
            }
        }*/
        else {
        // Jos on kulunut yli 'wait_delay' ms, niin siirretään palikkaa alaspäin yhden rivin verran.
        if ((SDL_GetTicks() - start) > wait_delay) {
        
            for (int i = 0; i < 4; i++)
                block_ptr->pieces[i]->y++;
            block_ptr->y++;
            
            biggest_y = find_biggest_y();
            if ((block_ptr->pieces[biggest_y]->y > 19) || !CheckSquares(block_ptr)) {
                
                for (int i = 0; i < 4; i++)
                    block_ptr->pieces[i]->y--;
                block_ptr->y--;
                AllocateSquares(block_ptr);
                block_ptr->dead = true;

                //if(Mix_PlayChannel(-1, block_drop, 0) == -1)
                //    return 1;
                
                //
				sprite_rect.x = sprite_coords[block_ptr->block_type]*15;
				for (int i = 0; i < block_ptr->pieces.size(); i++) {
					blit_surface(((SCREEN_WIDTH - field->w) /2) + block_ptr->pieces[i]->x * 15,
								((SCREEN_HEIGHT - field->h) /2) + block_ptr->pieces[i]->y * 15,
								block_sprite, screen, &sprite_rect);
				}
        
				check_for_lines();
				
				if (!deleted_lines.empty()) {
					score += (num_lines * 100) * (multiplier > 1 ? multiplier : 1);
					num_lines = 0;
					multiplier = 0;
					
					delete_lines();
                
                    if (Mix_PlayChannel(-1, line_delete, 0) == -1)
                        return 1;
                    line = false;
                } else
                    if(Mix_PlayChannel(-1, block_drop, 0) == -1)
                        return 1;
            
                block_ptr = next_block_ptr;
                blockvector.push_back(block_ptr);
                next_block_ptr = get_new_block();
                
            }
            
            start = SDL_GetTicks();
        }
    } // else
    
    if (line_counter > 10) {
        if (wait_delay > 100)
            wait_delay -= 100;
        //wait_delay = 200;
        level++;
        line_counter = 0;
    }
         
        // 2: Piirretään grafiikka ruudulle.
        
        blit_surface(0, 0, background, screen);    
        blit_surface((SCREEN_WIDTH - field->w)/2, 15, field, screen);
                        
        for (bi = blockvector.begin(); bi < blockvector.end(); bi++) {
            sprite_rect.x = sprite_coords[(*bi)->block_type]*15;
            
            for (int i = 0; i < (*bi)->pieces.size(); i++) {
                // bc_array[BLOCK_INDEX + BLOCK_FRAME_NUMBER]
                /*
                blit_surface(((SCREEN_WIDTH - field->w) /2) + (*blockIterator)->x * 15 + (15 * bp.bc_array[(*blockIterator)->block_type+(*blockIterator)->position].x[i]),
                            ((SCREEN_HEIGHT - field->h) /2) + (*blockIterator)->y * 15 + (15 * bp.bc_array[(*blockIterator)->block_type+(*blockIterator)->position].y[i]),
                             block_sprite, screen, &sprite_rect);
                */
                
                // Debug-moodi: testataan, toimiiko blockin dead-parametri ollenkaan.
                // Haluan nähdä, ymmärtääkö ohjelma sen, että jotkut palikat ovat kuolleita.
                //
                // Kuolleet palikat saavat oman spriten, joten check_for_lines()-algoritmin
                // bugi ei johdu ainakaan dead-parametrista.
                //
                // /*(*bi)->dead*/
                /*
                if (squareTable[(*bi)->pieces[i]->x][(*bi)->pieces[i]->y] == 1) {
                    sprite_rect.x = 0;
                    sprite_rect.y = 0;
                    sprite_rect.w = 15;
                    sprite_rect.h = 15;
                    blit_surface(((SCREEN_WIDTH - field->w) /2) + (*bi)->pieces[i]->x * 15,
                                ((SCREEN_HEIGHT - field->h) /2) + (*bi)->pieces[i]->y * 15,
                                 dead_sprite, screen, &sprite_rect);
                
                } else {    */
                
                blit_surface(((SCREEN_WIDTH - field->w) /2) + (*bi)->pieces[i]->x * 15,
                            ((SCREEN_HEIGHT - field->h) /2) + (*bi)->pieces[i]->y * 15,
                             block_sprite, screen, &sprite_rect);
                
                
            }
        } 
        
        // Piirretään seuraava palikka ruudun oikeaan yläkulmaan.
        sprite_rect.x = sprite_coords[next_block_ptr->block_type]*15;
        for (int i = 0; i < next_block_ptr->pieces.size(); i++)
            blit_surface(((SCREEN_WIDTH - field->w) /2) + field->w - 15 + next_block_ptr->pieces[i]->x * 15,
                           15 + next_block_ptr->pieces[i]->y * 15,
                           block_sprite, screen, &sprite_rect);
                
        if (rotate_right || rotate_left) {
            if(Mix_PlayChannel(-1, block_rotate, 0) == -1)
                return 1;
            rotate_right = false;
            rotate_left  = false;
        }
        
		stringstream score_s;
		stringstream level_s;
		
		score_s << score;
		
        //write_text("", 0, 320);
		//write_text("OPQRSTUVWXYZ0123456789", 140, 320);
		write_text("TETRIS", 10, 20);
        write_text("NEXT", 280, 95);
		write_text("SCORE", 280, 130);
		write_text(score_s.str().c_str(), 280, 150);
	write_text("LEVEL", 280, 170);
	level_s << level;
	write_text(level_s.str().c_str(), 280, 190);
        
        if ((SDL_GetTicks() - timer) < (1000 / FRAMES_PER_SECOND))
            SDL_Delay((1000 / FRAMES_PER_SECOND) - (SDL_GetTicks() - timer));
    
        if (SDL_Flip(screen) == -1)
        {
            return 1;    
        }

    }
    
	SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0x00, 0x00, 0x00));
	// tulosta "game over"-ruutu
	game_over();
	my_getch();
		
	open_highscore_file();
	
	// tarkista highscorelista.
	
	// jotta "game over"-teksti ei jäisi leijumaan ruudulle.
	SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0x00, 0x00, 0x00));
	check_highscore_list(score);
	
	// näytä highscoret.
	SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0x00, 0x00, 0x00));
	show_highscore_list();
	my_getch();
	
	if (update_highscore_file)
		save_highscores();

	clean_up();
	
	//myfile << "new_count = " << new_count << endl;
	//myfile << "delete_count = " << delete_count << endl;
    return 0;
}
