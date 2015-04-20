/* In memory of Marina... */

/*
* <Yield, Porting of renice.>
* Copyright (C) <2015> <Jacopo De Luca>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.

* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.

* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <Windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "argsx.h"

#define BASENAME "yield"
#define VERSION "1.00"

#define MSG_FATAL "[FATAL]: "
#define MSG_ERROR "[ERROR]: "

typedef struct target_info
{
	unsigned int pid;
	char *procName;
	int priority;
	bool setMode;
}target_info;

void show_help();
void handle_last_error();
bool get_process_info(target_info *target);
char *parse_priority(int priority);

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		show_help();
		exit(0);
	}

	target_info target = { 0, NULL, 0, false };
	ax_lopt lopt[] = { { "set", ARGSX_REQ_ARG, 's' }, { "help", ARGSX_NOARG, '\0' }, { "version", ARGSX_NOARG, '\0' } };
	
	int rchr;
	while ((rchr = argsx(argc, argv, "s!\0", lopt, sizeof(lopt), '-')) != -1)
	{
		switch (rchr)
		{
		case 's':
			target.setMode = true;
			if (strcmp(ax_arg, (char*)"idle") == 0)
				target.priority = IDLE_PRIORITY_CLASS;
			else if (strcmp(ax_arg, (char*)"low") == 0)
				target.priority = BELOW_NORMAL_PRIORITY_CLASS;
			else if (strcmp(ax_arg, (char*)"normal") == 0)
				target.priority = NORMAL_PRIORITY_CLASS;
			else if (strcmp(ax_arg, (char*)"medium") == 0)
				target.priority = ABOVE_NORMAL_PRIORITY_CLASS;
			else if (strcmp(ax_arg, (char*)"high") == 0)
				target.priority = HIGH_PRIORITY_CLASS;
			else if (strcmp(ax_arg, (char*)"realtime") == 0)
				target.priority = REALTIME_PRIORITY_CLASS;
			else
			{
				printf("Invalid argument -s %s\n", ax_arg);
				exit(-1);
			}
			break;
		case ARGSX_LOPT:
			switch (ax_loptidx)
			{
			case 1:
				show_help();
				exit(0);
				break;
			case 2:
				printf("%s V: %s", BASENAME, VERSION);
				exit(0);
				break;
			}
			break;
		case ARGSX_BAD_OPT:
			exit(0);
			break;
		case ARGSX_FEW_ARGS:
			exit(0);
			break;
		case ARGSX_NONOPT:
			if ((target.pid = (int)strtol(ax_arg, NULL, 10)) == 0)
				target.procName = ax_arg;
			break;
		}
	}

	if (get_process_info(&target)==false)
	{
		printf("%sThe process does not exist!\n",MSG_ERROR);
		exit(-1);
	}

	HANDLE procHandle;
	procHandle = OpenProcess(PROCESS_SET_INFORMATION | PROCESS_QUERY_INFORMATION, FALSE, target.pid);
	int currentPriority = GetPriorityClass(procHandle);
	if (!target.setMode)
		printf("%s [PID: %d] Priority: %s\n",target.procName,target.pid, parse_priority(currentPriority));
	else
	{
		if (SetPriorityClass(procHandle, target.priority) != 0)
			printf("%s [PID: %d] old priority %s, new priority %s\n", target.procName, target.pid, parse_priority(currentPriority), parse_priority(target.priority));
		else
			handle_last_error();
	}
	CloseHandle(procHandle);
}

void show_help()
{
	printf("%s V: %s\n\n"
		"Use: %s [OPTION] [PROCESS_NAME | PID]\n"
		"Alter priority of running processes .\n\n"
		"\t-s, --set\tset process priority\n\n"
		"\t\tidle\n"
		"\t\tlow\n"
		"\t\tnormal\n"
		"\t\thigh\n"
		"\t\trealtime\n\n"
		"\t--help\t\tShow this help and exit\n"
		"\t--version\tShow version information and exit\n\n",BASENAME,VERSION,BASENAME);
}

void handle_last_error()
{
	TCHAR *err;
	if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // default language
		(LPTSTR)&err,
		0,
		NULL))
		return;
	printf("%s%s\n", MSG_FATAL, err);
	LocalFree(err);
}

bool get_process_info(target_info *target)
{
	bool success = false;
	HANDLE syssnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 procinf;
	procinf.dwSize = sizeof(PROCESSENTRY32);
	while (Process32Next(syssnap, &procinf) != FALSE)
	{
		if (target->pid != 0)
		{
			if (target->pid == procinf.th32ProcessID)
			{
				target->procName = (char*)malloc(strlen(procinf.szExeFile) + 1);
				if (target->procName == NULL)
				{
					printf("%sMemory allocation error\n", MSG_FATAL);
					exit(-1);
				}
				strcpy(target->procName, procinf.szExeFile);
				success = true;
				break;
			}
		}
		else if (strcmp(target->procName, procinf.szExeFile) == 0)
		{
			target->pid = procinf.th32ProcessID;
			success = true;
			break;
		}
	}
	CloseHandle(syssnap);
	return success;
}

char *parse_priority(int priority)
{
	switch (priority)
	{
	case IDLE_PRIORITY_CLASS:
		return (char*)"[0x00008000] IDLE";
	case BELOW_NORMAL_PRIORITY_CLASS:
		return (char*) "[0x00004000] LOW";
	case NORMAL_PRIORITY_CLASS:
		return (char*)"[0x00000020] NORMAL";
	case ABOVE_NORMAL_PRIORITY_CLASS:
		return (char*) "[0x00008000] MEDIUM";
	case HIGH_PRIORITY_CLASS:
		return (char*) "[0x00000080] HIGH";
	case REALTIME_PRIORITY_CLASS:
		return (char*) "[0x00000100] REALTIME";
	}
}