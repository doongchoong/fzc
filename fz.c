#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "fz.h"



/*
    퍼지점수 구하기
    Smith-Waterman 알고리즘 변형
    +------------+----------+
    |T(r-1, c-1) |          |
    +------------+----------+
    |T(r  , c-1) | T(r, c)  |
    +------------+----------+

    T(r, c) = MAX(
            T(r-1, c-1) + match_score + bonus, 
            T(r, c-1)   + penalty 
        )
    
    직전 T 값은 최대값만 선택되어 왔으므로 항상 최대 값을 보장함
*/

/* 퍼지점수를 위해 설정된 보너스 값 */
static int g_score_match       = 16;
static int g_bonus_boundary    = 8;
static int g_bonus_no_alnum    = 8;
static int g_bonus_camel       = 5;
static int g_bonus_continuous  = 4;
static int g_penalty_ingap     = -1;
static int g_penalty_firstgap  = -3;


int get_fuzzy_score(char* pat, char* txt, int* fscore, int position[])
{
    static int bonus [ MAX_PATH_LEN + 1];
    static int matrix[ (MAX_PATH_LEN+1) * (MAX_PATTERN+1)];
    static int cont  [ (MAX_PATH_LEN+1) * (MAX_PATTERN+1)];
    int rowsize = strlen(pat) + 1;
    int colsize = strlen(txt) + 1;

#define IDX(r,c) (((r) * colsize) + (c))

    memset(bonus, 0x00, sizeof(int) * colsize);
    memset(matrix, 0x00, sizeof(int) * rowsize * colsize);
    memset(cont  , 0x00, sizeof(int) * rowsize * colsize);

    /* 보너스 계산 */
    char t_cur, t_pre = 0;
    for(int col = 1; col < colsize; col++)
    {
        t_cur = txt[col-1];
        if(isalnum(t_pre) == 0 && isalnum(t_cur))
            bonus[col] = g_bonus_boundary;
        else if(isalnum(t_cur) == 0)
            bonus[col] = g_bonus_no_alnum;
        else if(islower(t_pre) && isupper(t_cur))
            bonus[col] = g_bonus_camel;
        t_pre = t_cur;
    }
    char p_cur;
    int first_col = 1;
    int max_score = 0; int max_col = 0;

    for(int row=1; row < rowsize; row++)
    {
        p_cur = pat[row - 1];
        int is_gap = 0;
        int is_first = 0;

        for(int col = first_col; col < colsize; col++)
        {
            t_cur = txt[col-1];
            
            int is_select = 0; /* false */
            int diag_score = matrix[IDX(row-1, col-1)];
            int left_score = matrix[IDX(row  , col-1)];
            int bonus_score = 0;
            int cont_cnt = 0;            
            /* 왼쪽 점수 결정 */
            if(is_gap)
                left_score += g_penalty_ingap;
            else
                left_score += g_penalty_firstgap; /* 첫 연속 실패시 패널티가 크다. */
            /* match */
            if(tolower(p_cur) == tolower(t_cur))
            {
                /* 첫번째 시작 col 기록 */
                if(is_first == 0)
                {
                    is_first = 1;
                    first_col = col+1;
                }
                /* 연속 개수 선정 */
                cont_cnt = cont[IDX(row-1, col-1)] + 1;
                /* 대각선 */
                diag_score += g_score_match;
                /* 보너스 결정 */
                bonus_score = bonus[col];
                if(cont_cnt > 1)
                {
                    /* 연속이면 현재보너스, 연속보너스점수, 연속시작문자보너스 중 최대값을 선정해서 정함 */
                    if(bonus_score < g_bonus_continuous)
                        bonus_score = g_bonus_continuous;
                    if(bonus_score < bonus[col - cont_cnt + 1])
                        bonus_score = bonus[col - cont_cnt + 1];
                    bonus_score += 1;
                }
                /* 대각선이 더 큰경우 패턴 선택 */
                if(left_score < diag_score + bonus_score)
                    is_select = 1; /* true */
            }
            /* 선택되면 대각선, 안될경우 왼쪽 선택한다. */
            if(is_select)
            {
                is_gap = 0; /* false */
                matrix[IDX(row, col)] = diag_score + bonus_score;
                cont  [IDX(row, col)] = cont_cnt;
            }
            else
            {
                is_gap = 1; /* true */
                matrix[IDX(row  , col)] = left_score ;
            }
            /* 음수값 처리 */
            if(matrix[IDX(row  , col)] < 0)
                matrix[IDX(row  , col)] = 0;
            /* 최대값 구하기 */
            if(row == rowsize-1 && max_score < matrix[IDX(row  , col)])
            {
                max_score = matrix[IDX(row  , col)];
                max_col = col;
            }
        }
        /* 한글자도 매치되지 않은 경우 실패 */
        if(is_first == 0)
            return 0;
    }

    /* 최대값 */
    *fscore = max_score;
    /* 역추적 */
    int back_row = rowsize - 1;
    int back_col = max_col;
    while(back_row >= 1)
    {
        if(matrix[IDX(back_row, back_col-1)] <= matrix[IDX(back_row, back_col)])
        {
            back_row--; 
            position[back_col - 1] = 1;
        }
        back_col --;
    }

#undef IDX
    return 1;
}


