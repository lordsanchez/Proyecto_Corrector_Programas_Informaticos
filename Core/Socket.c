#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define bufferSize 50 // size of the messages send by the sistem to transfer the codes
#define maxLengthIdWorker 3 	// it has to be equal to the length of the maximum identifier used in a worker, the identifieres go from 0 to max number of workers. 
#define maxLengthIpNames 10 	//it establish tha maximum number of characters that would have any ip in the sistem
#define maxLengthPortNames 10	//it establish tha maximum number of characters that would have any name of a port in the sistem
int BUFFER_SIZE = 1024;

/*To do this code i have used the code and the examples that you cna found at 
 * 
 * http://www.chuidiang.com/clinux/sockets/sockets_simp.php#servidor under the licence creative Comons
 * 
 * 
 * 
 * */

int Read_Socket (int fd, char *Datos, int Longitud)
{
	int Leido = 0;
		int Aux = 0;

		if ((fd == -1) || (Datos == NULL) || (Longitud < 1))
			return -1;
			
	if (Longitud == 1) {
		while (1) {
			Aux = read(fd, Datos + Leido, 1);
			if (Aux > 0)
			{
				return 1;	
			} else if (Aux < 0) {
				return -1; 
			}
		}
	}else {
		while (Leido < Longitud)
		{
			Aux = read(fd, Datos + Leido, Longitud - Leido);
			if (Aux > 0)
			{
				Leido = Leido + Aux;
				if (Leido == Longitud) {
					return Leido;
				}
					
			}
			else
			{
				if (Aux == 0) 
					return Leido;
				if (Aux == -1)
				{
					switch (errno)
					{
						case EINTR:
						case EAGAIN:
							usleep (100);
							break;
						default:
							return -1;
					}
				}
			}
		}
		return Leido;
	}
}


int Write_Socket (int fd, char *Datos, int Longitud)
{
	
	int Escrito = 0;
	int Aux = 0;

	
	if ((fd == -1) || (Datos == NULL) || (Longitud < 1))
		return -1;

	while (Escrito < Longitud)
	{	
		Aux = write(fd, Datos + Escrito, Longitud - Escrito);
		if (Aux > 0)
		{
			Escrito = Escrito + Aux;
		}
		else
		{
			if (Aux == 0)
				return Escrito;
			else
				return -1;
		}
	}
	return Escrito;
}

int LengthNumber(int number) {
	int a = 0;
	while (number > 0) {
		a++;
		number /=10;
	}
	return a;
}

