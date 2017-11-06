//Name: Deepkumar Patel, NetID: dgp52
//Name: Viraj Patel, NetID: vjp60

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "libnetfiles.h"

extern int h_errno;

//Holds the client mode
int permi = 0;

#define PORT 9672
#define BUFFER_SIZE 1024

struct hostent *serverAddr;

//Enum for flags
typedef enum {
	O_RDONLY, O_WRONLY, O_RDWR
} OpenFlags;

//Enum for different types of messages
typedef enum {
	FLAG_ERROR, ERRNO_ERROR, HOST_ERROR, 
	PERMI_ERROR
} ErrorType;

//Shows error messages
void showError (ErrorType errorIs) {
	switch(errorIs){
		case FLAG_ERROR:
			printf("ERROR: Invalid Flag\n");
		break;
		case ERRNO_ERROR:
			printf("ERROR: %s\n", strerror(errno));
		break;
		case HOST_ERROR:
			if(h_errno == 1){
				printf("ERROR: Host not found\n");
			}
		break;
		case PERMI_ERROR:
			printf("ERROR: Wrong permission\n");
		break;
	}
}

//Shows an error with permissions
void showPermissionError(int error){
  if(error == -1){
    printf("ERROR: file is open in unrestricted mode\n");
  } else if (error == -2) {
    printf("ERROR: file is open in exclusive mode\n");
  } else if (error == -3){
    printf("ERROR: file is open in transaction mode\n");
  } else if (error == -4) {
    printf("ERROR: Invalid file descriptor\n");
  }
}

//Returns 0 if the host exists and file mode is valid
int netserverinit(char * hostname, int filemode){
	h_errno = 0;
	serverAddr = gethostbyname(hostname);
	if (serverAddr == NULL){
		showError(HOST_ERROR);
		return -1;		
    } else {
    	if(filemode==0 || filemode==1 || filemode==2) {
			permi = filemode;
			return 0;
		} else {
			showError(PERMI_ERROR);
			return -1;
		}
  }
}

//Returns the file descriptor of the file
int netopen(const char*pathname, int flags) {
	OpenFlags flag = flags;
	int filedescriptor = -1;
	const char *mode;
	const char *filepathname = pathname;
	if(flag == O_RDONLY){
		mode = "O_RDONLY";
	} else if (flag == O_WRONLY) {
		mode = "O_WRONLY";
	} else if (flag == O_RDWR) {
		mode = "O_RDWR";
	} else {
		showError(FLAG_ERROR);
	}
	//Convert the permission to string
	char *permission; //By default all client will have Unrestricted mode
	if(permi == 0){
		permission = "0";
	} else if (permi == 1) {
		permission = "1";
	} else if (permi == 2) {
		permission = "2";
	} else {
		permission = "0";
	}

	//Make a socket
	int netOpenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(netOpenSocket < 0) {
		showError(ERRNO_ERROR);
		return filedescriptor;
	}
	//Connect to the net file server
	struct sockaddr_in socketAddrStruct;
	bzero((char *) &socketAddrStruct, sizeof(socketAddrStruct));
	socketAddrStruct.sin_family = AF_INET;
	socketAddrStruct.sin_port = htons(PORT);
	bcopy((char *)serverAddr->h_addr, (char *)&socketAddrStruct.sin_addr.s_addr, serverAddr->h_length);
   	int checkConnection = connect(netOpenSocket,(struct sockaddr *)&socketAddrStruct,sizeof(socketAddrStruct));
    //Check the connection
    if (checkConnection < 0) {
        showError(ERRNO_ERROR);
        return filedescriptor;	
    }
    //Send following info to server
    char finalmsg[BUFFER_SIZE];
    bzero(finalmsg,BUFFER_SIZE);
    strcpy(finalmsg, filepathname);
    strcat(finalmsg, " ");
	  strcat(finalmsg, mode);
    strcat(finalmsg, " ");
    strcat(finalmsg, permission);
    strcat(finalmsg, " ");
	  strcat(finalmsg, "1");
    int n = write(netOpenSocket,finalmsg,strlen(finalmsg));
    if (n < 0) {
        showError(ERRNO_ERROR);
        return filedescriptor;	    
    }
    //Try to read from server
    bzero(finalmsg,BUFFER_SIZE);
    n = read(netOpenSocket,finalmsg,BUFFER_SIZE-1);
    if (n < 0) {
        showError(ERRNO_ERROR);
        return filedescriptor;	
    } 
    //Read the file descriptor and errno message from server
    char copyReadmsg[BUFFER_SIZE];
    bzero(copyReadmsg, BUFFER_SIZE);
    strcpy(copyReadmsg, finalmsg);

    //Check if file descriptor is -1
	int temp = -1;
  	char * pch;
  	pch = strtok (copyReadmsg," ");
  	while (pch != NULL) {
  		temp = atoi(pch);
    	pch = strtok (NULL, " ");
  	}
  	if(temp == -1){
  		filedescriptor = -1;
  		//Get the errno code
  		char errnoCode[BUFFER_SIZE];
  		strcpy(errnoCode, finalmsg);
  		char errnoVal[BUFFER_SIZE];
  		char fDesc[BUFFER_SIZE];
  		strcpy(errnoVal, strtok(errnoCode, " "));
  		strcpy(fDesc, strtok(NULL, " "));

  		//Convert Enum code to int
  		int serverErrnoCode = atoi(errnoVal);
      //Check if the error code is negative
      if(serverErrnoCode < 0){
        //Set Permiddion error
        showPermissionError(serverErrnoCode);
      } else {
        //Set Errno to serverEnumcode
        errno = serverErrnoCode;
        showError(ERRNO_ERROR);
      }
  	} else {
  		filedescriptor = temp;
  	}
  close(netOpenSocket);
	return filedescriptor;
}

