// Copyright 2020 Robert Carneiro, Derek Meer, Matthew Tabak, Eric Lujan
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
// associated documentation files (the "Software"), to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge, publish, distribute,
// sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
// NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#include "GLogger.h"
#include "GType.h"
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>

using namespace shmea;


/*
 * Logger logger;
 * logger.log("socks", "Visit from 192.168.1.123");
 * logger.verbose("socks", "His name is Joe Shmoe and many other details. Long sentence and lots of data here!!!!!!! hi");
 * ...
 * logger.debug("socks", "192.168.1.123 WHY ARE YOU BREAKING??");
 * ...
 * logger.debug("socks", "192.168.1.123 WHY ARE YOU BREAKING?? " + brokenVariable);
 * ...
 * logger.debug("encryption", "HELLO0");
 * ...
 * logger.debug("encryption", "HELLO1");
 * ...
 * logger.debug("encryption", "HELLO2");
 */

const shmea::GString GLogger::LOG_SYMBOLS = "vdiWEF";

GLogger::GLogger()
{
	printLevel = LOG_NONE;
	surpressVerbose = false;
	surpressDebug = false;
	surpressInfo = false;
	surpressWarning = false;
	surpressError = false;
	surpressFatal = false;
}

GLogger::GLogger(int newPrintLevel)
{
	printLevel = newPrintLevel;
	surpressVerbose = false;
	surpressDebug = false;
	surpressInfo = false;
	surpressWarning = false;
	surpressError = false;
	surpressFatal = false;

	// bounds check
	if(printLevel < LOG_NONE)
		printLevel = LOG_NONE;
	if(printLevel > LOG_FATAL)
	    printLevel = LOG_FATAL;
}

GLogger::GLogger(const GLogger& logger2)
{
	copy(logger2);
}

GLogger::~GLogger()
{
	clear();
}

void GLogger::copy(const GLogger& logger2)
{
    // copy
    printLevel = logger2.printLevel;

    verboseKeys = logger2.verboseKeys;
    debugKeys = logger2.debugKeys;
    infoKeys = logger2.infoKeys;
    warningKeys = logger2.warningKeys;
    errorKeys = logger2.errorKeys;
    fatalKeys = logger2.fatalKeys;

    verboseLog = logger2.verboseLog;
    debugLog = logger2.debugLog;
    infoLog = logger2.infoLog;
    warningLog = logger2.warningLog;
    errorLog = logger2.errorLog;
    fatalLog = logger2.fatalLog;
}

void GLogger::clear()
{
	verboseKeys.clear();
	debugKeys.clear();
	infoKeys.clear();
	warningKeys.clear();
	errorKeys.clear();
	fatalKeys.clear();

	verboseLog.clear();
	debugLog.clear();
	infoLog.clear();
	warningLog.clear();
	errorLog.clear();
	fatalLog.clear();
}

void GLogger::setPrintLevel(int newPrintLevel)
{
	printLevel = newPrintLevel;
}

int GLogger::getPrintLevel() const
{
	return printLevel;
}

void GLogger::surpress(int logType)
{
	switch (logType)
	{
	    case LOG_VERBOSE:
		surpressVerbose = true;
		break;

	    case LOG_DEBUG:
		surpressDebug = true;
		break;

	    case LOG_INFO:
		surpressInfo = true;
		break;

	    case LOG_WARNING:
		surpressWarning = true;
		break;

	    case LOG_ERROR:
		surpressError = true;
		break;

	    case LOG_FATAL:
		surpressFatal = true;
		break;
	}
}

void GLogger::unsurpress(int logType)
{
	switch (logType)
	{
	    case LOG_VERBOSE:
		surpressVerbose = false;
		break;

	    case LOG_DEBUG:
		surpressDebug = false;
		break;

	    case LOG_INFO:
		surpressInfo = false;
		break;

	    case LOG_WARNING:
		surpressWarning = false;
		break;

	    case LOG_ERROR:
		surpressError = false;
		break;

	    case LOG_FATAL:
		surpressFatal = false;
		break;
	}
}

bool GLogger::surpressCheck(int logType) const
{
	switch (logType)
	{
	    case LOG_VERBOSE:
		return surpressVerbose;

	    case LOG_DEBUG:
		return surpressDebug;

	    case LOG_INFO:
		return surpressInfo;

	    case LOG_WARNING:
		return surpressWarning;

	    case LOG_ERROR:
		return surpressError;

	    case LOG_FATAL:
		return surpressFatal;
	}

	return false;
}