/*
    파일리스트 및 퍼지점수 구하기

    - 재귀호출 디렉토리 순회하면서 파일명 추출
    - 파일명 자체는 메모리 풀에 '\0' 으로 구분되도록 기록
    - fscore structure 는 파일명의 포인터
    - _match는 직전 최대 매치된 길이 기록 (백스페이스로 패턴이 하나 줄었을때 빠르게 찾기위함)

*/



/* marlloc 을 기반으로  */
static char*    g_file_names;
static char*    g_file_names_cursor;
static fscore_t* g_fscore;
static int      g_fscore_count;
static fscore_t** g_cands;
static int       g_cands_count;


static int comp_cand(const void* a, const void* b);

static void update_files(int prefix_len, char* path)
{
    /* 파일리스트 갱신 */
    strcpy(g_file_names_cursor, &path[prefix_len+1]);
    g_fscore[g_fscore_count].fname = g_file_names_cursor;
    g_fscore[g_fscore_count].score = MAX_FILE_NUM - g_fscore_count;
    g_fscore[g_fscore_count]._match = MAX_PATTERN;
    g_file_names_cursor += strlen(&path[prefix_len+1]) + 1;
    /* 후보도 바로 갱신 */
    g_cands[g_cands_count++] = &g_fscore[g_fscore_count];
    g_fscore_count++;
}

