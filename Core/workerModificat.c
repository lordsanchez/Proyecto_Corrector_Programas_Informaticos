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

#define maxLengthIdWorker  3 //LengthNumber(numberWorkers) 	// it has to be equal to the length of the maximum identifier used in a worker, the identifieres go from 0 to max number of workers. 
#define maxLengthIpNames 10 	//it establish tha maximum number of characters that would have any ip in the sistem
#define maxLengthPortNames 10	//it establish tha maximum number of characters that would have any name of a port in the sistem

int needPassAll = -1;
int numberPublicTests = -1;
int numberPrivateTests = -1;
float limit = -1;
int compilerSample = -1;

//----------------------------used for time control
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


//-----------------------
char *trim(char *s) //used to parse xml files
{
  char *start = s;
  while(*start && isspace(*start))
    ++start;
  char *i = start;
  char *end = start;

  while(*i)
  {
    if( !isspace(*(i++)) )
      end = i;
  }
  *end = 0;
  return start;
}

int getInfoAboutProblem(int idProblem) { //given the id of the problem, this function gets the value of the variables needPassAll,
							//numberPublicTest, and numberPrivateTest of the problem.
							//If the folder with the id of the problem dosen't exist, then it returns -1.
	xmlDoc *document;
	xmlNode *root, *first_child, *node;
	xmlChar *value;
	char line[128];
	sprintf(line, "./TESTS/%i/info.xml", idProblem);
	FILE *fp;
	fp = fopen(line, "r");
	if(fp == NULL) {
		return -1;
	}
	fclose(fp);
	document = xmlReadFile(line, NULL, 0);
	root = xmlDocGetRootElement(document);
	
	first_child = root->children;
	for (node = first_child; node; node = node->next) {
		value=xmlNodeGetContent(node);
		
		if (strcmp(node->name, "needPassAll") == 0) {
			needPassAll = atoi(trim((char*)value));
		
		} else if (strcmp(node->name, "numberPublicTests") == 0) {
			numberPublicTests = atoi(trim((char*)value));
		
		} else if (strcmp(node->name, "numberPrivateTests") == 0) {
			numberPrivateTests = atoi(trim((char*)value));
		
		} else if (strcmp(node->name, "limit") == 0) {
			limit = atof(trim((char*)value));
		
		} else if (strcmp(node->name, "compilerSample") == 0) {
			compilerSample = atoi(trim((char*)value));
		} 
    }
	return 1;
}

//this function is used to see if the worker has all the data needed to resolve a problem, in case that
//it hasn't the data, then the problem data is resquested.
void getInfo(int Socket_Worker_Entrada, int idProblem) {
	int problemInWorker = getInfoAboutProblem(idProblem);
	printf("problemInWorker = %i. I have the info of the problem. needPassAll= %i, numberPublicTest = %i, numberPrivateTest = %i\n", problemInWorker, needPassAll, numberPublicTests, numberPrivateTests);
	if (problemInWorker == -1) {
		char idProblemChar[10];
		sprintf(idProblemChar, "%i", idProblem);
		SendMessage(Socket_Worker_Entrada, 8,  idProblemChar, NULL, NULL); //REQUEST PROBLEM
		
		printf("starting reciving file\n");	 
		FILE *pipe2;
		char line[128];
		int linenr;
		sprintf(line, "nc -l -p 1234 > lastTestRecived.tar.gz");
		printf("comand: %s\n", line);
		pipe2 = popen(line, "r");
		if (pipe2 == NULL) {  /* check for errors */
			perror("Error on pipe opening for the add problem operation()\n");
		}
		
		linenr = 1;
		while (fgets(line, 128, pipe2) != NULL) {
			printf("Script output line %d: %s", linenr, line);
			++linenr;
		}
		pclose(pipe2);
		printf("end reciving file\n");
		//unzip the test
		FILE *pipe;
		int compresor = 1; //tar.gz unique avaiable until the moment
		
		sprintf(line, "./unzipWorker %s %i %i", "lastTestRecived.tar.gz", idProblem, compresor);
		printf("comand: %s\n", line);
		pipe = popen(line, "r");
		if (pipe == NULL) {  /* check for errors */
			perror("Error onpip opening for the add problem operation()\n");
		}
		
		linenr = 1;
		while (fgets(line, 128, pipe) != NULL) {
			printf("Script output line %d: %s", linenr, line);
			++linenr;
		}
		pclose(pipe);
		int problemInWorker = getInfoAboutProblem(idProblem);
		printf("needPassALL = %i\n", needPassAll);
		printf("numberPublicTests = %i\n", numberPublicTests);
		printf("numberPrivateTests = %i\n", numberPrivateTests);
		printf("limit = %f\n", limit);
	}else {
		printf("needPassALL = %i\n", needPassAll);
		printf("numberPublicTests = %i\n", numberPublicTests);
		printf("numberPrivateTests = %i\n", numberPrivateTests);
		printf("limit = %f\n", limit);
		SendMessage(Socket_Worker_Entrada, 9, NULL, NULL, NULL); //OK
	}
}

