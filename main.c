#include <glib/gi18n.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
//#define _test_

#ifdef _test_
    #define FONTFILE "arial.ttf"
    #define MUSICFILEMENU "music/menu.mp3"
    #define MUSICFILEPLACING "music/placing.mp3"
    #define MUSICFILEGAME "music/battle00.mp3"
    #define MUSICFILEEXPLOSION "music/Explosion.wav"
    #define MUSICFILESPLASH "music/Splash.wav"
    #define DATADIR ""
#else
#ifdef __linux__
    #define FONTFILE "/usr/share/BATTLESHIPS/arial.ttf"
    #define MUSICFILEMENU "/usr/share/BATTLESHIPS/music/menu.mp3"
    #define MUSICFILEPLACING "/usr/share/BATTLESHIPS/music/placing.mp3"
    #define MUSICFILEGAME "/usr/share/BATTLESHIPS/music/battle00.mp3"
    #define MUSICFILEEXPLOSION "/usr/share/BATTLESHIPS/music/Explosion.wav"
    #define MUSICFILESPLASH "/usr/share/BATTLESHIPS/music/Splash.wav"
    #define DATADIR "/usr/share/BATTLESHIPS/locale"
#else
    #define FONTFILE "res\\arial.ttf"
    #define MUSICFILEMENU "res\\music\\menu.mp3"
    #define MUSICFILEPLACING "res\\music\\placing.mp3"
    #define MUSICFILEGAME "res\\music\\battle00.mp3"
    #define MUSICFILEEXPLOSION "res\\music\\Explosion.wav"
    #define MUSICFILESPLASH "res\\music\\Splash.wav"
    #define DATADIR ""
#endif // __linux__
#endif // _test_



// rozmiary okna, komórki i czcionki
int window_width, window_height, cell_size, font_def_size, font_height;
// dane statków gracza i bota
#define GRID_SIZE 10
int player_board[GRID_SIZE][GRID_SIZE];
int enemy_board[GRID_SIZE][GRID_SIZE];
int enemy_hits[GRID_SIZE][GRID_SIZE];
int player_hits[GRID_SIZE][GRID_SIZE];

bool is_running = true;
bool player_turn = true; // Tura gracza
int player_count = 0, enemy_count  = 0, wait = 0;// licznik ruchów gracza, wroga i oczekiwania
unsigned game_result = 0; // 0 trwa gra, 1 wygrana gracza, 2 wygrana bota

// zmienne potrzebne przy umieszczaniu statków
#define TOTAL_SHIPS 10
const int ships_to_place[TOTAL_SHIPS] = {4, 3, 3, 2, 2, 2, 1, 1, 1, 1};// długości statków
int current_ship_index = 0;// numer umieszczanego statku
int direction = 0; // 0 - poziomo, 1 - pionowo
// zmienne potrzebne przy wyświetlaniu komunikatów
int message_1 = 0, message_2 = 0;

SDL_Color red_color = {255, 0, 0, 255}; // Czerwony kolor dla przegranej
SDL_Color green_color = {0, 255, 0, 255}; // Czerwony kolor dla przegranej
SDL_Color blue_color = {0, 0, 255, 255}; // Czerwony kolor dla przegranej
SDL_Color white_color = {255, 255, 255, 255}; // Czerwony kolor dla przegranej
// Koordynaty myszy dla podglądu statku
int mouse_x = 0, mouse_y = 0;

typedef enum {
    MENU,// START GAME, OPTIONS, STATS, QUIT
    PLACING_SHIPS,
    GAME,
    STATS,// BACK
    OPT_MUSIC,// ON, OFF, BACK
    OPT_DIFFICULTY,// EASY, HARD, BACK
    EXIT
} GameState;

GameState game_state = MENU;

typedef struct {
    unsigned games_played;
    unsigned wins;
    unsigned bNoMusic;// 1 - music off, 0 - on
    unsigned bHard;// 1 - hard difficulity, 0 - easy
} OPTIONS;

OPTIONS stats = {0, 0, 0, 0};
// uchwyt pliku z błędami działania
char *pathopt,*pathlog;
FILE* flog = NULL;
// okno i renderer SDL
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
TTF_Font *font = NULL;
// Muzyka
Mix_Music *menu_music, *placing_music;
Mix_Chunk *explosion_sound, *splash_sound;
#define NUM_TRACKS 16
Mix_Music *battle_music[NUM_TRACKS];
// Aktualny stan muzyki
int current_menu_track = 0; // Do przełączania utworów menu
int current_battle_track = 0;
// zmienne tekstowe
char *szMes[8];
// wpisy w menu
char *szMenuM30, *szMenuM31, *szMenuM32, *szMenuM33, *szMenuM34, *szMenuM00, *szMenuM01, *szMenuM02, *szMenuM03;
char *szMenuM04, *szMenuM05, *szMenuM10, *szMenuM11A, *szMenuM11B, *szMenuM12, *szMenuM20, *szMenuM21A, *szMenuM21B, *szMenuM22;