int SendMessage(int fd, int messageCode, char id[maxLengthIdWorker], char ip[maxLengthIpNames], char port[maxLengthPortNames])
{
	//printf("MandarMensage, info del menssage: messageCode = %i, id = %s, ip = %s, port = %s\n", messageCode, id, ip, port);	
	int messageLength, w;
	messageLength = 2; // it implies that the sistem would have a maximum of (0..99) codes
	char message[messageLength];
	if(messageCode == 1) { //PETICIO
		sprintf(message,"01");
		int w = Write_Socket(fd, message, 2);
		if (w ==-1) {
			perror("Problem writing on socket\n");
			return 0;
		} 
		//printf("MenssageCode Send 1\n");
		return 1;
		
	}else if(messageCode == 2) {//QUEUE FULL
		sprintf(message,"02");
		w = Write_Socket(fd, message, 2);
		if (w ==-1) {
			perror("Problem writing on socket\n");
			return 0;
		} 
		//printf("MenssageCode Send 2\n");
		return 1;
		
	}else if(messageCode == 3) {//QUEUED
		sprintf(message,"03");
		w = Write_Socket(fd, message, 2);
		if (w ==-1) {
			perror("Problem writing on socket\n");
			return 0;
		} 
		int longitud = strlen(id);
		int total = LengthNumber(longitud)+longitud+1;
		char menssage2[total];
		sprintf(menssage2,"%i:%s",longitud, id);
		int w = Write_Socket(fd, menssage2, total);
		if (w ==-1) {
			perror("Problem writing on socket\n");
			return 0;
		} 
		//printf("MenssageCode Send 3\n");
		//printf("Menssage Send: %s, with length: %i\n", menssage2, total);
		return 1;
		
		
	}else if(messageCode == 4) {//READY
		sprintf(message,"04");
		w = Write_Socket(fd, message, 2);
		if (w ==-1) {
			perror("Problem writing on socket\n");
			return 0;
		} 
		int longitud = strlen(id);
		int total = LengthNumber(longitud)+longitud+1;
		char menssage2[total];
		sprintf(menssage2,"%i:%s",longitud,id);
		int w = Write_Socket(fd, menssage2, total);
		if (w ==-1) {
			perror("Problem writing on socket\n");
			return 0;
		} 
		//printf("MenssageCode Send 4\n");
		//printf("Menssage Send: %s, with length: %i\n", menssage2, total);
		return 1;
		
	}else if(messageCode == 5) {//BEGIN READY
		sprintf(message,"05");
		w = Write_Socket(fd, message, 2);
		if (w ==-1) {
			perror("Problem writing on socket\n");
			return 0;
		} 
		
		int longitud = strlen(ip);
		int total = LengthNumber(longitud)+longitud+1;
		char menssage2[total];
		sprintf(menssage2,"%i:%s",longitud,ip);
		int w = Write_Socket(fd, menssage2, total);
		if (w ==-1) {
			perror("Problem writing on socket\n");
			return 0;
		} 
		//printf("MenssageCode Send 5\n");
		//printf("Menssage Send: %s, with length: %i\n", menssage2, total);
		
		longitud = strlen(port);
		total = LengthNumber(longitud)+longitud+1;
		menssage2[total];
		sprintf(menssage2,"%i:%s",longitud,port);
		w = Write_Socket(fd, menssage2, total);
		if (w ==-1) {
			perror("Problem writing on socket\n");
			return 0;
		} 
		
		//printf("Menssage Send: %s, with length: %i\n", menssage2, total);
		return 1;
	}else if(messageCode == 6) {//GO
		sprintf(message,"06");
		w = Write_Socket(fd, message, 2);
		if (w ==-1) {
			perror("Problem writing on socket\n");
			return 0;
		} 
		
		int longitud = strlen(ip);
		int total = LengthNumber(longitud)+longitud+1;
		char menssage2[total];
		sprintf(menssage2,"%i:%s",longitud,ip);
		int w = Write_Socket(fd, menssage2, total);
		if (w ==-1) {
			perror("Problem writing on socket\n");
			return 0;
		} 
		//printf("MenssageCode Send 6\n");
		//printf("Menssage Send: %s, with length: %i\n", menssage2, total);
		
		longitud = strlen(port);
		total = LengthNumber(longitud)+longitud+1;
		menssage2[total];
		sprintf(menssage2,"%i:%s",longitud,port);
		w = Write_Socket(fd, menssage2, total);
		if (w ==-1) {
			perror("Problem writing on socket\n");
			return 0;
		} 
		
		longitud = strlen(id);
		total = LengthNumber(longitud)+longitud+1;
		menssage2[total];
		sprintf(menssage2,"%i:%s",longitud,id);
		w = Write_Socket(fd, menssage2, total);
		if (w ==-1) {
			perror("Problem writing on socket\n");
			return 0;
		} 
		
		//printf("Menssage Send: %s, with length: %i\n", menssage2, total);
		return 1;
	}else if(messageCode == 7) {//ASSIGN ID
		sprintf(message,"07");
		w = Write_Socket(fd, message, 2);
		if (w ==-1) {
			perror("Problem writing on socket\n");
			return 0;
		} 
		
		int longitud = strlen(id);
		int total = LengthNumber(longitud)+longitud+1;
		char menssage2[total];
		sprintf(menssage2,"%i:%s",longitud,id);
		int w = Write_Socket(fd, menssage2, total);
		if (w ==-1) {
			perror("Problem writing on socket in the message Assign id\n");
			return 0;
		} 
		//printf("MenssageCode Send 7\n");
		//printf("Menssage Send: %s, with length: %i\n", menssage2, total);
		return 1;
	}else if(messageCode == 8) {//REQUEST PROBLEM
		sprintf(message,"08");
		w = Write_Socket(fd, message, 2);
		if (w ==-1) {
			perror("Problem writing on socket\n");
			return 0;
		} 
		
		int longitud = strlen(id);
		int total = LengthNumber(longitud)+longitud+1;
		char menssage2[total];
		sprintf(menssage2,"%i:%s",longitud,id);
		int w = Write_Socket(fd, menssage2, total);
		if (w ==-1) {
			perror("Problem writing on socket in the message Assign id\n");
			return 0;
		} 
		return 1;
	}else if(messageCode == 9) {//WORK OK
		sprintf(message,"09");
		w = Write_Socket(fd, message, 2);
		if (w ==-1) {
			perror("Problem writing on socket\n");
			return 0;
		} 
		return 1;
	}/*else if(messageCode == 10) {//FAIL COMPILATION
		sprintf(message,"10");
		w = Write_Socket(fd, message, 2);
		if (w ==-1) {
			perror("Problem writing on socket\n");
			return 0;
		} 
		return 1;
	}*/
	return 0;
}

