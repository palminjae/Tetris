#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>

// 플랫폼별 헤더 파일 포함
#ifdef _WIN32
    #include <windows.h>
    #include <conio.h>
    
    // MinGW에서 누락되었다고 해서 상수 직접 정의함
    #ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
    #define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
    #endif
    
    #define SLEEP_MS(ms) Sleep(ms)
    
    // 윈도우에서 유독 깜빡임이 심해서 고쳐보기
    void clear_Windows_screen(void) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        DWORD count;
        DWORD cellCount;
        COORD homeCoords = { 0, 0 };
        
        if (hConsole == INVALID_HANDLE_VALUE) return;
        
        // 현재 화면 정보 가져오기
        if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) return;
        cellCount = csbi.dwSize.X * csbi.dwSize.Y;
        
        // 화면을 공백으로 채우기
        if (!FillConsoleOutputCharacter(hConsole, (TCHAR)' ', cellCount, homeCoords, &count)) return;
        
        // 속성 초기화
        if (!FillConsoleOutputAttribute(hConsole, csbi.wAttributes, cellCount, homeCoords, &count)) return;
        
        // 커서를 (0,0)으로 이동
        SetConsoleCursorPosition(hConsole, homeCoords);
    }
    #define CLEAR_SCREEN() clear_Windows_screen()
    
    // 커서 숨기기
    void hide_cursor(void) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(hConsole, &cursorInfo);
        cursorInfo.bVisible = FALSE;
        SetConsoleCursorInfo(hConsole, &cursorInfo);
    }
    
    // 커서 보이게 하기
    void show_cursor(void) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(hConsole, &cursorInfo);
        cursorInfo.bVisible = TRUE;
        SetConsoleCursorInfo(hConsole, &cursorInfo);
    }
    
    // 커서 특정 위치로 옮기기
    void goto_xy(int x, int y) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        COORD coord;
        coord.X = x;
        coord.Y = y;
        SetConsoleCursorPosition(hConsole, coord);
    }
    
    // 계속 깜빡여서 화면 갱신
    void update_game_screen(void) {
        // 커서만 홈으로 이동 (화면을 지우지 않음)
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        COORD homeCoords = {0, 0};
        SetConsoleCursorPosition(hOut, homeCoords);
    }
    
    // 화면 버퍼링을 위한 함수
    void setup_console_buffer(void) {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
        
        // 버퍼 크기 설정
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(hOut, &csbi);
        COORD bufferSize = {80, 50};  // 적당한 크기로 설정
        SetConsoleScreenBufferSize(hOut, bufferSize);
    }
    
#else
    #include <sys/time.h>
    #include <termios.h>
    #include <unistd.h>
    #include <sys/ioctl.h>
    #include <sys/types.h>
    #define SLEEP_MS(ms) usleep((ms) * 1000)
    #define CLEAR_SCREEN() printf("\033[2J\033[H")
    
    // Unix,Linux용 함수
    void hide_cursor(void) { printf("\033[?25l"); fflush(stdout); }
    void show_cursor(void) { printf("\033[?25h"); fflush(stdout); }
    void goto_xy(int x, int y) { printf("\033[%d;%dH", y+1, x+1); fflush(stdout); }
    
    // macOS/Linux에서 화면 클리어 -> 이건 윈도우랑은 또 다름 -> 다행히 깜빡임은 없음
    void update_game_screen(void) { 
        printf("\033[H");
        fflush(stdout); 
    }
    
    void setup_console_buffer(void) { 
        printf("\033[2J\033[H");
        fflush(stdout);
    }
#endif

/* 게임 상수 정의 */
#define LEFT 0
#define RIGHT 1
#define DOWN 2
#define ROTATE 3

#define I_BLOCK 0
#define T_BLOCK 1
#define S_BLOCK 2
#define Z_BLOCK 3
#define L_BLOCK 4
#define J_BLOCK 5
#define O_BLOCK 6

#define GAME_START 0
#define GAME_END 1

/* 블록 정의 */
char i_block[4][4][4] = {
    {
        {1, 1, 1, 1}, 
        {0, 0, 0, 0}, 
        {0, 0, 0, 0}, 
        {0, 0, 0, 0}
    },
    {  
        {0, 0, 0, 1}, 
        {0, 0, 0, 1}, 
        {0, 0, 0, 1}, 
        {0, 0, 0, 1} 
    },
    {
        {0, 0, 0, 0}, 
        {0, 0, 0, 0}, 
        {0, 0, 0, 0}, 
        {1, 1, 1, 1} 
    },
    {
        {1, 0, 0, 0}, 
        {1, 0, 0, 0}, 
        {1, 0, 0, 0}, 
        {1, 0, 0, 0} 
    }
};

