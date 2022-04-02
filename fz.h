/**
 * @file  fz.h file
 * Fuzzy finder 관련 함수 정의
 * Smith-Waterman 알고리즘 을 변형하여 파일명 검색하기 위한
 * 라이브러리
 */
#ifndef __FZ_H__
#define __FZ_H__

#define MAX_FILE_NUM (1024 * 1024)
#define MAX_PATH_LEN (512)
#define MAX_PATTERN  (32)

/**
 * @struct fscore_st
 * @brief  파일명 퍼지스코어 mapping 구조체
 *
 * @var fscore_t::fname
 * 	파일명
 * @var fscore_t::score
 * 	Fuzzy 점수
 * @var fscore_t::_match
 * 	Curses 구현에서 내부적으로 사용하는 값 (직전 최대 매치 패턴 길이)
 */
typedef struct fscore_st
{
    char* fname;
    int score;
    int _match; 
}fscore_t;

/**
 * @struct fscore_list_t
 * @brief  파일명 리스트 구조체
 *
 * @var fscore_list_t::scores
 * 	원본, 파일명과 퍼지스코어 등이 관리되는 배열.
 * @var fscore_list_t::len
 * 	원본의 파일명 배열의 크기
 * @var fscore_list_t::cands
 * 	후보, 성능을 높이기 위해 포인터의 배열 사용.
 * @var fscore_list_t::cands_cnt
 * 	후보의 개수
 */
typedef struct 
{
    fscore_t*  scores;
    int  len;

    fscore_t** cands; 
    int  cands_cnt;

    /* 내부적으로 사용되는 파일명 POOL, 사용안해도 괜찮다.  */
    /* [file1\0file2\0             ]*/
    /*                *cursor       */
    char*  _fname_pool;
    char*  _fname_cursor;
} fscore_list_t;

/**
 * @brief  Fuzzy Score를 구하는 함수
 * @details Smith-Waterman 알고리즘을 일부변형하여 파일검색으로 사용
 * @param[in] pat  패턴문자열 (입력)
 * @param[in] txt  검색문자열 (대상)
 * @param[out] fscore  Fuzzy 점수
 * @param[out] position  검색대상문자열에서 패턴이 일치한 위치
 * @return 패턴 일치했는지 여부값
 * @retval 1  패턴일치 성공
 * @retval 0  패턴일치 실패
 */
int get_fuzzy_score(char* pat, char* txt, int* fscore, int position[]);


/**
 * @brief  list 객체 초기화
 * @param[in,out] list  초기화할 파일명 리스트 객체
 */
void  init_list (fscore_list_t* list);
/**
 * @brief  list 객체 아이템 추가
 * @param[in,out] list  파일명 리스트 객체
 * @param[in] item  추가할 파일명
 */
void   add_list (fscore_list_t* list, char* item);
/**
 * @brief  list 객체 헤제
 * @param[in,out] list  해제할 파일명 리스트 객체
 */
void clear_list (fscore_list_t* list);

/**
 * @brief  주어진 디렉토리를 기반으로 모든 하위파일명 메모리로드
 * @param[in,out] list  로드된 파일명리스트
 * @param[in] path  Base-Path
 * @param[in] isfile   0이면 디렉토리목록, 그 외는 파일목록
 */
void  load_file_list ( fscore_list_t* list, char* path, int isfile);

/**
 * @brief  로드된 메모리 헤제 및 정리
 * @param[in,out] list  로드된 파일명리스트
 */
void clear_file_list ( fscore_list_t* list);

/**
 * @brief  주어진 패턴에 따라 퍼지검색 후보 갱신
 * @details 퍼지스코어를 구해서 list->cands 에 후보를 등록, list->cands_cnt 개수만큼 생성됨
 * @param[in,out] list  로드된 파일명리스트
 * @param[in] pat  입력 퍼지 패턴 
 */
void update_candidates_by_fuzzy_score ( fscore_list_t* list , char* pat );


#endif
