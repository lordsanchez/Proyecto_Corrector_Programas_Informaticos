
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

#define numberWorkers 1 	// maximum number of workers the sistem will have.
#define queueLength numberWorkers*2 	//length of the queue that contains the petitions.
#define maxSizeQueue LengthNumber(queueLength)	// It must containt the length of the number of the maximum amount of petitions tha tcan contain the queue
#define maxLengthIdWorker  3 //LengthNumber(numberWorkers) 	// it has to be equal to the length of the maximum identifier used in a worker, the identifieres go from 0 to max number of workers. 
#define maxLengthIpNames 10 	//it establish tha maximum number of characters that would have any ip in the sistem
#define maxLengthPortNames 10	//it establish tha maximum number of characters that would have any name of a port in the sistem



struct worker
{
	char id[maxLengthIdWorker]; 	//worker's identifier.
	char ip[maxLengthIpNames]; 	//ip of the machine that hosts the worker.
	char port[maxLengthPortNames];	 //port used by the worker in the maachine that hosts it.
	int	ready; 	//it marcs if the worker is free, if its true it means that the worker is ready to work.
};

struct queue {
	int max; //maximum number of elements that the sistem can put in the queue, its value its equal to queueLength
	int elements[queueLength]; //elements that are contianed in the queue
	int first; //position of the first element stored in the queue
	int nextFree; //position of the first element free in the queue, it is ready to store and element
	int numberOfElements;	//contains the number of elements in the queue
};

int EmptyQueue(struct queue *target) { //return 1 if the queue is free of elements, empty queue
	if ((*target).numberOfElements == 0) return 1;
	return 0;
}

int FullQueue(struct queue *target) { // return 1 if the queue is full, and cannot admit new elements.
	if ((*target).numberOfElements == (*target).max) return 1;
	return 0;
}

int PutInQueue(struct queue *target, int fd) { //it returns -1 if and error has ocurred, the queue dosen't addmit more elements
	if (EmptyQueue(target)== 1) {
		(*target).elements[0] = fd;
		(*target).nextFree = 1;
		(*target).first = 0;
		(*target).numberOfElements++;
		return 1;
		
	} else { // the queue had some elements
		if (FullQueue(target) == 0) { // The queue isn't full
			(*target).elements[(*target).nextFree] = fd;
			//printf("!!!!!!!Stored FD =%i, position = \n", fd);
			(*target).nextFree++;
			if ((*target).nextFree == (*target).max) (*target).nextFree  = 0;
			(*target).numberOfElements++;
			return 1;
			
		} else { //full queue!
			return -1;
		}	
	}
} 

int Next(struct queue *target) { //It returns the next element in the queue, beofre using Next, you must look if it contains elements using EmptyQueue
	if (EmptyQueue(target)!= 1) {
		int aux = (*target).elements[(*target).first];
		//printf("NEXT valor == %i\n", aux);
		(*target).first++;
		if ((*target).first == (*target).max) (*target).first = 0;
		(*target).numberOfElements--;
		return aux;	
	} else {
		//printf("Cola vacia\n");
		return -1;
	}
	
}



void PrintQueue(struct queue *target) { //this function prints all the elements stored in the queue starting by the first, it is used for debuging.
	//printf("The elements of the queue are the next: \n");
	if (EmptyQueue(target) == 0)  {
		int i = (*target).first;
		int cont = (*target).numberOfElements;
		while(cont > 0) {
			if (i == (*target).max) i = 0;
			//printf("elemento %i: %i\n", i, (*target).elements[i]);
			i++;
			cont--;
		}
		//printf("MAX = %i, FIRST = %i, NEXTFREE == %i, #Elements = %i \n", (*target).max, (*target).first, (*target).nextFree, (*target).numberOfElements);
	} else {
		printf("The queue is empty\n");
	}
}

int GetNumberOfElements(struct queue *target) {
	return (*target).numberOfElements;
}

struct queue MakeQueue() {
	struct queue a;
	a.max = queueLength; 
	a.first = 0;
	a.nextFree = 0;
	a.numberOfElements = 0;
	return a;
}

