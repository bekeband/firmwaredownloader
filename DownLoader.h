/*
 * Copyright 2013 bekemod.hu
 */

//
// File:  DownLoader.h
//

#ifndef _downloader_H
#define	_downloader_H

#define SCI_INTERFACE_NAME "COM"

#define BYTESIZE   8
#define PARITY     NOPARITY
#define STOPBITS   ONESTOPBIT
#define ASCII_XON  0x11
#define ASCII_XOFF 0x13

#define BUFFER_SIZE         4096
#define READ_BUFFER_TIMEOUT 1000

#define PM30F_ROW_SIZE 32
#define PM33F_ROW_SIZE 64*8
#define EE30F_ROW_SIZE 16

/* Defines downloader commands. */

#define COMMAND_NACK     0x00
#define COMMAND_ACK      0x01
#define COMMAND_READ_PM  0x02
#define COMMAND_WRITE_PM 0x03
#define COMMAND_READ_EE  0x04
#define COMMAND_WRITE_EE 0x05
#define COMMAND_READ_CM  0x06
#define COMMAND_WRITE_CM 0x07
#define COMMAND_RESET    0x08
#define COMMAND_READ_ID  0x09

#define PM_SIZE 1536 /* Max: 144KB/3/32=1536 PM rows for 30F. */
#define EE_SIZE 128 /* 4KB/2/16=128 EE rows */
#define CM_SIZE 8

typedef unsigned short int usint;

/* Enumerated of PIC families. */

enum eFamily
{
  dsPIC30F,
  dsPIC33F,
  PIC24H,
  PIC24F
};

typedef struct
{
  char*         pName;
  usint         Id;
  usint         ProcessId;
  eFamily       Family;
} sDevice;

typedef struct 
{
    long brate;
    long value;
} s_baudrate;

#define MODULE_PIC_ID           0x1C1
#define MODULE_PROCESS_ID       0x01
#define MODULE_PIC_FAMILY       dsPIC30F

#endif	/* _downloader_H */