void init_chars(){
    setlocale( LC_ALL, "" );
    bindtextdomain( "BATTLESHIPS", DATADIR );
    bind_textdomain_codeset( "BATTLESHIPS", "UTF-8" );
    textdomain( "BATTLESHIPS" );

    szMes[0] = _("Place ships (RMB - rotate)");
    szMes[1] = _("Shoot");
    szMes[2] = _("Hit");
    szMes[3] = _("Hit and sunk");
    szMes[4] = _("Miss");
    szMes[5] = _("Victory");
    szMes[6] = _("Defeat");
    szMes[7] = _("Click to go to main menu");
    szMenuM30 = _("Statistics");
    szMenuM31 = _("Games played:");
    szMenuM32 = _("Wins:");
    szMenuM33 = _("Reset");
    szMenuM34 = _("Back");
    szMenuM00 = _("Menu");
    szMenuM01 = _("Start game");
    szMenuM02 = _("Statistics");
    szMenuM03 = _("Music");
    szMenuM04 = _("Difficulty");
    szMenuM05 = _("Exit");
    szMenuM10 = _("Music");
    szMenuM11A = _("On");
    szMenuM11B = _("Off");
    szMenuM12 = _("Back");
    szMenuM20 = _("Difficulty");
    szMenuM21A = _("Easy");
    szMenuM21B = _("Hard");
    szMenuM22 = _("Back");
}

// funkcja zapisujące błędy działania programu
void writelog(const char* a, const char* b) {
    if(flog == NULL) {
        flog = fopen(pathlog,"w");
    }
    if(flog) {
        if(b) fprintf(flog,"%s: %s\n",a,b);
        else fprintf(flog,"%s\n",a);
    }
}

// Funkcja inicjalizująca SDL
bool init_sdl() {
    gchar *home = g_build_filename( g_get_home_dir(), ".BATTLESHIPS", NULL );
    g_mkdir_with_parents(home,0777);
    pathopt = g_build_filename( home, "options.dat", NULL );
    pathlog = g_build_filename( home, "log.txt", NULL );
    g_free( home );
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        writelog("SDL initialization error",SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow("BATTLESHIPS", 0, 0, 640, 480, SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);
    if (!window) {
        writelog("Error creating window", SDL_GetError());
        return false;
    }
    SDL_MaximizeWindow(window);
    SDL_GetWindowSize(window, &window_width, &window_height);
    SDL_SetWindowSize(window, window_width, window_height);
    SDL_SetWindowResizable(window, SDL_FALSE);
    SDL_GetWindowSize(window, &window_width, &window_height);
    font_def_size = window_height/20;
    font_height = 6*font_def_size/5;
    int cs1 = window_width / (2*GRID_SIZE+3);
    int cs2 = (window_height - 3*font_def_size/2) / (GRID_SIZE + 2);
    cell_size = (cs1 < cs2) ? cs1 : cs2;

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        writelog("Error creating renderer", SDL_GetError());
        return false;
    }
    //SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_ADD);

    if (TTF_Init() == -1) {
        writelog("Error initializing SDL_ttf", TTF_GetError());
        return false;
    }

    font = TTF_OpenFont(FONTFILE, font_def_size); // Podaj ścieżkę do czcionki
    if (!font) {
        writelog("Error loading font", TTF_GetError());
        return false;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        writelog("Error initializing SDL_mixer", Mix_GetError());
        return false;
    }

    return true;
}

// Funkcja odtwarzająca następny utwór bitewny
void play_battle_music() {
    if (Mix_PlayingMusic()) Mix_HaltMusic();// Zatrzymaj aktualną muzykę
    if(!stats.bNoMusic) {
        Mix_Music *current_music = battle_music[current_battle_track];
        if (current_music) {
            Mix_PlayMusic(current_music, -1);// Odtwarzanie aktualnego utworu w nieskończoność
            current_battle_track = (current_battle_track + 1) % NUM_TRACKS;// Przejście do następnego utworu
        }
    }
}

// Funkcja odtwarzająca muzykę w menu
void play_menu_music() {
    if (Mix_PlayingMusic()) Mix_HaltMusic();// Zatrzymaj aktualną muzykę
    if(!stats.bNoMusic) Mix_PlayMusic(menu_music, -1);
}