char t_block[4][4][4] = {
    {
        {1, 0, 0, 0},   
        {1, 1, 0, 0},   
        {1, 0, 0, 0},   
        {0, 0, 0, 0}
    },
    {
        {1, 1, 1, 0},   
        {0, 1, 0, 0},   
        {0, 0, 0, 0},   
        {0, 0, 0, 0}
    },
    {
        {0, 0, 1, 0},   
        {0, 1, 1, 0},   
        {0, 0, 1, 0},   
        {0, 0, 0, 0}
    },
    {
        {0, 0, 0, 0},   
        {0, 1, 0, 0},   
        {1, 1, 1, 0},   
        {0, 0, 0, 0}
    }
};

char s_block[4][4][4] = {
    {
        {1, 0, 0, 0},   
        {1, 1, 0, 0},   
        {0, 1, 0, 0},   
        {0, 0, 0, 0}
    },
    {
        {0, 1, 1, 0},   
        {1, 1, 0, 0},   
        {0, 0, 0, 0},   
        {0, 0, 0, 0}
    },
    {
        {1, 0, 0, 0},   
        {1, 1, 0, 0},   
        {0, 1, 0, 0},   
        {0, 0, 0, 0}
    },
    {
        {0, 1, 1, 0},   
        {1, 1, 0, 0},   
        {0, 0, 0, 0},   
        {0, 0, 0, 0}
    }
};

char z_block[4][4][4] = {
    {
        {0, 1, 0, 0},   
        {1, 1, 0, 0},   
        {1, 0, 0, 0},   
        {0, 0, 0, 0}
    },
    {
        {1, 1, 0, 0},   
        {0, 1, 1, 0},   
        {0, 0, 0, 0},   
        {0, 0, 0, 0}
    },
    {
        {0, 1, 0, 0},   
        {1, 1, 0, 0},   
        {1, 0, 0, 0},   
        {0, 0, 0, 0}
    },
    {
        {1, 1, 0, 0},   
        {0, 1, 1, 0},   
        {0, 0, 0, 0},   
        {0, 0, 0, 0}
    }
};

char l_block[4][4][4] = {
    {
        {1, 0, 0, 0},   
        {1, 0, 0, 0},   
        {1, 1, 0, 0},   
        {0, 0, 0, 0}
    },
    {
        {1, 1, 1, 0},   
        {1, 0, 0, 0},   
        {0, 0, 0, 0},   
        {0, 0, 0, 0}
    },
    {
        {0, 1, 1, 0},   
        {0, 0, 1, 0},   
        {0, 0, 1, 0},   
        {0, 0, 0, 0}
    },
    {
        {0, 0, 0, 0},   
        {0, 0, 1, 0},   
        {1, 1, 1, 0},   
        {0, 0, 0, 0}
    }
};

char j_block[4][4][4] = {
    {
        {0, 1, 0, 0},  
        {0, 1, 0, 0},  
        {1, 1, 0, 0},   
        {0, 0, 0, 0}
    },
    {
        {1, 0, 0, 0},   
        {1, 1, 1, 0},   
        {0, 0, 0, 0},   
        {0, 0, 0, 0}
    },
    {
        {0, 1, 1, 0},   
        {0, 1, 0, 0},   
        {0, 1, 0, 0},   
        {0, 0, 0, 0}
    },
    {
        {0, 0, 0, 0},   
        {1, 1, 1, 0},   //회전이 이상하게 되어있음 
        {0, 0, 1, 0},   
        {0, 0, 0, 0}
    }
};

char o_block[4][4][4] = {
    {
        {1, 1, 0, 0},   
        {1, 1, 0, 0},   
        {0, 0, 0, 0},   
        {0, 0, 0, 0}
    },
    {
        {1, 1, 0, 0},   
        {1, 1, 0, 0},   
        {0, 0, 0, 0},   
        {0, 0, 0, 0}
    },
    {
        {1, 1, 0, 0},   
        {1, 1, 0, 0},   // 네모가 갑자기 자식을 낳음
        {0, 0, 0, 0},   
        {0, 0, 0, 0}
    },
    {
        {1, 1, 0, 0},   
        {1, 1, 0, 0},   
        {0, 0, 0, 0},   
        {0, 0, 0, 0}
    }
};