void CompileProblem(int compiler, int compileProblemFromProfessor, int idProblem) { //this function is used to mute the process to gcc or g++ and compile the program requested with the compiler.
	if (compileProblemFromProfessor == 0) { //compile a problem submission of a student
		int fd2 = open("error.txt", O_RDWR | O_CREAT | O_TRUNC, 0644);
		if (fd2 < 0) {
			perror("Fallo en open() num14\n");
			//exit(EXIT_FAILURE);
		}
		dup2(fd2, 2);   // make stderr go to file 
		close(fd2);
		if(compiler == 0) {
			execlp("gcc", "gcc", "codeRecived.c", "-o", "CodeCompilated",  NULL);
		
		} else if (compiler == 1) {
			execlp("g++", "g++", "codeRecived.cpp", "-o", "CodeCompilated",  NULL);
		}
	} else if (compileProblemFromProfessor == 1) {
		if(compilerSample == 0) {
			char line[50];
			sprintf(line, "./TESTS/%i/correct.c", idProblem);
			execlp("gcc", "gcc", line, "-o", "CodeProfessorCompilated",  NULL);
		
		} else if (compilerSample == 1) {
			char line[50];
			sprintf(line, "./TESTS/%i/correct.cpp", idProblem);
			execlp("g++", "g++", line, "-o", "CodeProfessorCompilated",  NULL);
		}
	}
}


int TestStopingAtFirstError(int idProblem){
		//geting time 
	int numberOfTests = numberPublicTests;
	char input[30];
	char ruta[30];
	char output[30];
	char comentari[100];
	int status;
	int error = 0;
	int i, ret; 
	for (i = 0; i < numberOfTests && error == 0; i++) {
		sprintf(ruta, "./TESTS/%i/publics/%i",idProblem, i);
		ret = fork();
		if (ret == 0) { //child
		
			int fd3 = open("out.txt", O_RDWR | O_CREAT | O_TRUNC, 0777);
			if (fd3 < 0) {
				perror("Error on open 1\n");
				//exit(EXIT_FAILURE);
			}
			dup2(fd3, 1);   
			close(fd3);
		
			sprintf(input, "%s.in", ruta);
			int fd4 = open(input, O_RDONLY, 0777);
			if (fd4 < 0) {
				perror("Error on open 2\n");
				//exit(EXIT_FAILURE);
			}
			dup2(fd4, 0);
			close(fd4);
			execlp("./CodeCompilated", "./CodeCompilated", NULL);

		} else if (ret > 0) { //parent
			wait(&status);
			
			if (status == 0) {
				ret = fork();
				if (ret == 0) { //child
					int fd5 = open("diff.txt", O_RDWR | O_CREAT | O_TRUNC, 0777);
					if (fd5 < 0) {
						perror("Error on open 3\n");
						//exit(EXIT_FAILURE);
					}
					dup2(fd5, 1);  
					close(fd5);
					sprintf(output, "%s.out", ruta);
					execlp("diff", "diff", output, "./out.txt", NULL);					
				
				} else if (ret > 0){
					wait(&status);
					if (status != 0) { // diff ha sigut diferent
						error = 1;
						int fd5 = open("result.txt", O_RDWR | O_CREAT | O_APPEND, 0777);
						if (fd5 < 0) {
							perror("Error on open 4\n");
							//exit(EXIT_FAILURE);
						}
						dup2(fd5, STDOUT_FILENO);  
						close(fd5);
						ret = fork();
						if (ret == 0) { //child
							printf("\t<public%i>\n", i); 
							sprintf(comentari, "%s.com", ruta);
							printf("\t\t<comentari%i>\n", i);  
							printf("\t\t there are differneces in this test\n"); 
							execlp("cat", "cat", "result.txt", comentari, "diff.txt",NULL);
							
						} else if (ret > 0){ //parent
							wait(&status);
							printf("\t\t</comentari%i>\n", i);
							printf("\t</public%i>\n", i);  
							
						}
					} else { // diff igual
						int fd5 = open("result.txt", O_RDWR | O_CREAT |  O_APPEND, 0777);
						if (fd5 < 0) {
							perror("Error on open 5()\n");
							//exit(EXIT_FAILURE);
						}
						dup2(fd5, STDOUT_FILENO);  
						close(fd5);
						printf("\t<public%i>\n", i);  
						printf("\t\t<comentari%i> Well done, there are no differences between the solutions in this test</comentari%i>\n", i, i);  
						printf("\t</public%i>\n", i); 
						
						
					}
				} else if (ret == -1) {
					perror("Error on fork \n");
					//exit(EXIT_FAILURE);
				}
			} else { 
				error = 1;
				int fd5 = open("result.txt", O_RDWR | O_CREAT |  O_APPEND, 0777);
				if (fd5 < 0) {
					perror("Error on open 6\n");
					//exit(EXIT_FAILURE);
				}
				dup2(fd5, STDOUT_FILENO);  
				close(fd5);
				printf("\t<error> The return code of your program is diferent than 0, if you are using C lenguage, you must finish the main routine with a return 0 </error>\n");
				printf("</result>\n");
			}
		//recull el resultat en el fitxer que envio
		
		} else if (ret == -1) {
			error = 1;
			perror("Error on forks\n");
			//exit(EXIT_FAILURE);
		}
	}
	numberOfTests = numberPrivateTests;
	for (i = 0; i < numberOfTests && error == 0; i++) {
		sprintf(ruta, "./TESTS/%i/privates/%i",idProblem, i);
		ret = fork();
		if (ret == 0) { //child
			int fd3 = open("out.txt", O_RDWR | O_CREAT | O_TRUNC, 0777);
			if (fd3 < 0) {
				perror("Error on open 7\n");
				//exit(EXIT_FAILURE);
			}
			dup2(fd3, 1);   
			close(fd3);
		
			sprintf(input, "%s.in", ruta);
			int fd4 = open(input, O_RDONLY, 0777);
			if (fd4 < 0) {
				perror("Error on open 8\n");
				//exit(EXIT_FAILURE);
			}
			dup2(fd4, 0);
			close(fd4);
			execlp("./CodeCompilated", "./CodeCompilated", NULL);

		} else if (ret > 0) { //parent
			wait(&status);
			if (status == 0) {
				ret = fork();
				if (ret == 0) { //child
					int fd5 = open("diff.txt", O_RDWR | O_CREAT | O_TRUNC, 0777);
					if (fd5 < 0) {
						perror("Error on open\n");
						//exit(EXIT_FAILURE);
					}
					dup2(fd5, 1);  
					close(fd5);
					sprintf(output, "%s.out", ruta);
					execlp("diff", "diff", output, "./out.txt", NULL);					
				
				} else if (ret > 0) {
					wait(&status);
					if (status != 0) { // diff ha sigut diferent
						error = 1;
						int fd5 = open("result.txt", O_RDWR | O_CREAT | O_APPEND, 0777);
						if (fd5 < 0) {
								perror("Error on open 10\n");
								//exit(EXIT_FAILURE);
							}
						dup2(fd5, STDOUT_FILENO);  
						close(fd5);
						ret = fork();
						if (ret == 0) {
							printf("\t<private%i>\n", i);  
							printf("\t\t<comentari%i>\n", i); 
							sprintf(comentari, "./TESTS/%i/privates/comentari.txt", i);
							execlp("cat", "cat", "result.txt", comentari, NULL);
						} else {
							wait(&status);
							printf("\t\t</comentari%i>\n", i); 
							printf("\t</private%i>\n", i);  
						}					
					
					} else { // diff igual
						int fd5 = open("result.txt", O_RDWR | O_CREAT |  O_APPEND, 0777);
						if (fd5 < 0) {
							perror("Error on open 11\n");
							//exit(EXIT_FAILURE);
						}
						dup2(fd5, STDOUT_FILENO);  
						close(fd5);
						printf("\t<private%i>\n", i);  
						printf("\t\t<comentari%i> Well done, there are no differences between the solutions in this test</comentari%i>\n", i, i);  
						printf("\t</private%i>\n", i);  	
					}
				
				} else if (ret == -1) {
					error = 1;
					perror("Error on fork\n");
					//exit(EXIT_FAILURE);
				}
			} else { 
				error = 1;
				int fd5 = open("result.txt", O_RDWR | O_CREAT |  O_APPEND, 0777);
				if (fd5 < 0) {
					perror("Error on open 12\n");
					//exit(EXIT_FAILURE);
				}
				dup2(fd5, STDOUT_FILENO);  
				close(fd5);
				printf("\t<error> The return code of your program is diferent than 0, if you are using C lenguage, you must finish the main routine with a return 0 </error>\n");
				printf("</result>\n");  
			}
		//recull el resultat en el fitxer que envio
		
		} else if (ret == -1) {
			perror("Error on fork\n");
			//exit(EXIT_FAILURE);
		}
	}
	
	if (error == 0) return 1;
	return -1;
}