// Funkcja odtwarzająca muzykę przy dodawaniu statków
void play_placing_music() {
    if (Mix_PlayingMusic()) Mix_HaltMusic();// Zatrzymaj aktualną muzykę
    if(!stats.bNoMusic) Mix_PlayMusic(placing_music, -1);
}

// Funkcja ładująca pliki audio
void load_audio() {
    // Ładowanie muzyki do menu
    menu_music = Mix_LoadMUS(MUSICFILEMENU);
    if (!menu_music) {
        writelog("Error loading menu music", Mix_GetError());
    }
    placing_music = Mix_LoadMUS(MUSICFILEPLACING);
    if (!placing_music) {
        writelog("Error loading placing music", Mix_GetError());
    }
    explosion_sound = Mix_LoadWAV(MUSICFILEEXPLOSION);
    if (!explosion_sound) {
        writelog("Error loading menu music", Mix_GetError());
    }
    splash_sound = Mix_LoadWAV(MUSICFILESPLASH);
    if (!splash_sound) {
        writelog("Error loading placing music", Mix_GetError());
    }
    // Ładowanie muzyki do bitwy
    char path[256];
    strcpy(path,MUSICFILEGAME);
    size_t len = strlen(path);
    for(int i = 0; i < NUM_TRACKS; i++) {
        path[len-6] = '0'+(i/10);
        path[len-5] = '0'+(i%10);// we change digit in filename
        battle_music[i] = Mix_LoadMUS(path);
        if (!battle_music[i]) {
            writelog("Error loading battle music", Mix_GetError());
        }
    }
    for(int i = NUM_TRACKS-1; i > 0; i--) {
        Mix_Music *temp = battle_music[i];
        int tr = rand() % (i+1);
        battle_music[i] = battle_music[tr];
        battle_music[tr] = temp;
    }
    play_menu_music(); // Odtwarzaj muzykę menu
}

// Funkcja czyszcząca SDL
void cleanup_sdl() {
    if (menu_music) Mix_FreeMusic(menu_music);
    for(int i=0; i<4; i++)
        if (battle_music[i]) Mix_FreeMusic(battle_music[i]);
    Mix_CloseAudio();
    if(font) TTF_CloseFont(font);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    if(flog) fclose(flog);
    g_free( pathopt );
    g_free( pathlog );
    TTF_Quit();
    SDL_Quit();
}

// Funkcja inicjalizujaca plansze
void init_board(int board[GRID_SIZE][GRID_SIZE]) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) board[i][j] = 0;
    }
}

// Funkcja do inicjalizacji tablicy trafien
void init_hits(int hits[GRID_SIZE][GRID_SIZE]) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) hits[i][j] = 0;
    }
}

// Funkcja do sprawdzenia poprawnosci umieszczenia statku z zachowaniem 1 komorki dystansu
bool can_place_ship_with_distance(int board[GRID_SIZE][GRID_SIZE], int row, int col, int length, int direction) {
    int r = row + (direction == 1 ? length : 1);
    int c = col + (direction == 0 ? length : 1);
    if(r > GRID_SIZE || c > GRID_SIZE) return false;
    for (int i = -1; i <= length; i++) {
        for (int j = -1; j <= 1; j++) {
            r = row + (direction == 1 ? i : j);
            c = col + (direction == 0 ? i : j);
            if (r >= 0 && r < GRID_SIZE && c >= 0 && c < GRID_SIZE && board[r][c] == 1) return false;
        }
    }
    return true;
}

// Funkcja do umieszczenia statku
void place_ship(int board[GRID_SIZE][GRID_SIZE], int row, int col, int length, int direction) {
    for (int i = 0; i < length; i++) {
        int r = row + direction * i;
        int c = col + (1 - direction) * i;
        board[r][c] = 1;
    }
}

// Funkcja do ustawiania statkow przez bota
void place_ships_randomly(int board[GRID_SIZE][GRID_SIZE]) {
    for (int i = 0; i < TOTAL_SHIPS; i++) {
        bool placed = false;
        while (!placed) {
            int row = rand() % GRID_SIZE;
            int col = rand() % GRID_SIZE;
            int dir = rand() % 2;

            if (can_place_ship_with_distance(board, row, col, ships_to_place[i], dir)) {
                place_ship(board, row, col, ships_to_place[i], dir);
                placed = true;
            }
        }
    }
}

