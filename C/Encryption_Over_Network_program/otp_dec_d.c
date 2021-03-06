/********************************************************************************
 * Author David Ramirez
 * Date: 11/24/17
 * Description: Recieves plain text and key from otp_enc program, performs one-time
 * 		pad style decryption, and return decrypted text to otp_enc. This program
 * 		is meant to be run in the background as a daemon. Use this program with
 * 		command "otp_enc_d port".
 * *****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

//global constant
#define MAX_REC_SIZE 70000

//global variables
int portNumber;

//error function to report issue
void errorMsg(const char *msg) 
{ 
	perror(msg); 
} 

//decrypt cipher text
char* decrypt(int recLen, char* totalCT, char* totalK)
{
	char keyChar;
	char cipherChar;
	char decChar;
	char* decText = malloc(recLen);
	memset(decText, '\0', sizeof(decText));

	//for each char in cipher text
	int i =0;
	for (i; i < (recLen-1); i++)
	{
		keyChar = totalK[i];
		//if key character is a space, set it to value right after capital letters
		if (keyChar == 32)
		{
			keyChar = 91;
		}
		
		cipherChar = totalCT[i];
		//if text character is a space, set it to value right after capital letters
		if (cipherChar == 32)
		{
			cipherChar = 91;
		}
	
		//subtract 65 from both key and text chars to have possible values represented as 0-26
		keyChar -= 65;
		cipherChar -= 65;

		decChar = cipherChar - keyChar;

		//make sure value is between 0 and 26
		if (decChar < 0)
		{
			decChar += 27;
		}

		else
		{
			decChar = decChar % 27;	
		}	

		//if char is space, reflect that
		if (decChar == 26)
		{
			decChar = 32;
			decText[i] = decChar;
		}
		//make into capital char
		else
		{
			decChar += 65;
			decText[i] = decChar;
		}
	}

	//return decrypted text
	return decText;
}

//connect to client, receive key and dipher text, send decrypted text back to client
void connRecSend()
{
	int listenSocketFD, establishedConnectionFD,  charsRead;
	socklen_t sizeOfClientInfo;
	char buffer[256];
	struct sockaddr_in serverAddress, clientAddress;

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) 
	{
		errorMsg("OTP_DEC_D: ERROR opening socket");
		exit(1);
	}

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
	{
		errorMsg("OTP_DEC_D: ERROR on binding");
		exit(1);
	}

	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections

	while (1){
		// Accept a connection, blocking if one is not available until one connects
		sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
		if (establishedConnectionFD < 0)
		{
			errorMsg("OTP_DEC_D: ERROR on accept");
			exit(1);
		}

		//create child process for each connection	
		pid_t pid = fork();
		switch(pid)
		{
			case -1:
				errorMsg("OTP_DEC_D: ERROR creating child process");

			//child process
			case 0:
				// Get the message from the client and display it
				memset(buffer, '\0', 256);
				charsRead = recv(establishedConnectionFD, buffer, 1, 0); // Read the client's message from the socket
				if (charsRead < 0)
				{
					errorMsg("OTP_DEC_D: ERROR reading from socket");
					exit(1);
				}

				//if client is not correct decryption client, error and exit
				if (strcmp(buffer, "d") != 0)
				{
					int incorrectClient = 1;
					int charsWritten = send(establishedConnectionFD, &incorrectClient, 1, 0);
					if (charsWritten < 0)
					{
						errorMsg("OTP_DEC_D: ERROR writing to socket");
						exit(1);  
					}
					fprintf(stderr, "OTP_DEC_D: ERROR incorrect client");
					exit(1);
				}
			
				//send code resenting correct server to client
				int correctClient = 0;
                                int charsWritten = send(establishedConnectionFD, &correctClient, 1, 0);
                                if (charsWritten < 0)
                                {
                                       errorMsg("OTP_DEC_D: ERROR writing to socket");
                                       exit(1);
                                }

				//recieve the length of the plain text
				int recLen;
				charsRead = recv(establishedConnectionFD, &recLen, sizeof(recLen), 0); 
				recLen +=1;
			
				//recieve key	
				char* totalKey = malloc(recLen);
				memset(totalKey, '\0', sizeof(totalKey));
				int totalRead = 0;
				while (totalRead < recLen)
				{

					//make sure the text to be received is not more than max
					if (recLen > MAX_REC_SIZE)
					{
						recLen = MAX_REC_SIZE;
					}
				
					//create buffer to hold each received message
					char recText[recLen - totalRead];

					//receive key from client
					charsRead = recv(establishedConnectionFD, recText, sizeof(recText), 0);
					totalRead += charsRead;
					strcat(totalKey, recText);
				}

				//recieve cipher text
				totalRead = 0;
				char* totalCipherText = malloc(recLen + 1);
				memset(totalCipherText, '\0', sizeof(totalCipherText));

				while (totalRead < recLen )
				{
					//make sure the text to be received is not more than max
					if (recLen > MAX_REC_SIZE)
					{
						recLen = MAX_REC_SIZE;
					}
				
					//create buffer to hold each received message
					char recText[recLen - totalRead];

					//receive key from client
					charsRead = recv(establishedConnectionFD, recText, sizeof(recText), 0);
					totalRead += charsRead;
					strcat(totalCipherText, recText);
				}

				//perform decryption
				char* decText = decrypt(recLen,totalCipherText, totalKey);

				//while not all of the character have been sent, keep sending key
        			int totalChars = 0;
        			charsWritten = 0;
        			while (totalChars < recLen)
        			{

                			int sendLength;
                			//if the length of the sent text is less than the max send
                			//then send the enitre text. If not, send max send size of text
                			if (recLen <= MAX_REC_SIZE)
                			{
                			        sendLength = recLen;
                			}
                			else
                			{
                			        sendLength = MAX_REC_SIZE;
        		        	}
		
        	        		//copy unwritten chars in keyBuff to sendThis
        	        		char sendThis[sendLength - totalChars +1];
        	        		memset(sendThis, '\0', sizeof(sendThis));
        	        		strncpy(sendThis, decText + totalChars, sizeof(sendThis));
	
	                		//send unwritten chars to server
	                		charsWritten = send(establishedConnectionFD,sendThis,sizeof(sendThis)-1, 0);
          		      		if (charsWritten < 0)
                			{
                			        errorMsg("OTP_DEC_D: ERROR");
                			        exit(1);
                			}

                			//keep track of total chars written to server
                			totalChars += charsWritten;
        			}
	
				//free all mallocd memory
				free(decText);
				free(totalCipherText);
				free(totalKey);				
				
				//exit child process
				exit(0);
		}
	}
	close(listenSocketFD);
}

int main(int argc, char *argv[])
{
	//make sure at least 2 args passed
	if (argc < 2) 
	{ 
		fprintf(stderr,"OTP_DEC_D: USAGE ERROR %s port\n", argv[0]); 
		exit(1); 
	} // Check usage & args
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string

	//connect to client, recieve key and cipher text, return decoded text to client
	connRecSend();

	return 0;
}	
