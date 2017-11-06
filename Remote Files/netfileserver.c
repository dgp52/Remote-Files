//Name: Deepkumar Patel, NetID: dgp52
//Name: Viraj Patel, NetID: vjp60

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>

#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h> 


#define PORT 9672
#define BUFFER_SIZE 1024

//Struct to hold all clients (Linked list)
typedef struct node {
    //File name or Path
    char* fName; 
    //Client IP address
    char* clientIP;
    //Unrestricted = 0, Exculsive = 1, Transaction = 2;
    int permission;
    //Read = 0, write = 1, readwrite = 2;
    int flag;
    //File descriptor
    int FD;
    //Points to next structure
    struct node * next;
} node_t;

//Initalize head to null
node_t * head = NULL;

//Enum for different types of error
typedef enum {
	ERRNO_ERROR, THREAD_C_ERROR, FUNCTION_ERROR
} ErrorType;

//Struct to pass inside thread
struct tInfo {
  //Returned accept value
  int isAcceptedVal;
  //Buffer from client
  char buff[BUFFER_SIZE];
  struct sockaddr_in clientAdd;
};

//Show Error messages
void showError (ErrorType errorIs) {
	switch(errorIs){
		case ERRNO_ERROR:
			printf("ERROR: %s\n", strerror(errno));
		break;
		case THREAD_C_ERROR:
			printf("ERROR: worker thread not created\n");
		break;
		case FUNCTION_ERROR:
			printf("ERROR: invalid function argument\n");
			exit(0);
		break;
	}
}

//Check to see if the file exist
int fileExists(const char *filename) {
    struct stat st;
    int result = stat(filename, &st);
    return result == 0;
}

//Add the new client to the head of the linked list
void addToHead(node_t ** head, char* f, char *ip, int p, int fl, int fd) {
    node_t * temp_node;
    temp_node = malloc(sizeof(node_t));
    temp_node->fName = f;
    temp_node->clientIP = ip;
    temp_node->permission = p;
    temp_node->flag = fl;
    temp_node->FD = fd;
    temp_node->next = *head;
    *head = temp_node;
}

//Check if the client is opening a new file
int isNewFile(node_t *head, char *f){
  int returnedVal = 0;
  node_t *current = head;
  while(current!=NULL){
    if(strcmp(current->fName,f) == 0){
      returnedVal = 0;
      break;
    } else {
      returnedVal = 1;
    }
    current = current -> next;
  }
  return returnedVal;
}

//Set an error to their corresponding modes or permissions
int errorPermission(int permission){
  int returnedVal;
  if (permission == 0){
    //Unrestricted error = -1
    returnedVal = -1;
  } else if (permission == 1) {
    //Exclusive error = -2
    returnedVal = -2;
  } else {
    //Transaction error = -3
    returnedVal = -3;
  }
  return returnedVal;
}

//Returns 1 if the file is safe to open otherwise returns an error code
int unrestrictedMode(node_t *head, int permission, int flag){
  int returnedVal = 0;
  node_t *current = head;
  //Unrestricted mode with read
  if(permission == 0 && flag == 0){
    while(current!=NULL){
      if(current->permission == 2) {
        returnedVal = errorPermission(current->permission);
        break;
      } else {
        returnedVal = 1;
      }
      current = current -> next;
    }
  } else if (permission == 0 && flag == 1) {
    //Unrestricted mode with write
    while(current!=NULL){
      if(current->permission == 2 || (current->permission == 1 && current->flag > 0)){
        returnedVal = errorPermission(current->permission);
        break;
      } else {
        returnedVal = 1;
      }
      current = current -> next;
    }
  } else if (permission == 0 && flag == 2) {
    //Unrestricted mode with read write
    while(current!=NULL){
      if(current->permission == 2 || (current->permission == 1 && current->flag > 0)){
        returnedVal = errorPermission(current->permission);
        break;
      } else {
        returnedVal = 1;
      }
      current = current -> next;
    }
  }
  return returnedVal;
}