// Funkcja do rysowania planszy
void draw_grid(int x_offset, int board[GRID_SIZE][GRID_SIZE], int hits[GRID_SIZE][GRID_SIZE]) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    for (int i = 0; i <= GRID_SIZE; i++) {
        SDL_RenderDrawLine(renderer, x_offset, (i+1) * cell_size, x_offset + GRID_SIZE * cell_size, (i+1) * cell_size);
        SDL_RenderDrawLine(renderer, x_offset + i * cell_size, cell_size, x_offset + i * cell_size, (GRID_SIZE + 1) * cell_size);
    }

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (hits != NULL && hits[i][j]) {
                if (board[i][j] == 1) {// Trafienie - czerwony kwadrat
                    SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
                    SDL_Rect cell = {x_offset + j * cell_size + 1, (i+1) * cell_size + 1, cell_size - 2, cell_size - 2};
                    SDL_RenderFillRect(renderer, &cell);
                } else {// Pudlo - rysowanie krzyzyka
                    if(hits[i][j] == 1) SDL_SetRenderDrawColor(renderer, 255, 0, 255, SDL_ALPHA_OPAQUE);
                    else SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
                    SDL_RenderDrawLine(renderer, x_offset + j * cell_size, (i+1) * cell_size,
                                       x_offset + (j + 1) * cell_size, (i+2) * cell_size);
                    SDL_RenderDrawLine(renderer, x_offset + (j+1) * cell_size, (i+1) * cell_size,
                                       x_offset + j * cell_size, (i+2) * cell_size);
                }
            } else if ((board[i][j] == 1) && ((x_offset == cell_size) || game_result)) {// Statek gracza
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, SDL_ALPHA_OPAQUE);
                SDL_Rect cell = {x_offset + j * cell_size + 1, (i+1) * cell_size + 1, cell_size - 2, cell_size - 2};
                SDL_RenderFillRect(renderer, &cell);
            }
        }
    }
}

// Funkcja do ustawiania statkow przez gracza
void handle_ship_placement(int x, int y) {
    int col = x / cell_size - 1;
    int row = y / cell_size - 1;
    if ((col >= 0) && (col < GRID_SIZE) && (row >= 0) && (row < GRID_SIZE) &&
        can_place_ship_with_distance(player_board, row, col, ships_to_place[current_ship_index], direction)) {
        place_ship(player_board, row, col, ships_to_place[current_ship_index], direction);
        current_ship_index++;
        if (current_ship_index >= TOTAL_SHIPS){
            game_state = GAME;
            play_battle_music();// Odtwarzanie muzyki bitewnej
            message_2 = 1;
        }
    }
}

// Funkcja do sprawdzenia czy wszystkie statki zostaly zniszczone
bool all_ships_destroyed(int board[GRID_SIZE][GRID_SIZE], int hits[GRID_SIZE][GRID_SIZE]) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (board[i][j] == 1 && hits[i][j] == 0) return false;
        }
    }
    return true;
}

// sprawdza czy statek nie jest już zatopiony
// orientacja (o ile nie NULL) 0 - niewiadomo, 1 - pionowa, 2 - pozioma
bool check_if_sunk(int board[GRID_SIZE][GRID_SIZE], int hits[GRID_SIZE][GRID_SIZE], int row, int col, int *orientation) {
    int il = row -1,ir = row +1,ju = col -1,jd = col +1;
    if(orientation) *orientation = 0;
    for(; il >= 0; il--) {
        if(board[il][col] == 0) break;
        if(orientation) *orientation = 1;
        if(hits[il][col] == 0) return false;
    }
    for(; ir < GRID_SIZE; ir++) {
        if(board[ir][col] == 0) break;
        if(orientation) *orientation = 1;
        if(hits[ir][col] == 0) return false;
    }
    for(; ju >= 0; ju--) {
        if(board[row][ju] == 0) break;
        if(orientation) *orientation = 2;
        if(hits[row][ju] == 0) return false;
    }
    for(; jd < GRID_SIZE; jd++) {
        if(board[row][jd] == 0) break;
        if(orientation) *orientation = 2;
        if(hits[row][jd] == 0) return false;
    }
    if(il < 0) il = 0;
    if(ir >= GRID_SIZE) ir = GRID_SIZE - 1;
    if(ju < 0) ju = 0;
    if(jd >= GRID_SIZE) jd = GRID_SIZE - 1;
    for(int i = il; i <= ir; i++)// oznaczamy pola w które już nie trzeba celować
        for(int j = ju; j <= jd; j++)
            if(hits[i][j] == 0) hits[i][j] = 2;
    return true;
}
// Obsluga klikniec na plansze przeciwnika
void handle_attack_click(int x, int y) {
    int col = x / cell_size - GRID_SIZE - 2;
    int row = y / cell_size - 1;
    // Sprawdź, czy wybrana komórka znajduje się w polu
    if ((col >= 0) && (col < GRID_SIZE) && (row >= 0) && (row < GRID_SIZE) && !enemy_hits[row][col]) {
        player_count++;
        enemy_hits[row][col] = 1;
        if (enemy_board[row][col] == 1) {
            if(check_if_sunk(enemy_board,enemy_hits,row,col,NULL)) message_2 = 3;
            else message_2 = 2;
            if(!stats.bNoMusic) Mix_PlayChannel(0,explosion_sound,0);
        } else {
            message_2 = 4;
            message_1 = 0;
            player_turn = false; // Przejście do bota
            if(!stats.bNoMusic) Mix_PlayChannel(0,splash_sound,0);
        }
    }
}