int ReceiveMessage(int fd, int  *messageCode, char id[maxLengthIdWorker], char ip[maxLengthIpNames], char port[maxLengthPortNames])
{
	char recvBUFF[2];
	int r = Read_Socket(fd, recvBUFF, 2);
	if (r ==-1) {
			perror("Problem reading on socket\n");
			return 0;
	} 
	char messageid[maxLengthIdWorker] = "";
	char messageip[maxLengthIpNames] = "";
	char messageport[maxLengthPortNames] = "";
	
	*messageCode = atoi(recvBUFF);
	
	//printf("MessageCode recived = %i\n",*messageCode);
	if(*messageCode == 1) { //PETICIO
		return 1;
	}else if(*messageCode == 2) {//QUEUE FULL
		return 1;
	}else if(*messageCode == 3) {//QUEUED
		// Reading the first part of the message, the id
		char size[5];
		char auxSize[5];
		int r;
		int cont = 1;

		r = Read_Socket(fd, size, 1);
		if (r ==-1) {
			perror("Problem reading on socket\n");
			sleep(2);
			return 0;
		}

		r = Read_Socket(fd, auxSize, 1);
		if (r ==-1) {
			perror("Problem reading on socket\n");
			sleep(2);
			return 0;
		}
		//printf("auxSize = %s\n", auxSize);
		while (auxSize[0] != ':') {
			size[cont] = auxSize[0];
			++cont;
			
			r = Read_Socket(fd, auxSize, 1);
			if (r ==-1) {
				perror("Problem reading on socket\n");
				return 0;
			}
			//printf("auxSize = %s\n", auxSize);
			
		}
		
		int numberCharToRead = atoi(size);
		r = Read_Socket(fd, messageid, numberCharToRead);
		if (r ==-1) {
			perror("Problem reading on socket\n");
			return 0;
		} 
		strcpy(id, messageid);
		return 1;
	}else if(*messageCode == 4) {//READY
	// Reading the first part of the message, the id
		char size[5];
		char auxSize[5];
		int r;
		int cont = 1;

		r = Read_Socket(fd, size, 1);
		if (r ==-1) {
			perror("Problem reading on socket\n");
			sleep(2);
			return 0;
		}

		r = Read_Socket(fd, auxSize, 1);
		if (r ==-1) {
			perror("Problem reading on socket\n");
			sleep(2);
			return 0;
		}
		//printf("auxSize = %s\n", auxSize);
		while (auxSize[0] != ':') {
			size[cont] = auxSize[0];
			++cont;
			
			r = Read_Socket(fd, auxSize, 1);
			if (r ==-1) {
				perror("Problem reading on socket\n");
				return 0;
			}
			//printf("auxSize = %s\n", auxSize);
			
		}
		
		int numberCharToRead = atoi(size);
		r = Read_Socket(fd, messageid, numberCharToRead);
		if (r ==-1) {
			perror("Problem reading on socket\n");
			return 0;
		} 
		//printf("id rebuda en 4 = %s \n", messageid);
		strcpy(id, messageid);
		return 1;
		
	}else if(*messageCode == 5) {//BEGIN READY
		// Reading the first part of the message, the ip
		char size[5];
		char auxSize[5];
		int r;
		int cont = 1;

		r = Read_Socket(fd, size, 1);
		if (r ==-1) {
			perror("Problem reading on socket\n");
			sleep(2);
			return 0;
		}

		r = Read_Socket(fd, auxSize, 1);
		if (r ==-1) {
			perror("Problem reading on socket\n");
			sleep(2);
			return 0;
		}
		//printf("auxSize = %s\n", auxSize);
		while (auxSize[0] != ':') {
			size[cont] = auxSize[0];
			++cont;
			
			r = Read_Socket(fd, auxSize, 1);
			if (r ==-1) {
				perror("Problem reading on socket\n");
				return 0;
			}
			//printf("auxSize = %s\n", auxSize);
			
		}
		
		int numberCharToRead = atoi(size);
		//printf("number to read: %i\n", numberCharToRead);
		r = Read_Socket(fd, messageip, numberCharToRead);
		if (r ==-1) {
			perror("Problem reading on socket for getting ip\n");
			return 0;
		} 
		strncpy(ip, messageip, numberCharToRead);
		
		// Reading the second part of the message, the port
		cont = 1;

		r = Read_Socket(fd, size, 1);
		if (r ==-1) {
			perror("Problem reading on socket\n");
			sleep(2);
			return 0;
		}

		r = Read_Socket(fd, auxSize, 1);
		if (r ==-1) {
			perror("Problem reading on socket\n");
			sleep(2);
			return 0;
		}
		//printf("auxSize = %s\n", auxSize);
		while (auxSize[0] != ':') {
			size[cont] = auxSize[0];
			++cont;
			
			r = Read_Socket(fd, auxSize, 1);
			if (r ==-1) {
				perror("Problem reading on socket\n");
				return 0;
			}
			//printf("auxSize = %s\n", auxSize);
			
		}
		
		numberCharToRead = atoi(size);
		r = Read_Socket(fd, messageport, numberCharToRead);
		if (r ==-1) {
			perror("Problem reading on socket for geting port\n");
			return 0;
		} 	
		strncpy(port, messageport, numberCharToRead);
		return 1;
		
	}else if(*messageCode == 6) {//GO
	// Reading the first part of the message, the ip
		char size[5];
		char auxSize[5];
		int r;
		int cont = 1;

		r = Read_Socket(fd, size, 1);
		if (r ==-1) {
			perror("Problem reading on socket\n");
			sleep(2);
			return 0;
		}

		r = Read_Socket(fd, auxSize, 1);
		if (r ==-1) {
			perror("Problem reading on socket\n");
			sleep(2);
			return 0;
		}
		//printf("auxSize = %s\n", auxSize);
		while (auxSize[0] != ':') {
			size[cont] = auxSize[0];
			++cont;
			
			r = Read_Socket(fd, auxSize, 1);
			if (r ==-1) {
				perror("Problem reading on socket\n");
				return 0;
			}
			//printf("auxSize = %s\n", auxSize);
			
		}
		
		int numberCharToRead = atoi(size);
		//printf("number to read: %i\n", numberCharToRead);
		r = Read_Socket(fd, messageip, numberCharToRead);
		if (r ==-1) {
			perror("Problem reading on socket for getting ip\n");
			return 0;
		} 
		strncpy(ip, messageip, numberCharToRead);
		//printf("ip = %s\n", ip);
		// Reading the second part of the message, the port
		cont = 1;

		r = Read_Socket(fd, size, 1);
		if (r ==-1) {
			perror("Problem reading on socket\n");
			sleep(2);
			return 0;
		}

		r = Read_Socket(fd, auxSize, 1);
		if (r ==-1) {
			perror("Problem reading on socket\n");
			sleep(2);
			return 0;
		}
		//printf("auxSize = %s\n", auxSize);
		while (auxSize[0] != ':') {
			size[cont] = auxSize[0];
			++cont;
			
			r = Read_Socket(fd, auxSize, 1);
			if (r ==-1) {
				perror("Problem reading on socket\n");
				return 0;
			}
			//printf("auxSize = %s\n", auxSize);
			
		}
		
		numberCharToRead = atoi(size);
		r = Read_Socket(fd, messageport, numberCharToRead);
		if (r ==-1) {
			perror("Problem reading on socket for geting port\n");
			return 0;
		} 	
		strncpy(port, messageport, numberCharToRead);
		
		cont = 1;
		r = Read_Socket(fd, size, 1);
		if (r ==-1) {
			perror("Problem reading on socket\n");
			sleep(2);
			return 0;
		}

		r = Read_Socket(fd, auxSize, 1);
		if (r ==-1) {
			perror("Problem reading on socket\n");
			sleep(2);
			return 0;
		}
		//printf("auxSize = %s\n", auxSize);
		while (auxSize[0] != ':') {
			size[cont] = auxSize[0];
			++cont;
			
			r = Read_Socket(fd, auxSize, 1);
			if (r ==-1) {
				perror("Problem reading on socket\n");
				return 0;
			}
			//printf("auxSize = %s\n", auxSize);
			
		}
		
		numberCharToRead = atoi(size);
		r = Read_Socket(fd, messageport, numberCharToRead);
		if (r ==-1) {
			perror("Problem reading on socket for geting port\n");
			return 0;
		} 	
		strncpy(id, messageport, numberCharToRead);
		return 1;
		
	}else if(*messageCode == 7) {//ASIGN ID
		// Reading the first part of the message, the id
		char size[5];
		char auxSize[5];
		int r;
		int cont = 1; 
		r = Read_Socket(fd, size, 1);
		if (r ==-1) {
			perror("Problem reading on socket\n");
			sleep(2);
			return 0;
		}
		r = Read_Socket(fd, auxSize, 1);
		if (r ==-1) {
			perror("Problem reading on socket\n");
			sleep(2);
			return 0;
		}
		//printf("auxSize = %s\n", auxSize);
		while (auxSize[0] != ':') {
			size[cont] = auxSize[0];
			++cont;
			
			r = Read_Socket(fd, auxSize, 1);
			if (r ==-1) {
				perror("Problem reading on socket\n");
				return 0;
			}
			//printf("auxSize = %s\n", auxSize);	
		}
		
		int numberCharToRead = atoi(size);
		r = Read_Socket(fd, messageid, numberCharToRead);
		if (r ==-1) {
			perror("Problem reading on socket\n");
			return 0;
		} 
		strcpy(id, messageid);
		return 1;	

	}else if(*messageCode == 8) {//REQUEST PROBLEM
		// Reading the first part of the message, the id
		char size[5];
		char auxSize[5];
		int r;
		int cont = 1; 
		r = Read_Socket(fd, size, 1);
		if (r ==-1) {
			perror("Problem reading on socket\n");
			sleep(2);
			return 0;
		}
		r = Read_Socket(fd, auxSize, 1);
		if (r ==-1) {
			perror("Problem reading on socket\n");
			sleep(2);
			return 0;
		}
		//printf("auxSize = %s\n", auxSize);
		while (auxSize[0] != ':') {
			size[cont] = auxSize[0];
			++cont;
			
			r = Read_Socket(fd, auxSize, 1);
			if (r ==-1) {
				perror("Problem reading on socket\n");
				return 0;
			}
			//printf("auxSize = %s\n", auxSize);	
		}
		
		int numberCharToRead = atoi(size);
		strcpy(messageid, "");
		r = Read_Socket(fd, messageid, numberCharToRead);
		if (r ==-1) {
			perror("Problem reading on socket\n");
			return 0;
		} 
		strncpy(id, messageid, numberCharToRead);
		return 1;	

	}else if(*messageCode == 9) {//OK
		return 1;
	}
}