//Returns 1 if the file is safe to open otherwise returns an error code
int transactionMode(node_t *head, int permission, int flag){
  int returnedVal = 0;
  node_t *current = head;
  //Transaction mode with read
  if(permission == 2 && flag == 0){
    while(current!=NULL){
      if(current->permission == 0 || current->permission == 1 ||
         (current->permission == 2)) {
        returnedVal = errorPermission(current->permission);
        break;
      } else {
        returnedVal = 1;
      }
      current = current -> next;
    }
  } else if (permission == 2 && flag == 1) {
    //Transaction mode with write
    while(current!=NULL){
      if(current->permission == 0 || current->permission == 1 || 
        (current->permission == 2)) {
        returnedVal = errorPermission(current->permission);
        break;
      } else {
        returnedVal = 1;
      }
      current = current -> next;
    }
  } else if (permission == 2 && flag == 2) {
    //Transaction mode with readwrite
    while(current!=NULL){
      if(current->permission == 0 || current->permission == 1 ||
       (current->permission == 2)) {
        returnedVal = errorPermission(current->permission);
        break;
      } else {
        returnedVal = 1;
      }
      current = current -> next;
    }
  }
  return returnedVal;
}

//Returns 1 if the file is safe to open otherwise returns an error code
int exclusiveMode(node_t *head, int permission, int flag) {
  int returnedVal = 0;
  node_t *current = head;
  //Exclusive mode with read
  if(permission == 1 && flag == 0) {
    while(current!=NULL){
      if(current->permission == 2) {
        returnedVal = errorPermission(current->permission);
        break;
      } else {
        returnedVal = 1;
      }
      current = current -> next;
    }
  } else if (permission == 1 && flag == 1) {
   //Exclusive mode with write
    while(current!=NULL){
      if(current->permission == 2 || (current->permission == 1 && current->flag > 0) ||
       (current->permission == 0 && current->flag > 0)) {
        returnedVal = errorPermission(current->permission);
        break;
      } else {
        returnedVal = 1;
      }
      current = current -> next;
    }
  } else if (permission == 1 && flag == 2) {
   //Exclusive mode with readwrite
    while(current!=NULL){
      if(current->permission == 2 || (current->permission == 1 && current->flag > 0) ||
       (current->permission == 0 && current->flag > 0)) {
        returnedVal = errorPermission(current->permission);
        break;
      } else {
        returnedVal = 1;
      }
      current = current -> next;
    }
  }
  return returnedVal;
}

//Convert the read, write, readwrite flags to int
//Read = 0, Write = 1, Readwrite = 2
int convertFlagToInt(char *c){
  int returnedVal;
  if (strcmp(c,"O_RDONLY") == 0) {
    returnedVal = 0;
  } else if (strcmp(c,"O_WRONLY") == 0) {
    returnedVal = 1;
  } else {
    returnedVal = 2;
  }
  return returnedVal;
}

//Returns the index of the node that needs to be deleted
//It uses clients ip and file descriptor to figure out the index
int indexToDelete(node_t *head, char *ip, int fd) {
  int returnedVal = -5;
  int tempsize = 0;
  node_t *current = head;
  while(current != NULL) {
    if((strcmp(current->clientIP, ip) == 0) && (current->FD == fd)){
      tempsize++;
      returnedVal = tempsize;
      break;
    } else {
      tempsize ++;
      returnedVal = -5;
    }
    current = current -> next;
  }
  return returnedVal;
}

//Deletes the node on the given index
void deleteNode(node_t **head, int position){
    node_t *prev, *tmp;
    int i;
    if(head == NULL || *head == NULL || position < 0) {
      return ;
    }
    if(position == 0){
        tmp = (*head)->next;
        free(*head);
        *head = tmp;
    } else {
        prev = *head;
        tmp = (*head)->next;
        for(i=1;i<position;++i){
            prev = tmp;
            tmp = tmp->next;
            if(tmp == NULL) {
              return ;
            }
        }
        prev->next = tmp->next;
        free(tmp);
    }
}