/* 전역 변수 */
char tetris_table[21][10];

struct result {
    char name[30];
    long point;
    int year;
    int month;
    int day;
    int hour;
    int min;
    int rank;
} temp_result;

int block_number = 0;
int next_block_number = 0;
int block_state = 0;
int x = 3, y = 0;
int game = GAME_END;
int best_point = 0;
long point = 0;
int ghost_y = 0;

// 플랫폼별 키보드 입력 처리 
#ifdef _WIN32
// Windows용 getch 구현 
int getch_nonb(void) {
    if (_kbhit()) {
        return _getch();
    }
    return EOF;
}

void init_keyboard(void) {
    // Windows에서는 특별한 초기화 필요없음 
}

void reset_keyboard(void) {
    //Windows에서는 복구 필요없음
}

void flush_input_buffer(void) {
    while (_kbhit()) {
        _getch();
    }
}

void Player_name(char* name, int max_len) {
    printf("\n\t\t\tEnter your name: ");
    fflush(stdout);
    
    int i = 0;
    int ch;
    
    while (_kbhit()) {
        _getch();
    }
    
    while (i < max_len - 1) {
        ch = _getch();
        
        if (ch == 13) {
            break;
        }
        else if (ch == 8 && i > 0) {
            printf("\b \b");
            fflush(stdout);
            i--;
        }
        else if (ch >= 32 && ch <= 126) {
            printf("%c", ch);
            fflush(stdout);
            name[i++] = ch;
        }
        else if (ch == 224) {
            _getch();
        }
    }
    
    name[i] = '\0';
    printf("\n");
    
    if (strlen(name) == 0) {
        strcpy(name, "Anonymous");
        printf("\t\t\tUsing 'Anonymous' as default name.\n");
    }
}

#else
// Unix, Linux, macOS용 getch 구현
static struct termios old_tty, new_tty;

void init_keyboard(void) {
    tcgetattr(STDIN_FILENO, &old_tty);
    new_tty = old_tty;
    new_tty.c_lflag &= ~(ICANON | ECHO);
    new_tty.c_cc[VMIN] = 0;
    new_tty.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tty);
}

void reset_keyboard(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tty);
}

int getch_nonb(void) {
    char ch;
    ssize_t bytesRead = read(STDIN_FILENO, &ch, 1);
    if (bytesRead == 1) {
        return (int)ch;
    }
    return EOF;
}

void flush_input_buffer(void) {
    tcflush(STDIN_FILENO, TCIFLUSH);
}

/* 개선된 이름 입력 함수 - Unix/Linux/macOS */
void Player_name(char* name, int max_len) {

     // 키보드를 표준 모드로 완전히 복원
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tty);
    // macOS 전용 강력한 버퍼 정리
    #ifdef __APPLE__
        tcflush(STDIN_FILENO, TCIFLUSH);
        usleep(100000);  // 0.1초 대기
    #endif
    
    // 입력 버퍼 비우기
    int c;
    while ((c = getchar()) != '\n' && c != EOF);

    printf("\n\t\t\tEnter your name: ");
    fflush(stdout);

    if (fgets(name, max_len, stdin) != NULL) {
        // 개행 문자 제거
        name[strcspn(name, "\n")] = 0;
    }
    

    
    // 빈 이름 처리 - 재입력 기회 제공
    if (strlen(name) == 0) {
        printf("\t\t\tName cannot be empty! Please enter again: ");
        if (fgets(name, max_len, stdin) != NULL) {
            name[strcspn(name, "\n")] = 0;
        }
        
        if (strlen(name) == 0) {
            strcpy(name, "Anonymous");
            printf("\t\t\tUsing 'Anonymous' as default name.\n");
        }
    }
}
#endif

/* 함수 프로토타입 선언 */
int print_menu(void);
int init_tetris_table(void);
int print_tetris_sc(void);
int game_start(void);
int refresh(int);
int move_block(int);
int drop(void);
int collision_test(int);
int check_one_line(void);
int print_result(void);
int search_result(void);
void calculate_ghost_position(void);
void refresh_with_ghost(int);