shmea::GString GLogger::getDateTime() const
{
	char timeString[100];
	struct timeval tv;
	struct timezone tz;
	gettimeofday(&tv, &tz);
	strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", localtime(&tv.tv_sec));
	shmea::GString strDateTime(timeString);
	return strDateTime;
}

shmea::GString GLogger::generateLogFName() const
{
	char timeString[100];
	struct timeval tv;
	struct timezone tz;
	gettimeofday(&tv, &tz);
	strftime(timeString, sizeof(timeString), "%Y-%m-%d-H%H", localtime(&tv.tv_sec));
	shmea::GString strDateTime(timeString);
	return strDateTime;
}

void GLogger::log(int logType, shmea::GString category, shmea::GString message)
{
	shmea::GString strDateTime = getDateTime();

	if(printLevel == LOG_NONE)
	    return;

	if(surpressCheck(logType))
	    return;

	if(printLevel > logType)
	    return;

	// Print to console
	printf("[%c]%s [%s]: %s\n", LOG_SYMBOLS[logType], strDateTime.c_str(), category.c_str(), message.c_str());

	// Write to file
	shmea::GString logDir = "logs/";
	shmea::GString logFile = generateLogFName() + ".log";
	shmea::GString lockFile = ".lock-" + logFile;

	// Check if log directory exists
	struct stat st;
	if(stat(logDir.c_str(), &st) == -1)
	{
	    // Create directory
	    mkdir(logDir.c_str(), 0700);
	    printf("+%s\n", logDir.c_str());
	}

	// Check if lock file exists
	if(stat((logDir + lockFile).c_str(), &st) == -1)
	{
	    // Create lock file
	    FILE* lockFD = fopen((logDir + lockFile).c_str(), "w");
	    fclose(lockFD);

	    // Check if log file exists
	    if(stat((logDir + logFile).c_str(), &st) == -1)
	    {
		// Create log file
		FILE* logFD = fopen((logDir + logFile).c_str(), "w");
		fprintf(logFD, "[%c]%s [%s]: %s\n", LOG_SYMBOLS[logType], strDateTime.c_str(), category.c_str(), message.c_str());
		fclose(logFD);
	    }
	    else
	    {
		// Append to log file
		FILE* logFD = fopen((logDir + logFile).c_str(), "a");
		fprintf(logFD, "[%c]%s [%s]: %s\n", LOG_SYMBOLS[logType], strDateTime.c_str(), category.c_str(), message.c_str());
		fclose(logFD);
	    }

	    // Remove lock file
	    remove((logDir + lockFile).c_str());
	}
	else
	{
	    // Wait for lock file to be removed
	    while(stat((logDir + lockFile).c_str(), &st) != -1)
	    {
		// Wait
	    }

	    // Append to log file
	    FILE* logFD = fopen((logDir + logFile).c_str(), "a");
	    fprintf(logFD, "[%c]%s [%s]: %s\n", LOG_SYMBOLS[logType], strDateTime.c_str(), category.c_str(), message.c_str());
	    fclose(logFD);
	}

	// Store in memory (not necassary)
	switch (logType)
	{
	    case LOG_VERBOSE:
		//verboseKeys.addString(category);
		//verboseLog.addString(message);
		break;

	    case LOG_DEBUG:
		//debugKeys.addString(category);
		//debugLog.addString(message);
		break;

	    case LOG_INFO:
		//infoKeys.addString(category);
		//infoLog.addString(message);
		break;

	    case LOG_WARNING:
		//warningKeys.addString(category);
		//warningLog.addString(message);
		break;

	    case LOG_ERROR:
		//errorKeys.addString(category);
		//errorLog.addString(message);
		break;

	    case LOG_FATAL:
		//fatalKeys.addString(category);
		//fatalLog.addString(message);
		break;
	}
}

void GLogger::verbose(shmea::GString category, shmea::GString message)
{
    log(LOG_VERBOSE, category, message);
}

void GLogger::debug(shmea::GString category, shmea::GString message)
{
    log(LOG_DEBUG, category, message);
}

void GLogger::info(shmea::GString category, shmea::GString message)
{
    log(LOG_INFO, category, message);
}

void GLogger::warning(shmea::GString category, shmea::GString message)
{
    log(LOG_WARNING, category, message);
}

void GLogger::error(shmea::GString category, shmea::GString message)
{
    log(LOG_ERROR, category, message);
}

void GLogger::fatal(shmea::GString category, shmea::GString message)
{
    log(LOG_FATAL, category, message);
}