/* 재귀호출 파일리스트 추출 */
static void get_file_list_recur (int prefix_len, const char* base_path, int isfile)
{
    DIR *dir;  
    struct dirent *ent;
    char path [ MAX_PATH_LEN ];
    struct stat sb;

    dir = opendir(base_path);
    if( dir == NULL )
        return;

    while( (ent = readdir(dir)) )
    {
        strcpy(path, base_path);
        strcat(path, "/");
        strcat(path, ent->d_name);
        
#ifdef _DIRENT_HAVE_D_TYPE
        /* Posix 표준이 아니다. GNU에서 제공 */
        if(ent->d_type == DT_DIR)
        {
#else
        /* 디렉토리 여부를 별도로 확인 overhead */
        stat( path, &sb);        
        if( S_ISDIR(sb.st_mode) )
        {
#endif
            if(strcmp(ent->d_name, "..") == 0 ||
               strcmp(ent->d_name, ".")  == 0 )
                   continue;
            get_file_list_recur(prefix_len, path, isfile);
            if(isfile == 0)
                update_files(prefix_len, path);
        }
        else
        {
            if(isfile)
                update_files(prefix_len, path);
        }
    }
}

/* 파일 메모리 로드  */
void load_file_list(char* path, int isfile, int* total_count, int* cands_count, fscore_t*** cands)
{
    /* initialize */
    g_file_names = (char*)    malloc ( MAX_FILE_NUM * MAX_PATH_LEN );
    g_file_names_cursor = g_file_names;
    g_fscore = (fscore_t*) malloc ( MAX_FILE_NUM * sizeof(fscore_t));
    g_fscore_count = 0;
    g_cands = (fscore_t**) malloc ( MAX_FILE_NUM * sizeof(fscore_t*));
    g_cands_count = 0;

    int prefix_len = strlen(path);
    get_file_list_recur( prefix_len , path, isfile);

    *total_count = g_fscore_count;

    /* 후보정렬 */
    qsort( g_cands, g_cands_count, sizeof(fscore_t*), comp_cand);
    *cands_count = g_cands_count;
    *cands = g_cands;
}

void clear_file_list()
{
    if(g_file_names != NULL)
        free(g_file_names);
    if(g_fscore != NULL)
        free(g_fscore);
    if(g_cands!= NULL)
        free(g_cands);
    g_file_names = NULL;
    g_fscore = NULL;
    g_cands= NULL;
    g_file_names_cursor = NULL;
    g_fscore_count = 0;
    g_cands_count= 0;
}


/* 역순정렬 비교함수 */
static int comp_cand(const void* a, const void* b)
{
    fscore_t* aa = *((fscore_t**)a);
    fscore_t* bb = *((fscore_t**)b);

    if(aa->score > bb->score)
        return -1;
    if(aa->score < bb->score)
        return 1;

    int alen = strlen(aa->fname);
    int blen = strlen(bb->fname);

    if(alen < blen)
        return -1;
    if(alen > blen)
        return 1;

    if(strcmp(aa->fname, bb->fname) > 0)
        return -1;
    if(strcmp(aa->fname, bb->fname) < 0)
        return 1;
    return 0;
}


/* 후보 퍼지점수 갱신 */
void update_cands_fuzzy_score(char * pat, int* cands_count, fscore_t*** cands)
{
    int patlen = strlen(pat);
    int position [MAX_PATH_LEN];
    g_cands_count = 0;

    /* 모든파일에 대해 퍼지계산 */
    for(int i=0; i < g_fscore_count; i++)
    {
        int score = 0;

        /* 이전에 실패한것은 건너뛴다. */
        if(g_fscore[i]._match < patlen)
            continue;

        /* 여기서는 포지션을 쓰지 않으므로 초기화 등을 하지 않음 */
        int ret = get_fuzzy_score(
                    pat, g_fscore[i].fname,
                    &score, position);

        g_fscore[i]._match = patlen;
        g_fscore[i].score = score;

        if(ret)
        {
            /* 성공한 것들만 후보에 올린다. */
            g_fscore[i]._match = MAX_PATTERN;
            g_cands[g_cands_count++] = &g_fscore[i];
        }
    }

    /* 정렬 및 출력 */
    qsort( g_cands, g_cands_count, sizeof(fscore_t*), comp_cand);
    *cands_count = g_cands_count;
    *cands = g_cands;
}



#ifdef FZ_BIN_MAIN
/* curses 기반 바이너리 컴파일시 매크로 정의하여 빌드 */
#ifdef CURSES
	#include <curses.h>
#else
	#include <ncurses.h>
#endif
#define MAX_FZ_INPUT (32)

char g_ascii_code[16];
char* control_codes[] = {
"<NUL>", "<SOH>", "<STX>", "<ETX>", "<EOT>", "<ENQ>", 
"<ACK>", "<BEL>", "<BS>", "<HT>", "<LF>", "<VT>", "<FF>", 
"<CR>", "<SO>", "<SI>", "<DLE>", "<DC1>", "<DC2>", "<DC3>", 
"<DC4>", "<NAK>", "<SYN>", "<ETB>", "<CAN>", "<EM>", "<SUB>", 
"<ESC>", "<FS>", "<GS>", "<RS>", "<US>"
};

char* get_ascii_nm(int c)
{
    if(0 <= c && c < 0x20)
    {
        return control_codes[c];
    }
    else if( 0x20 <= c && c < 0x7f)
    {
        sprintf(g_ascii_code, "'%c'", c);
        return g_ascii_code;
    }
    else if( c >= 0x7f )
    {
        return "<DEL>";
    }
    else
        return "";
}


/*
    Get key-sequence in raw mode

    위쪽 아래쪽 화살표가 동시에 눌렸을 경우 : 
First:
        <ESC> '[' 'A' <ESC> '[' 'B'
          *  ------>
    - 키 입력을 대기하다가 모든 키 시퀀스를 kbuf에 저장
    - 첫번째 키 시퀀스를 seq에 저장

Seconds:
        <ESC> '[' 'A' <ESC> '[' 'B'
                        *  ------>
    - 두번째 키 시퀀스를 seq 저장
*/
#define KEY_SEQ_SIZE (8)

static int raw_keys(int kbuf[], int buflen, int* buf_idx, int* err_cnt, int seq[])
{
    if( kbuf[*buf_idx] == 0)
    {
        int key, cnt=0;
        *buf_idx = 0;
        memset(kbuf, 0x00, sizeof(int) * buflen);
        key = getch();
        while(key == ERR)
        {  /* meaningless ERR key skip , found it old unix */
            cnt++; key = getch();
            *err_cnt = cnt;
            if(cnt > 128)
                usleep(50 * 1000);
        }
        kbuf[0] = key;
        if(key == 0x1b) /* ESC */
        {
            timeout(0); /* non-blocking */
            for(int i= 1 ; i < buflen; i++)
            {
                key = getch();
                if(i == 1 && key == ERR)
                    return 0; /* ESC KEY */
                if(key == ERR)
                    break;
                kbuf[i] = key;
            }
            timeout(-1); /* blocking */
        }        
    }

    int is_esc = 0;
    int seq_idx = 0;
    memset(seq, 0x00, sizeof(int) * KEY_SEQ_SIZE);

    for(int i= *buf_idx; i < buflen && kbuf[i] != 0; i++)
    {
        if(is_esc)
        {
            if( kbuf[i] == 0x1b )  /* ESC :   ESC [ A  (here) ESC  */
                break;
            seq[seq_idx++] = kbuf[i];
            if( kbuf[i] == 0x7e )  /* ~   :   ESC [ 1 5 ~  (here) */
                break;
        }
        else
        {
            seq[seq_idx++] = kbuf[i];
            if( kbuf[i] == 0x1b ) /* ESC :  start escape sequence */
                is_esc = 1;
            else
                break;
        }
    }
    
    if(seq[0] == 0x1b)
        *buf_idx  += seq_idx + 1;
    else
        *buf_idx += 1;
    
    return 1;
}

static void draw_title(int totcnt, int cndcnt, char* env_nm, char* base_path)
{
    attron(COLOR_PAIR(2));
    mvprintw(0, 0, "  FUZZY FINDER by lhw, ESC:exit [%d/%d] BasePath(%s): %s ", cndcnt, totcnt , env_nm, base_path);
    attroff(COLOR_PAIR(2));
}

static void draw_input(char* txt, int len)
{
    mvaddstr( 2, 1, "fz> ");
    mvaddstr( 2, 5, txt);
    attron(COLOR_PAIR(2));
    mvaddstr( 2, 5+len, " "); /* 커서 흉내 */
    attroff(COLOR_PAIR(2));
}

/* 퍼지를 다시 돌려서 패턴일치 위치를 색으로 표시 */
static void draw_fname(int select, int row, char* pat, char* txt)
{
    int score = 0;
    int position[MAX_PATH_LEN] = {0};
    get_fuzzy_score(pat, txt, &score, position);

    for(int i=0; txt[i] != '\0'; i++)
    {        
        if(position[i] == 1)
        {
            attron(COLOR_PAIR(3));
            mvaddch(row, 4+i, txt[i]);
            attroff(COLOR_PAIR(3));
        }
        else
        {
            mvaddch(row, 4+i, txt[i]);
        }        
    }
}

/* 선택 표출 */
static void draw_flist(int select, int maxrow, char* pat, fscore_t** cnd, int cndcnt)
{
    int base = 4;
    for(int i=0; i < maxrow - base -1 && i < cndcnt; i++)
    {
        if(select == i)
        {
            attron(COLOR_PAIR(1));
            mvaddstr( base+i, 1, "=>");
            attroff(COLOR_PAIR(1));
            draw_fname(1, base+i, pat, cnd[i]->fname);
        }
        else
        {
            mvaddstr( base+i, 1, "- ");
            draw_fname(0, base+i, pat, cnd[i]->fname);
        }
    }
}

static void draw_keyseq(int seqs[], int maxrow)
{
    char buf[256];
    strcpy(buf, "Key input: ");
    for(int i=0; i < KEY_SEQ_SIZE && seqs[i] != 0; i++)
    {
        strcat( buf, get_ascii_nm(seqs[i]));
    }
    mvaddstr(maxrow-1, 1, buf);
}


static void curses_main(char* base_path, char* env_nm, int totcnt, int cndcnt, fscore_t** cnd)
{
    int maxrow=0; int maxcol = 0;
    int kbufs[64] ={0};
    int kbuf_idx = 0;
    int err_cnt = 0;
    int seqs[KEY_SEQ_SIZE] = {0};

    /* 표준출력을 사용하기 위해 initscr 대신 신규tty 터미널 생성 */
    FILE *f = fopen("/dev/tty", "rb+");
    SCREEN *screen = newterm(NULL, f, f);
    set_term(screen);
    /* color */
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_WHITE);
    init_pair(2, COLOR_BLACK, COLOR_WHITE);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);

    /* raw mode로 지정 */
    /* linux는 별 문제가 없으나 old unix에서 keypad(, TRUE)가 먹히지 않음 */
    curs_set(0);
    noecho();
    raw();
    getmaxyx(stdscr,maxrow,maxcol);

    int select = 0; 
    char input_buf[ MAX_FZ_INPUT + 1 ];
    int  input_buf_cnt = 0;
    int isupdate=0; int isenter=0;
    memset(input_buf, 0x00, sizeof(input_buf));

    draw_title(totcnt, cndcnt, env_nm, base_path);
    draw_input(input_buf, input_buf_cnt);
    draw_flist(select, maxrow, input_buf, cnd, cndcnt);

    while(raw_keys(kbufs, 64, &kbuf_idx, &err_cnt, seqs )) /* ESC key exit */
    {
        erase();
        isupdate = 0;
        if(seqs[0] == 0x1b) /* ESC  */
        {
            if(seqs[1] == 0x5b)
            {
                if(seqs[2] == 0x41) /* key up */
                    if(select > 0)
                        select--;
                if(seqs[2] == 0x42) /* key down */
                    if(select+1 < maxrow - 4 - 1 && select+1 < cndcnt)
                        select++;
            }
        }
        else
        {
            if(seqs[0] >= 0x20 && seqs[0] <= 0x7E)
            {
                /*  ascii pritable range */
                if(input_buf_cnt < MAX_FZ_INPUT &&
                   input_buf_cnt >= 0        )
                {
                    input_buf[input_buf_cnt++] = seqs[0];
                    select = 0; /* 최대값으로 다시 지정 */
                    isupdate = 1;
                }
            }
            else if(seqs[0] == 0x7f) /* back space */
            {
                if(input_buf_cnt > 0)
                {
                    input_buf[--input_buf_cnt] = '\0';
                    isupdate = 1;
                }
            }
            else if(seqs[0] == 0x0a) /* line feed (enter) */
            {
                isenter = 1;
            }
        }

        /* 후보갱신 */
        if(isupdate)
            update_cands_fuzzy_score(input_buf, &cndcnt, &cnd);

        draw_title(totcnt, cndcnt, env_nm, base_path);
        draw_input(input_buf, input_buf_cnt);
        draw_flist(select, maxrow, input_buf, cnd, cndcnt);
        draw_keyseq(seqs, maxrow);

        if(isenter == 1)
        {
            break;
        }

        refresh();
    }
    endwin();
    delscreen(screen);
    fclose(f);
    /* curses end */

    if(isenter == 1)
    {
        /* 절대경로로 바꾸어 출력한다. */
        char input_path[ MAX_PATH_LEN ];
        sprintf(input_path , "%s/%s\n", base_path, cnd[select]->fname);        
        fprintf(stdout, "%s", input_path );
    }
}

