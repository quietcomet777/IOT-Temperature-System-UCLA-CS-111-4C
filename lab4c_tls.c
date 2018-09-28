//NAME: Dan Molina
//EMAIL: digzmol45@gmail.com
//ID: 104823116


#include <mraa/aio.h>
#include <mraa/gpio.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>
#include <ctype.h>
#include<sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <resolv.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

int run_flag = 1;

void reverse(char *str, int len)
{
    int i=0, j=len-1, temp;
    while (i<j)
    {
        temp = str[i];
        str[i] = str[j];
        str[j] = temp;
        i++; j--;
    }
}


int intToStr(int x, char str[], int d)
{
    int i = 0;
    while (x)
    {
        str[i++] = (x%10) + '0';
        x = x/10;
    }
 
    // If number of digits required is more, then
    // add 0s at the beginning
    while (i < d)
        str[i++] = '0';
 
    reverse(str, i);
    str[i] = '\0';
    return i;
}

void ftoa(float n, char *res, int afterpoint)
{
    // Extract integer part
    int ipart = (int)n;
 
    // Extract floating part
    float fpart = n - (float)ipart;
 
    // convert integer part to string
    int i = intToStr(ipart, res, 0);
 
    // check for display option after point
    if (afterpoint != 0)
    {
        res[i] = '.';  // add dot
 
        // Get the value of fraction part upto given no.
        // of points after dot. The third parameter is needed
        // to handle cases like 233.007
        fpart = fpart * pow(10, afterpoint);
 
        intToStr((int)fpart, res + i + 1, afterpoint);
    }
}

void buttonPressed()
{
	run_flag = 0;
}

