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
 * @brief  주어진 디렉토리를 기반으로 모든 하위파일명 메모리로드
 * @param[in] path  Base-Path
 * @param[in] isfile   0이면 디렉토리목록, 그 외는 파일목록
 * @param[out] total_count 로드된 파일 총 갯수
 * @param[out] cands_count  퍼지검색에 표출될 후보 개수
 * @param[out] cands    후보배열, fscore_t* 의 배열에 대한 포인터
 */
void load_file_list(char* path, int isfile, int* total_count, int* cands_count, fscore_t*** cands);

/**
 * @brief  로드된 메모리 헤제 및 정리
 */
void clear_file_list();

/**
 * @brief  주어진 패턴에 따라 퍼지검색 후보 갱신
 * @param[in] pat   입력 패턴
 * @param[out] cands_count  퍼지검색에 표출될 후보 개수
 * @param[out] cands    후보배열 frank_t* 의 배열에 대한 포인터
 */
void update_cands_fuzzy_score(char * pat, int* cands_count, fscore_t*** cands);



#endif