/* 고스트 블록 위치 계산 */
void calculate_ghost_position(void) {
    int original_y = y;
    
    ghost_y = y;
    while (1) {
        y = ghost_y + 1;
        
        if (collision_test(DOWN) == 1) {
            ghost_y = y - 1;
            break;
        }
        ghost_y++;
    }
    
    y = original_y;
}

/* 고스트 블록을 포함한 화면 새로고침 */
void refresh_with_ghost(int block) {
    int i, j;
    int block_array_x, block_array_y;
    char (*block_pointer)[4][4] = NULL;
    
    /* 먼저 움직이는 블록 지우기 */
    for(i = 0; i < 20; i++) {
        for(j = 1; j < 9; j++) {
            if(tetris_table[i][j] == 2 || tetris_table[i][j] == 3) {
                tetris_table[i][j] = 0;
            }
        }
    }
    
    switch(block) {
        case I_BLOCK: block_pointer = i_block; break;
        case T_BLOCK: block_pointer = t_block; break;
        case S_BLOCK: block_pointer = s_block; break;
        case Z_BLOCK: block_pointer = z_block; break;
        case L_BLOCK: block_pointer = l_block; break;
        case J_BLOCK: block_pointer = j_block; break;
        case O_BLOCK: block_pointer = o_block; break;
        default: return;
    }
    
    /* 고스트 블록 위치 계산 */
    calculate_ghost_position();
    
    /* 고스트 블록 그리기 */
    if (ghost_y != y) {  /* 현재 블록과 다른 위치에만 고스트 표시 */
        for(i = 0; i < 4; i++) {
            for(j = 0; j < 4; j++) {
                block_array_x = j + x;
                block_array_y = i + ghost_y;
                
                if(block_pointer[block_state][i][j] == 1) {
                    if (block_array_y >= 0 && block_array_y < 20 && 
                        block_array_x > 0 && block_array_x < 9) {
                        tetris_table[block_array_y][block_array_x] = 3;
                    }
                }
            }
        }
    }
    
    /* 현재 블록 그리기 */
    for(i = 0; i < 4; i++) {
        for(j = 0; j < 4; j++) {
            block_array_x = j + x;
            block_array_y = i + y;
            
            if(block_pointer[block_state][i][j] == 1) {
                if (block_array_y >= 0 && block_array_y < 20 && 
                    block_array_x > 0 && block_array_x < 9) {
                    tetris_table[block_array_y][block_array_x] = 2;
                }
            }
        }
    }
}

int print_menu(void) {
    int menu = 0;
    while(1) {
        CLEAR_SCREEN();
        printf("\n\n\t\t\t\tText Tetris");
        printf("\n\t\t\t============================");
        printf("\n\t\t\t\tGAME MENU\t\n");
        printf("\n\t\t\t============================");
        printf("\n\t\t\t   1) Game Start");
        printf("\n\t\t\t   2) Search history");
        printf("\n\t\t\t   3) Record Output");
        printf("\n\t\t\t   4) QUIT");
        printf("\n\t\t\t============================");
        printf("\n\t\t\t\t\t SELECT : ");
        
        flush_input_buffer();
        if (scanf("%d", &menu) != 1) {
            flush_input_buffer();
            continue;
        }
        flush_input_buffer();

        if(menu >= 1 && menu <= 4) {
            return menu;
        }
    }
    return 0;
}

int init_tetris_table(void) {
    int i, j;
    for(i = 0; i < 20; i++)
        for(j = 0; j < 10; j++)
            tetris_table[i][j] = 0;
    for(i = 0; i < 21; i++) {
        tetris_table[i][0] = 1;
        tetris_table[i][9] = 1;
    }
    for(j = 1; j < 9; j++)
        tetris_table[20][j] = 1;
    return 0;
}