/*This function sends the file to the worker and gives it the necessari inforamtion to compile and test it.
 * idProblem: contains the identifier of the problem
 * compiler: is a code that refers to the lenguage and the compiler that worker will use, if it is -1 it means that the file is a result form a compilation done in a worker
 * numberOfPackets: Referes to the total number of packets that the client will send to the worker
 * size: Is the size of each one of this packets, except form the last.
 * lastSize: Size of the last packet. 
 * :idWorker in case that you are using compiler option -1, id of the worker that have done the compilation, execution and tests
 * */
int SendFile(int fd, int idProblem, int compiler, char fileSendName[20], int idWorker) {
	
	int numberOfPackets;
	int size;
	int lastSize;
	
	FILE *file_to_send;
	file_to_send = fopen(fileSendName,"r");
	if(!file_to_send) {
		printf("Error opening file\n");
		close(fd);
		return -1;
	} else {
		
		fseek (file_to_send, 0, SEEK_END); size =ftell (file_to_send);
		rewind(file_to_send);

		numberOfPackets = size/bufferSize;
		lastSize = size%bufferSize;
		if (lastSize != 0) numberOfPackets++;
		
		char aux[100];
		sprintf(aux,"%i:%i:%i:%i:%i", idProblem, compiler, numberOfPackets, lastSize, idWorker);
		int length = strlen(aux);
		int messageLength = length+LengthNumber(length)+1;
		char message[messageLength];
		sprintf(message,"%i:%s", length, aux);
		printf("SendFile:the message has length= %i, message = %s\n",length, message);
		
		
		int w = Write_Socket(fd, message,  messageLength);
		if (w ==-1) {
			perror("Problema escribinedo en socket\n");
		} else {

		}
		char remoteFILE[bufferSize];
		
		int count = 0;
		int character;
		rewind(file_to_send);

		while(!(feof(file_to_send))) {
			while(count < bufferSize && (character=getc(file_to_send))!=EOF) { // mentre no sigui final del archiu
				remoteFILE[count] = character;
				count++;	
			}
			w = Write_Socket(fd, remoteFILE,  count);
			if (w ==-1) {
				perror("Problema escribinedo en socket\n");
			} else {
				//printf("SendFile: mandados %i caracteres\n", w);
				count = 0;
			}
		}
		fclose(file_to_send);
	}
	return 1;	
}
	
