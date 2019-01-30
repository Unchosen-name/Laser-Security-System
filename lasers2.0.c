#include "gpiolib_addr.h"
#include "gpiolib_reg.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/watchdog.h> 	//needed for the watchdog specific constants
#include <unistd.h> 		//needed for sleep
#include <sys/ioctl.h> 		//needed for the ioctl function
#include <sys/time.h>           //for gettimeofday()

//Below is a macro that had been defined to output appropriate logging messages
//You can think of it as being similar to a function
//file        - will be the file pointer to the log file
//time        - will be the current time at which the message is being printed
//programName - will be the name of the program, in this case it will be Lab4Sample
//str         - will be a string that contains the message that will be printed to the file.
#define PRINT_MSG(file, time, programName, sev, str) \
	do{ \
         char* severity; \
         switch(sev){ \
            case 1: \
            severity = "Debug"; \
            break; \
            case 2: \
            severity = "Info"; \
            break; \
            case 3: \
            severity = "Warning"; \
            break; \
            case 4: \
            severity = "Error"; \
            break; \
            case 5: \
            severity = "Critical"; \
            break; \
         }\
			fprintf(file, "%s : %s : %s : %s", time, programName, severity, str); \
			fflush(file); \
	}while(0)
/*
#define PRINT_MSG(file, time, programName, sev, str) \
	do{ \
			fprintf(logFile, "%s : %s : %s", time, programName, str); \
			fflush(logFile); \
	}while(0)*/
//HARDWARE DEPENDENT CODE BELOW
#ifndef MARMOSET_TESTING

/* You may want to create helper functions for the Hardware Dependent functions*/

//This function accepts an errorCode. You can define what the corresponding error code
//will be for each type of error that may occur.
// Types of Errors:
// -1 - GPIO failed to initialized
// -2 - Invalid time entered
// -3 - No time entered
// -4 - Invalid diode number
void errorMessage(int errorCode)
{
   fprintf(stderr, "An error occured; the error code was %d \n", errorCode);
}


//This function should initialize the GPIO pins
GPIO_Handle initializeGPIO()
{
   GPIO_Handle gpio;
   gpio = gpiolib_init_gpio();
   if (gpio == NULL)
   {
      errorMessage(-1);
   }
   return gpio;
}

//This function will get the current time using the gettimeofday function
void getTime(char* buffer)
{
	//Create a timeval struct named tv
   struct timeval tv;

	//Create a time_t variable named curtime
   time_t curtime;


	//Get the current time and store it in the tv struct
   gettimeofday(&tv, NULL); 

	//Set curtime to be equal to the number of seconds in tv
   curtime=tv.tv_sec;

	//This will set buffer to be equal to a string that in
	//equivalent to the current date, in a month, day, year and
	//the current time in 24 hour notation.
   strftime(buffer,30,"%m-%d-%Y  %T.",localtime(&curtime));

} 

void cleanArray(char* dirty, int num){
   for (int i =0 ; i < num ; i++)
   {
      dirty[i] = ' ';
   }
}

enum READER {STARTS, END, IGNORE, SPACES, NEXT, EQUALS, WORDS, TIMEOUT, LOG_FILE, STATS_FILE};
enum READER r = STARTS;
enum READER curr;
enum READER prevs;