int TestNonStopingAtError (int idProblem){
	int numberOfTests = numberPublicTests;
	char input[30];
	char ruta[30];
	char output[30];
	char comentari[100];
	int status;
	int error = 0;
	int i, ret; 
	for (i = 0; i < numberOfTests; i++) {
		sprintf(ruta, "./TESTS/%i/publics/%i",idProblem, i);
		ret = fork();
		if (ret == 0) { //child
		
			int fd3 = open("out.txt", O_RDWR | O_CREAT | O_TRUNC, 0777);
			if (fd3 < 0) {
				perror("Error on open 1\n");
				//exit(EXIT_FAILURE);
			}
			dup2(fd3, 1);   
			close(fd3);
		
			sprintf(input, "%s.in", ruta);
			int fd4 = open(input, O_RDONLY, 0777);
			if (fd4 < 0) {
				perror("Error on open 2\n");
				//exit(EXIT_FAILURE);
			}
			dup2(fd4, 0);
			close(fd4);
			execlp("./CodeCompilated", "./CodeCompilated", NULL);

		} else if (ret > 0) { //parent
			wait(&status);
			
			if (status == 0) {
				ret = fork();
				if (ret == 0) { //child
					int fd5 = open("diff.txt", O_RDWR | O_CREAT | O_TRUNC, 0777);
					if (fd5 < 0) {
						perror("Error on open 3\n");
						//exit(EXIT_FAILURE);
					}
					dup2(fd5, 1);  
					close(fd5);
					sprintf(output, "%s.out", ruta);
					execlp("diff", "diff", output, "./out.txt", NULL);					
				
				} else if (ret > 0){
					wait(&status);
					if (status != 0) { // diff ha sigut diferent
						error = 1;
						int fd5 = open("result.txt", O_RDWR | O_CREAT | O_APPEND, 0777);
						if (fd5 < 0) {
							perror("Error on open 4\n");
							//exit(EXIT_FAILURE);
						}
						dup2(fd5, STDOUT_FILENO);  
						close(fd5);
						ret = fork();
						if (ret == 0) { //child
							printf("\t<public%i>\n", i); 
							sprintf(comentari, "%s.com", ruta);
							printf("\t\t<comentari%i>\n", i);  
							printf("\t\t there are differneces in this test\n"); 
							execlp("cat", "cat", "result.txt", comentari, "diff.txt",NULL);
							
						} else if (ret > 0){ //parent
							wait(&status);
							printf("\t\t</comentari%i>\n", i);
							printf("\t</public%i>\n", i);  
							
						}
					} else { // diff igual
						int fd5 = open("result.txt", O_RDWR | O_CREAT |  O_APPEND, 0777);
						if (fd5 < 0) {
							perror("Error on open 5()\n");
							//exit(EXIT_FAILURE);
						}
						dup2(fd5, STDOUT_FILENO);  
						close(fd5);
						printf("\t<public%i>\n", i);  
						printf("\t\t<comentari%i> Well done, there are no differences between the solutions in this test</comentari%i>\n", i, i);  
						printf("\t</public%i>\n", i); 
						
						
					}
				} else if (ret == -1) {
					perror("Error on fork \n");
					//exit(EXIT_FAILURE);
				}
			} else { 
				error = 1;
				int fd5 = open("result.txt", O_RDWR | O_CREAT |  O_APPEND, 0777);
				if (fd5 < 0) {
					perror("Error on open 6\n");
					//exit(EXIT_FAILURE);
				}
				dup2(fd5, STDOUT_FILENO);  
				close(fd5);
				printf("\t<error> The return code of your program is diferent than 0, if you are using C lenguage, you must finish the main routine with a return 0 </error>\n");
				printf("</result>\n");
			}
		//recull el resultat en el fitxer que envio
		
		} else if (ret == -1) {
			error = 1;
			perror("Error on forks\n");
			//exit(EXIT_FAILURE);
		}
	}
	numberOfTests = numberPrivateTests;
	for (i = 0; i < numberOfTests; i++) {
		sprintf(ruta, "./TESTS/%i/privates/%i",idProblem, i);
		ret = fork();
		if (ret == 0) { //child
			int fd3 = open("out.txt", O_RDWR | O_CREAT | O_TRUNC, 0777);
			if (fd3 < 0) {
				perror("Error on open 7\n");
				//exit(EXIT_FAILURE);
			}
			dup2(fd3, 1);   
			close(fd3);
		
			sprintf(input, "%s.in", ruta);
			int fd4 = open(input, O_RDONLY, 0777);
			if (fd4 < 0) {
				perror("Error on open 8\n");
				//exit(EXIT_FAILURE);
			}
			dup2(fd4, 0);
			close(fd4);
			execlp("./CodeCompilated", "./CodeCompilated", NULL);

		} else if (ret > 0) { //parent
			wait(&status);
			if (status == 0) {
				ret = fork();
				if (ret == 0) { //child
					int fd5 = open("diff.txt", O_RDWR | O_CREAT | O_TRUNC, 0777);
					if (fd5 < 0) {
						perror("Error on open\n");
						//exit(EXIT_FAILURE);
					}
					dup2(fd5, 1);  
					close(fd5);
					sprintf(output, "%s.out", ruta);
					execlp("diff", "diff", output, "./out.txt", NULL);					
				
				} else if (ret > 0) {
					wait(&status);
					if (status != 0) { // diff ha sigut diferent
						error = 1;
						int fd5 = open("result.txt", O_RDWR | O_CREAT | O_APPEND, 0777);
						if (fd5 < 0) {
								perror("Error on open 10\n");
								//exit(EXIT_FAILURE);
							}
						dup2(fd5, STDOUT_FILENO);  
						close(fd5);
						ret = fork();
						if (ret == 0) {
							printf("\t<private%i>\n", i);  
							printf("\t\t<comentari%i>\n", i); 
							sprintf(comentari, "./TESTS/%i/privates/comentari.txt", i);
							execlp("cat", "cat", "result.txt", comentari, NULL);
						} else {
							wait(&status);
							printf("\t\t</comentari%i>\n", i); 
							printf("\t</private%i>\n", i);  
						}					
					
					} else { // diff igual
						int fd5 = open("result.txt", O_RDWR | O_CREAT |  O_APPEND, 0777);
						if (fd5 < 0) {
							perror("Error on open 11\n");
							//exit(EXIT_FAILURE);
						}
						dup2(fd5, STDOUT_FILENO);  
						close(fd5);
						printf("\t<private%i>\n", i);  
						printf("\t\t<comentari%i> Well done, there are no differences between the solutions in this test</comentari%i>\n", i, i);  
						printf("\t</private%i>\n", i);  	
					}
				
				} else if (ret == -1) {
					error = 1;
					perror("Error on fork\n");
					//exit(EXIT_FAILURE);
				}
			} else { 
				error = 1;
				int fd5 = open("result.txt", O_RDWR | O_CREAT |  O_APPEND, 0777);
				if (fd5 < 0) {
					perror("Error on open 12\n");
					//exit(EXIT_FAILURE);
				}
				dup2(fd5, STDOUT_FILENO);  
				close(fd5);
				printf("\t<error> The return code of your program is diferent than 0, if you are using C lenguage, you must finish the main routine with a return 0 </error>\n");
				printf("</result>\n");  
			}
		//recull el resultat en el fitxer que envio
		
		} else if (ret == -1) {
			perror("Error on fork\n");
			//exit(EXIT_FAILURE);
		}
	}
	
	if (error == 0) return 1;
	return -1;
}

