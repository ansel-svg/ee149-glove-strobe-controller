/*
	This will be used to open and read the receiver's serial port with termios settings
	(cfmakeraw, 115200 baud, CLOCAL, HUPCL off, VMIN 1, VTIME 0). It strips \r from
	read lines so the pipe program always sees \n-only endings.

	Input: "path": const char* (serial port path, e.g. /dev/serial/by-id/...)

	Output: "fd": int (file descriptor for the opened port),
	"line": char[] (one line per call, \r stripped, -1 on error/EOF)

	Interfaces with: usb_port.h, pipe_bridge.lf (SerialReader reactor)
*/
