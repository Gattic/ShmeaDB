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

GLogger::GLogger()
{
	printLevel = LOG_NONE;
	surpressInfo = false;
	surpressVerbose = false;
	surpressDebug = false;
	surpressWarning = false;
	surpressError = false;
	surpressFatal = false;
}

GLogger::GLogger(int newPrintLevel)
{
	printLevel = newPrintLevel;
	surpressInfo = false;
	surpressVerbose = false;
	surpressDebug = false;
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

    infoKeys = logger2.infoKeys;
    verboseKeys = logger2.verboseKeys;
    debugKeys = logger2.debugKeys;
    warningKeys = logger2.warningKeys;
    errorKeys = logger2.errorKeys;
    fatalKeys = logger2.fatalKeys;

    infoLog = logger2.infoLog;
    verboseLog = logger2.verboseLog;
    debugLog = logger2.debugLog;
    warningLog = logger2.warningLog;
    errorLog = logger2.errorLog;
    fatalLog = logger2.fatalLog;
}

void GLogger::clear()
{
	infoKeys.clear();
	verboseKeys.clear();
	debugKeys.clear();
	warningKeys.clear();
	errorKeys.clear();
	fatalKeys.clear();

	infoLog.clear();
	verboseLog.clear();
	debugLog.clear();
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
	    case LOG_INFO:
		surpressInfo = true;
		break;

	    case LOG_VERBOSE:
		surpressVerbose = true;
		break;

	    case LOG_DEBUG:
		surpressDebug = true;
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
	    case LOG_INFO:
		surpressInfo = false;
		break;

	    case LOG_VERBOSE:
		surpressVerbose = false;
		break;

	    case LOG_DEBUG:
		surpressDebug = false;
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
	    case LOG_INFO:
		return surpressInfo;

	    case LOG_VERBOSE:
		return surpressVerbose;

	    case LOG_DEBUG:
		return surpressDebug;

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
	switch (logType)
	{
	    case LOG_INFO:
		info(category, message);
		break;

	    case LOG_VERBOSE:
		verbose(category, message);
		break;

	    case LOG_DEBUG:
		debug(category, message);
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

void GLogger::info(shmea::GString category, shmea::GString message)
{
	infoKeys.addString(category);
	infoLog.addString(message);

	if(printLevel == LOG_NONE)
	    return;

	if(printLevel <= LOG_INFO)
	    printf("[i][%s]: %s\n", category.c_str(), message.c_str());
}

void GLogger::verbose(shmea::GString category, shmea::GString message)
{
	verboseKeys.addString(category);
	verboseLog.addString(message);

	if(printLevel == LOG_NONE)
	    return;

	if(printLevel <= LOG_VERBOSE)
	    printf("[v][%s]: %s\n", category.c_str(), message.c_str());
}

void GLogger::debug(shmea::GString category, shmea::GString message)
{
	debugKeys.addString(category);
	debugLog.addString(message);

	if(printLevel == LOG_NONE)
	    return;

	if(printLevel <= LOG_DEBUG)
	    printf("[d][%s]: %s\n", category.c_str(), message.c_str());
}

void GLogger::warning(shmea::GString category, shmea::GString message)
{
	warningKeys.addString(category);
	warningLog.addString(message);

	if(printLevel == LOG_NONE)
	    return;

	if(printLevel <= LOG_WARNING)
	    printf("[W][%s]: %s\n", category.c_str(), message.c_str());
}

void GLogger::error(shmea::GString category, shmea::GString message)
{
	errorKeys.addString(category);
	errorLog.addString(message);

	if(printLevel == LOG_NONE)
	    return;

	if(printLevel <= LOG_ERROR)
	    printf("[E][%s]: %s\n", category.c_str(), message.c_str());
}

void GLogger::fatal(shmea::GString category, shmea::GString message)
{
	fatalKeys.addString(category);
	fatalLog.addString(message);

	if(printLevel == LOG_NONE)
	    return;

	if(printLevel <= LOG_FATAL)
	    printf("[F][%s]: %s\n", category.c_str(), message.c_str());
}