void readConfig(FILE* config, int* timeout, char* logFile, char* statsFile){

   char buffer[255];
   char logName[8] = "LOGFILE";
   char timeName[17]= "WATCHDOG_TIMEOUT";
   char statsName[10] = "STATSFILE";
   
   *timeout = 0;

   int log;
   int stats;
   int timer;   

   int logCount = 0;
   int statsCount = 0;
   int currCount = 0;
   char current[100];

   int i = 0;
  
   while(fgets(buffer,255,config) != NULL)
   {
	logCount = 0;
	statsCount = 0;
	currCount = 0;
	cleanArray(current,100);
	i= 0;
	r = STARTS;
      switch(r)
      {
         case STARTS:
            if (buffer[i] == ' ')
            {
               r = SPACES;
            }
            else if (buffer[i] == '\n')
            {
               r = NEXT;
            }
            else if (buffer[i] >= 32 && buffer[i] <= 126){
               r = WORDS;
               current[currCount] = buffer[i];
               currCount++;
            }
            else if (buffer[i] == '#')
            {
               r = IGNORE;
            }
            else if (buffer[i] == EOF)
            {
               r = END;
            }  
            prevs = STARTS;
            break;
         
         case END:
		i = 400;
            break;
         
         case IGNORE:
            currCount = 0;
            cleanArray(current, 100);
            if (buffer[i] == '\n')
            {
               r = NEXT;
            }
            else if (buffer[i] == ' ')
            {
               r = IGNORE;
            }
            else if (buffer[i] >= 32 && buffer[i] <= 126){
               r = IGNORE;      
            }
            else if (buffer[i] == '#')
            {
               r = IGNORE;
            }
            else if (buffer[i] == EOF)
            {
               r = END;
            }  
            prevs = IGNORE;
            break;
         
         case SPACES:
            log = 1;
            stats = 1;
            timer = 1;
            current[currCount+1] = '\0';
            if (prevs == WORDS)
            {
               for (int j = 0; j < currCount; j++)
               {
                  if (!(current[j] == logName[j]) && j < 8)
                  {
                     log = 0;
                  }
                  if (!(current[j] == statsName[j]) && j < 17)
                  {
                     stats = 0;
                  }
                  if (!(current[j] == timeName[j]) && j < 10)
                  {
                     timer = 0;
                  }
               }
            }
            else
            {
               log = 0;
               stats = 0;
               timer = 0;
            }
            if (log)
            {
               curr = LOG_FILE;
            }
            else if (stats)
            {
               curr  = STATS_FILE;
            }
            else if (timer)
            {
               curr = TIMEOUT;
            }
            cleanArray(current, 100);
            currCount = 0;
            if (buffer[i] == ' ')
            {
               r = SPACES;
            }
            else if (buffer[i] == '\n')
            {
               r = NEXT;
            }
            else if (buffer[i] >= 32 && buffer[i] <= 126){
               if (curr == TIMEOUT)
               {
                  *timeout = *timeout*10 + (buffer[i] - '0');
                  r = TIMEOUT;
               }
               else if (curr == LOG_FILE)
               {
                  current[currCount] = buffer[i];
                  r = LOG_FILE;
               }
               else if (curr == STATS_FILE)
               {
                  current[currCount] = buffer[i];
                  r = STATS_FILE;
               }
               else
               {
                  r = WORDS;
                  current[currCount] = buffer[i];
                  currCount++;      
               }
            }
            else if (buffer[i] == '#')
            {
               r = IGNORE;
            }
            else if (buffer[i] == EOF)
            {
               r = END;
            }
            prevs = SPACES;
            break;
         
         case NEXT:
            currCount = 0;
            cleanArray(current,100);
            if (buffer[i] == ' ')
            {
               r = SPACES;
            }
            else if (buffer[i] == '\n')
            {
               r = NEXT;
            }
            else if (buffer[i] >= 32 && buffer[i] <= 126){
               r = WORDS;
               current[currCount] = buffer[i];
               currCount++;
            }
            else if (buffer[i] == '#')
            {
               r = IGNORE;
            }
            else if (buffer[i] == EOF)
            {
               r = END;
            }  
            prevs = NEXT;
            break;
         
         case EQUALS:
            currCount = 0;
            if (buffer[i] == ' ')
            {
               r = SPACES;
            }
            else if (buffer[i] == '\n')
            {
               r = NEXT;
            }
            else if (buffer[i] >= 32 && buffer[i] <= 126)
	{
               if (curr == TIMEOUT)
               {
                  *timeout = *timeout*10 + (buffer[i] - '0');
                  r = TIMEOUT;
               }
               else if (curr == LOG_FILE)
               {
                  current[currCount] = buffer[i];
                  r = LOG_FILE;
               }
               else if (curr == STATS_FILE)
               {
                  current[currCount] = buffer[i];
                  r = STATS_FILE;
               }
               else
               {
                  r = WORDS;
                  current[currCount] = buffer[i];
                  currCount++;      
               }
            }
            else if (buffer[i] == '#')
            {
               r = IGNORE;
            }
            else if (buffer[i] == EOF)
            {
               r = END;
            }  
            prevs = EQUALS;
            break;
                 
         case WORDS:
            if (buffer[i] == ' ')
            {
               r  = SPACES;
            }
            else if (buffer[i] == '\n')
            {
               r = NEXT;
            }
            else if (buffer[i] >= 32 && buffer[i] <= 126)
            {
            	current[currCount] = buffer[i];
		currCount++;
		r = WORDS;
            }
            prevs = WORDS;
            break;
                  
         case TIMEOUT:
            if (buffer[i] == ' ')
            {
               curr = IGNORE;
               r = SPACES;
            }
            else if (buffer[i] == '\n')
            {
               curr = IGNORE;
               r = NEXT;
            }
            else if (buffer[i] >= '0' && buffer[i] <= '9')
		{
               *timeout  = *timeout*10 + (buffer[i] - '0');
               r = TIMEOUT;   
            }
            else if (buffer[i] == '#')
            {
               r = IGNORE;
            }
            else if (buffer[i] == EOF)
            {
               r = END;
            }  
            prevs = TIMEOUT;
            break;
         
         case LOG_FILE:
            logCount++;
            if (buffer[i] == ' ')
            {
               for (int j = 0; j < logCount; j++)
               {
                  logFile[j] = current[j];
               }
               logFile[logCount] = 0;
               curr = IGNORE;
               r = SPACES;
            }
            else if (buffer[i] == '\n')
            {
               for (int j = 0; j < logCount; j++)
               {
                  logFile[j] = current[j];
               }
               logFile[logCount] = 0;
               curr = IGNORE;
               r = NEXT;
            }
            else if (buffer[i] >= 32 && buffer[i] <= 126)
		{
               current[currCount] = buffer[i];
               currCount++;
               r = LOG_FILE;        
            }
            else if (buffer[i] == '#')
            {
               r = IGNORE;
            }
            else if (buffer[i] == EOF)
            {
               for (int j = 0; j < logCount; j++)
               {
                  logFile[j] = current[j];
               }
               r = END;
            }  
            prevs = LOG_FILE;
            break;
         
         case STATS_FILE:
            statsCount++;
            if (buffer[i] == ' ')
            {
               for (int j = 0; j < statsCount; j++)
               {
                  statsFile[j] = current[j];
               }
               curr = IGNORE;
               r = SPACES;
            }
            else if (buffer[i] == '\n')
            {
               for (int j = 0; j < statsCount; j++)
               {
                  statsFile[j] = current[j];
               }
               curr = IGNORE;
               r = NEXT;
            }
            else if (buffer[i] >= 32 && buffer[i] <= 126){
               current[currCount] = buffer[i];
               currCount++;
               r = STATS_FILE;        
            }
            else if (buffer[i] == '#')
            {
               r = IGNORE;
            }
            else if (buffer[i] == EOF)
            {
               r = END;
            }  
            prevs = STATS_FILE;
            break;
      }
      i++;
   }
}