int TemporalAnalisisOfStudentSample(int idProblem) {
	float pubicsTimesPofessorTest[numberPublicTests];
	float privatesTimesPofessorTest[numberPrivateTests];
	double time_spent;
	struct timeval begin, end;
	int status;
	
	FILE *fp;
	char line[128];
	char input[30];
	char ruta[30];
	
	sprintf(line, "./time.txt", idProblem);
	fp = fopen(line, "r");
	if(fp == NULL) {
		perror("Error opening time.txt");
	}
	int linenr = 0;
	int auxIndex = 0;
	while (linenr < (numberPublicTests+numberPrivateTests)) {
		fgets(line, 128, fp);
		fgets(line, 128, fp);
		if (linenr < numberPublicTests) {
			pubicsTimesPofessorTest[auxIndex] = atof(line);
			auxIndex++;
			if (auxIndex == numberPublicTests) auxIndex = 0;
		} else  { 
			privatesTimesPofessorTest[auxIndex] = atof(line);
			auxIndex++;
		}
		++linenr;
	}
	fclose(fp);
	remove("./time.txt");
	
	int i, ret;
	int numberOfTests = numberPublicTests;
	int timePassed = 1;
	int fd5 = open("result.txt", O_RDWR | O_APPEND | O_CREAT, 0644);
	if (fd5 < 0) {
		perror("Error on open 13\n");
		//exit(EXIT_FAILURE);
	}
	dup2(fd5, 1);   // make stexit go to file 
	printf("<result>\n");  
	pthread_t idThread;
	int error;
	intptr_t milliseconds;
	struct ControlTime control;
	close(fd5);
	for (i = 0; i < numberOfTests; i++) {
		sprintf(ruta, "./TESTS/%i/publics/%i",idProblem, i);
		gettimeofday(&begin, NULL); 
		ret = fork();
		if (ret == 0) { //child
		
			int fd3 = open("out.txt", O_RDWR | O_CREAT | O_TRUNC, 0777);
			if (fd3 < 0) {
				perror("Error on open 1\n");
				//exit(EXIT_FAILURE);
			}
			dup2(fd3, 1);   
			close(fd3);
		
			sprintf(input, "%s.in", ruta);
			int fd4 = open(input, O_RDONLY, 0777);
			if (fd4 < 0) {
				perror("Error on open 2\n");
				//exit(EXIT_FAILURE);
			}
			dup2(fd4, 0);
			close(fd4);
			execlp("./CodeCompilated", "./CodeCompilated", NULL);

		} else if (ret > 0) { //parent
			control.pid = ret;
			control.miliSeconds = pubicsTimesPofessorTest[i]*limit*100;
			error = pthread_create(&idThread, NULL, functionThread,  &control);
			if (error != 0)
			{
				perror ("Can't create thread");
				exit (-1);
			}
			wait(&status);
			gettimeofday(&end, NULL);
			time_spent=(end.tv_sec - begin.tv_sec)*1000 + (end.tv_usec - begin.tv_usec)/1000.0;
			if (status == 2) { //The limit of time surpased and execution terminated by control
				printf("\t\t<publicTime%i> FAIL, process TERMINATED you exceded the maximum execution time allowed, check you aren't doing and infinite loop t: %f miliseconds </publicTime%i>\n", i, time_spent, i); 
				timePassed = 0;
					
			} else if (time_spent <= pubicsTimesPofessorTest[i]*limit) {
				//printf("\t\t<publicTime%i> OK: %f miliseconds </publicTime%i>\n", i, time_spent, i); 

			} else {	
				timePassed = 0;
				printf("\t\t<publicTime%i> FAIL: %f miliseconds </publicTime%i>\n", i, time_spent, i); 
				printf("HELP professor time = %f miliseconds, with limit aplied = %f\n", pubicsTimesPofessorTest[i], pubicsTimesPofessorTest[i]*limit); 
			}

		} else if (ret == -1) {
			perror("Fallo en fork()\n");
			//exit(EXIT_FAILURE);
		}
	}
	numberOfTests = numberPrivateTests;
	for (i = 0; i < numberOfTests; i++) {
		sprintf(ruta, "./TESTS/%i/privates/%i",idProblem, i);
		gettimeofday(&begin, NULL); 
		ret = fork();
		if (ret == 0) { //child
			int fd3 = open("out.txt", O_RDWR | O_CREAT | O_TRUNC, 0777);
			if (fd3 < 0) {
				perror("Error on open\n");
				//exit(EXIT_FAILURE);
			}
			dup2(fd3, 1);   
			close(fd3);
		
			sprintf(input, "%s.in", ruta);
			int fd4 = open(input, O_RDONLY, 0777);
			if (fd4 < 0) {
				perror("Error en open\n");
				//exit(EXIT_FAILURE);
			}
			
			dup2(fd4, 0);
			close(fd4);
			execlp("./CodeCompilated", "./CodeCompilated", NULL);

		} else if (ret > 0) { //parent
			control.pid = ret;
			control.miliSeconds = privatesTimesPofessorTest[i]*100*limit;
			error = pthread_create(&idThread, NULL, functionThread,  &control);
			if (error != 0)
			{
				perror ("Can't create thread");
				exit (-1);
			}
			wait(&status);
			gettimeofday(&end, NULL);
			time_spent=(end.tv_sec - begin.tv_sec)*1000 + (end.tv_usec - begin.tv_usec)/1000.0;
			if (status == 2) { //The limit of time surpased and execution terminated by control
				printf("\t\t<privateTime%i> FAIL, process TERMINATED you exceded the maximum execution time allowed, check you aren't doing and infinite loop t: %f miliseconds </privateTime%i>\n", i, time_spent, i); 
				timePassed = 0;
			}else if (time_spent <= privatesTimesPofessorTest[i]*limit) {
				//printf("\t\t<privateTime%i> OK: %f miliseconds </privateTime%i>\n", i, time_spent, i); 

			} else {	
				timePassed = 0;
				printf("\t\t<privateTime%i> FAIL: %f miliseconds </privateTime%i>\n", i, time_spent, i); 
				printf("HELP professor time = %f miliseconds, with limit aplied = %f\n", privatesTimesPofessorTest[i], privatesTimesPofessorTest[i]*limit); 
			}
					
		} else if (ret == -1) {
			perror("Fallo en fork()\n");
			//exit(EXIT_FAILURE);
		}			
	}	
	if (timePassed == 1) return 1;
	return -1;
}