// Ruch bota
void bot_turn() {
    int row, col;
    enemy_count++;
    if (stats.bHard == 0) {// tryb łatwy
        while (1) {
            row = rand() % GRID_SIZE;
            col = rand() % GRID_SIZE;
            if (player_hits[row][col] == 0) break;
        }
        player_hits[row][col] = true;
        if (player_board[row][col] == 0) {
            message_1 = 4;
            player_turn = true;
        } else {
            if(check_if_sunk(player_board,player_hits,row,col,NULL)) message_1 = 3;
            else message_1 = 2;
        }
        return;
    }
    // tryb trudny
    // Zmienne statyczne do przechowywania listy potencjalnych ruchów i łańcucha celów
    static int candidate_moves[4][2];
    static int candidate_count = 0;
    static int last_orientation = 0;

    if (candidate_count > 0) {// Jeśli istnieją ruchy kandydujące, wybierz jeden z nich
        int idx = rand() % candidate_count;
        row = candidate_moves[idx][0];
        col = candidate_moves[idx][1];
        for(idx++ ;idx < candidate_count; idx++) {// chcemy zachować kolejność orientacji najpierw pion potem poziom
            candidate_moves[idx-1][0] = candidate_moves[idx][0];
            candidate_moves[idx-1][1] = candidate_moves[idx][1];
        }
        candidate_count--;
    } else {// Jeśli nie ma potencjalnych ruchów, wybierz losową nieostrzelaną komórkę
        while (1) {
            row = rand() % GRID_SIZE;
            col = rand() % GRID_SIZE;
            if (player_hits[row][col] == 0) break;
        }
    }

    player_hits[row][col] = 1;
    if (player_board[row][col] == 1) {// trafiony
        int orientation;
        if(check_if_sunk(player_board,player_hits,row,col,&orientation)) {// trafiony zatopiony
            message_1 = 3;
            candidate_count = 0;// nie ma ruchów kandydujących
            last_orientation = 0;
        } else {
            message_1 = 2;
            // po pierwszym trafieniu w okręt orientation = 0 = last_orientation
            // po drugim trafieniu orientation != 0 = last_orientation
            // przy sprawdzaniu 3 i 4 tego trafienia orientation = last_orientation != 0
            if(orientation != 2) {
                if(last_orientation != orientation) {// usuwamy nieprawidłowe ruchy kandydujące (poziome) przy drugim trafieniu
                    for(int i = candidate_count -1; i >= 0; i--) {
                        if(candidate_moves[i][0] == row) candidate_count--;
                        else break;
                    }
                }
                if(row > 0 && !player_hits[row-1][col]) {// w przypadku orientacji != 0 zostanie dodany co najwyżej jeden
                    candidate_moves[candidate_count][0] = row - 1;
                    candidate_moves[candidate_count][1] = col;
                    candidate_count++;
                }
                if(row < GRID_SIZE -1 && !player_hits[row+1][col]) {
                    candidate_moves[candidate_count][0] = row + 1;
                    candidate_moves[candidate_count][1] = col;
                    candidate_count++;
                }
            }
            if(orientation != 1) {
                if(last_orientation != orientation) {// usuwamy nieprawidłowe ruchy kandydujące (pionowe) przy drugim trafieniu
                    int i = 0;
                    for(;i < candidate_count; i++) {
                        if(candidate_moves[i][1] == row) break;
                    }
                    candidate_count -= i;
                    for(int j = 0 ; j < candidate_count ;j++) {
                        candidate_moves[j][0] = candidate_moves[i+j][0];
                        candidate_moves[j][1] = candidate_moves[i+j][1];
                    }
                }
                if(col > 0 && !player_hits[row][col-1]) {// w przypadku orientacji != 0 zostanie dodany co najwyżej jeden
                    candidate_moves[candidate_count][0] = row;
                    candidate_moves[candidate_count][1] = col - 1;
                    candidate_count++;
                }
                if(col < GRID_SIZE -1 && !player_hits[row][col+1]) {
                    candidate_moves[candidate_count][0] = row;
                    candidate_moves[candidate_count][1] = col + 1;
                    candidate_count++;
                }
            }
            last_orientation = orientation;
        }
    } else {// Jeśli strzał nie trafił w statek
        message_1 = 4;
        player_turn = true;
    }
}