/*This function gets the file form the worker side, it gets all the data necessari to recibe the file, and after this it gets it all.
 * 
 * */
int RecieveFile (int fd, int *idProblem, int *compiler) {
    if ((fd == -1)) {
		perror("Problem fd\n");
        return -1;
     }
    char size[5];
    char auxSize[5];
    int r;
    int cont = 1;

    r = Read_Socket(fd, size, 1);
    if (r ==-1) {
        perror("Problem reading on socket\n");
        sleep(2);
        return 0;
    }

    r = Read_Socket(fd, auxSize, 1);
    if (r ==-1) {
        perror("Problem reading on socket\n");
        sleep(2);
        return 0;
    }
	//printf("auxSize = %s\n", auxSize);
    while (auxSize[0] != ':') {
        size[cont] = auxSize[0];
        ++cont;
        
        r = Read_Socket(fd, auxSize, 1);
        if (r ==-1) {
            perror("Problem reading on socket\n");
            return 0;
        }
        //printf("auxSize = %s\n", auxSize);
        //printf("size = %s\n", size);
    }
    
   
    int numberCharToRead = atoi(size);
   // printf("numberCharToread = %i\n", numberCharToRead);
    char message[numberCharToRead];
    r = Read_Socket(fd, message, numberCharToRead);
    if (r ==-1) {
        perror("Problem reading on socket\n");
        return 0;
    }    
    printf("message = %s\n", message);
    char aux[numberCharToRead];
    int numberOfPackets;
    int idWorker;
    int lastSize;
   
    int i;
    cont = 0;
    int numberVariablesRead = 0;
    for (i = 0; i < numberCharToRead; ++i) {
        if (message[i] != ':') {
            aux[cont] = message[i];
            ++cont;
        } else {			
			aux[cont] = '\0';
			if (numberVariablesRead == 0) {	
				*idProblem = atoi(aux);
				cont = 0;
				++numberVariablesRead;
			} else if (numberVariablesRead == 1) {
				*compiler = atoi(aux);
				cont = 0;
				++numberVariablesRead;
			} else if (numberVariablesRead == 2) {
				numberOfPackets = atoi(aux);
				cont = 0;
				++numberVariablesRead;
			} else if (numberVariablesRead == 3) {
				lastSize = atoi(aux);
				cont = 0;
				++numberVariablesRead;
			}
		}
        if(i == (numberCharToRead - 1)) {
            aux[cont] = '\0';    
            idWorker = atoi(aux);         
        }
    }
    //printf("RecieveFile: idProblem = %i, compiler = %i, numberOfPackets = %i, lastSize = %i, idWorker = %i\n", *idProblem, *compiler, numberOfPackets, lastSize, idWorker);
    //finished reading the configuration of the messatges
    char recvBUFF[bufferSize];
    FILE *recvFILE;
   
    if (*compiler == 0 || *compiler == 2 ) {
		recvFILE = fopen ("codeRecived.c","w");
	
	}else if (*compiler == 1 || *compiler == 3){
		recvFILE = fopen ("codeRecived.cpp","w");
	
	} else if (*compiler == -1 ) {
		char fileName[20];
		sprintf(fileName, "%i.txt", idWorker);
		recvFILE = fopen (fileName, "w");
	}
   
   
    int recivedPackets = 0;
    int readOfThisPacket = 0;
    while(recivedPackets < numberOfPackets -1)//recive all the packets excepting the last with a diferent size
    {
       
        r = Read_Socket(fd, recvBUFF, sizeof(recvBUFF));
        
        fwrite (recvBUFF , sizeof(recvBUFF[0]),  sizeof(recvBUFF), recvFILE);
        readOfThisPacket+= r;
        if (readOfThisPacket == sizeof(recvBUFF)) {
            ++recivedPackets;
            readOfThisPacket = 0;
        }   
    }
               
    //reciving last packet
    r = Read_Socket(fd, recvBUFF, lastSize);
    fwrite (recvBUFF, sizeof(recvBUFF[0]),  lastSize, recvFILE);
    fclose(recvFILE);
    //printf("Finished Reciving File\n");
    return 1;
}