//this function is used by the worker to know the execution time of the problem using a corret code provided by the professor.
//After this execution, the sistem will compare this execution time with the ones generated by the students code. 
void TemporalAnalisisOfProfessorSample(int idProblem) { //it calculates the times of the diferent tests in miliseconds. 10^-3 seconds.
	
	double time_spent;
	struct timeval begin, end;

	//public
	int numberOfTests = numberPublicTests;
	char input[30];
	char ruta[30];
	char output[30];
	char comentari[100];
	int status;
	int i, ret;
	for (i = 0; i < numberOfTests; i++) {
		sprintf(ruta, "./TESTS/%i/publics/%i",idProblem, i);
		gettimeofday(&begin, NULL); 
		ret = fork();
		if (ret == 0) { //child
		
			int fd3 = open("out.txt", O_RDWR | O_CREAT | O_TRUNC, 0777);
			if (fd3 < 0) {
				perror("Error on open 1\n");
				//exit(EXIT_FAILURE);
			}
			dup2(fd3, 1);   
			close(fd3);
		
			sprintf(input, "%s.in", ruta);
			int fd4 = open(input, O_RDONLY, 0777);
			if (fd4 < 0) {
				perror("Error on open 2\n");
				//exit(EXIT_FAILURE);
			}
			dup2(fd4, 0);
			close(fd4);
			execlp("./CodeProfessorCompilated", "./CodeProfessorCompilated", NULL);

		} else if (ret > 0) { //parent
			wait(&status);
			gettimeofday(&end, NULL);
			time_spent=(end.tv_sec - begin.tv_sec)*1000 + (end.tv_usec - begin.tv_usec)/1000.0;
			int fd5 = open("time.txt", O_RDWR | O_CREAT |  O_APPEND, 0777);
			if (fd5 < 0) {
				perror("Error on open 5\n");
				//exit(EXIT_FAILURE);
			}
			dup2(fd5, STDOUT_FILENO);  
			close(fd5);
			printf("TEST PUBLIC %i CORRECT\n", i);
			printf("%f\n", time_spent);

		} else if (ret == -1) {
			perror("Error on fork\n");
			//exit(EXIT_FAILURE);
		}
	}
	numberOfTests = numberPrivateTests;
	for (i = 0; i < numberOfTests; i++) {
		sprintf(ruta, "./TESTS/%i/privates/%i",idProblem, i);
		gettimeofday(&begin, NULL); 
		ret = fork();
		if (ret == 0) { //child
			int fd3 = open("out.txt", O_RDWR | O_CREAT | O_TRUNC, 0777);
			if (fd3 < 0) {
				perror("Error on open 7\n");
				//exit(EXIT_FAILURE);
			}
			dup2(fd3, 1);   
			close(fd3);
		
			sprintf(input, "%s.in", ruta);
			int fd4 = open(input, O_RDONLY, 0777);
			if (fd4 < 0) {
				perror("Error on open 8\n");
				//exit(EXIT_FAILURE);
			}
			dup2(fd4, 0);
			close(fd4);
			execlp("./CodeProfessorCompilated", "./CodeProfessorCompilated", NULL);

		} else if (ret > 0) { //parent
			wait(&status);
			gettimeofday(&end, NULL);
			time_spent=(end.tv_sec - begin.tv_sec)*1000 + (end.tv_usec - begin.tv_usec)/1000.0;
			int fd5 = open("time.txt", O_RDWR | O_CREAT |  O_APPEND, 0777);
			if (fd5 < 0) {
				perror("Error on open 11\n");
				//exit(EXIT_FAILURE);
			}
			dup2(fd5, STDOUT_FILENO);  
			close(fd5);
			printf("TEST PRIVATE %i CORRECT\n", i);	
			printf("%f\n", time_spent);		
				
		} else if (ret == -1) {
			perror("Fallo en fork()\n");
			//exit(EXIT_FAILURE);
		}			
	}
}