int print_tetris_sc(void) {
    int i, j;
    char (*block_pointer)[4][4] = NULL;
    
    switch(next_block_number) {
        case I_BLOCK: block_pointer = i_block; break;
        case T_BLOCK: block_pointer = t_block; break;
        case S_BLOCK: block_pointer = s_block; break;
        case Z_BLOCK: block_pointer = z_block; break;
        case L_BLOCK: block_pointer = l_block; break;
        case J_BLOCK: block_pointer = j_block; break;
        case O_BLOCK: block_pointer = o_block; break;
        default: return 1;
    }
    
    update_game_screen();

    printf("<< TETRIS >>\n\n");
    printf("Next Block\n");
    
    for(i = 0; i < 4; i++) {
        printf("    ");
        for(j = 0; j < 4; j++) {
            if(block_pointer[0][i][j] == 1)
#ifdef _WIN32
                printf("[]");
#else
                printf("🟥");
#endif
            else
                printf("  ");
        }
        printf("\n");
    }
    
    printf("\n");
    for(i = 0; i < 21; i++){
        printf("    ");
        for(j = 0; j < 10; j++){
            if(j == 0 || j == 9 || i == 20)
#ifdef _WIN32
                printf("||");
#else
                printf("⬜");
#endif
            else{
                if(tetris_table[i][j] == 1)
#ifdef _WIN32
                    printf("[]");
#else
                    printf("🟩");
#endif
                else if(tetris_table[i][j] == 2)
#ifdef _WIN32
                    printf("##");
#else
                    printf("🟥");
#endif
                else if(tetris_table[i][j] == 3)
#ifdef _WIN32
                    printf("--");
#else
                    printf("⬛");
#endif
                else
                    printf("  ");
            }
        }
        printf("\n");
    }
    
    printf("\nCurrent Score: %ld\n", point);
    printf("Best Score: %d\n", best_point);
    printf("\nControls: J(left) L(right) K(down) I(rotate) A(drop) P(quit)\n");
    printf("Ghost block shows where your piece will land\n");
    
    for(i = 0; i < 3; i++) {
        printf("                                                                                \n");
    }
    
    return 0;
}

int game_start(void) {
    int key;
    long frame_count = 0;
    int drop_interval = 30;
    
    init_tetris_table();
    srand(time(NULL));
    
    block_number = rand() % 7;
    next_block_number = rand() % 7;
    
    game = GAME_START;
    point = 0;
    x = 3;
    y = 0;
    block_state = 0;
    
    init_keyboard();
    setup_console_buffer();
    CLEAR_SCREEN();
    hide_cursor();
    
    refresh_with_ghost(block_number);
    print_tetris_sc();
    
    while(game == GAME_START) {
        key = getch_nonb();

        if(key != EOF) {
            switch(key) {
                case 'j':
                case 'J':
                    move_block(LEFT);
                    break;
                case 'l':
                case 'L':
                    move_block(RIGHT);
                    break;
                case 'k':
                case 'K':
                    move_block(DOWN);
                    break;
                case 'i':
                case 'I':
                    move_block(ROTATE);
                    break;
                case 'a':
                case 'A':
                    drop();
                    break;
                case 'p':
                case 'P':
                    game = GAME_END;
                    break;
                default:
                    break;
            }
        }

        frame_count++;
        if (frame_count % drop_interval == 0) {
            move_block(DOWN);
        }

        refresh_with_ghost(block_number);
        print_tetris_sc();

        SLEEP_MS(33);  // 프레임 레이트를 30fps로 개선 (33ms)
    }
    
    show_cursor();
    reset_keyboard();

    CLEAR_SCREEN();
    printf("\n\n\t\t\tGAME OVER!\n");
    printf("\n\t\t\tYour Score: %ld\n", point);
    
    if(point > best_point) {
        best_point = point;
        printf("\n\t\t\tNEW BEST SCORE!\n");
    }
    
    printf("\n\t\t\tPress Enter to save score...\n");
    
#ifdef _WIN32
    _getch();
#else
    // macOS/Linux에서 키보드 모드를 표준으로 복원한 후 엔터 대기
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tty);
    getchar();
#endif
    
    temp_result.point = point;
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    temp_result.year = tm->tm_year + 1900;
    temp_result.month = tm->tm_mon + 1;
    temp_result.day = tm->tm_mday;
    temp_result.hour = tm->tm_hour;
    temp_result.min = tm->tm_min;
    
    Player_name(temp_result.name, sizeof(temp_result.name));
    
    FILE *fp = fopen("tetris_result.dat", "ab");
    if(fp != NULL) {
        fwrite(&temp_result, sizeof(struct result), 1, fp);
        fclose(fp);
        printf("\n\t\t\tScore saved successfully!\n");
    } else {
        printf("\n\t\t\tFailed to save score!\n");
    }
    
    SLEEP_MS(2000);
    flush_input_buffer();
    
    return 1;
}

