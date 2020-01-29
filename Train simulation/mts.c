/*

Author: Mek Obchey
V00880355
CSC 360 Assignment 2
Simple Train Simulator with Threads
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
// #include <zconf.h>
#include <unistd.h>

#define MAX_SIZE 1000 //size of threads used in initialization
#define MAX_STATIONS 4 //number of stations
#define MAX_TIME 100 //loading and crossing time in the range [1,100)
#define SLEEP_OFFSET 100000 //used to offset usleep (10^6) by 10^5 to get 10th of a second

pthread_mutex_t loadLock;
pthread_mutex_t master;
pthread_mutex_t mainTrack;
pthread_cond_t startSignal = PTHREAD_COND_INITIALIZER; //convar initialization
pthread_cond_t stationSignal = PTHREAD_COND_INITIALIZER;
pthread_cond_t crossSignal = PTHREAD_COND_INITIALIZER;
int start = 0;
// int timer = 0;
int enqueueing = 0;

typedef struct _train {
	int id;
	char direction;
	int load_time;
	int cross_time;
	int hasCrossSignal;
	int hasCrossed;
} train;

//------------------------queue------------------------------------
struct node {
	int id;
	struct node* next;
};

struct node* q1;	//Station E
struct node* q2;	//Station e
struct node* q3;	//Station W
struct node* q4;	//Station w

void enqueue(int stationId, int trainId) {
	struct node* newNode = malloc(sizeof(struct node));
	struct node *ptr;
	if(stationId == 1)
		ptr = q1;
	else if(stationId == 2)
		ptr = q2;
	else if(stationId == 3)
		ptr = q3;
	else if(stationId == 4)
		ptr = q4;

	newNode->id = trainId;
	newNode->next = NULL;

	if(ptr == NULL) {
		if(stationId == 1)
			q1 = newNode;
		else if(stationId == 2)
			q2 = newNode;
		else if(stationId == 3)
			q3 = newNode;
		else if(stationId == 4)
			q4 = newNode;
	}
	else {
		while(ptr->next != NULL) {
			ptr = ptr->next;
		}
		ptr->next = newNode;
	}
}

void dequeue(int stationId) {
	struct node* station;
	if(stationId == 1)
		station = q1;
	else if(stationId == 2)
		station = q2;
	else if(stationId == 3)
		station = q3;
	else if(stationId == 4)
		station = q4;

	if(station != NULL) {
		station = station->next;
	}
}
//-----------------------------------------------------------------


/*
	Shrink the train array to a specified size.
*/
void shrink(train trainList[], int size) {
	train* new_trainList = (train *)malloc(sizeof(train)*MAX_SIZE);;
	for(int i=0; i<size; i++) {
		new_trainList[i] = trainList[i];
	}
	trainList = new_trainList;
}


int readInput(char* filename, train* trainList) {
	FILE *f = fopen(filename, "r");
	int i = 0;	//for indexing and counting all trains
	if(f == NULL) {
		return -1;
	}
	char direction;
	int load_time;
	int cross_time;
	while(!feof(f)) {
		fscanf(f, "%c %d %d\n", &direction, &load_time, &cross_time);
		trainList[i].id = i;
		trainList[i].direction = direction;
		trainList[i].load_time = load_time % MAX_TIME;
		trainList[i].cross_time = cross_time % MAX_TIME;
		trainList[i].hasCrossSignal = 0;
		trainList[i].hasCrossed = 0;
		i++;
	}
	shrink(trainList, i);
	fclose(f);
	return i;
}

//for debugging
void printTrain(train* trainList, int size) {
	for(int x = 0; x < size; x++){
		printf("Train : %d %c %d %d\n", trainList[x].id, trainList[x].direction, 
			trainList[x].load_time, trainList[x].cross_time);
	}
}

void printTime(int d) {
	int size = 6;
	int mod = 10;
	int digits[size];
	for(int i=0; i<size; i++) {
		digits[size-i] = d % mod;
		d /= 10;
	}
	for(int j=0; j<size-1; j += 2) {
		if(j == size-2)
			printf("%d%d.%d", digits[j], digits[j+1], digits[j+2]);
		else
			printf("%d%d:",digits[j], digits[j+1]);
	}
}