// Funkcja zapisywania statystyk do pliku
void save_stats() {
    FILE *file = fopen(pathopt, "wb");
    if (file) {
        fwrite(&stats,sizeof(stats),1,file);
        fclose(file);
    }
}

// Funkcja ładowania statystyk z pliku
void load_stats() {
    FILE *file = fopen(pathopt, "rb");
    if (file) {
        fread(&stats,sizeof(stats),1,file);
        fclose(file);
    }
}

// Resetowanie stanu gry
void reset_game() {
    // Ponowne inicjalizowanie plansz
    init_board(player_board);
    init_board(enemy_board);
    // Ponowne inicjalizowanie trafień
    init_hits(player_hits);
    init_hits(enemy_hits);
    // Resetowanie zmiennych związanych z rozmieszczaniem statków
    game_result = 0;
    current_ship_index = 0;
    direction = 0;
    mouse_x = 0;
    mouse_y = 0;
    // Losowe rozmieszczenie statków przeciwnika
    place_ships_randomly(enemy_board);
    // Resetowanie tury
    player_turn = true;
    wait = 0;
    player_count = 0;
    enemy_count = 0;
    message_2 = 1;
}

// Funkcja rysująca podgląd statku
void draw_preview_ship(int x, int y, int length, int direction) {
    int col = x / cell_size - 1;
    int row = y / cell_size - 1;
    if(col >= 0 && col < GRID_SIZE && row >= 0 && row < GRID_SIZE) {
        if(can_place_ship_with_distance(player_board, row, col, ships_to_place[current_ship_index], direction))
            SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // niebieski kolor
        else
            SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255); // różowy kolor

        for (int i = 0; i < length; i++) {
            int r = row + (direction * i);
            int c = col + i - (direction * i);

            if (r >= 0 && r < GRID_SIZE && c >= 0 && c < GRID_SIZE) {
                SDL_Rect cell = {(c+1)* cell_size + 1, (r+1) * cell_size + 1, cell_size - 2, cell_size - 2};
                SDL_RenderFillRect(renderer, &cell);
            }
        }
    }
}

// Funkcja renderująca tekst
void render_text(const char *text, int x, int y, SDL_Color color) {
    SDL_Surface *text_surface = TTF_RenderUTF8_Solid(font, text, color);
    if (!text_surface) {
        writelog("Error creating text surface", TTF_GetError());
        return;
    }
    SDL_Texture *text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
    SDL_Rect dest_rect = {x, y, text_surface->w, text_surface->h};

    SDL_FreeSurface(text_surface);
    SDL_RenderCopy(renderer, text_texture, NULL, &dest_rect);
    SDL_DestroyTexture(text_texture);
}

void show_menu(char **list, int n, int first_with_backgroun){
    int w,h;
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);// czarne tło
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, SDL_ALPHA_OPAQUE);// niebieskie tło tekstu
    for(int i = 0; i < n; i++){
        if(i >= first_with_backgroun) {// początkowe wpisy mogą nie mieć tła dla czcionki
            SDL_Rect cell = {cell_size, cell_size + 2*i*font_def_size, window_width - 2*cell_size, font_height};
            SDL_RenderFillRect(renderer, &cell);
        }
        TTF_SizeUTF8(font,list[i],&w,&h);
        render_text(list[i], (window_width - w) / 2, cell_size + 2*i*font_def_size, white_color);
    }
    SDL_RenderPresent(renderer);
}