//Returns number of bytes actually read
int netread(int fildes, void *buf, size_t nbyte) {
	//Convert file descriptor and nbyte to string for buffer
	char filedescriptor[BUFFER_SIZE];
  	snprintf(filedescriptor, sizeof(filedescriptor), "%d", fildes);
  	char bytesToRead[BUFFER_SIZE];
  	snprintf(bytesToRead, sizeof(bytesToRead), "%d", (unsigned int)nbyte);
  	int returnedFunValue = -1;
  	//Make a socket
	int netOpenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(netOpenSocket < 0) {
		showError(ERRNO_ERROR);
		return returnedFunValue;
	}
	//Connect to the net file server
	struct sockaddr_in socketAddrStruct;
	bzero((char *) &socketAddrStruct, sizeof(socketAddrStruct));
	socketAddrStruct.sin_family = AF_INET;
	socketAddrStruct.sin_port = htons(PORT);
	bcopy((char *)serverAddr->h_addr, (char *)&socketAddrStruct.sin_addr.s_addr, serverAddr->h_length);
   	int checkConnection = connect(netOpenSocket,(struct sockaddr *)&socketAddrStruct,sizeof(socketAddrStruct));
 	//Check the connection
    if (checkConnection < 0) {
        showError(ERRNO_ERROR);
        return returnedFunValue;	
    }
    //Send following info to server
    char finalmsg[BUFFER_SIZE];
    bzero(finalmsg,BUFFER_SIZE);
    strcpy(finalmsg, filedescriptor);
    strcat(finalmsg, " ");
	strcat(finalmsg, bytesToRead);
    strcat(finalmsg, " ");
	strcat(finalmsg, "2");
    int n = write(netOpenSocket,finalmsg,strlen(finalmsg));
    if (n < 0) {
        showError(ERRNO_ERROR);
        return returnedFunValue;	    
    }
    //Try to read from server
    bzero(finalmsg,BUFFER_SIZE);
    n = read(netOpenSocket,finalmsg,BUFFER_SIZE-1);
    if (n < 0) {
        showError(ERRNO_ERROR);
        return returnedFunValue;	
    }

    //Read the buffer, read bytes, errno message from server
    char copyReadmsg[BUFFER_SIZE];
    bzero(copyReadmsg, BUFFER_SIZE);
    strcpy(copyReadmsg, finalmsg);

	//Check if return value is -1
	int temp = -1;
  	char * pch;
  	pch = strtok (copyReadmsg," ");
  	while (pch != NULL) {
  		temp = atoi(pch);
    	pch = strtok (NULL, " ");
  	}
  	if(temp == -1){
  		returnedFunValue = -1;
  		//Get the errno code
  		char errnoCode[BUFFER_SIZE];
  		strcpy(errnoCode, finalmsg);
  		char errnoVal[BUFFER_SIZE];
  		char reVal[BUFFER_SIZE];
  		strcpy(errnoVal, strtok(errnoCode, " "));
  		strcpy(reVal, strtok(NULL, " "));

  		//Convert Enum code to int
  		int serverErrnoCode = atoi(errnoVal);

  		//Set Errno to serverEnumcode
  		errno = serverErrnoCode;
  		showError(ERRNO_ERROR);
  	} else {
  		returnedFunValue = temp;
  		char sizeByte[BUFFER_SIZE];
  		snprintf(sizeByte, sizeof(sizeByte), "%d", temp);
  		//-1 is a space
  		int len = strlen(sizeByte)+1;
  		//Copy string to buf
  		strncpy(buf, finalmsg, strlen(finalmsg)-len);
  	}
    close(netOpenSocket);
    return returnedFunValue;
}