int SendFileRAR(int fd, char fileSendCompleteRoute[100]) {
	
	FILE *file = fopen(fileSendCompleteRoute, "rb");
	if(file==NULL)
	{
		printf("File opern error");
		return -1;
	}

	//Read data from file and send it using the socket
	for (;;)
	{
		unsigned char buffer[BUFFER_SIZE];
		int bytesRead = fread(buffer, 1, BUFFER_SIZE, file);
		
		if(bytesRead > 0)
		{
			//write(fd, buffer, bytesRead);
			Write_Socket(fd, buffer, bytesRead);
		}

		if (bytesRead < BUFFER_SIZE)
		{
			if (feof(file)) {
				printf("End of file\n");
				break;
			}
			if (ferror(file)) {
				printf("Error reading\n");
				break;
			}
		}
	}
    return 1;
}

int RecieveFileRAR (int fd, char fileRecivedName[100]) {
	
     //Create file where data will be stored   
    FILE *file = fopen(fileRecivedName, "wb");
    if(file == NULL)
    {
        printf("Error opening file");
        return -1;
    }
    
    //Receive data in chunks of BUFFER_SIZE bytes
    int numberBytesReceived = 0;
    char buffer[BUFFER_SIZE];
    memset(buffer, '0', sizeof(buffer));
    long totalRecived = 0;
    while((numberBytesReceived = read(fd, buffer, BUFFER_SIZE)) > 0)
    {
		if(numberBytesReceived < 0)
		{
			printf("Error reading on the socket\n");
			return -1;
		}
		totalRecived += numberBytesReceived;
        fwrite(buffer, 1,numberBytesReceived,file);
    }
	printf("Bytes received %li\n",totalRecived); 
    return 1;
}

