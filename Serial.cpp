/** Serial.cpp
 *
 * A very simple serial port control class that does NOT require MFC/AFX.
 *
 * @author Hans de Ruiter
 *
 * @version 0.1 -- 28 October 2008
 */
#include "stdafx.h"
#include <iostream>
using namespace std;

#include "Serial.h"

Serial::Serial(tstring &commPortName, int bitRate)
{
	commHandle = CreateFile(commPortName.c_str(), GENERIC_READ|GENERIC_WRITE, 0,NULL, OPEN_EXISTING, 
		0, NULL);

	if(commHandle == INVALID_HANDLE_VALUE) 
	{
		throw("ERROR: Could not open com port");
	}
	else 
	{
		// set timeouts
		COMMTIMEOUTS cto = { MAXDWORD, 0, 30, 0, 30};
		DCB dcb;
		if(!SetCommTimeouts(commHandle,&cto))
		{
			Serial::~Serial();
			throw("ERROR: Could not set com port time-outs");
		}

		// set DCB
		memset(&dcb,0,sizeof(dcb));
		dcb.DCBlength = sizeof(dcb);
		dcb.BaudRate = bitRate;
		dcb.fBinary = 1;
		dcb.fDtrControl = DTR_CONTROL_ENABLE;
		dcb.fRtsControl = RTS_CONTROL_ENABLE;

		dcb.Parity = NOPARITY;
		dcb.StopBits = ONESTOPBIT;
		dcb.ByteSize = 8;

		if(!SetCommState(commHandle,&dcb))
		{
			Serial::~Serial();
			throw("ERROR: Could not set com port parameters");
		}
	}
}

Serial::~Serial()
{
	CloseHandle(commHandle);
}

int Serial::write(const char *buffer)
{
	DWORD numWritten;
	WriteFile(commHandle, buffer, strlen(buffer), &numWritten, NULL); 

	return numWritten;
}

int Serial::write(const char *buffer, int buffLen)
{
	DWORD numWritten;
	WriteFile(commHandle, buffer, buffLen, &numWritten, NULL); 

	return numWritten;
}

int Serial::MavisSendComData (int* lock, int code, int data)
{
	int temp1, temp2, temp3, temp4;
	char buffer[7];
	//najpre treba da formatiran buffer pa onda da pozovem serial write funkciju
	buffer[0]=255;
	buffer[1]=code+128;
	buffer[2]=temp1=data&0x0F;
	buffer[3]=temp2=(data&0xF0)>>4;  
	buffer[4]=temp3=(data&0xF00)>>8;
	buffer[5]=temp4=(data&0xF000)>>12;
	buffer[6]=(temp1+temp2+temp3+temp4+128+code)&0x7F;
	DWORD numRead, numWriten;
	numWriten=write(buffer,7);
	numRead=read(buffer,6,false);
	if (numRead==6) {
		if ((buffer[1]==10)&&(buffer[2]==10)) return 0;
		else return -1;
	}
	else return -2;
}

int Serial::MavisGetComData (int* lock, int code, int *data)
{
	int checksum=0;
	int local_data;
	char buffer[7];
	//najpre treba da formatiran buffer pa onda da pozovem serial write funkciju
	buffer[0]=255;
	buffer[1]=code+192;
	buffer[2]=(code+192)&0x7F;
	DWORD numRead, numWriten;
	numWriten=write(buffer,3);
	numRead=read(buffer,6,false);
	local_data=buffer[4];
	local_data=(local_data<<4)|buffer[3];
	local_data=(local_data<<4)|buffer[2];
	local_data=(local_data<<4)|buffer[1];
	*data=local_data;
	checksum=(buffer[1]+buffer[2]+buffer[3]+buffer[4])&0x7F;
	if (numRead==6) {		
		if (buffer[5]==checksum) return 0;
		else return -1;
	}
	else return -2;

}

int Serial::read(char *buffer, int buffLen, bool nullTerminate)
{
	DWORD numRead;
	if(nullTerminate)
	{
		--buffLen;
	}

	BOOL ret = ReadFile(commHandle, buffer, buffLen, &numRead, NULL);

	if(!ret)
	{
		return 0;
	}

	if(nullTerminate)
	{
		buffer[numRead] = '\0';
	}

	return numRead;
}

#define FLUSH_BUFFSIZE 10

void Serial::flush()
{
	char buffer[FLUSH_BUFFSIZE];
	int numBytes = read(buffer, FLUSH_BUFFSIZE, false);
	while(numBytes != 0)
	{
		numBytes = read(buffer, FLUSH_BUFFSIZE, false);
	}
}