int main(int argc, char* argv[])
{

	mraa_aio_context tempatureSensor = mraa_aio_init(1);
	mraa_gpio_context button = mraa_gpio_init(60);

	mraa_gpio_dir(button, MRAA_GPIO_IN);

	const int B = 4275;
	const int R0 = 100000;
	int logFd = -1;
	int scale = 1; // 1 for farienheit 0 for celcius
	int period = 1;
	int tempRead = 0;
	int bigBufCounter = 0;
	int option;
	int sysFail;
	time_t rawTime;
	int NextReport = 0;
	struct tm *sTime;
	char *id = NULL;
	char *hostName = NULL;
	int portNum = -1;

	mraa_gpio_isr(button,MRAA_GPIO_EDGE_RISING,&buttonPressed,NULL);


	struct option longopts[] =
		{
   		{ "period", 1,     NULL, 'p' },       //struct used for handling command line arguments
   		{ "scale",  1,     NULL, 's' },
   		{ "log",    1,     NULL, 'l' },
		{ "id",     1,     NULL, 'i' },
		{ "host",   1,     NULL, 'h' },
  		{0,0,0,0}
		};

	while((option = getopt_long(argc, argv, ":pslih:", longopts, NULL)) != -1)
	{

		switch(option)
		{

			case 'i':
			if(strlen(optarg) != 9)
			{
				fprintf(stderr,"id must be 9 digits\n");
				exit(1);
			}

			id = optarg;
			break;

			case 'h':
			hostName = optarg;
			break;


			case 'p':
			period = atoi(optarg);
			break;

			case 's':
			if(strlen(optarg) > 1)
			{
				fprintf(stderr, "%s\n", "Option should only be 1 char");
				exit(1);
			}

			if(*optarg == 'C')
			{
				scale = 0;
			}

			else if(*optarg == 'F')
			{
				scale = 1;
			}

			else
			{
				fprintf(stderr, "%s\n", "Option not supported");
				exit(1);
			}

			break;

			case 'l':
			logFd = creat(optarg, 0666);
			if(logFd < 0)
			{
				fprintf(stderr, "%s\n", "Bad File Name");
			}
			break;

			default:
			fprintf(stderr,"%s\n", "Argument Not Supported");
			exit(1);

		}

	}

	if(optind +1 < argc)
	{
		fprintf(stderr, "%s\n", "you need the port number" );
	}

	if(optind < argc)
	{
		portNum = atoi(argv[optind]);
	}

	if(portNum == -1)
	{
		fprintf(stderr, "%s\n", "No port number" );
		exit(1);
	}


	if(logFd == -1)
	{
		fprintf(stderr,"You did not give a log\n");
		exit(1);
	}

	if(id == NULL)
	{
		fprintf(stderr,"You need to give an id\n");
		exit(1);
	}

	if(hostName == NULL)
	{
		fprintf(stderr,"You need to give a host name\n");
		exit(1);
	}


	int socketFd;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    socketFd = socket(AF_INET, SOCK_STREAM, 0);

    if(socketFd < 0) 
    {
        fprintf(stderr,"ERROR, Opening Socket\n");
        exit(2);
    }

        server = gethostbyname(hostName);

    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(1);
    }

    memset((char *) &serv_addr, 0, sizeof(serv_addr)); //same socket code used in tcp and lab 1b

    serv_addr.sin_family = AF_INET;

    memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);

    serv_addr.sin_port = htons(portNum);

    if(connect(socketFd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
    {
        fprintf(stderr, "ERROR connecting\n");
        exit(2);
    }

 
    SSL_CTX *ctx;   // here is ssl connection code coppied from a UTAH tutorial 
    SSL *ssl;

    SSL_library_init();		
    ctx = SSL_CTX_new(SSLv23_client_method());			/* Create new context */
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }

    ssl = SSL_new(ctx);						/* create new SSL connection state */
    SSL_set_fd(ssl, socketFd);				/* attach the socket descriptor */
	SSL_connect(ssl);	
        

    char idString[15];
    sprintf(idString, "ID=%s\n", id);
    SSL_write(ssl, idString, strlen(idString));

   	write(logFd,idString,strlen(idString));



	struct pollfd stdinPoll[1];
	stdinPoll[0].fd = socketFd; // changed fd
	stdinPoll[0].events = POLLIN;
	char buf[1024];
	char bigBuf[4096];
	memset(buf,0,sizeof(buf));
	memset(bigBuf,0,sizeof(bigBuf));
	int reportOn = 1;
	char *PossibleCommand1 = (char*)malloc(sizeof(char)* 5);
	char *PossibleCommand2 = (char*)malloc(sizeof(char)* 8);
	char *timeString;
	char intbuf[10];

	while(run_flag)
	{
		rawTime = time(NULL);
		if((rawTime >= NextReport) & reportOn )
		{
			NextReport = rawTime + period;
			sTime = localtime(&rawTime);
			timeString = asctime(sTime);
			tempRead = mraa_aio_read(tempatureSensor);

			float R = 1023.0/tempRead-1.0;
			R = R0*R;

			float temperature = 1.0/(log(R/R0)/B+1/298.15)-273.15;

			if(scale)
			{
				temperature = temperature * 9 / 5 + 32;
			}


			ftoa(temperature,intbuf,1);

			char serverWriteBuf[30];
			int k;
			int currentChars = 0;
			for(k = 11;  k < 20;k++)
			{
				serverWriteBuf[currentChars] = timeString[k];
				currentChars++;
			}

			for(k = 0; k < (signed int)strlen(intbuf); k++)
			{
				serverWriteBuf[currentChars] = intbuf[k];
				currentChars++;
			}

			serverWriteBuf[currentChars] = '\n';
			currentChars++;

			SSL_write(ssl,serverWriteBuf,currentChars);

			if(logFd > 0)
			{
				write(logFd,serverWriteBuf,currentChars);
			}

		}

		poll(stdinPoll,1,0);

		if(stdinPoll[0].revents)
		{
			sysFail = SSL_read(ssl,&buf,1024);
			int k;

			for(k = 0; k < sysFail; k++)
			{
				if(buf[k] == '\n')
				{
					bigBuf[bigBufCounter] = '\0';
					if(!strcmp(bigBuf,"OFF"))
					{
						run_flag = 0;
						if(logFd > -1)
						{
							write(logFd,"OFF\n",4);
						}
					}

					else if(!strcmp(bigBuf,"SCALE=F"))
					{
						scale = 1;
						if(logFd > -1)
						{
							write(logFd,"SCALE=F\n",8);
						}
					}

					else if(!strcmp(bigBuf,"SCALE=C"))
					{
						scale = 0;
						if(logFd > -1)
						{
							write(logFd,"SCALE=C\n",8);
						}
					}

					else if(!strcmp(bigBuf,"STOP"))
					{
						reportOn = 0;
						if(logFd > -1)
						{
							write(logFd,"STOP\n",5);
						}
					}

					else if(!strcmp(bigBuf,"START"))
					{
						reportOn = 1;
						if(logFd > -1)
						{
							write(logFd,"START\n",6);
						}
					}

					else
					{
						strncpy(PossibleCommand1,bigBuf,5);
						strncpy(PossibleCommand2,bigBuf,8);
						PossibleCommand1[4] = '\0';
						PossibleCommand2[7] = '\0';

						if(!strcmp(PossibleCommand1,"LOG "))
						{
							int k;
							int currentChars = 0;
							char nextbuf[50];
							for(k = 4; k < bigBufCounter-4; k++)
							{
								nextbuf[currentChars] = bigBuf[k];
								currentChars++;
							}

							nextbuf[currentChars] = '\n';
							currentChars++;

							SSL_write(ssl,nextbuf,strlen(nextbuf));
							if(logFd > -1)
							{
								write(logFd,bigBuf,bigBufCounter);
								write(logFd,"\n",1);
							}
						}

						else if(!strcmp(PossibleCommand2,"PERIOD="))
						{
							int k;
							for(k = 7; k < bigBufCounter; k++)
							{
								if(!isdigit(bigBuf[k]))
								{
									fprintf(stderr,"%s\n","Thats not a valid period");
									exit(1);
								}
							}
							period = atoi((bigBuf+7));
							if(logFd > -1)
							{
								write(logFd,bigBuf,strlen(bigBuf));
								write(logFd,"\n",1);
							}
						}

						else
						{
							fprintf(stderr,"%s\n", "We Dont support that command");
							exit(1);
						}

					}

					memset(bigBuf,0,sizeof(bigBuf));
					bigBufCounter = 0;
				}

				else
				{
					bigBuf[bigBufCounter] = buf[k];
					bigBufCounter++;
				}
			}
			memset(buf,0,sizeof(buf));
		}




	}
	 rawTime = time(NULL);
	 sTime = localtime(&rawTime);
	 timeString = asctime(sTime);
	 int k;
	 int currentChars = 0;
	 char lastBuf[20];

	 for(k = 11;  k < 20;k++)
	{
		lastBuf[currentChars] = timeString[k];
		currentChars++;
	}

	strcat(lastBuf,"SHUTDOWN\n");

	SSL_write(ssl,lastBuf,currentChars+9);

	if(logFd > -1)
	{
		write(logFd,timeString+11,9);
		write(logFd,"SHUTDOWN\n",9);
	}

	mraa_gpio_close(button);
	mraa_aio_close(tempatureSensor);
	return 0;
}
