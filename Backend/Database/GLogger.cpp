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

void GLogger::log(int logType, shmea::GString category, shmea::GString message)
{
	shmea::GString strDateTime = getDateTime();

	infoKeys.addString(category);
	infoLog.addString(message);

	if(printLevel == LOG_NONE)
	    return;

	if(surpressCheck(logType))
	    return;

	if(printLevel <= logType)
	    printf(strDateTime + " [%c][%s]: %s\n", LOG_SYMBOLS[logType], category.c_str(), message.c_str());

	switch (logType)
	{
	    case LOG_VERBOSE:
		verbose(category, message);
		break;

	    case LOG_DEBUG:
		debug(category, message);
		break;

	    case LOG_INFO:
		info(category, message);
		break;

	    case LOG_WARNING:
		warning(category, message);
		break;

	    case LOG_ERROR:
		error(category, message);
		break;

	    case LOG_FATAL:
		fatal(category, message);
		break;
	}
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

void GLogger::info(shmea::GString category, shmea::GString message)
{
	shmea::GString strDateTime = getDateTime();

	infoKeys.addString(category);
	infoLog.addString(message);

	if(printLevel == LOG_NONE)
	    return;

	if(surpressCheck(LOG_INFO))
	    return;

	if(printLevel <= LOG_INFO)
	    printf(strDateTime + " [i][%s]: %s\n", category.c_str(), message.c_str());
}

void GLogger::verbose(shmea::GString category, shmea::GString message)
{
	shmea::GString strDateTime = getDateTime();

	verboseKeys.addString(category);
	verboseLog.addString(message);

	if(printLevel == LOG_NONE)
	    return;


	if(surpressCheck(LOG_VERBOSE))
	    return;

	if(printLevel <= LOG_VERBOSE)
	    printf(strDateTime + " [v][%s]: %s\n", category.c_str(), message.c_str());
}

void GLogger::debug(shmea::GString category, shmea::GString message)
{
	shmea::GString strDateTime = getDateTime();

	debugKeys.addString(category);
	debugLog.addString(message);

	if(printLevel == LOG_NONE)
	    return;

	if(surpressCheck(LOG_DEBUG))
	    return;

	if(printLevel <= LOG_DEBUG)
	    printf(strDateTime + " [d][%s]: %s\n", category.c_str(), message.c_str());
}

void GLogger::warning(shmea::GString category, shmea::GString message)
{
	shmea::GString strDateTime = getDateTime();

	warningKeys.addString(category);
	warningLog.addString(message);

	if(printLevel == LOG_NONE)
	    return;

	if(surpressCheck(LOG_WARNING))
	    return;

	if(printLevel <= LOG_WARNING)
	    printf(strDateTime + " [W][%s]: %s\n", category.c_str(), message.c_str());
}

void GLogger::error(shmea::GString category, shmea::GString message)
{
	shmea::GString strDateTime = getDateTime();

	errorKeys.addString(category);
	errorLog.addString(message);

	if(printLevel == LOG_NONE)
	    return;

	if(surpressCheck(LOG_ERROR))
	    return;

	if(printLevel <= LOG_ERROR)
	    printf(strDateTime + " [E][%s]: %s\n", category.c_str(), message.c_str());
}

void GLogger::fatal(shmea::GString category, shmea::GString message)
{
	shmea::GString strDateTime = getDateTime();

	fatalKeys.addString(category);
	fatalLog.addString(message);

	if(printLevel == LOG_NONE)
	    return;

	if(surpressCheck(LOG_FATAL))
	    return;

	if(printLevel <= LOG_FATAL)
	    printf(strDateTime + " [F][%s]: %s\n", category.c_str(), message.c_str());
}
