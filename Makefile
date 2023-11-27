# Makefile
all: mytaskmanager

mytaskmanager: main.c system_info.c file_system.c resources.c processes.c
	gcc -o mytaskmanager main.c system_info.c file_system.c resources.c processes.c `pkg-config --cflags --libs gtk+-3.0`

clean:
	rm -f mytaskmanager

