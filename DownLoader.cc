/*
 * Copyright 2013 bekemod.hu
 */

#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include "DownLoader.h"
#include "cmd.h"
#include "mem.h"


//#define __DEBUG__

#if !defined (_WIN32)
  #define Sleep(X) usleep(X)
  #include <termios.h>
#else
  #include <windows.h>
  #include <tchar.h>
  #include <stdio.h>
#endif


#if !defined (_WIN32)
s_baudrate BAUD_RATES[]={{50, B50}, {75, B75}, {110, B110}, {134, B134}, {150, B150},
{200, B200}, {300, B300}, {600, B600}, {1200, B1200}, {1800, B1800}, {2400, B2400},
{4800, B4800}, {9600, B9600}, {19200, B19200}, {38400, B38400}, {57600, B57600},
{115200, B115200}, {230400, B230400}, {460800, B460800}, {500000, B500000}, 
{576000, B576000}, {921600, B921600}, {1000000, B1000000}, {1152000, B1152000},
{1500000, B1500000}, {2000000, B2000000}, {2500000, B2500000}, {3000000, B3000000},
{3500000, B3500000}, {4000000, B4000000}, {0,0}};
#else
s_baudrate BAUD_RATES[]={{110, CBR_110}, {300, CBR_300}, {600, CBR_600}, {1200, CBR_1200},
     {2400, CBR_2400}, {4800, CBR_4800}, {9600, CBR_9600}, {14400, CBR_14400}, {19200, CBR_19200},
     {38400, CBR_38400}, {57600, CBR_57600}, {115200, CBR_115200}, {128000, CBR_128000}, 
     {256000, CBR_256000}};   
#endif

/*HANDLE OpenConnection (HANDLE *pComDev,  char *pPortName, char *pBaudRate);*/
bool    WriteCommBlock (FILETYPE FDHandle, char *pBuffer ,  int BytesToWrite);
int    ReadCommBlock  (FILETYPE FDHandle, char *pBuffer,   int MaxLength);
/*BOOL   CloseConnection(HANDLE *pComdDev);*/

bool ReceiveData(FILETYPE FDHandle, char * pBuffer, int BytesToReceive);
void PrintUsage(void);
void PrintFeatures();
/*eFamily ReadID(HANDLE *pComDev);*/
void    ReadPM(FILETYPE FDHandle, char * pReadPMAddress, eFamily Family);
void    ReadEE(FILETYPE FDHandle, char * pReadEEAddress, eFamily Family);
void    SendHexFile(FILETYPE FDHandle, FILE * pFile, eFamily Family);
/* Global features here. */
const char*   pBaudRate       = "9600";
long    baudrate_code   = 13; /* !! Must equal the BAUD_RATES["9600"].value !!
                               * for correct default baud rate settings !!! */
char*   pInterfaceName = NULL;
char*   pReadPMAddress = NULL;
char*   pReadEEAddress = NULL;

int Default_Timeout = 10; /* Default timeout in 1/10 sec. 10*1/10 7 1 sec. */
int DEBUG_MODE = 0;

long SearchBaudRates(long baudrates)
{ int i = 0;
    while ((BAUD_RATES[i].brate != 0) && ((BAUD_RATES[i].brate) != baudrates))
    {
        i++;
    }
    return BAUD_RATES[i].value;
}

#if !defined (_WIN32)
#else
void GetLastErrorAndExit()
{
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );
  printf ("Last error: %s.\n", lpMsgBuf);
  LocalFree(lpMsgBuf);
  LocalFree(lpDisplayBuf);
  ExitProcess(dw); 
};
#endif