int refresh(int block) {
    int i, j;
    int block_array_x, block_array_y;
    char (*block_pointer)[4][4] = NULL;
    
    for(i = 0; i < 20; i++) {
        for(j = 1; j < 9; j++) {
            if(tetris_table[i][j] == 2)
                tetris_table[i][j] = 0;
        }
    }
    
    switch(block) {
        case I_BLOCK: block_pointer = i_block; break;
        case T_BLOCK: block_pointer = t_block; break;
        case S_BLOCK: block_pointer = s_block; break;
        case Z_BLOCK: block_pointer = z_block; break;
        case L_BLOCK: block_pointer = l_block; break;
        case J_BLOCK: block_pointer = j_block; break;
        case O_BLOCK: block_pointer = o_block; break;
        default: return 1;
    }
    
    for(i = 0; i < 4; i++) {
        for(j = 0; j < 4; j++) {
            block_array_x = j + x;
            block_array_y = i + y;
            
            if(block_pointer[block_state][i][j] == 1) {
                if (block_array_y >= 0 && block_array_y < 20 && 
                    block_array_x > 0 && block_array_x < 9) {
                    tetris_table[block_array_y][block_array_x] = 2;
                }
            }
        }
    }
    
    return 0;
}

int move_block(int command) {
    int old_x = x;
    int old_y = y;
    int old_block_state = block_state;
    
    switch(command) {
        case LEFT:
            x--;
            break;
        case RIGHT:
            x++;
            break;
        case DOWN:
            y++;
            break;
        case ROTATE:
            block_state = (block_state + 1) % 4;
            break;
    }
    
    if(collision_test(command) == 1) {
        x = old_x;
        y = old_y;
        block_state = old_block_state;
        
        if(command == DOWN) {
            int i, j;
            char (*block_pointer)[4][4] = NULL;
            
            switch(block_number) {
                case I_BLOCK: block_pointer = i_block; break;
                case T_BLOCK: block_pointer = t_block; break;
                case S_BLOCK: block_pointer = s_block; break;
                case Z_BLOCK: block_pointer = z_block; break;
                case L_BLOCK: block_pointer = l_block; break;
                case J_BLOCK: block_pointer = j_block; break;
                case O_BLOCK: block_pointer = o_block; break;
                default: return 1;
            }
            
            for(i = 0; i < 4; i++) {
                for(j = 0; j < 4; j++) {
                    if(block_pointer[old_block_state][i][j] == 1) {
                        if(i + old_y >= 0 && i + old_y < 20 && 
                           j + old_x > 0 && j + old_x < 9)
                            tetris_table[i + old_y][j + old_x] = 1;
                    }
                }
            }
            
            check_one_line();
            
            block_number = next_block_number;
            next_block_number = rand() % 7;
            block_state = 0;
            x = 3;
            y = 0;
            
            if(collision_test(DOWN) == 1) {
                game = GAME_END;
            }
        }
        
        return 1;
    }
    
    return 0;
}

int drop(void) {
    while(collision_test(DOWN) == 0) {
        y++;
    }
    
    y--;
    move_block(DOWN);
    
    return 0;
}

int collision_test(int command) {
    (void)command;

    int i, j;
    int test_x = x;
    int test_y = y;
    int test_block_state = block_state;
    char (*block_pointer)[4][4] = NULL;
    
    switch(block_number) {
        case I_BLOCK: block_pointer = i_block; break;
        case T_BLOCK: block_pointer = t_block; break;
        case S_BLOCK: block_pointer = s_block; break;
        case Z_BLOCK: block_pointer = z_block; break;
        case L_BLOCK: block_pointer = l_block; break;
        case J_BLOCK: block_pointer = j_block; break;
        case O_BLOCK: block_pointer = o_block; break;
        default: return 1;
    }
    
    for(i = 0; i < 4; i++) {
        for(j = 0; j < 4; j++) {
            if(block_pointer[test_block_state][i][j] == 1) {
                int block_array_x = j + test_x;
                int block_array_y = i + test_y;
                
                if(block_array_x < 1 || block_array_x > 8) return 1;
                if(block_array_y > 19) return 1;
                
                if (block_array_y >= 0 && tetris_table[block_array_y][block_array_x] == 1) {
                    return 1;
                }
            }
        }
    }
    
    return 0;
}