//Returns number of bytes written to the file
int netwrite(int fildes, const void *buf, size_t nbyte) {
	//Convert file descriptor and nbyte to string for buffer
	char filedescriptor[BUFFER_SIZE];
  	snprintf(filedescriptor, sizeof(filedescriptor), "%d", fildes);
  	char bytesToWrite[BUFFER_SIZE];
  	snprintf(bytesToWrite, sizeof(bytesToWrite), "%d", (unsigned int)nbyte);
  	int returnedFunValue = -1;
  	//Make a socket
	int netOpenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(netOpenSocket < 0) {
		showError(ERRNO_ERROR);
		return returnedFunValue;
	}
	//Connect to the net file server
	struct sockaddr_in socketAddrStruct;
	bzero((char *) &socketAddrStruct, sizeof(socketAddrStruct));
	socketAddrStruct.sin_family = AF_INET;
	socketAddrStruct.sin_port = htons(PORT);
	bcopy((char *)serverAddr->h_addr, (char *)&socketAddrStruct.sin_addr.s_addr, serverAddr->h_length);
   	int checkConnection = connect(netOpenSocket,(struct sockaddr *)&socketAddrStruct,sizeof(socketAddrStruct));
   	//Check the connection
    if (checkConnection < 0) {
        showError(ERRNO_ERROR);
        return returnedFunValue;	
    }
    //Send following info to server
    char finalmsg[BUFFER_SIZE];
    bzero(finalmsg,BUFFER_SIZE);
    strcat(finalmsg, filedescriptor);
    strcat(finalmsg, " ");
	strcat(finalmsg, bytesToWrite);
    strcat(finalmsg, " ");
	strcat(finalmsg, buf);
	strcat(finalmsg, " ");
	strcat(finalmsg, "3");
    int n = write(netOpenSocket,finalmsg,strlen(finalmsg));
    if (n < 0) {
        showError(ERRNO_ERROR);
        return returnedFunValue;	    
    }

    //Try to read from server
    bzero(finalmsg,BUFFER_SIZE);
    n = read(netOpenSocket,finalmsg,BUFFER_SIZE-1);
    if (n < 0) {
        showError(ERRNO_ERROR);
        return returnedFunValue;	
    }

    //Read the file descriptor and errno message from server
    char copyReadmsg[BUFFER_SIZE];
    bzero(copyReadmsg, BUFFER_SIZE);
    strcpy(copyReadmsg, finalmsg);

    //Check if file descriptor is -1
	int temp = -1;
  	char * pch;
  	pch = strtok (copyReadmsg," ");
  	while (pch != NULL) {
  		temp = atoi(pch);
    	pch = strtok (NULL, " ");
  	}
  	if(temp == -1){
  		returnedFunValue = -1;
  		//Get the errno code
  		char errnoCode[BUFFER_SIZE];
  		strcpy(errnoCode, finalmsg);
  		char errnoVal[BUFFER_SIZE];
  		char fDesc[BUFFER_SIZE];
  		strcpy(errnoVal, strtok(errnoCode, " "));
  		strcpy(fDesc, strtok(NULL, " "));

  		//Convert Enum code to int
  		int serverErrnoCode = atoi(errnoVal);

  		//Set Errno to serverEnumcode
  		errno = serverErrnoCode;
  		showError(ERRNO_ERROR);
  	} else {
  		returnedFunValue = temp;
  	}
    close(netOpenSocket);
    return returnedFunValue;
} 