//This function should accept the diode number (1 or 2) and output
//a 0 if the laser beam is not reaching the diode, a 1 if the laser
//beam is reaching the diode or -1 if an error occurs.
// Pins 3 and 17 initialized as pins ready for input from the photodiode output
#define LASER1_PIN_NUM 17
#define LASER2_PIN_NUM 3
int laserDiodeStatus(GPIO_Handle gpio, int diodeNumber)
{
   // Checks if the GPIO is initialized
   if(gpio == NULL)
   {
      return -1;
   }
   
   // Checks the current diode
   if(diodeNumber == 1)
   {
      // Initializes the level register for the number
      uint32_t level_reg = gpiolib_read_reg(gpio, GPLEV(0));
   
      // Checks if the pin state of the first bit in the current pin is on or off (1 or 0)
      // Returns the respective value
      if(level_reg & (1 << LASER1_PIN_NUM))
      {
         return 1;
      }
      else
      {
         return 0;
      }
   }
   else if (diodeNumber == 2)
   {
   
      uint32_t level_reg = gpiolib_read_reg(gpio, GPLEV(0));
      
      if(level_reg & (1 << LASER2_PIN_NUM))
      {
         return 1;
      }
      else
      {
         return 0;
      }
   }
   // If somehow the diodeNumber is not 1 or 2
   else
   {  
      // Returns an error code
      errorMessage(-4);
      return -1;
   }
}

#endif
//END OF HARDWARE DEPENDENT CODE

//This function will output the number of times each laser was broken
//and it will output how many objects have moved into and out of the room.

