#ifndef SUDOKU_H
#define SUDOKU_H

const bool DEBUG_MODE = false;
enum { ROW=9, COL=9, N = 81, NEIGHBOR = 20 };
const int NUM = 9;

struct job_t {
	int puzzleNo;			//��Ŀ�������ļ��еı��  �ڵ�n�о���n
	int board[81];			//��Ŀ
};

struct compare {
	// ">" ��ʾ puzzleNoС�����ȼ���
	bool operator() (job_t j1, job_t j2) {
		return j1.puzzleNo > j2.puzzleNo;
	}
};
extern int neighbors[N][NEIGHBOR];
extern int board[N];
extern int spaces[N];
extern int nspaces;
extern int (*chess)[COL];

void init_neighbors();
void input(const char in[N]);
void init_cache();

bool available(int guess, int cell);

bool solve_sudoku_basic(int which_space);
bool solve_sudoku_min_arity(int which_space);
bool solve_sudoku_min_arity_cache(int which_space);
bool solve_sudoku_dancing_links(int unused,job_t& job);
bool solved();
#endif