//Returns zero on success
int netclose(int fildes) {
	//Convert file descriptor to string for buffer
	char filedescriptor[BUFFER_SIZE];
  	snprintf(filedescriptor, sizeof(filedescriptor), "%d", fildes);
  	int returnedFunValue = -1;
  	//Make a socket
	int netOpenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(netOpenSocket < 0) {
		showError(ERRNO_ERROR);
		return returnedFunValue;
	}
	//Connect to the net file server
	struct sockaddr_in socketAddrStruct;
	bzero((char *) &socketAddrStruct, sizeof(socketAddrStruct));
	socketAddrStruct.sin_family = AF_INET;
	socketAddrStruct.sin_port = htons(PORT);
	bcopy((char *)serverAddr->h_addr, (char *)&socketAddrStruct.sin_addr.s_addr, serverAddr->h_length);
   	int checkConnection = connect(netOpenSocket,(struct sockaddr *)&socketAddrStruct,sizeof(socketAddrStruct));
   	//Check the connection
    if (checkConnection < 0) {
        showError(ERRNO_ERROR);
        return returnedFunValue;	
    }
    //Send following info to server
    char finalmsg[BUFFER_SIZE];
    bzero(finalmsg,BUFFER_SIZE);
    strcat(finalmsg, filedescriptor);
    strcat(finalmsg, " ");
	strcat(finalmsg, "4");
    int n = write(netOpenSocket,finalmsg,strlen(finalmsg));
    if (n < 0) {
        showError(ERRNO_ERROR);
        return returnedFunValue;	    
    }
	//Try to read from server
    bzero(finalmsg,BUFFER_SIZE);
    n = read(netOpenSocket,finalmsg,BUFFER_SIZE-1);
    if (n < 0) {
        showError(ERRNO_ERROR);
        return returnedFunValue;	
    }
	//Read the file descriptor and errno message from server
    char copyReadmsg[BUFFER_SIZE];
    bzero(copyReadmsg, BUFFER_SIZE);
    strcpy(copyReadmsg, finalmsg);
	//Check if file descriptor is -1
	int temp = -1;
  	char * pch;
  	pch = strtok (copyReadmsg," ");
  	while (pch != NULL) {
  		temp = atoi(pch);
    	pch = strtok (NULL, " ");
  	}
  	if(temp == -1){
  		returnedFunValue = -1;
  		//Get the errno code
  		char errnoCode[BUFFER_SIZE];
  		strcpy(errnoCode, finalmsg);
  		char errnoVal[BUFFER_SIZE];
  		char fDesc[BUFFER_SIZE];
  		strcpy(errnoVal, strtok(errnoCode, " "));
  		strcpy(fDesc, strtok(NULL, " "));

  		//Convert Enum code to int
  		int serverErrnoCode = atoi(errnoVal);
      //If the error code is greater than zero then show errno error
      if(serverErrnoCode > 0){
        //Set Errno to serverEnumcode
        errno = serverErrnoCode;
        showError(ERRNO_ERROR);
      } else {
        showPermissionError(serverErrnoCode);
      }
  	} else {
  		returnedFunValue = temp;
  	}
    close(netOpenSocket);
    return returnedFunValue;
}