//laser1Count will be how many times laser 1 is broken (the left laser).
//laser2Count will be how many times laser 2 is broken (the right laser).
//numberIn will be the number  of objects that moved into the room.
//numberOut will be the number of objects that moved out of the room.
void outputMessage(int laser1Count, int laser2Count, int numberIn, int numberOut)
{
   printf("Laser 1 was broken %d times \n", laser1Count);
   printf("Laser 2 was broken %d times \n", laser2Count);
   printf("%d objects entered the room \n", numberIn);
   printf("%d objects exitted the room \n", numberOut);
}

#ifndef MARMOSET_TESTING

// Initializes the enumerated state machine
// Features the start, when the left laser is off, the right laser is off, when both are off, and when both are on again
// Creates a state variable to store current value
// Creates a state variable to store the previous value
enum STATE {START, LEFT_OFF, RIGHT_OFF, BOTH_OFF, BOTH_ON};
enum STATE s = START;
enum STATE prev = START;

// Main functions with functionality
int main(const int argc, const char* const argv[])
{
	//We want to accept a command line argument that will be the number
	//of seconds that the program will run for, so we need to ensure that
	//the user actually gives us a time to run the program for
   /*if(argc < 2)
   {
      errorMessage(-3);
      return -1;
   }*/
	
   // Create a string that contains the program name
   const char* argName = argv[0];
   
   int i = 0;
   int nameLength = 0;
   
   while(argName[i] != 0)
   {
      nameLength++;
      i++;
   }
   
   char programName[nameLength];
   i = 0;
   
   // Copy the name of the program without the ./ at the start of argv[0]
   while (argName[i+2] != 0)
   {
      programName[i] = argName[i+2];
      i++;
   }
   
   // Create a file pointer named configFile
   // Set it to point to the laserConfig.cfg, setting it to read it
   FILE* configFile = fopen("/home/pi/laserConfig.cfg", "r");
   
   // Outputs a warning message if file cannot be opened
   if (!configFile)
   {
      perror("The config file could not be opened");
      return -1;
   }
   
   // Declare variables to pass into the readConfig function
   int timeout;
   char logFileName[50];
   char statsFileName[50];
   
   // Call the readConfig function to read from the config file
   readConfig(configFile, &timeout, logFileName, statsFileName);
   
   //Close the configFile after finished reading from the file
   fclose(configFile);
   
   // Create a file pointer to point to the log file
   // Set it to point to the file from the congfig file and append to whatever it writes to
   FILE* logFile = fopen(logFileName, "a");
   
   // Outputs a warning message if file cannot be opened
   if (!configFile)
   {
      perror("The config file could not be opened");
      return -1;
   }
   
   FILE* statsFile = fopen(statsFileName, "a");

   //Create a char array that will be used to hold the time values
   char timeVal[30];
   getTime(timeVal);
   
   //Initialize the GPIO pins
   GPIO_Handle gpio = initializeGPIO();
   //Get the current time
   getTime(timeVal);
   //Log that GPIO pins have been initialized
   PRINT_MSG(logFile, timeVal, programName, 2, "The GPIO pins have been initialized\n\n");
   
   //This variable will be used to access the /dev/watchdog file, similar to how the GPIO_Handle works
   int watchdog;
   
   //We use the open function here to open the /dev/watchdog file. If it does
   //not open, then we output an error message. We do not use fopen() 
   //because we not want to create a file it doesn't exist
   if ((watchdog = open("/dev/watchdog", O_RDWR | O_NOCTTY)) < 0) {
      printf("Error: Couldn't open watchdog device! %d\n", watchdog);
      return -1;
   } 
   
   //Get the current time
   getTime(timeVal);
   //Log that the watchdog file has been opened
   PRINT_MSG(logFile, timeVal, programName, 2, "The Watchdog file has been opened\n\n");
   
   //This line uses the ioctl function to set the time limit of the watchdog
	//timer to 15 seconds. The time limit can not be set higher that 15 seconds
	//so please make a note of that when creating your own programs.
	//If we try to set it to any value greater than 15, then it will reject that
	//value and continue to use the previously set time limit
   ioctl(watchdog, WDIOC_SETTIMEOUT, &timeout);
	
	//Get the current time
   getTime(timeVal);
	//Log that the Watchdog time limit has been set
   PRINT_MSG(logFile, timeVal, programName, 2, "The Watchdog time limit has been set\n\n");

	//The value of timeout will be changed to whatever the current time limit of the
	//watchdog timer is
   ioctl(watchdog, WDIOC_GETTIMEOUT, &timeout);

	//This print statement will confirm to us if the time limit has been properly
	//changed. The \n will create a newline character similar to what endl does.
   printf("The watchdog timeout is %d seconds.\n\n", timeout);
   

   // These variables will represent the number of times the respective laser is broken
   // as well as indicate the number of object that are in and out of the room
   int laser1Count = 0;
   int laser2Count = 0;
   int numberIn = 0;
   int numberOut = 0;

   // Initialize the state of the laser diodes
   int laser_state1 = laserDiodeStatus(gpio, 1);
   int laser_state2 = laserDiodeStatus(gpio, 2);

   time_t curTime = time(NULL);

	//In the while condition, we check the current time minus the start time and
	//see if it is less than the number of seconds that was given from the 
	//command line.
   while(1)
   {
   	//The code in this while loop is identical to the code from buttonBlink
   	//in lab 2. You can think of the laser/photodiode setup as a type of switch
      laser_state1 = laserDiodeStatus(gpio, 1);
      laser_state2 = laserDiodeStatus(gpio, 2);
      
      switch(s)
      {  
         // If it just started...
         case START:
            // and the left laser is turned off...
            if (!laser_state1 && laser_state2)
            {
               // Log that the left laser turned off
               getTime(timeVal);
               PRINT_MSG(logFile, timeVal, programName, 2, "The left laser turned off\n\n");
               // Current state is LEFT_OFF
               s = LEFT_OFF;
               prev = START;
               laser1Count++;
            }
            // and the right laser is turned off...
            if (!laser_state2 && laser_state1)
            {
               // Log that the right laser turned off
               getTime(timeVal);
               PRINT_MSG(logFile, timeVal, programName, 2, "The right laser turned off\n\n");
               // Current state is RIGHT_OFF
               s = RIGHT_OFF;
               prev = START;
               laser2Count++;
            }
            break;
         
         // If both lasers are on ...
         case BOTH_ON:
            // and the left laser is turned off...
            if (!laser_state1 && laser_state2)
            {
               // Log that the left laser turned off
               getTime(timeVal);
               PRINT_MSG(logFile, timeVal, programName, 2, "The left laser turned off\n\n");
               // Current state is LEFT_OFF
               s = LEFT_OFF;
               prev = BOTH_ON;
               laser1Count++; //
            }
            // and the right laser is turned off...
            if (!laser_state2 && laser_state1)
            {
               // Log that the right laser turned off
               getTime(timeVal);
               PRINT_MSG(logFile, timeVal, programName, 2, "The right laser turned off\n\n");
               // Current state is RIGHT_OFF
               s = RIGHT_OFF;
               prev = BOTH_ON;
               laser2Count++; //
            }
            // Stores current state
            break;
         
         // If both lasers aren't on ...
         case BOTH_OFF:
            // and the left laser is now back on...
            if (laser_state1 && !laser_state2)
            {
               // Log that the right laser is turned off
               getTime(timeVal);
               PRINT_MSG(logFile, timeVal, programName, 2, "The right laser is turned off\n\n");
               // Current state is RIGHT_OFF
               s = RIGHT_OFF;
               prev = BOTH_OFF;
            }
            // and the right laser is now back on...
            if (laser_state2 && !laser_state1)
            {
               // Log that the left laser is turned off
               getTime(timeVal);
               PRINT_MSG(logFile, timeVal, programName, 2, "The left laser is turned off\n\n");
               // Current state is LEFT_OFF
               s = LEFT_OFF;
               prev = BOTH_OFF;
            }
            // Stores current state
            break;
         
         // If left laser is off...
         case LEFT_OFF:
            // and is turned back on...
            if (laser_state1)
            {
               // and the right laser is off...
               if (!laser_state2)
               {  
                  // Log that the right laser is turned off
                  getTime(timeVal);
                  PRINT_MSG(logFile, timeVal, programName, 2, "The right laser is turned off\n\n");
                  // Current state is RIGHT_OFF
                  // Successfully moves from left laser to right
                  s = RIGHT_OFF;
                  prev = LEFT_OFF;
                  laser2Count++;
               }
               // and the lasers are both on...
               else if (laser_state2)
               {
                  // Log that both lasers are on
                  getTime(timeVal);
                  PRINT_MSG(logFile, timeVal, programName, 2, "Both lasers are turned on\n\n");
                  // Current state is BOTH_ON
                  s = BOTH_ON;
                  // and if previous state was when both were off...
                  if (prev == BOTH_OFF)
                  {
                     // Log that the object moved out
                     getTime(timeVal);
                     PRINT_MSG(logFile, timeVal, programName, 2, "An object has moved out\n\n");
                     // An object has gone from right to left
                     numberOut++;
                  }
                  prev = LEFT_OFF;
               }
            }
            // and the right laser is turned off...
            if (!laser_state2)
            {
               //Log that both lasers are off
               getTime(timeVal);
               PRINT_MSG(logFile, timeVal, programName, 2, "Both lasers are turned off\n\n");
               // Current state is BOTH_OFF
               s = BOTH_OFF;
               prev = LEFT_OFF;
               laser2Count++;
            }
            break;
         
         // If right laser is off...
         case RIGHT_OFF:
            // and is turned back on...
            if (laser_state2)
            {
               // and the left laser is off...
               if (!laser_state1)
               {
                  //Log that the left laser is off
                  getTime(timeVal);
                  PRINT_MSG(logFile, timeVal, programName, 2, "The left laser is turned off\n\n");
                  // Current state is LEFT_OFF
                  // Successfully moves object from right laser to left
                  s = LEFT_OFF;
                  prev = RIGHT_OFF;
                  laser1Count++;
               }
               // and lasers are both on...
               else if (laser_state1)
               {
                  //Log that both lasers are on  
                  getTime(timeVal);
                  PRINT_MSG(logFile, timeVal, programName, 2, "Both lasers are turned on\n\n");
                  // Current state is BOTH_ON
                  s = BOTH_ON;
                  // and if previously both lasers were off...
                  if (prev == BOTH_OFF)
                  {
                     //Log that an object has moved in
                     getTime(timeVal);
                     PRINT_MSG(logFile, timeVal, programName, 2, "An object has moved in\n\n");
                     // An object has moved left to right
                     numberIn++;
                  }
                  prev = RIGHT_OFF;
               }
            }
            // and the left laser is turned off...
            if (!laser_state1)
            {
               //Log that the both lasers are turned off
               getTime(timeVal);
               PRINT_MSG(logFile, timeVal, programName, 2, "Both of the lasers are turned off\n\n");
               // Current state is BOTH_OFF
               s = BOTH_OFF;
               prev = RIGHT_OFF;
               laser1Count++;
            }
            break;
      }
      // Inputs a time delay 
      usleep(10000);
      
      if (!(curTime % 2000000))
      {
      //This ioctl call will write to the watchdog file and prevent 
      //the system from rebooting. It does this every 2 seconds, so 
      //setting the watchdog timer lower than this will cause the timer
      //to reset the Pi after 1 second
         ioctl(watchdog, WDIOC_KEEPALIVE, 0);
         getTime(timeVal);
      //Log that the Watchdog was kicked
         PRINT_MSG(logFile, timeVal, programName, 2, "The Watchdog was kicked\n\n");
         
         // Updates the stats file 
         getTime(timeVal);
         fprintf(statsFile, "%s : %s : Laser 1 was broken %d times \n%s : %s : Laser 2 was broken %d times \n%s : %s : An object went in %d times \n%s : %s : An object went out %d times \n", timeVal, programName, laser1Count, time, programName, laser2Count, time, programName, numberIn, time, programName, numberOut); 
         fflush(statsFile);
      }
   }
   
   // Outputs the message that indicates the number of laser breaks, and number of objects in and out of the room
   outputMessage(laser1Count, laser2Count, numberIn, numberOut);
 
  
   //Writing a V to the watchdog file will disable to watchdog and prevent it from
	//resetting the system
   write(watchdog, "V", 1);
   getTime(timeVal);
	//Log that the Watchdog was disabled
   PRINT_MSG(logFile, timeVal, programName, 2, "The Watchdog was disabled\n\n");

	//Close the watchdog file so that it is not accidentally tampered with
   close(watchdog);
   getTime(timeVal);
	//Log that the Watchdog was closed
   PRINT_MSG(logFile, timeVal, programName, 2, "The Watchdog was closed\n\n");

   //Free the GPIO pins
   gpiolib_free_gpio(gpio);
   getTime(timeVal);
   
   //Log that the GPIO pins were freed
   PRINT_MSG(logFile, timeVal, programName, 2, "The GPIO pins have been freed\n\n");
   //Return to end program
   
   return 0;

}

#endif