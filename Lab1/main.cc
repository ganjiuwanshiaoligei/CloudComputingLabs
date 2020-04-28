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
//�����������߽���

int total_solved = 0;
int total = 0;

bool (*solve)(int,job_t&) = solve_sudoku_dancing_links; //���ýⷨ


long int numOfPuzzle = 0;	//��Ŀ���
//�������
queue<struct job_t> JobQueue;
//������� ���ȶ���
priority_queue<struct job_t, vector<struct job_t>, compare> SolveQueue;


//�������
pthread_mutex_t jobQueueMutex = PTHREAD_MUTEX_INITIALIZER;
//�������������
pthread_mutex_t totalSolvedCountMutex = PTHREAD_MUTEX_INITIALIZER;

int numOfJob = 10;			//ͬʱ��ȡ��������Ŀ
int numOfSem = 1000;		//�ź�����������
int numOfSolve = 4;			//�����߳�����


sem_t queueEmpty;			//������пյ��ź���
sem_t queueFull;			//������������ź���


FILE* fp;					//��ȡ�ļ���ָ��

int64_t now()				//�������ڵ�ʱ��
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000 + tv.tv_usec;
}
//���� ��������
void* read(void* a) {
	while (1) {				//���Զ�ȡ����
		//�����ٽ���
		sem_wait(&queueEmpty);
		//�߳���
		pthread_mutex_lock(&jobQueueMutex);
		char puzzle[85];
		int numOfRead = 0;	//��ǰһ�ζ�ȡ������ ����ȡ������С��һ��Ӧ�ö�ȡ������ʱ���жϸ��ļ��Ѿ���ȡ���
		
		while (fgets(puzzle, sizeof puzzle, fp) != NULL){
			if (strlen(puzzle) >= 81) {
				job_t newJob;
				newJob.puzzleNo = numOfPuzzle++;//��ȡ��Ŀ���
				for (int i = 0; i < 81; i++) {	//����Ŀ���ַ���ת��int��
					newJob.board[i] = puzzle[i] - '0';
				}
				JobQueue.push(newJob);			//���
				numOfRead++;
				if (numOfRead >= numOfJob) {
					break;
				}
			}	
		}			
		//�ͷ���
		pthread_mutex_unlock(&jobQueueMutex);
		sem_post(&queueFull);
		if (numOfRead < numOfJob) {
			return 0;
		}
		
	}
 }

void* solved(void* a) {
	while (1) {
		//�����ٽ���
		sem_wait(&queueFull);
		//�߳���
		pthread_mutex_lock(&jobQueueMutex);
		job_t newJob[numOfJob];
		int numOfSolved = 0;
		while (1) {
			if (JobQueue.size() <= 0) {
				for (int i = 0; i < numOfSolved; i++) {
					//������ɹ�������true
					if (solve(0, newJob[i])) {
						//�ɹ�����������
						pthread_mutex_lock(&totalSolvedCountMutex);
						++total_solved;
						SolveQueue.push(newJob[i]);
						pthread_mutex_unlock(&totalSolvedCountMutex);
					}
					else {//solve������false ��ʾ�޽�
						printf("No solution:\n");	//����Ŀ�޽� ����ӡ��Ӧ��Ŀ
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
			//������ɹ�������true
			if (solve(0, newJob[i])) {
				//�ɹ�����������
				pthread_mutex_lock(&totalSolvedCountMutex);
				++total_solved;
				SolveQueue.push(newJob[i]);
				pthread_mutex_unlock(&totalSolvedCountMutex);
			}
			else {//solve������false ��ʾ�޽�
				printf("No solution\n");
			}
		}
	}
}

void getFileSource() {
	char fileName[20] = "";
	//�����ļ���
	scanf("%s", fileName);
	fp = fopen(fileName, "r");
}

int main(int argc, char* argv[])
{
  init_neighbors();

  char puzzle[128];

  getFileSource();//��ȡ�ļ���

  sem_init(&queueEmpty, 0, numOfSem);
  sem_init(&queueFull, 0, 0);
  //��ʼ���߳�
  pthread_t problemReader;    							//�����߳� 
  pthread_t problemSolvers[numOfSolve];					//�����߳�
  int64_t start = now();
  
  pthread_create(&problemReader, NULL, read, NULL);  //���������߳�  
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