/*
* Se le pasa un socket de servidor y acepta en el una conexion de cliente.
* devuelve el descriptor del socket del cliente o -1 si hay problemas.
* Esta funcion vale para socket AF_INET o AF_UNIX.
*/
int Servidor_Acepta_Conexion_Del_Cliente (int Descriptor)
{
	socklen_t Longitud_Cliente;
	struct sockaddr Cliente;
	int Hijo;

	/*
	* La llamada a la funcion accept requiere que el parametro 
	* Longitud_Cliente contenga inicialmente el tamano de la
	* estructura Cliente que se le pase. A la vuelta de la
	* funcion, esta variable contiene la longitud de la informacion
	* util devuelta en Cliente
	*/
	Longitud_Cliente = sizeof (Cliente);
	Hijo = accept (Descriptor, &Cliente, &Longitud_Cliente);
	if (Hijo == -1)
		return -1;

	/*
	* Se devuelve el descriptor en el que esta "enchufado" el cliente.
	*/
	return Hijo;
}

/*
* Abre un socket servidor de tipo AF_INET. Devuelve el descriptor
*	del socket o -1 si hay probleamas
* Se pasa como parametro el nombre del servicio. Debe estar dado
* de alta en el fichero /etc/services
*/
int Servidor_Abre_Socket_Inet (char *Servicio, int longitudColaEspera)
{
	struct sockaddr_in Direccion;
	struct sockaddr Cliente;
	socklen_t Longitud_Cliente;
	struct servent *Puerto;
	int Descriptor;

	/*
	* se abre el socket
	*/
	Descriptor = socket (AF_INET, SOCK_STREAM, 0);
	if (Descriptor == -1)
	 	return -1;

	/*
	* Se obtiene el servicio del fichero /etc/services
	*/
	Puerto = getservbyname (Servicio, "tcp");
	if (Puerto == NULL)
		return -1;

	/*
	* Se rellenan los campos de la estructura Direccion, necesaria
	* para la llamada a la funcion bind()
	*/
	Direccion.sin_family = AF_INET;
	Direccion.sin_port = htons(Puerto->s_port);
	Direccion.sin_addr.s_addr = htonl(INADDR_ANY);
	//Direccion.sin_port = (Puerto->s_port);
	//Direccion.sin_addr.s_addr = (INADDR_ANY);
	if (bind (
			Descriptor, 
			(struct sockaddr *)&Direccion, 
			sizeof (Direccion)) == -1)
	{
		close (Descriptor);
		return -1;
	}

	/*
	* Se avisa al sistema que comience a atender llamadas de clientes
	*/
	if (listen (Descriptor, longitudColaEspera) == -1)
	{
		close (Descriptor);
		return -1;
	}

	/*
	* Se devuelve el descriptor del socket servidor
	*/
	return Descriptor;
}

/*
/ Conecta con un servidor remoto a traves de socket INET
*/
int Cliente_Abre_Conexion_Inet (char *Host_Servidor, char *Servicio)
{
	struct sockaddr_in Direccion;
	struct servent *Puerto;
	struct hostent *Host;
	int Descriptor;

	Puerto = getservbyname (Servicio, "tcp");
	if (Puerto == NULL)
		return -1;

	Host = gethostbyname (Host_Servidor);
	if (Host == NULL)
		return -1;

	Direccion.sin_family = AF_INET;
	Direccion.sin_addr.s_addr = ((struct in_addr *)(Host->h_addr))->s_addr;
	Direccion.sin_port = htons(Puerto->s_port);
	//Direccion.sin_port = (Puerto->s_port);
	
	Descriptor = socket (AF_INET, SOCK_STREAM, 0);
	if (Descriptor == -1)
		return -1;

	if (connect (
			Descriptor, 
			(struct sockaddr *)&Direccion, 
			sizeof (Direccion)) == -1)
	{
		return -1;
	}

	return Descriptor;
}