void* workerThreadHandler(void* tInfo){
    int finalPermission; 

    struct tInfo* ti = (struct tInfo*)tInfo;
    int acc = ti->isAcceptedVal;
    char buffer[BUFFER_SIZE];
    strcpy(buffer, ti-> buff);
    struct sockaddr_in clientAdd = ti-> clientAdd;

    //Get the Client IP Address
    char clientip[50];
    strcpy(clientip, inet_ntoa(clientAdd.sin_addr));

	 //Initally set the value to 0
	 static char outgoingBuffer[BUFFER_SIZE];
	//Determine which function was called
	//open = 1
	//read = 2
	//write = 3
	//close = 4
	char whichFunction[BUFFER_SIZE];
	strcpy(whichFunction, buffer);
	int inputfunc = 0;
  	char * pch;
  	pch = strtok (whichFunction," ");
  	while (pch != NULL) {
  		inputfunc = atoi(pch);
    	pch = strtok (NULL, " ");
  	}

  	//Open = 1
  	if(inputfunc == 1){
  		//Clear the outgoing buffer
	    bzero(outgoingBuffer,BUFFER_SIZE);

  		char buff[BUFFER_SIZE];
  		strcpy(buff, buffer);

  		//Extract Values from buffer
  		char filePath[BUFFER_SIZE];
  		char flag[BUFFER_SIZE];
      char permission[BUFFER_SIZE];
  		char index[BUFFER_SIZE];
  		strcpy(filePath, strtok(buff, " "));
  		strcpy(flag, strtok(NULL, " "));
      strcpy(permission,  strtok(NULL, " "));
  		strcpy(index, strtok(NULL, " "));

      char fileName[BUFFER_SIZE];
      //Returns 0 if file doesnt exist, 1 otherwise.
      if(fileExists(filePath)){
        //initalize finalPermission to 0;
        finalPermission = 0;
        //Extract filename if it exist.
        char *nameF = '\0';
        char *temPath = strdup(filePath);
        nameF = basename(temPath);
        strcpy(fileName, nameF);
        //Check the permission and set finalpermission
        if (head == NULL) {
          //Create the head if its not created and add the first client
          head = malloc(sizeof(node_t));
          head-> fName = fileName;
          head -> clientIP = clientip;
          head -> permission = atoi(permission);
          head -> flag = convertFlagToInt(flag);
          head -> FD = -1;
          head -> next = NULL; 
          finalPermission = 1;
        } else {
          if(isNewFile(head, fileName) == 1){
            //If its a new file then add the file
           addToHead(&head, fileName, clientip, atoi(permission), convertFlagToInt(flag), -1);
           finalPermission = 1;
          } else {
            //Check for all the permissions 
            if(atoi(permission) == 0){
              int un = unrestrictedMode(head,atoi(permission),convertFlagToInt(flag));
              if (un == 1) {
                addToHead(&head, fileName, clientip, atoi(permission), convertFlagToInt(flag), -1);
                finalPermission = un;
              } else {
                finalPermission = un;
              }
            } else if (atoi(permission) == 1) {
              int ex = exclusiveMode(head,atoi(permission),convertFlagToInt(flag));
               if (ex == 1) {
                addToHead(&head, fileName, clientip, atoi(permission), convertFlagToInt(flag), -1);
                finalPermission = ex;
              } else {
                finalPermission = ex;
              }
            } else if (atoi(permission) == 2) {
              int trans = transactionMode(head,atoi(permission),convertFlagToInt(flag));
              if(trans == 1){
                addToHead(&head, fileName, clientip, atoi(permission), convertFlagToInt(flag), -1);
                finalPermission = trans;
              } else {
                finalPermission = trans;
              }
            }
          }
        }
        //Final permission will return 1 if its safe to access file
        if(finalPermission == 1){
            //Convert string flag to int
            int numFlag;
            if(strcmp(flag,"O_RDONLY") == 0) {
            numFlag = 0;
            } else if (strcmp(flag, "O_WRONLY") == 0) {
            numFlag = 1;
            } else if (strcmp(flag, "O_RDWR") == 0){
            numFlag = 2;
            } else {
              showError(ERRNO_ERROR);
              snprintf(outgoingBuffer, sizeof(outgoingBuffer), "%d", errno);
              strcat(outgoingBuffer, " ");
              strcat(outgoingBuffer, "-1");
            }

            //Create the file descriptor
            int filedescriptor;
            filedescriptor = open(filePath,numFlag);
            if(filedescriptor == -1) {
              //Add the value of FD in struct
              deleteNode(&head,0);
              showError(ERRNO_ERROR);
              snprintf(outgoingBuffer, sizeof(outgoingBuffer), "%d", errno);
              strcat(outgoingBuffer, " ");
              strcat(outgoingBuffer, "-1");
            } else {
              //Add the value of FD in struct
              head->FD = filedescriptor;
              char intoChar[BUFFER_SIZE];
              snprintf(intoChar, sizeof(intoChar), "%d", filedescriptor);
              strcpy(outgoingBuffer, intoChar);
            }
        } else {
          //Otherwise show permission Errors
          snprintf(outgoingBuffer, sizeof(outgoingBuffer), "%d", finalPermission);
          strcat(outgoingBuffer, " ");
          strcat(outgoingBuffer, "-1");
        }

      } else {
        //File doesnt Exist
        showError(ERRNO_ERROR);
        snprintf(outgoingBuffer, sizeof(outgoingBuffer), "%d", errno);
        strcat(outgoingBuffer, " ");
        strcat(outgoingBuffer, "-1");
      }

  	} else if (inputfunc == 2) { 
  		//Read = 2
  		//Clear the outgoing buffer
	    bzero(outgoingBuffer,BUFFER_SIZE);

	    char buff[BUFFER_SIZE];
  		strcpy(buff, buffer);

  		//Extract Values from buffer
  		char filedescriptor[BUFFER_SIZE];
  		char bytesToRead[BUFFER_SIZE];
  		char index[BUFFER_SIZE];
  		strcpy(filedescriptor, strtok(buff, " "));
  		strcpy(bytesToRead, strtok(NULL, " "));
  		strcpy(index, strtok(NULL, " "));

  		//Convert file descriptor and byte size to int
  		int numFd = atoi(filedescriptor);
  		int numByte = atoi(bytesToRead);

  		//Buffer that goes to client
  		char clientBuff[BUFFER_SIZE];
  		clientBuff[BUFFER_SIZE-1] = '\0';
  		int rnet = read(numFd,clientBuff,numByte);
  		if(rnet == -1){
  			showError(ERRNO_ERROR);
  			snprintf(outgoingBuffer, sizeof(outgoingBuffer), "%d", errno);
  			strcat(outgoingBuffer, " ");
  			strcat(outgoingBuffer, "-1");
  		} else {
  			char intoChar[BUFFER_SIZE];
  			snprintf(intoChar, sizeof(intoChar), "%d", rnet);
  			strcpy(outgoingBuffer, clientBuff);
  			strcat(outgoingBuffer, " ");
  			strcat(outgoingBuffer, intoChar);
  		}

  	} else if (inputfunc == 3) {
  		//Write = 3
  		//Clear the outgoing buffer
	    bzero(outgoingBuffer,BUFFER_SIZE);
	    
	    char buff[BUFFER_SIZE];
  		strcpy(buff, buffer);
  		//Extract Values from buffer
  		char filedescriptor[BUFFER_SIZE];
  		char bytesToRead[BUFFER_SIZE];
  		char toWrite[BUFFER_SIZE];
  		strcpy(filedescriptor, strtok(buff, " "));
  		strcpy(bytesToRead, strtok(NULL, " "));

  		int size = strlen(filedescriptor)+strlen(bytesToRead)+2;
  		char temp[BUFFER_SIZE];
  		strcpy(temp, buffer);
  		bzero(toWrite,BUFFER_SIZE);
  		int i;
  		for(i=size;temp[i]!='\0';i++){
  			toWrite[i-size] = temp[i];
  		}
  		//Remove the last two characters
  		toWrite[strlen(toWrite)-2] = '\0';
  		
		  //Convert file descriptor and byte size to int
  		int numFd = atoi(filedescriptor);
  		int numByte = atoi(bytesToRead);

  		//Try to Write
  		int wnet = write(numFd,toWrite,numByte);
  		if(wnet == -1){
  			showError(ERRNO_ERROR);
  			snprintf(outgoingBuffer, sizeof(outgoingBuffer), "%d", errno);
  			strcat(outgoingBuffer, " ");
  			strcat(outgoingBuffer, "-1");
  		} else {
  			char intoChar[BUFFER_SIZE];
  			snprintf(intoChar, sizeof(intoChar), "%d", wnet);
  			strcpy(outgoingBuffer, intoChar);
  		}

  	} else if (inputfunc == 4) {
  		//Close = 4
  		//Clear the outgoing buffer
	    bzero(outgoingBuffer,BUFFER_SIZE);

	    char buff[BUFFER_SIZE];
  		strcpy(buff, buffer);

  		//Extract Values from buffer
  		char filedescriptor[BUFFER_SIZE];
  		char index[BUFFER_SIZE];
  		strcpy(filedescriptor, strtok(buff, " "));
  		strcpy(index, strtok(NULL, " "));

  		//Pass the file descriptor to close
  		int fdToInt = atoi(filedescriptor);

      //Check IP and FD if they match then delete the node/client from linked list
      int temp = indexToDelete(head,clientip,fdToInt);
      if (temp > 0) {
        deleteNode( &head,temp-1);
        int fdReturnVal = close(fdToInt);
        if(fdReturnVal == -1){
          showError(ERRNO_ERROR);
          snprintf(outgoingBuffer, sizeof(outgoingBuffer), "%d", errno);
          strcat(outgoingBuffer, " ");
          strcat(outgoingBuffer, "-1");
        } else {
          char intoChar[BUFFER_SIZE];
          snprintf(intoChar, sizeof(intoChar), "%d", fdReturnVal);
          strcpy(outgoingBuffer, intoChar);
        }
      } else {
        //Otherwise show Close Errors
        snprintf(outgoingBuffer, sizeof(outgoingBuffer), "%d", -4);
        strcat(outgoingBuffer, " ");
        strcat(outgoingBuffer, "-1");
      }

  	} else {
  		showError(FUNCTION_ERROR);
  	}
    //Write the value back to Client
    int n;
    n = write(acc,outgoingBuffer,strlen(outgoingBuffer));
    if (n < 0) {
      showError(ERRNO_ERROR);
      return 0;     
    } 
    pthread_exit(0);
}

