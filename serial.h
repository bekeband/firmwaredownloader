#ifndef __serial_h__
#define __serial_h__

//#include <wx/wx.h>
#include <stdint.h>
#include <string>
#include <array>

#if defined(__GNUC__)
#include <termios.h>
#elif defined(MACOSX)
#include <termios.h>
#elif defined(WINDOWS)
#include <windows.h>
#endif

class Serial
{
public:
	Serial();
	~Serial();
	//wxArrayString 
        array port_list();
	int Open(const string& name);
	//wxString 
        string error_message();
	int Set_baud(int baud);
	int Set_baud(const string& baud_str);
	int Read(void *ptr, int count);
	int Write(const void *ptr, int len);
	int Input_wait(int msec);
	void Input_discard(void);
	int Set_control(int dtr, int rts);
	void Output_flush();
	void Close(void);
	int Is_open(void);
	string get_name(void);
private:
	int port_is_open;
	string port_name;
	int baud_rate;
	string error_msg;
private:
#if defined(LINUX) || defined(MACOSX)
	int port_fd;
	struct termios settings_orig;
	struct termios settings;
#elif defined(WINDOWS)
	HANDLE port_handle;
	COMMCONFIG port_cfg_orig;
	COMMCONFIG port_cfg;
#endif
};

#endif // __serial_h__