void show_usage()
{
    char* usage = 
        " Fuzzy file finder \n"\
        "    부분일치, 약어일치 등으로 파일을 검색합니다.\n\n"\
        "    $ fz [-hd] [Argument]\n"\
        "\n"\
        "    Option:\n"\
        "       -h      help\n"\
        "       -d      directory 검색모드  기본은 file 검색 \n"\
        "       -e      ENV 'FZ_BASE_PATH' 의 경로로 고정    \n"\
        "               이 옵션이 없으면 서브디렉토리 만 적용\n"\
        "\n"
    ;

    fprintf(stdout, "%s", usage);
}


int main(int argc, char* argv[])
{
    int c;
    int isfile = 1;
    int isenv = 0;

    /* option */
    while( (c = getopt(argc, argv, "hde")) != -1)
    {
        switch(c)
        {
            case 'h':
                show_usage();
                exit(1);
                break;
            case 'd':
                isfile = 0;
                break;
            case 'e':
                isenv = 1;
                break;
            case '?':
                printf("Unknown Flags\n");
                show_usage();
                exit(1);
                break;
        }
    }

    /* base-path  */
    char base_path[512];
    strcpy(base_path, ".");

    /* 환경변수의 값으로 base 지정 */
    char* env_nm = "cwd";
    char* env = getenv("FZ_BASE_PATH");
    if(env)
    {
        /*strcpy(base_path, env); */
        char* currpath = getcwd(NULL, MAX_PATH_LEN);
        realpath(env, base_path);

        if(isenv)
        {
            /* env옵션이 켜져있으면 어떤 경로위치에서든 환경변수위치를 기준으로 함 */
            strcpy(base_path, env);
            env_nm = "ENV";
        }
        else
        {
            /* 작업디렉토리가 base path 안에 있는 서브디렉토리이면  */
            if(strncmp( base_path,  currpath, strlen(base_path)) == 0)
            {
                strcpy(base_path, env);
                env_nm = "ENV";
            }
            else
                strcpy(base_path, ".");
        }

        if(currpath != NULL)
            free(currpath);
    }


    /* fz */
    int totcnt = 0;
    int cndcnt = 0;
    fscore_t** cnds;
    load_file_list(base_path, isfile, &totcnt, &cndcnt, &cnds);
    curses_main(base_path, env_nm, totcnt, cndcnt, cnds);
    clear_file_list();
    return 0;
}


#endif
