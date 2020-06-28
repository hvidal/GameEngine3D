#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#define DEBUG true

class Log
{
	static void printTime()
	{
		time_t now;
		time(&now);
		struct tm *current;
		current = localtime(&now);
		printf("[%02i:%02i:%02i] ", current->tm_hour, current->tm_min, current->tm_sec);
	}

public:
	static void info(const char* msg, ...)
	{
		printTime();
		va_list argptr;
		va_start(argptr, msg);
		vfprintf(stdout, msg, argptr);
		fprintf(stdout, "\n");
		va_end(argptr);
	}

	static void error(const char* msg, ...)
	{
		printTime();
		va_list argptr;
		va_start(argptr, msg);
		vfprintf(stderr, msg, argptr);
		fprintf(stderr, "\n");
		va_end(argptr);
	}

	static void debug(const char* msg, ...)
	{
		if (DEBUG) {
			printTime();
			printf("DEBUG ");
			va_list argptr;
			va_start(argptr, msg);
			vfprintf(stdout, msg, argptr);
			fprintf(stdout, "\n");
			va_end(argptr);
		}
	}
};