int main(int argc, char *argv[]) {
	//Create server socket
	int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(serverSocket < 0) {
		showError(ERRNO_ERROR);
		return 0;
	}
	struct sockaddr_in serverAddressStructure;
	bzero((char *) &serverAddressStructure, sizeof(serverAddressStructure));
	serverAddressStructure.sin_family = AF_INET;
	serverAddressStructure.sin_port = htons(PORT);
	serverAddressStructure.sin_addr.s_addr = INADDR_ANY;
	if(bind(serverSocket, (struct sockaddr *) &serverAddressStructure, sizeof(serverAddressStructure)) < 0) {
		showError(ERRNO_ERROR);
		return 0;
	}
	//Listen for Client request
	int listenClient = listen(serverSocket, 10);
	if (listenClient < 0) {
		showError(ERRNO_ERROR);
		return 0;
	}

	//Try to Accept clients connection
	int isAccepted;
	struct sockaddr_in clientAddressStructure;
  socklen_t clientStructSize = sizeof(clientAddressStructure);
	while(1) {
		  isAccepted = accept(serverSocket, (struct sockaddr *) &clientAddressStructure, &clientStructSize);
		  if (isAccepted < 0) {
        	showError(ERRNO_ERROR);
        	return 0;
		  }

		  //Create a worker thread
		  pthread_t workerThread;
	    struct tInfo *ti = malloc(sizeof(struct tInfo));
  
      char finalmsg[BUFFER_SIZE];
	    bzero(finalmsg,BUFFER_SIZE);
	    int n = read(isAccepted,finalmsg,BUFFER_SIZE-1);
	    if (n < 0) {
	        showError(ERRNO_ERROR);
	        return 0;	
	    }

      ti->isAcceptedVal = isAccepted;
      strcpy( ti->buff, finalmsg );
      ti-> clientAdd = clientAddressStructure;
	    //Create the Thread and pass the struct
	    if(pthread_create (&workerThread,NULL,workerThreadHandler,(void *)ti) !=0){
	    	showError(THREAD_C_ERROR);
	    	return 0;
	    }
	}
	close(serverSocket);
	return 0;
}