// Funkcja obsługi zdarzeń
void handle_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) is_running = false;
        // Obsługa zdarzeń w zależności od aktualnego stanu gry
        switch (game_state) {
            case MENU:
                if (event.type == SDL_MOUSEBUTTONDOWN) {
                    mouse_x = event.button.x;
                    mouse_y = event.button.y;
                    if(mouse_x >= cell_size && mouse_x < window_width - cell_size && mouse_y >= cell_size) {
                        if((mouse_y - cell_size)%(2*font_def_size) < font_height) {
                            int sel_menu = (mouse_y - cell_size)/(2*font_def_size);
                            switch(sel_menu) {
                                case 1: // Rozpocznij grę
                                    reset_game();// Resetowanie planszy i stanu gry
                                    play_placing_music();// Odtwarzanie muzyki bitewnej
                                    game_state = PLACING_SHIPS;
                                    break;
                                case 2: // Wyświetl statystyki
                                    game_state = STATS;
                                    break;
                                case 3: // Ustawienia
                                    game_state = OPT_MUSIC;
                                    break;
                                case 4: // Ustawienia
                                    game_state = OPT_DIFFICULTY;
                                    break;
                                case 5: // Wyjście
                                    game_state = EXIT;
                                    is_running = false;
                                    break;
                            }
                        }
                    }
                }
                break;
            case OPT_DIFFICULTY:
                 if (event.type == SDL_MOUSEBUTTONDOWN) {
                    mouse_x = event.button.x;
                    mouse_y = event.button.y;
                    if(mouse_x >= cell_size && mouse_x < window_width - cell_size && mouse_y >= cell_size) {
                        if((mouse_y - cell_size)%(2*font_def_size) < font_height) {
                            int sel_menu = (mouse_y - cell_size)/(2*font_def_size);
                            switch(sel_menu) {
                                case 1: // Łatwy/trudny poziom
                                    stats.bHard = 1 - stats.bHard;
                                    break;
                                case 2:
                                    game_state = MENU;
                                    break;
                            }
                        }
                    }
                }
                break;
            case PLACING_SHIPS:
                if (event.type == SDL_MOUSEBUTTONDOWN) {
                    mouse_x = event.button.x;
                    mouse_y = event.button.y;
                    if(event.button.button==SDL_BUTTON_LEFT) {
                        handle_ship_placement(mouse_x, mouse_y); // Obsługa rozmieszczania statków
                    } else if(event.button.button==SDL_BUTTON_RIGHT) {
                        direction = (direction + 1) % 2; // Obracanie statku
                    }
                } else if (event.type == SDL_MOUSEMOTION) {
                    mouse_x = event.motion.x;
                    mouse_y = event.motion.y;
                }
                break;
            case GAME:
                if (event.type == SDL_MOUSEBUTTONDOWN) {
                    mouse_x = event.button.x;
                    mouse_y = event.button.y;
                    if(event.button.button==SDL_BUTTON_LEFT) {
                        if(game_result && mouse_x >= cell_size && mouse_x < (GRID_SIZE+1)*cell_size && mouse_y >= cell_size*(GRID_SIZE+1) + font_def_size) {
                            play_menu_music();
                            game_state = MENU;
                        } else if ((game_result == 0) && player_turn && wait == 0) {
                            handle_attack_click(mouse_x, mouse_y); // Atakowanie planszy przeciwnika
                        }
                    }
                }
                break;
            case STATS:
                if (event.type == SDL_MOUSEBUTTONDOWN) {
                    mouse_x = event.button.x;
                    mouse_y = event.button.y;
                    if(mouse_x >= cell_size && mouse_x < window_width - cell_size && mouse_y >= cell_size) {
                        if((mouse_y - cell_size)%(2*font_def_size) < font_height) {
                            int sel_menu = (mouse_y - cell_size)/(2*font_def_size);
                            switch(sel_menu) {
                                case 3: // Resetowanie statystyk
                                    stats.games_played = 0;
                                    stats.wins = 0;
                                    break;
                                case 4: // Powrót do menu
                                    game_state = MENU;
                                    break;
                            }
                        }
                    }
                }
                break;
            case OPT_MUSIC:
                if (event.type == SDL_MOUSEBUTTONDOWN) {
                    mouse_x = event.button.x;
                    mouse_y = event.button.y;
                    if(mouse_x >= cell_size && mouse_x < window_width - cell_size && mouse_y >= cell_size) {
                        if((mouse_y - cell_size)%(2*font_def_size) < font_height) {
                            int sel_menu = (mouse_y - cell_size)/(2*font_def_size);
                            switch(sel_menu) {
                                case 1: // Włączanie/wyłączanie muzyki
                                    if (stats.bNoMusic) {
                                        stats.bNoMusic = 0;
                                        play_menu_music();
                                    } else {
                                        Mix_HaltMusic();
                                        stats.bNoMusic = 1;
                                    }
                                    break;
                                case 2: // Powrót do menu
                                    game_state = MENU;
                                    break;
                            }
                        }
                    }
                }
                break;
        }
    }
}

