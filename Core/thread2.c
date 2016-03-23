#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <libxml/parser.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>
#include  <signal.h>
#include  <sys/ipc.h>


/*
void sig_handler(int signal) {
	if (signal == SIGINT) {
		printf("recived SIGINT, my pid = %i\n", getpid());
	}
	
}*/
struct ControlTime{
   int  pid; 
   int  miliSeconds; //amount of ms
};

void *functionThread (void *parameter);



//sleep miliseconds
void sleep_ms(int milliseconds)
{
	struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

//this function is what the thread that controls the time will execute, it takes PID and time in miliseconds from parameter.
//If time is acomplished it will proced to kill the proces with pid ecual to the pid in parameter.
void *functionThread (void *parameter)
{
	struct ControlTime *control = parameter;
	sleep_ms(control->miliSeconds);
	kill(control->pid, SIGINT);
}
int TemporalAnalisisOfStudentSample(int idProblem) {
	double time_spent;
	struct timeval begin, end;
	int status;
	
	FILE *fp;
	char line[128];
	char input[30];
	char ruta[30];
	
	int i, ret;
	int numberOfTests = 2;
	int timePassed = 1;
	int fd5 = open("result.txt", O_RDWR | O_APPEND | O_CREAT, 0644);
	if (fd5 < 0) {
		perror("Error on open 13\n");
		//exit(EXIT_FAILURE);
	}
	dup2(fd5, 1);   // make stexit go to file 
	pthread_t idThread;
	int error;
	intptr_t milliseconds;
	struct ControlTime control;
	close(fd5);
	for (i = 0; i < 1; i++) {
		sprintf(ruta, "./TESTS/%i/publics/%i",idProblem, i);
		gettimeofday(&begin, NULL); 
		ret = fork();
		if (ret == 0) { //child
			int fd6 = open("child.txt", O_RDWR | O_APPEND | O_CREAT, 0644);
			if (fd6 < 0) {
				perror("Error on open 13\n");
				//exit(EXIT_FAILURE);
			}
			dup2(fd6, 1);   // make stexit go to file 
			printf("CHILD with PID = %i, and time = %i\n", getpid(), 10*2*10); 
			
			int fd7 = open("childMute.txt", O_RDWR | O_APPEND | O_CREAT, 0644);
			if (fd7 < 0) {
				perror("Error on open 13\n");
				//exit(EXIT_FAILURE);
			}
			dup2(fd7, 1);   // make stexit go to file 
			execlp("./CodeCompilated", "./CodeCompilated", NULL);

		} else if (ret > 0) { //parent
			control.pid = ret;
			control.miliSeconds = 10000;
			error = pthread_create(&idThread, NULL, functionThread,  &control);
			if (error != 0)
			{
				perror ("No puedo crear thread");
				exit (-1);
			}
			wait(&status);
			printf("PARENT\n"); 
			printf("status = %i\n", status); 
			gettimeofday(&end, NULL);
			time_spent=(end.tv_sec - begin.tv_sec)*1000 + (end.tv_usec - begin.tv_usec)/1000.0;
			printf("\t\t<publicTime%i> OK: %f miliseconds </publicTime%i>\n", i, time_spent, i); 

		} else if (ret == -1) {
			perror("Fallo en fork()\n");
			//exit(EXIT_FAILURE);
		}
	}
}


main()
{
	TemporalAnalisisOfStudentSample(1);
}