char* getFullDirection(char c) {
	char* result = malloc(sizeof(10));
	if(!(c - 'e') || !(c - 'E'))
		result = "East";
	else if(!(c - 'w') || !(c - 'W'))
		result = "West";
	return result;
}


void printStation(int stationId) {
	struct node* sptr;
	printf("At Station %d: ", stationId);
	if(stationId == 1)
		sptr = q1;
	else if(stationId == 2)
		sptr = q2;
	else if(stationId == 3)
		sptr = q3;
	else if(stationId == 4)
		sptr = q4;
	if(sptr != NULL) {
		 do {
			printf("[%d] ", sptr->id);
			sptr = sptr->next;
		} while(sptr != NULL);
	}
	printf("\n");
}

void printStations() {
	for(int i=1; i<=MAX_STATIONS; i++) {
		printStation(i);
	}
}

void queueTrain(train *tptr) {
	int id = tptr->id;
	char direction = tptr->direction;
	pthread_mutex_lock(&master);
	if(!(direction - 'E')) {
		enqueue(1, id);
	}
	else if(!(direction - 'e')) {
		enqueue(2, id);
	}
	else if(!(direction - 'W')) {
		enqueue(3, id);
	}
	else if(!(direction - 'w')) {
		enqueue(4, id);
	}
	printStations();
	pthread_mutex_unlock(&master);
}


void *train_does(void *ptr){
	while(!start) {
		pthread_cond_signal(&startSignal);
	}
	train *tptr = (train *)ptr;
	usleep(tptr->load_time * SLEEP_OFFSET);
	printTime(tptr->load_time);
	printf(" Train %2d is ready to go %4s\n", tptr->id, getFullDirection(tptr->direction));


	//------------------------------might be a problem---------------
	// pthread_mutex_lock(&master);
	// while(!enqueueing) {
	// 	pthread_cond_wait(&stationSignal, &master);
	// }
	// queueTrain(tptr);
	// pthread_cond_signal(&crossSignal);
	// enqueueing = 1;
	// pthread_mutex_unlock(&master);

	//---------------------------------------------------------------

	return NULL;
}

int getStationId(train* tptr) {
	if(tptr != NULL) {
		char direction = tptr->direction;
		if(!(direction - 'E'))
			return 1;
		else if(!(direction - 'e'))
			return 2;
		else if(!(direction - 'W'))
			return 3;
		else if(!(direction - 'w'))
			return 4;
	}
	return 0;
}

void dispatchNextTrain(train* tptr) {
	tptr->hasCrossSignal = 1;
	dequeue(getStationId(tptr));
	tptr->hasCrossed = 1;
}

int main(int argc, char *argv[]) {
	train trainList[MAX_SIZE];
	int n = readInput(argv[1], trainList);
	pthread_t train_threads[n];
	pthread_mutex_init(&loadLock, NULL);
	pthread_mutex_init(&master, NULL);
	pthread_mutex_init(&mainTrack, NULL);


	if(n == -1) {
		printf("File not found.\n");
		return 0;
	}

  	for(int i=0; i<n; ++i) {
	    pthread_create(&train_threads[i], NULL, train_does, &trainList[i]);
  	}

	pthread_mutex_lock(&loadLock);
  	start = 1;
	pthread_cond_broadcast(&startSignal);
	pthread_mutex_unlock(&loadLock);


	pthread_t controller;

//-----------------------------------------------------------------
	// pthread_mutex_lock(&master);
	// while(enqueueing) {
	// 	pthread_cond_wait(&crossSignal, &master);
	// }
	// pthread_cond_signal(&stationSignal);
	// // dispatchNextTrain(trainList);
	// // enqueueing = 0;
	// pthread_mutex_unlock(&master);
//-----------------------------------------------------------------

	
	//debugging
	printTrain(trainList, n);


	//clean up
	for(int i=0; i<n; i++) {
		pthread_join(train_threads[i], NULL);
	}
	pthread_cond_destroy(&startSignal);
	pthread_cond_destroy(&crossSignal);
	pthread_cond_destroy(&stationSignal);
  	pthread_mutex_destroy(&loadLock);
	pthread_mutex_destroy(&mainTrack);
  	pthread_mutex_destroy(&master);


	return 0;
}