int main (int argc, char *argv[])
{
	if(argc!=3){
		printf("Number of parameters incorrect. Use: ./[port With workers][port with clients]\n");
		sleep(5);
		perror("fail in the parameters");
	}
	if (strlen(argv[1]) > maxLengthPortNames) {
		printf("Incorrect length of the port name, it is too length\n");
		sleep(5);
		perror("fail in the length of the port with workers");
	}
	
	if (strlen(argv[2]) > maxLengthPortNames) {
		printf("Incorrect length of the port name, it is too length\n");
		sleep(5);
		perror("fail in the length of the port with clients");
	}
	
	printf("MANAGER!!\n");
	fd_set selector;
	int Socket_Manager_Worker, Socket_Manager_Client;
	int maxfd , fdClient;
	
	//Controling the state of the workers
	struct worker workers[numberWorkers];
	int socketsWorkers[numberWorkers];//contains the descriptors of the sockets with all the workers
	int firstWorkerReady = numberWorkers; // it marks the first worker ready to be used, (the worker with the lowest id), the id goes from 0 to (numberWorkers -1), so if the value is numberWorkers or another invalid value, it means that it isn't any worker avaiable.
	int nextIdToAssignToWorker = 0; //it contains the next id that will be asigned to a worker
	
	//Control the petitions recived form clients.
	struct queue requestsClients = MakeQueue();
	int socketsClients[numberWorkers];
	
	
	char id[maxLengthIdWorker] ="";
	char ip[maxLengthIpNames] = "";
	char port[maxLengthPortNames] = "";
	
	char auxs[maxLengthIdWorker] = "";
	int messageCode;
	int position;
	int fd_petition;
	int i, find, aux;
	
	Socket_Manager_Worker = Servidor_Abre_Socket_Inet(argv[1], SOMAXCONN);
	if (Socket_Manager_Worker == -1)
	{
		printf ("MANAGER: Problem opening socket for the workers\n");
		sleep(5);
		perror("Problem opening socket for the workers");
	}
	
	Socket_Manager_Client = Servidor_Abre_Socket_Inet(argv[2], numberWorkers); 
	if (Socket_Manager_Client == -1)
	{
		printf ("MANAGER: Problem opening socket for the clients\n");
		sleep(5);
		perror("Problem opening socket for the clients");
	}
	
	
	// This is for debuggin, puts all the output in a file named managerOUT.txt
	/*remove("managerOUT.txt");
	int fd5 = open("managerOUT.txt", O_RDWR | O_CREAT | O_TRUNC, 0777);
	if (fd5 < 0) {
		perror("Fallo en open()\n");
		//exit(EXIT_FAILURE);
	}
	dup2(fd5, 1);  
	close(fd5);*/
	
	while(1) {
		char id[maxLengthIdWorker] ="";
		char ip[maxLengthIpNames] = "";
		char port[maxLengthPortNames] = "";

		FD_ZERO(&selector);
		FD_SET(Socket_Manager_Worker, &selector);
		FD_SET(Socket_Manager_Client, &selector);
		
		if (Socket_Manager_Client > Socket_Manager_Worker ) {
			maxfd=Socket_Manager_Client+1;
		
		} else {
			maxfd=Socket_Manager_Worker+1;
		}
		
		for (i=0; i<nextIdToAssignToWorker; i++) { 
			FD_SET (socketsWorkers[i], &selector);
			if (socketsWorkers[i] > maxfd) maxfd = socketsWorkers[i];
		}
		
		if (select(maxfd+1, &selector,NULL, NULL, NULL) < 0 ) { 
				printf("MANAGER: Error on select:");
				perror("Error on select");
		}
		
		//new connection form a Client
		if (FD_ISSET(Socket_Manager_Client, &selector) ) { 
			printf("Recived new connection from a Client\n");
			if ((fdClient=accept(Socket_Manager_Client, NULL, NULL)) < 0 ) {
				printf("MANAGER:Error acepting client connection (accept client)\n");
				perror("Error accepting the clinet");
			}
			ReceiveMessage(fdClient, &messageCode, id, ip, port);
			if(messageCode == 1) { //PETITION
				if (firstWorkerReady < (numberWorkers)) { //the manager have some worker avaiable to atend the petition
					sprintf(auxs, "%i", firstWorkerReady);
					//printf("GO 1!!!! fd = %i, ip = %s, port = %s, idWorker = %s\n", fdClient, workers[firstWorkerReady].ip, workers[firstWorkerReady].port, auxs);
					SendMessage(fdClient, 6, auxs, workers[firstWorkerReady].ip, workers[firstWorkerReady].port); //GO
					close(fdClient);
					workers[firstWorkerReady].ready = 0;
					//update the variable firstWorkerReady
					find = 0;
					for (i = firstWorkerReady+1; i < numberWorkers && find == 0; i++) {
						if (workers[i].ready == 1) {
							firstWorkerReady = i;
							find = 1;
						}
					}
					if (find == 0) firstWorkerReady = numberWorkers;
			
				} else { //the manager dosen't have any worker avaiable, if proced to store the petition
					aux = PutInQueue(&requestsClients, fdClient);
					if (aux == -1) { //the queue of petitions is full, the manager proced to notificate it to the client
						printf("Queue full\n");
						SendMessage(fdClient, 2, NULL, NULL, NULL); //QUEUE FULL	
						close(fdClient);
					} else {
						int numberPetitions= GetNumberOfElements(&requestsClients) -1;
						sprintf(id, "%i", numberPetitions);
						printf("Queued\n");
						SendMessage(fdClient, 3, id, ip, port); //QUEUED	
						PrintQueue(&requestsClients);
					}
				}
			}
		}
		
		//New connection with a new worker
		if (FD_ISSET(Socket_Manager_Worker,&selector)) { 
			printf("Recived a new connection form a new Worker\n");
			if ((socketsWorkers[nextIdToAssignToWorker] = accept(Socket_Manager_Worker,NULL,NULL)) < 0) {
				printf("MANAGER:Error on the connection (accept worker)\n");
				perror("Error on accept");
			}
			
			ReceiveMessage(socketsWorkers[nextIdToAssignToWorker], &messageCode, NULL, ip, port);
			if(messageCode == 5) { //BEGIN READY to be done answer to the worker with her id, store on the sistem a match between id and fd, 
				printf("BEGIN READY\n");
				position = nextIdToAssignToWorker;
				nextIdToAssignToWorker++;
				strcpy(workers[position].ip, ip);
				strcpy(workers[position].port, port);
				
				//this message is send to the worker telling the id that it will have in the sistem.
				
				sprintf(id, "%i", position);
				printf("id = %s\n", id);
				SendMessage(socketsWorkers[position], 7, id, ip, port); //ASSIGN ID
				if(EmptyQueue(&requestsClients) == 0) { //the manger have a petition to be attended and asign this petition to the new worker
					fd_petition = Next(&requestsClients);
					printf("GO 2!!!! fd = %i, ip = %s, port = %s\n", fd_petition, workers[position].ip, workers[position].port);
					SendMessage(fd_petition, 6, id, workers[position].ip, workers[position].port); //GO
					close(fd_petition);
					workers[position].ready = 0;
					
				} else { // the manager dosen't have any petition to be atended
					workers[position].ready = 1;
					if(firstWorkerReady > position) { 
						firstWorkerReady = position;
					}	
				}
			}
		}
		
		//Connection from one of the workers in the system, the worker uses the socket created excluseviliy to connect him with the manager.
		for (i=0; i<nextIdToAssignToWorker; i++) { 
			if (FD_ISSET (socketsWorkers[i], &selector)) {		
				ReceiveMessage(socketsWorkers[i], &messageCode, id, ip, port);
				printf("MANAGER:code = %i, id = %s, ip = %s, port = %s,\n", messageCode, id, ip, port);	
				if(messageCode == 4) { //READY received from a worker
					position = atoi(id);
					if(EmptyQueue(&requestsClients) == 0) { //There is a petition to be atended from a client, proced to assign the petition to the worker. 
						PrintQueue(&requestsClients);
						fd_petition = Next(&requestsClients);
						printf("GO 3!!!! fd = %i, ip = %s, port = %s\n", fd_petition, workers[position].ip, workers[position].port);
						SendMessage(fd_petition, 6, id, workers[position].ip, workers[position].port);
						close(fd_petition);
						
					} else { //I dont have petition to atend I mark the worker as avaiable for future petitions.
						workers[position].ready = 1;
						if(firstWorkerReady > position) { 
							firstWorkerReady = position;
						}	
					}	
				}
			}
		}
		
		//PrintQueue(&requestsClients);
	}//while
}
