#include <assert.h>
#include <time.h>
#include <semaphore.h>
#include <sys/time.h>
#include <time.h>
#include <queue>
#include <vector>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#include "sudoku.h"
using namespace std;
//生产者消费者解题

int total_solved = 0;
int total = 0;

bool (*solve)(int,job_t&) = solve_sudoku_dancing_links; //设置解法


long int numOfPuzzle = 0;	//题目编号
//任务队列
queue<struct job_t> JobQueue;
//结果队列 优先队列
priority_queue<struct job_t, vector<struct job_t>, compare> SolveQueue;


//任务池锁
pthread_mutex_t jobQueueMutex = PTHREAD_MUTEX_INITIALIZER;
//解题计数变量锁
pthread_mutex_t totalSolvedCountMutex = PTHREAD_MUTEX_INITIALIZER;

int numOfJob = 10;			//同时获取的任务数目
int numOfSem = 1000;		//信号量数量上限
int numOfSolve = 4;			//解题线程数量


sem_t queueEmpty;			//任务队列空的信号量
sem_t queueFull;			//任务队列满的信号量


FILE* fp;					//读取文件的指针

int64_t now()				//计算现在的时间
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000 + tv.tv_usec;
}
//读题 即生产者
void* read(void* a) {
	while (1) {				//尝试读取任务
		//进入临界区
		sem_wait(&queueEmpty);
		//线程锁
		pthread_mutex_lock(&jobQueueMutex);
		char puzzle[85];
		int numOfRead = 0;	//当前一次读取的任务 当读取的任务小于一次应该读取的任务时，判断该文件已经读取完毕
		
		while (fgets(puzzle, sizeof puzzle, fp) != NULL){
			if (strlen(puzzle) >= 81) {
				job_t newJob;
				newJob.puzzleNo = numOfPuzzle++;//获取题目编号
				for (int i = 0; i < 81; i++) {	//把题目从字符串转成int型
					newJob.board[i] = puzzle[i] - '0';
				}
				JobQueue.push(newJob);			//入队
				numOfRead++;
				if (numOfRead >= numOfJob) {
					break;
				}
			}	
		}			
		//释放锁
		pthread_mutex_unlock(&jobQueueMutex);
		sem_post(&queueFull);
		if (numOfRead < numOfJob) {
			return 0;
		}
		
	}
 }

void* solved(void* a) {
	while (1) {
		//进入临界区
		sem_wait(&queueFull);
		//线程锁
		pthread_mutex_lock(&jobQueueMutex);
		job_t newJob[numOfJob];
		int numOfSolved = 0;
		while (1) {
			if (JobQueue.size() <= 0) {
				for (int i = 0; i < numOfSolved; i++) {
					//如果求解成功返回了true
					if (solve(0, newJob[i])) {
						//成功求解计数增加
						pthread_mutex_lock(&totalSolvedCountMutex);
						++total_solved;
						SolveQueue.push(newJob[i]);
						pthread_mutex_unlock(&totalSolvedCountMutex);
					}
					else {//solve返回了false 表示无解
						printf("No solution:\n");	//该题目无解 并打印对应题目
						for (int j = 0; j < 81; j++) {
							printf("%d", newJob[i].board[j]);
						}
						printf("\n");
					}
				}
				pthread_mutex_unlock(&jobQueueMutex);
				sem_post(&queueFull);
				return 0;
			}
			
			newJob[numOfSolved++] = JobQueue.front();
			JobQueue.pop();
			if (numOfSolved >= numOfJob) {
				break;
			}
		}
		pthread_mutex_unlock(&jobQueueMutex);
		sem_post(&queueEmpty);
		for (int i = 0; i < numOfSolved; i++) {
			//如果求解成功返回了true
			if (solve(0, newJob[i])) {
				//成功求解计数增加
				pthread_mutex_lock(&totalSolvedCountMutex);
				++total_solved;
				SolveQueue.push(newJob[i]);
				pthread_mutex_unlock(&totalSolvedCountMutex);
			}
			else {//solve返回了false 表示无解
				printf("No solution\n");
			}
		}
	}
}

void getFileSource() {
	char fileName[20] = "";
	//输入文件名
	scanf("%s", fileName);
	fp = fopen(fileName, "r");
}

int main(int argc, char* argv[])
{
  init_neighbors();

  char puzzle[128];

  getFileSource();//获取文件名

  sem_init(&queueEmpty, 0, numOfSem);
  sem_init(&queueFull, 0, 0);
  //初始化线程
  pthread_t problemReader;    							//读题线程 
  pthread_t problemSolvers[numOfSolve];					//解题线程
  int64_t start = now();
  
  pthread_create(&problemReader, NULL, read, NULL);  //创建各类线程  
  for (int i = 0; i < numOfSolve; i++){
	  pthread_create(&problemSolvers[i], NULL, solved, (void*)i);
  }
 
  pthread_join(problemReader, NULL);
  for (int i = 0; i < numOfSolve; i++) {
	  pthread_join(problemSolvers[i], NULL);
  }
 // printf("pthread end\n");
  int64_t end = now();
  while (!SolveQueue.empty()) {
	  job_t newJob;
	  newJob = SolveQueue.top();
	  SolveQueue.pop();
	  //printf("%d\n", newJob.puzzleNo);
	  for (int j = 0; j < 81; j++) {
		  printf("%d", newJob.board[j]);
	  }
	  printf("\n");
  }

  double sec = (end-start)/1000000.0;
  printf("%f sec %f ms each %d\n", sec, 1000*sec/total, total_solved);

  return 0;
}