int check_one_line(void) {
    int i, j, k;
    int line_count = 0;
    
    for(i = 19; i >= 0; i--) {
        int line_flag = 1;
        
        for(j = 1; j < 9; j++) {
            if(tetris_table[i][j] == 0) {
                line_flag = 0;
                break;
            }
        }
        
        if(line_flag) {
            for(k = i; k > 0; k--) {
                for(j = 1; j < 9; j++) {
                    tetris_table[k][j] = tetris_table[k-1][j];
                }
            }
            
            for(j = 1; j < 9; j++) {
                tetris_table[0][j] = 0;
            }
            
            line_count++;
            i++;
        }
    }
    
    if(line_count == 1)
        point += 100;
    else if(line_count == 2)
        point += 300;
    else if(line_count == 3)
        point += 600;
    else if(line_count == 4)
        point += 1000;
    
    return line_count;
}

int print_result(void) {
    FILE *fp;
    struct result *result_pointer;
    int count = 0;
    int i, j;
    
    fp = fopen("tetris_result.dat", "rb");
    if(fp == NULL) {
        printf("\n\t\t\tNo records found!\n");
        printf("\n\t\t\tPress any key to continue...\n");
        flush_input_buffer();
#ifdef _WIN32
        _getch();
#else
        while(getch_nonb() == EOF) {
            SLEEP_MS(10);
        }
#endif
        return 1;
    }
    
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    count = (int)(file_size / sizeof(struct result));
    
    if(count == 0) {
        printf("\n\t\t\tNo records found!\n");
        printf("\n\t\t\tPress any key to continue...\n");
        flush_input_buffer();
#ifdef _WIN32
        _getch();
#else
        while(getch_nonb() == EOF) {
            SLEEP_MS(10);
        }
#endif
        fclose(fp);
        return 1;
    }
    
    result_pointer = (struct result*)malloc(sizeof(struct result) * count);
    if(result_pointer == NULL) {
        printf("\n\t\t\tMemory allocation failed!\n");
        fclose(fp);
        return 1;
    }
    
    fread(result_pointer, sizeof(struct result), count, fp);
    fclose(fp);
    
    struct result temp;
    for(i = 0; i < count - 1; i++) {
        for(j = 0; j < count - i - 1; j++) {
            if(result_pointer[j].point < result_pointer[j+1].point) {
                temp = result_pointer[j];
                result_pointer[j] = result_pointer[j+1];
                result_pointer[j+1] = temp;
            }
        }
    }
    
    for(i = 0; i < count; i++) {
        result_pointer[i].rank = i + 1;
    }
    
    CLEAR_SCREEN();
    printf("\n\t\t\t\tTETRIS RANKING\n");
    printf("\t\t\t================================\n");
    printf("\t\tRank\tName\t\tScore\t\tDate\n");
    printf("\t\t\t================================\n");
    
    for(i = 0; i < count && i < 10; i++) {
        printf("\t\t%d\t%-10s\t%ld\t\t%d-%02d-%02d %02d:%02d\n",
            result_pointer[i].rank,
            result_pointer[i].name,
            result_pointer[i].point,
            result_pointer[i].year,
            result_pointer[i].month,
            result_pointer[i].day,
            result_pointer[i].hour,
            result_pointer[i].min);
    }
    
    printf("\t\t\t================================\n");
    printf("\n\t\t\tPress any key to continue...\n");
    flush_input_buffer();
#ifdef _WIN32
    _getch();
#else
    while(getch_nonb() == EOF) {
        SLEEP_MS(10);
    }
#endif
    
    free(result_pointer);
    return 1;
}