int main (int argc, char *argv[])
{
	
	if(argc!=5){
		printf("Numero de parametros incorrectos. Uso: ./[ipGestor][portGestor][ipWorker(Meva)][portWorker(meu)]\n");
		perror("fallo en parametros");
	}
	
	if (strlen(argv[1]) > maxLengthIpNames) {
		printf("Incorrect length of the ip name, it is too length\n");
		sleep(5);
		perror("fail in the length of the ip with workers");
	}
	
	if (strlen(argv[2]) > maxLengthPortNames) {
		printf("Incorrect length of the port name, it is too length\n");
		sleep(5);
		perror("fail in the length of the port with clients");
	}
	
	if (strlen(argv[3]) > maxLengthIpNames) {
		printf("Incorrect length of the ip name, it is too length\n");
		sleep(5);
		perror("fail in the length of the ip with workers");
	}
	
	if (strlen(argv[4]) > maxLengthPortNames) {
		printf("Incorrect length of the port name, it is too length\n");
		sleep(5);
		perror("fail in the length of the port with clients");
	}
	
	int ret, i;
	int status;
	int compiler, idProblem;
	int testingAddProblem = 0;
	int problemInProfessorProblem = 0;
	int Socket_Manager_Worker, Socket_Worker_Entrada;
	int Socket_Worker_Client;
	
	int messageCode = -1;
	char id[maxLengthIdWorker] = "";
	char ip[maxLengthIpNames] = "";
	char port[maxLengthPortNames] = "";

	sprintf(ip,"%s", argv[3]);
	sprintf(port,"%s", argv[4]);
	
	FILE *pipe;
	char line[128];
	int linenr1, linenr;
	
	Socket_Manager_Worker = Cliente_Abre_Conexion_Inet (argv[1], argv[2]);
	if (Socket_Manager_Worker == -1)
	{
		printf ("WORKER: Error connecting with the manager \n");
		perror("Error connecting with the manager");
	}
	
	SendMessage(Socket_Manager_Worker, 5,  id, ip, port); //BEGIN READY
	ReceiveMessage(Socket_Manager_Worker, &messageCode, id, ip, port); //ASSIGNAR ID
	int myID = atoi(id);
	printf("My id == %s, my ip==%s and my port==%s, myID = %i\n", id, ip, port, myID);
	
    Socket_Worker_Client = Servidor_Abre_Socket_Inet(argv[4]);
    if (Socket_Worker_Client == -1)
	{
		printf ("Can't connect with the client manager %s\n", argv[5]);
	}
	//the worker modifies the output to write the answers of the problems, the standard output is preserved this way.
	int saved_stdout;
	saved_stdout = dup(1);
    while(1) {
		//restores the standard output
		dup2(saved_stdout, 1);
		close(saved_stdout);
		
		Socket_Worker_Entrada = Servidor_Acepta_Conexion_Del_Cliente(Socket_Worker_Client);
		if (Socket_Worker_Entrada == -1)
		{
			printf ("Can't open a socket to connect with the server\n");
			exit (-1);
		}
		testingAddProblem = 0;
		printf("Client connected\n");
		RecieveFile(Socket_Worker_Entrada, &idProblem, &compiler); 
		printf("Worker: idProblem = %i, compiler = %i\n", idProblem, compiler);
		
		getInfo(Socket_Worker_Entrada, idProblem);
		if (compiler == 2 || compiler == 3) {
			compiler = compiler-2;
			testingAddProblem = 1;
			printf("Worker: Proced to test if the test is correct, I'm returing form and add operation\n");
		}
		int testsPassed = 0;
		
		int result;
		//Compile profesor problem
		
		ret=fork();
		if(ret > 0) { /* the father process, wait until child compiles and response */
			wait(&status);
			if (status != 0) {				
				int fd5 = open("result.txt", O_RDWR | O_APPEND | O_CREAT, 0644);
				if (fd5 < 0) {
					perror("Error on open 13\n");
					//exit(EXIT_FAILURE);
				}
				dup2(fd5, 1);   // make stexit go to file 
				close(fd5); 
				printf("<result>\n");  
				printf("\t<error> The problem is not correctly created, please contact your professor or the admins to solve this problem, there is a problem with the profeesor code</error>\n");
				printf("\t<value> -1 </value>\n");
				printf("</result>\n");	
				SendFile(Socket_Worker_Entrada, -1, -1, "result.txt", myID);
				close(Socket_Worker_Entrada);
				remove("result.txt");
				problemInProfessorProblem = 1;
				SendMessage(Socket_Manager_Worker, 4, id, NULL, NULL);
				//In case that the worker was testing a new problem provided by the professor,
				//in case that the problem is not well created, the auxiliar files created to done the test must been erased.
				if (testingAddProblem == 1) {						
					sprintf(line, "rm -r ./TESTS/%i", idProblem);
					printf("line = %s\n", line);
					system(line);
				}
			}
		} else if (ret == 0) {// the first child process, compile the PROFESSOR program
			CompileProblem(compilerSample, 1, idProblem);
		} else if(ret == -1) {
			perror("Error on the fork(),\n");
			//exit(EXIT_FAILURE);
		}  
		
	
		//1 TEMPORAL ANALISIS, there is a limit time in execution time.
		if (problemInProfessorProblem == 0) {
			ret=fork();
			if(ret > 0) { /* the father process, wait until child compiles and response */
				if (compilerSample == 0 || compilerSample == 1) { //compiladors c, cpp
					wait(&status);
					if(status == 0) {
						TemporalAnalisisOfProfessorSample(idProblem);
						result = TemporalAnalisisOfStudentSample(idProblem);
						
					} else if (status != 0) {
						int fd5 = open("result.txt", O_RDWR | O_CREAT | O_TRUNC, 0644);
						if (fd5 < 0) {
							perror("Error on open\n");
							//exit(EXIT_FAILURE);
						}
						dup2(fd5, 1);   // make stexit go to file 
						close(fd5);
						ret = fork();
						if (ret == 0) { //child
							printf("<result>\n");  
							printf("\t<error> Error on compilation </error>\n");
							printf("\t<comentari>\n");	
							execlp("cat", "cat", "result.txt", "error.txt", NULL);
							
						} else if (ret > 0) {
							wait(&status);
							printf("\t</comentari>\n");
							printf("\t<value> 0 </value>\n");
							printf("</result>\n");
							SendFile(Socket_Worker_Entrada, 0, -1, "result.txt");
							close(Socket_Worker_Entrada);
							remove("result.txt");
							SendMessage(Socket_Manager_Worker, 4, id, NULL, NULL);
							if (testingAddProblem == 1) {						
								sprintf(line, "rm -r ./TESTS/%i", idProblem);
								printf("line = %s\n", line);
								system(line);
							}
						} else if (ret == -1) {
							perror("Error on fork()\n");	
						}
					}
				}
			} else if (ret == 0) {// the first child process, compile the program recieved
				CompileProblem(compilerSample, 0, idProblem);
				
			} else if(ret == -1) {
				perror("Error on the fork(),\n");
				//exit(EXIT_FAILURE);
			}  
		
			
			//SENDING RESULT if NOT PASSED TEMPORAL TEST
			if (result != 1) {
				int fd5 = open("result.txt", O_RDWR | O_APPEND | O_CREAT , 0644);
				if (fd5 < 0) {
					perror("Error on open 13\n");
					//exit(EXIT_FAILURE);
				}
				dup2(fd5, 1);   // make stexit go to file 
				close(fd5); 
				printf("\t<value> 2 </value>\n");
				printf("</result>\n");	
				SendFile(Socket_Worker_Entrada, 2, -1, "result.txt", myID);
				close(Socket_Worker_Entrada);
				remove("result.txt");
				SendMessage(Socket_Manager_Worker, 4, id, NULL, NULL);
			}

			
			if (result == 1) { //PROCED DIFF TEST, temporal is OK
				if (needPassAll) { 
					testsPassed = TestStopingAtFirstError(idProblem);
				} else { 
					testsPassed = TestNonStopingAtError(idProblem);
				}
				if (testsPassed == -1) { //some diff test had failed
					int fd5 = open("result.txt", O_RDWR | O_APPEND | O_CREAT, 0644);
					if (fd5 < 0) {
						perror("Error on open 13\n");
						//exit(EXIT_FAILURE);
					}
					dup2(fd5, 1);   // make stexit go to file 
					close(fd5);
					printf("\t<value> 1 </value>\n");
					printf("</result>\n");
					
					//Send the result to main server
					SendFile(Socket_Worker_Entrada, 1, -1, "result.txt", myID);
					close(Socket_Worker_Entrada);
					remove("result.txt");
					//Send to manager the READY message
					SendMessage(Socket_Manager_Worker, 4, id, NULL, NULL);
					//In case that the worker was testing a new problem provided by the professor,
					//in case that the problem is not well created, the auxiliar files created to done the test must been erased.
					if (testingAddProblem == 1) {						
						sprintf(line, "rm -r ./TESTS/%i", idProblem);
						printf("line = %s\n", line);
						system(line);
					}
				
				} else if (result == 1 && testsPassed) { //Time and diff passed
					int fd5 = open("result.txt", O_RDWR | O_APPEND | O_CREAT , 0644);
					if (fd5 < 0) {
						perror("Error on open 13\n");
						//exit(EXIT_FAILURE);
					}
					dup2(fd5, 1);   // make stexit go to file 
					close(fd5); 
					printf("\t<value> 3 </value>\n");
					printf("</result>\n");	
					SendFile(Socket_Worker_Entrada, 3, -1, "result.txt", myID);
					close(Socket_Worker_Entrada);
					remove("result.txt");
					SendMessage(Socket_Manager_Worker, 4, id, NULL, NULL);
			
				} else if (result == 1) { //Only time passed
					int fd5 = open("result.txt", O_RDWR | O_APPEND | O_CREAT , 0644);
					if (fd5 < 0) {
						perror("Error on open 13\n");
						//exit(EXIT_FAILURE);
					}
					dup2(fd5, 1);   // make stexit go to file 
					close(fd5); 
					printf("\t<value> 2 </value>\n");
					printf("</result>\n");	
					SendFile(Socket_Worker_Entrada, 2, -1, "result.txt", myID);
					close(Socket_Worker_Entrada);
					remove("result.txt");
					SendMessage(Socket_Manager_Worker, 4, id, NULL, NULL);
				}
			}
		}
	}
}