// Główna funkcja
int main(int argc, char * argv[]) {
    init_chars();
    srand(time(NULL)); // Zainicjalizuj generator liczb losowych
    // Inicjalizacja SDL i powiązanych bibliotek
    if (init_sdl()) {
        load_stats(); // Wczytaj statystyki z pliku
        load_audio();       // Wczytaj dźwięki i muzykę
        // Główna pętla gry
        while (is_running) {
            handle_events();// Przetwórz dane wejściowe i zdarzenia
            switch (game_state) {// Renderowanie w zależności od aktualnego stanu gry
                case MENU:
                    {
                    char* szMenuMain[6] = {szMenuM00, szMenuM01, szMenuM02, szMenuM03, szMenuM04, szMenuM05};
                    show_menu(szMenuMain,6,1);
                    }
                    break;
                case PLACING_SHIPS:
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
                    SDL_RenderClear(renderer);
                    draw_grid(cell_size, player_board, NULL); // Rysowanie planszy gracza bez trafień
                    // Rysowanie podglądu umieszczanego statku
                    draw_preview_ship(mouse_x, mouse_y, ships_to_place[current_ship_index], direction);
                    // wypisanie tekstu
                    render_text(szMes[0], cell_size, cell_size*(GRID_SIZE+1)+font_def_size, blue_color);
                    SDL_RenderPresent(renderer); // Aktualizacja ekranu
                    break;
                case GAME:
                    if(game_result == 0) {
                        if (all_ships_destroyed(enemy_board, enemy_hits)) {
                            game_result = 1;
                            stats.wins++;
                            stats.games_played++;
                        }
                        if (all_ships_destroyed(player_board, player_hits)) {
                            game_result = 2;
                            stats.games_played++;
                        }
                    }
                    // Renderowanie plansz gry
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);// Czarny tło
                    SDL_RenderClear(renderer);
                    draw_grid(cell_size, player_board, player_hits);// Rysowanie planszy gracza
                    draw_grid(cell_size*(GRID_SIZE + 2), enemy_board, enemy_hits);// Rysowanie planszy przeciwnika
                    if(game_result) {
                        player_turn = true;
                        message_2 = 4 + game_result;
                        message_1 = 7;
                        int h = player_count, w = enemy_count;
                        char text1[4] = "  0", text2[4] = "  0";
                        for(int i = 2; i >= 0; i--) {
                            if(w) text1[i] = (w % 10) + 48;
                            w /= 10;
                        }
                        for(int i = 2; i >= 0; i--) {
                            if(h) text2[i] = (h % 10) + 48;
                            h /= 10;
                        }
                        TTF_SizeUTF8(font,text2,&w,&h);
                        render_text(text2, cell_size*(2*GRID_SIZE+2)-w, cell_size*(GRID_SIZE+1)+font_def_size, white_color);

                        SDL_SetRenderDrawColor(renderer, 0, 255, 0, SDL_ALPHA_OPAQUE);
                        SDL_Rect cell = {cell_size, cell_size*(GRID_SIZE+1) + font_def_size, cell_size*GRID_SIZE, font_height};
                        SDL_RenderFillRect(renderer, &cell);
                        TTF_SizeUTF8(font,text1,&w,&h);
                        render_text(text1, cell_size*(GRID_SIZE+1)-w, cell_size*(GRID_SIZE+1)+font_def_size, white_color);
                    }
                    if(message_1) {
                        render_text(szMes[message_1], cell_size, cell_size*(GRID_SIZE+1)+font_def_size, blue_color);
                    }
                    if(message_2) {
                        render_text(szMes[message_2], cell_size*(GRID_SIZE+2), cell_size*(GRID_SIZE+1)+font_def_size, green_color);
                    }
                    SDL_RenderPresent(renderer);// Aktualizacja ekranu
                    if (!player_turn) { // Jeśli nie jest tura gracza, wykonuje ruch przeciwnik
                        if(wait == 0) {
                            bot_turn();
                            wait = 90;
                        }
                    } else {
                        if(wait == 1) {
                            message_2 = 1;// Strzelaj
                            message_1 = 0;
                        }
                    }
                    if(wait) wait--;
                    break;
                case STATS:
                    {
                        char buffer1[64], buffer2[64];
                        sprintf(buffer1, "%s %d", szMenuM31, stats.games_played);
                        sprintf(buffer2, "%s %d", szMenuM32, stats.wins);
                        char* szMenuStatistics[5] = {szMenuM30, buffer1, buffer2, szMenuM33, szMenuM34};
                        show_menu(szMenuStatistics,5,3);
                    }
                    break;
                case OPT_MUSIC:
                    {
                    char* szMenuMusic[3] = {szMenuM10, szMenuM11A, szMenuM12};
                    if(stats.bNoMusic) szMenuMusic[1] = szMenuM11B;
                    else szMenuMusic[1] = szMenuM11A;
                    show_menu(szMenuMusic,3,1);
                    }
                    break;
                case OPT_DIFFICULTY:
                    {
                    char* szMenuDifficulty[3] = {szMenuM20, szMenuM21A, szMenuM22};
                    if(stats.bHard) szMenuDifficulty[1] = szMenuM21B;
                    else szMenuDifficulty[1] = szMenuM21A;
                    show_menu(szMenuDifficulty,3,1);
                    }
                    break;
            }
            SDL_Delay(16);// ograniczamy fps do max 60
        }
        save_stats(&stats);// Zapisz zaktualizowane statystyki
    }
    cleanup_sdl();// Czyszczenie zasobów przed wyjściem
    return 0;
}