int search_result(void) {
    FILE *fp;
    struct result *result_pointer;
    char search_name[30];
    int count = 0;
    int i;
    int found = 0;
    
    CLEAR_SCREEN();
    printf("\n\t\t\tSearch Player Records\n");
    printf("\t\t\t====================\n");
    
#ifdef _WIN32
    printf("\n\t\t\tEnter name to search: ");
    fflush(stdout);
    
    int i_input = 0;
    int ch;
    const int max_input = 29;
    
    while (i_input < max_input) {
        ch = _getch();
        
        if (ch == 13) {
            break;
        }
        else if (ch == 8 && i_input > 0) {
            printf("\b \b");
            fflush(stdout);
            i_input--;
        }
        else if (ch >= 32 && ch <= 126) {
            printf("%c", ch);
            fflush(stdout);
            search_name[i_input++] = ch;
        }
        else if (ch == 224) {
            _getch();
        }
    }
    search_name[i_input] = '\0';
    printf("\n");
#else
    // Unix/Linux/macOS용 검색 이름 입력 - 개선된 버전
    struct termios temp_tty;
    tcgetattr(STDIN_FILENO, &temp_tty);
    temp_tty.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &temp_tty);
    
    // macOS 전용 강력한 버퍼 정리
    #ifdef __APPLE__
        tcflush(STDIN_FILENO, TCIFLUSH);
        usleep(100000);  // 0.1초 대기
    #endif
    
    printf("\n\t\t\tEnter name to search: ");
    fflush(stdout);
    
    // 입력 버퍼 비우기
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
    
    if (fgets(search_name, sizeof(search_name), stdin) != NULL) {
        search_name[strcspn(search_name, "\n")] = 0;
    }
    
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tty);
#endif
    
    fp = fopen("tetris_result.dat", "rb");
    if(fp == NULL) {
        printf("\n\t\t\tNo records found!\n");
        printf("\n\t\t\tPress any key to continue...\n");
        flush_input_buffer();
#ifdef _WIN32
        _getch();
#else
        while(getch_nonb() == EOF) {
            SLEEP_MS(10);
        }
#endif
        return 1;
    }
    
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    count = (int)(file_size / sizeof(struct result));
    
    if(count == 0) {
        printf("\n\t\t\tNo records found!\n");
        printf("\n\t\t\tPress any key to continue...\n");
        flush_input_buffer();
#ifdef _WIN32
        _getch();
#else
        while(getch_nonb() == EOF) {
            SLEEP_MS(10);
        }
#endif
        fclose(fp);
        return 1;
    }
    
    result_pointer = (struct result*)malloc(sizeof(struct result) * count);
    if(result_pointer == NULL) {
        printf("\n\t\t\tMemory allocation failed!\n");
        fclose(fp);
        return 1;
    }
    
    fread(result_pointer, sizeof(struct result), count, fp);
    fclose(fp);
    
    printf("\n\t\t\tSearch Results for: %s\n", search_name);
    printf("\t\t\t================================\n");
    
    for(i = 0; i < count; i++) {
        if(strcmp(result_pointer[i].name, search_name) == 0) {
            printf("\t\tName: %s\n", result_pointer[i].name);
            printf("\t\tScore: %ld\n", result_pointer[i].point);
            printf("\t\tDate: %d-%02d-%02d %02d:%02d\n",
                result_pointer[i].year,
                result_pointer[i].month,
                result_pointer[i].day,
                result_pointer[i].hour,
                result_pointer[i].min);
            printf("\t\t--------------------------------\n");
            found = 1;
        }
    }
    
    if(!found) {
        printf("\t\tNo records found for '%s'\n", search_name);
    }
    printf("\n\t\t\tPress any key to continue...\n");
    flush_input_buffer();
#ifdef _WIN32
    _getch();
#else
    while(getch_nonb() == EOF) {
        SLEEP_MS(10);
    }
#endif
    
    free(result_pointer);
    return 1;
}

int main(void) {
    int menu = 1;
    
    // 플랫폼별 초기 설정
#ifdef _WIN32
    // Windows 콘솔 UTF-8 설정
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    printf("Starting Cross-Platform Tetris...\n");
    printf("Platform: Windows\n");
    
    // Windows 콘솔 가상 터미널 처리 활성화
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
#elif defined(__APPLE__)
    printf("Starting Cross-Platform Tetris...\n");
    printf("Platform: macOS\n");
#elif defined(__linux__)
    printf("Starting Cross-Platform Tetris...\n");
    printf("Platform: Linux\n");
#else
    printf("Starting Cross-Platform Tetris...\n");
    printf("Platform: Unix\n");
#endif
    
    // 플랫폼 정보 표기 Windows, Linux, mac Os
    SLEEP_MS(1000);
    
    // 메인 게임 루프
    while(menu) {
        menu = print_menu();

        switch(menu) {
            case 1:
                game = GAME_START;
                game_start();
                break;
            case 2:
                search_result();
                break;
            case 3:
                print_result();
                break;
            case 4:
                printf("\n\t\t\tThank you for playing!\n");
                SLEEP_MS(1000);
                exit(0);
                break;
            default:
                // 유효하지 않은 메뉴 선택 - 루프 계속
                break;
        }
    }
    
    return 0;
}