// ****************************************************************************************
//
// Fubi Utils
// ---------------------------------------------------------
// Copyright (C) 2010-2013 Felix Kistler 
// 
// This software is distributed under the terms of the Eclipse Public License v1.0.
// A copy of the license may be obtained at: http://www.eclipse.org/org/documents/epl-v10.html
// 
// ****************************************************************************************
#include "FubiUtils.h"
#include "FubiConfig.h"

#include <stdarg.h>
#include <iostream>

using namespace Fubi;

void Logging::logDbg(const char* msg, ...)
{
#if (FUBI_LOG_LEVEL == FUBI_LOG_VERBOSE)
	va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
#endif
}
void Logging::logInfo(const char* msg, ...)
{
#if ((FUBI_LOG_LEVEL <= FUBI_LOG_ERR_WRN_INFO)  || (FUBI_LOG_LEVEL == FUBI_LOG_ERR_INFO))
	va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
#endif
}
void Logging::logWrn(const char* file, int line, const char* msg, ...)
{
#if (FUBI_LOG_LEVEL <= FUBI_LOG_ERR_WRN)
	printf("Fubi Warning (\"%s\":%d): ", file, line);
	va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
#endif
}
void Logging::logErr(const char* file, int line, const char* msg, ...)
{
#if (FUBI_LOG_LEVEL <= FUBI_LOG_ERR)
	printf("Fubi ERROR (\"%s\":%d): ", file, line);
	va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
#endif
}