int main(int argc, char**argv) {

  cmd_cCmd ProgCommand(argv, "i:b:p:e:");     
  FILE* pFile = NULL;
        
  
  
#if !defined (_WIN32)
  struct termios		OldSerial;
  struct termios		NewSerial;
  int FDSerial = -1;
#else
  DCB SerialDCB;
//  COMMTIMEOUTS  
  HANDLE FDSerial;
#endif
    
	while (ProgCommand.Next())
	{
		switch (ProgCommand.Option())
		{
			case 'i': /* Interface option: COM1, COM2, etc */
				if (ProgCommand.Arg() == NULL)
				{
					printf("\n-i requires argument\n");
					PrintUsage();
					return 0;
				}
				else
				{
					pInterfaceName = ProgCommand.Arg();
				}
		
				break;

			case 'b': /* Baudrate */
				if (ProgCommand.Arg() == NULL)
				{
					printf("\n-b requires argument\n");
					PrintUsage();
					return 0;
				}
				else
				{
					pBaudRate = ProgCommand.Arg();
                    long BaudRate = atol(pBaudRate);
                    baudrate_code = SearchBaudRates(BaudRate);
                
                    if (!baudrate_code)
                    {
                        printf("\nBad baudrate value.\n");
                        PrintUsage();
                        return 0;
                    };
				}
				break;

			case 'p': /* Read Program Memory */
				if (ProgCommand.Arg() == NULL)
				{
					printf("\n-p requires argument\n");
					PrintUsage();
					return 0;
				}
				else
				{
					pReadPMAddress = ProgCommand.Arg();
					assert(pReadPMAddress[0] == '0' && pReadPMAddress[1] =='x');
					assert(isxdigit(pReadPMAddress[2]));
					assert(isxdigit(pReadPMAddress[3]));
					assert(isxdigit(pReadPMAddress[4]));
					assert(isxdigit(pReadPMAddress[5]));
					assert(isxdigit(pReadPMAddress[6]));
					assert(isxdigit(pReadPMAddress[7]));
				}
		
				break;
				
			case 'e': /* Read EEPROM Memory */
				if (ProgCommand.Arg() == NULL)
				{
					printf("\n-e requires argument\n");
					PrintUsage();
					return 0;
				}
				else
				{
					pReadEEAddress = ProgCommand.Arg();
					assert(pReadEEAddress[0] == '0' && pReadEEAddress[1] =='x');
					assert(isxdigit(pReadEEAddress[2]));
					assert(isxdigit(pReadEEAddress[3]));
					assert(isxdigit(pReadEEAddress[4]));
					assert(isxdigit(pReadEEAddress[5]));
					assert(isxdigit(pReadEEAddress[6]));
					assert(isxdigit(pReadEEAddress[7]));
				}
		
				break;

			case '.':
				if ((pFile = fopen(ProgCommand.Arg(), "r")) == NULL)
				{
					printf("\nCan't open file: %s\n", ProgCommand.Arg());
					return 0;
				}else
        {
					printf("\nThe %s file sucessfully opened.\n", ProgCommand.Arg());  
        }

				break;

			default:
				printf("\nUnknown option `%c'\n", ProgCommand.Option());
				PrintUsage();
				return 0;
		}
	}  
  PrintFeatures();
  
  if ((!pInterfaceName) || (!strcmp((pInterfaceName), "nul")))
  {
   DEBUG_MODE = 1;
   printf("DEBUG MODE ON, no any serial port opened!");
  }
  else
  {
      
#if !defined (_WIN32)

  if((FDSerial = open( pInterfaceName, O_RDWR|O_NOCTTY )) < 0 )
#else
        /* Windows open serial communication. Arguments:
   * 1.: pInterfaceName     pointer the file name eg. COM1,... COM15
   * 2.: GENERIC_READ | GENERIC_WRITE These are is evident.
   * 3.: Prevents other processes from opening a file or device if they request 
   *    delete, read, or write access.
   * 4.: NULL If this parameter is NULL, the handle returned by CreateFile cannot be 
   *    inherited by any child processes the application may create and the file 
   *    or device associated with the returned handle gets a default security descriptor.
   * 5.: OPEN_EXISTING
   * 
   * The CreateFile function can create a handle to a communications resource, 
   * such as the serial port COM1. For communications resources, the 
   * dwCreationDisposition parameter must be OPEN_EXISTING, the dwShareMode 
   * parameter must be zero (exclusive access), and the hTemplateFile parameter 
   * must be NULL. Read, write, or read/write access can be specified, and the 
   * handle can be opened for overlapped I/O. specify a COM port number greater 
   * than 9, use the following syntax: "\\.\COM10". This syntax works for all 
   * port numbers and hardware that allows COM port numbers to be specified.
   */
  if ((FDSerial = CreateFile(pInterfaceName, GENERIC_READ | GENERIC_WRITE, 0, NULL, 
          OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
#endif
  {
    printf("Can't open %s Interface. ", pInterfaceName);
    GetLastErrorAndExit();
  }else
  {
    printf("The %s interface successfully opened.\n", pInterfaceName);      
  }
  
#if !defined (_WIN32)  
  
  tcgetattr(FDSerial,&OldSerial);   // Save serial port features
  /* CLOCAL = Ignore modem control lines.
   * CS8 = Character size mask. Values are CS5, CS6, CS7, or CS8.
   * CREAD = Enable receiver.
   */
  SerialDCB.c_cflag = baudrate_code | CS8 | CLOCAL | CREAD;
  /* IGNPAR = Ignore framing errors and parity errors. */
  SerialDCB.c_iflag = IGNPAR;
  /* ONLCR = (XSI) Map NL to CR-NL on output.*/
  SerialDCB.c_oflag = ONLCR;
  /* The setting of the ICANON canon flag in c_lflag determines whether the 
   * terminal is operating in canonical mode (ICANON set) or noncanonical mode 
   * (ICANON unset). By default, ICANON set.*/
  SerialDCB.c_lflag = 0;
  /* special characters 
   * The c_cc array defines the terminal special characters. 
   * The symbolic indices (initial values) and meaning are
   * Minimum number of characters for noncanonical read (MIN).
   */
  SerialDCB.c_cc[VMIN] = 0; 
  
  /* Timeout in deciseconds for noncanonical read (TIME). */
  
  SerialDCB.c_cc[VTIME] = Default_Timeout;  /* Timeout for Default (1 sec.))*/
  
  tcflush(FDSerial,TCIFLUSH);
  tcsetattr(FDSerial,TCSANOW,&SerialDCB);
#else
      
   //  Initialize the DCB structure.
   ZeroMemory(&SerialDCB, sizeof(DCB));
   SerialDCB.DCBlength = sizeof(DCB);

   //  Build on the current configuration by first retrieving all current settings.
   
   if (!GetCommState(FDSerial, &SerialDCB)) 
   {
      //  Handle the error.
      printf ("GetCommState failed in %s interface.\n", pInterfaceName);
      GetLastErrorAndExit();
   }
  
   SerialDCB.BaudRate = baudrate_code;
   SerialDCB.ByteSize = 8;
   SerialDCB.Parity = NOPARITY;
   SerialDCB.StopBits = ONESTOPBIT;
   SerialDCB.fRtsControl = RTS_CONTROL_DISABLE;
   SerialDCB.fDtrControl = DTR_CONTROL_DISABLE;
   SerialDCB.fBinary = TRUE;
   SerialDCB.fParity = FALSE;

   if (!SetCommState(FDSerial, &SerialDCB))
   {
      //  Handle the error.
      printf ("SetCommState failed in %s interface.\n", pInterfaceName);
      GetLastErrorAndExit();
   }

   CloseHandle(FDSerial);
   printf("Test end ---------------------------------------------------------");
   exit(0);
   
//   CloseFile(FDSerial);
   
#endif
  } /* DEBUG_MODE if */
//  StartAddress pReadPMAddress
  eFamily Family = dsPIC30F;
  
  
    /* ----------------------------------------------------------------------
   * 
   * 
   * ----------------------------------------------------------------------*/
  
  SendHexFile(FDSerial, pFile, Family);

  
	/* Process Read PM request and exit */
	if(pReadPMAddress != NULL)
	{
      
		ReadPM(FDSerial, pReadPMAddress, Family);
		return 0;
  }

	if(pReadEEAddress != NULL)
	{
		ReadEE(FDSerial, pReadEEAddress, Family);
		return 0;
	}  
  if (!DEBUG_MODE)
  {
#if !defined (_WIN32)
  
  close(FDSerial);
#else
  CloseHandle(FDSerial);
#endif      
  }
  
  printf("Program done.\n");
  return 0;
  

  printf("The user type 'q' for quit from program.\n");
  return 0;
}

/******************************************************************************/
void PrintUsage(void)
{
	printf("\nUsage: \"16-Bit Flash Programmer.exe\" -i interface [-bpe] hexfile\n\n");
	printf("Options:\n\n");
	printf("  -i\n");
	printf("       specifies serial interface name such as COM1, COM2, etc\n\n");
	printf("  -b\n");
	printf("       specifies baudrate for serial interface. Default is 9600\n\n");
	printf("  -p\n");
	printf("       read program flash. Must provide address to read in HEX format: -p 0x000100\n\n");
	printf("  -e\n");
	printf("       read EEPROM. Must provide address to read in HEX format: -e 0x7FFC00\n\n");
}

void PrintFeatures()
{
  printf("------------------- Print all features ------------------\n");
  printf("BaudRate      : %s, code = %ld\n", pBaudRate, baudrate_code);
  printf("InterfaceName : %s\n", pInterfaceName);
  printf("ReadPMAddress : %s\n", pReadPMAddress);
  printf("ReadEEAddress : %s\n", pReadEEAddress);
  printf("---------------------------------------------------------\n");
}

/*****************************************************************************
 * SendHexFile for linux 
 */
#if !defined (_WIN32)
void SendHexFile(int FDSerial, FILE * pFile, eFamily Family)
#else
void SendHexFile(HANDLE FDSerial, FILE * pFile, eFamily Family)
#endif
{
	char Buffer[BUFFER_SIZE];
	int  ExtAddr = 0;

	/* Initialize Memory Memory size =  */
	mem_cMemRow ** ppMemory = (mem_cMemRow **)malloc(sizeof(mem_cMemRow *) * 
          PM_SIZE + sizeof(mem_cMemRow *) * EE_SIZE + sizeof(mem_cMemRow *) * CM_SIZE);

	for(int Row = 0; Row < PM_SIZE; Row++)
	{
		ppMemory[Row] = new mem_cMemRow(mem_cMemRow::Program, 0x000000, Row, Family);
	}

	for(int Row = 0; Row < EE_SIZE; Row++)
	{
		ppMemory[Row + PM_SIZE] = new mem_cMemRow(mem_cMemRow::EEProm, 0x7FF000, Row, Family);
	}

	for(int Row = 0; Row < CM_SIZE; Row++)
	{
		ppMemory[Row + PM_SIZE + EE_SIZE] = new mem_cMemRow(mem_cMemRow::Configuration, 0xF80000, Row, Family);
	}
	
	printf("\nReading HexFile");

	while(fgets(Buffer, sizeof(Buffer), pFile) != NULL)
	{
		int ByteCount;
		int Address;
		int RecordType;

		sscanf(Buffer+1, "%2x%4x%2x", &ByteCount, &Address, &RecordType);
    
    if (DEBUG_MODE)
    {
      printf("ByteCount=%2x, Address=%4x, RecordType=%2x\n", ByteCount, Address, RecordType);
    }

		if(RecordType == 0)
		{
			Address = (Address + ExtAddr) / 2;
			
			for(int CharCount = 0; CharCount < ByteCount*2; CharCount += 4, Address++)
			{
				bool bInserted = false;

				for(int Row = 0; Row < (PM_SIZE + EE_SIZE + CM_SIZE); Row++)
				{
					if((bInserted = ppMemory[Row]->InsertData(Address, Buffer + 9 + CharCount)) == true)
					{
						break;
					}
				}

				if(bInserted != true)
				{
					printf("Bad Hex file: 0x%xAddress out of range\n", Address);
					assert(0);
				}
			}
		}
		else if(RecordType == 1)
		{
		}
		else if(RecordType == 4)
		{
			sscanf(Buffer+9, "%4x", &ExtAddr);

			ExtAddr = ExtAddr << 16;
		}
		else
		{
			assert(!"Unknown hex record type\n");
		}
	}

//	 Preserve first two locations for bootloader 
	{
		char Data[32];
		int          RowSize;

		if(Family == dsPIC30F)
		{
			RowSize = PM30F_ROW_SIZE;
		}
		else
		{
			RowSize = PM33F_ROW_SIZE;
		}

		Buffer[0] = COMMAND_READ_PM;
		Buffer[1] = 0x00;
		Buffer[2] = 0x00;
		Buffer[3] = 0x00;

		WriteCommBlock(FDSerial, Buffer, 4);

		Sleep(100);

		printf("\nReading Target\n");
    
    if (!DEBUG_MODE) ReceiveData(FDSerial, Buffer, RowSize * 3);
    else ;
		
		sprintf(Data, "%02x%02x%02x00%02x%02x%02x00",   Buffer[2] & 0xFF,
														Buffer[1] & 0xFF,
														Buffer[0] & 0xFF,
														Buffer[5] & 0xFF,
														Buffer[4] & 0xFF,
														Buffer[3] & 0xFF);

		ppMemory[0]->InsertData(0x000000, Data);
		ppMemory[0]->InsertData(0x000001, Data + 4);
		ppMemory[0]->InsertData(0x000002, Data + 8);
		ppMemory[0]->InsertData(0x000003, Data + 12);
	}

	for(int Row = 0; Row < (PM_SIZE + EE_SIZE + CM_SIZE); Row++)
	{
		ppMemory[Row]->FormatData();
	}

	printf("\nProgramming Device ");

	for(int Row = 0; Row < (PM_SIZE + EE_SIZE + CM_SIZE); Row++)
	{
//		ppMemory[Row]->SendData(FDSerial); FIXME
	}

	
	Buffer[0] = COMMAND_RESET; //Reset target device
	
	WriteCommBlock(FDSerial, Buffer, 1);

	Sleep(100); 

	printf(" Done.\n");
}

/******************************************************************************/
#if !defined (_WIN32)
bool WriteCommBlock(int FDSerial, char *pBuffer , int BytesToWrite)
#else
bool WriteCommBlock(HANDLE FDSerial, char *pBuffer , int BytesToWrite)
#endif
{
	bool       bWriteStat   = 0;

	printf("\nWriteCommBlock(%i, %p , %i)\n", FDSerial, pBuffer, BytesToWrite);

  if (!DEBUG_MODE)
//  write(FDSerial, pBuffer, BytesToWrite);

/*	if(WriteFile(*pComDev,pBuffer,BytesToWrite,&BytesWritten,&osWrite) == false)
	{
		assert(GetLastError() == ERROR_IO_PENDING);
		return (false);
	}*/

	return (true);
}

/******************************************************************************/
bool ReceiveData(FILETYPE FDSerial, char * pBuffer, int BytesToReceive)
{
	int Size = 0;

	while(Size != BytesToReceive)
	{
//		Size += read(FDSerial, pBuffer + Size, BytesToReceive - Size);  FIXME
    if (!Size) 
    {
        printf("ERROR ! Read timeout elapsed !\n");
        return -1;
    } else
    {
    }
	}
  return Size;
}

/******************************************************************************/
int ReadCommBlock(int FDSerial, char * pBuffer, int MaxLength )
{
    
    return read(FDSerial, pBuffer, MaxLength);
    
/*	DWORD      Length;
	COMSTAT    ComStat    = {0};
	DWORD      ErrorFlags = 0;
	OVERLAPPED osRead     = {0,0,0};*/

	/* only try to read number of bytes in queue */
//   ClearCommError(*pComDev, &ErrorFlags, &ComStat);

/*	Length = min((DWORD)MaxLength, ComStat.cbInQue);
   
	if(Length > 0)
	{
		if(ReadFile(*pComDev, pBuffer, Length, &Length, &osRead) == FALSE)
		{
			Length = 0 ;

			ClearCommError(*pComDev, &ErrorFlags, &ComStat);

			assert(ErrorFlags == 0);
		}
	}
	else
	{
		Length = 0;
		Sleep(1);
	}

	return (Length);*/
}

/******************************************************************************/
void ReadPM(FILETYPE FDSerial, char * pReadPMAddress, eFamily Family)
{
	int          Count;
	unsigned int ReadAddress;
	char         Buffer[BUFFER_SIZE];
	int          RowSize;

	if(Family == dsPIC30F)
	{
		RowSize = PM30F_ROW_SIZE;
	}
	else
	{
		RowSize = PM33F_ROW_SIZE;
	}

	sscanf(pReadPMAddress, "%x", &ReadAddress);

	ReadAddress = ReadAddress - ReadAddress % (RowSize * 2);
		
	Buffer[0] = COMMAND_READ_PM;
	Buffer[1] = ReadAddress & 0xFF;
	Buffer[2] = (ReadAddress >> 8) & 0xFF;
	Buffer[3] = (ReadAddress >> 16) & 0xFF;

	WriteCommBlock(FDSerial, Buffer, 4);

	if (ReceiveData(FDSerial, Buffer, RowSize * 3) > 0)
  {
    for(Count = 0; Count < RowSize * 3;)
    {
      printf("0x%06x: ", ReadAddress);
      printf("%02x",Buffer[Count++] & 0xFF);
      printf("%02x",Buffer[Count++] & 0xFF);
      printf("%02x ",Buffer[Count++] & 0xFF);
      printf("%02x",Buffer[Count++] & 0xFF);
      printf("%02x",Buffer[Count++] & 0xFF);
      printf("%02x ",Buffer[Count++] & 0xFF);
      printf("%02x",Buffer[Count++] & 0xFF);
      printf("%02x",Buffer[Count++] & 0xFF);
      printf("%02x ",Buffer[Count++] & 0xFF);
      printf("%02x",Buffer[Count++] & 0xFF);
      printf("%02x",Buffer[Count++] & 0xFF);
      printf("%02x\n",Buffer[Count++] & 0xFF);
      ReadAddress = ReadAddress + 8;
    }
  }
}

/******************************************************************************/
void ReadEE(FILETYPE FDHandle, char * pReadEEAddress, eFamily Family)
{
	int          Count;
	unsigned int ReadAddress;
	char         Buffer[BUFFER_SIZE];

	assert(Family != dsPIC33F);
	assert(Family != PIC24H);

	sscanf(pReadEEAddress, "%x", &ReadAddress);

	ReadAddress = ReadAddress - ReadAddress % (EE30F_ROW_SIZE * 2);
	
	Buffer[0] = COMMAND_READ_EE;
	Buffer[1] = ReadAddress & 0xFF;
	Buffer[2] = (ReadAddress >> 8) & 0xFF;
	Buffer[3] = (ReadAddress >> 16) & 0xFF;

	WriteCommBlock(FDHandle, Buffer, 4);

	if (ReceiveData(FDHandle, Buffer, EE30F_ROW_SIZE * 2) > 0)
  {

	for(Count = 0; Count < EE30F_ROW_SIZE * 2;)
	{
		printf("0x%06x: ", ReadAddress);

		printf("%02x",Buffer[Count++] & 0xFF);
		printf("%02x ",Buffer[Count++] & 0xFF);

		printf("%02x",Buffer[Count++] & 0xFF);
		printf("%02x ",Buffer[Count++] & 0xFF);

		printf("%02x",Buffer[Count++] & 0xFF);
		printf("%02x ",Buffer[Count++] & 0xFF);

		printf("%02x",Buffer[Count++] & 0xFF);
		printf("%02x ",Buffer[Count++] & 0xFF);

		printf("%02x",Buffer[Count++] & 0xFF);
		printf("%02x ",Buffer[Count++] & 0xFF);

		printf("%02x",Buffer[Count++] & 0xFF);
		printf("%02x ",Buffer[Count++] & 0xFF);

		printf("%02x",Buffer[Count++] & 0xFF);
		printf("%02x ",Buffer[Count++] & 0xFF);

		printf("%02x",Buffer[Count++] & 0xFF);
		printf("%02x\n",Buffer[Count++] & 0xFF);

		ReadAddress = ReadAddress + 16;
	}
  }
}
