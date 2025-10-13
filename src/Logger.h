/*
 * Logger.h
 *
 *  Created on: 11.09.2025
 *      Author: hocki
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include <Arduino.h>
#include <esp32-hal-log.h>

/* Deactivate TRACE to disable logger */
#define TRACE

/**
 * @brief Log level
 *
 */

typedef enum tLogLevel
{
    LOG_NONE    = ESP_LOG_NONE,       /*!< No log output */
    LOG_ERROR   = ESP_LOG_ERROR,      /*!< Critical errors, software module can not recover on its own */
    LOG_WARN    = ESP_LOG_WARN,       /*!< Error conditions from which recovery measures have been taken */
    LOG_INFO    = ESP_LOG_INFO,       /*!< Information messages which describe normal flow of events */
    LOG_DEBUG   = ESP_LOG_DEBUG,      /*!< Extra information which is not necessary for normal use (values, pointers, sizes, etc). */
    LOG_VERBOSE = ESP_LOG_VERBOSE,    /*!< Bigger chunks of debugging information, or frequent messages which can potentially flood the output. */
} tLogLevel;

/*
 * Every module using the LOG function should have a definition (in *.cpp or *.c file)
 * of LOG_LEVEL like the following.
 * Only messages with a log level higher or equal to the log level defined should be sent for tracing
 *
 * #define LOG_LEVEL   (LOG_ERROR)
 */
#ifdef TRACE
    #define LOG(level, ...)                             LoggerNS::Log( level, LOG_LEVEL, __VA_ARGS__ )
    #define LOG_LINE(level, ...)                        LoggerNS::LogLine( level, LOG_LEVEL, __FILE__, __LINE__, __VA_ARGS__ )

    #define LOG_WITH_REF(level, ref_level, ...)         LoggerNS::Log( level, ref_level, __VA_ARGS__)
    #define LOG_LINE_WITH_REF(level, ref_level, ...)    LoggerNS::LogLine( level, ref_level, __FILE__, __LINE__, __VA_ARGS__ )


    namespace LoggerNS
    {
        /*
        * @brief Helper function to convert log level to string
        */
        inline const char* LogLevelToString(uint8_t aLevel)
        {
            switch(aLevel)
            {
                case LOG_NONE       : return " NONE";
                case LOG_ERROR      : return "ERROR";
                case LOG_WARN       : return " WARN";
                case LOG_INFO       : return " INFO";
                case LOG_DEBUG      : return "DEBUG";
                case LOG_VERBOSE    : return " VERB";
                default             : return "  LOG";
            }
        }

        /**
         * @brief Logging function
         */
        template<typename... Args>
        inline void Log(uint8_t aLogLevel, uint8_t aRefLogLevel, const char* aText, Args... aArgs)
        {
            if (aLogLevel <= aRefLogLevel)
            {
                log_printf("[%s] ", LogLevelToString(aLogLevel));
                log_printf(aText, aArgs...);
                log_printf("\r\n");
            }
        }

        /**
         * @brief Logging macro with specific file and line
         */
        template<typename... Args>
        inline void LogLine(uint8_t aLogLevel, uint8_t aRefLogLevel, const char* aFile, uint8_t aLine, const char* aText, Args... aArgs)
        {
            if (aLogLevel <= aRefLogLevel)
            {
                log_printf("[%s] ", LogLevelToString(aLogLevel));
                log_printf("[%s:%d] ", pathToFileName(aFile), aLine);
                log_printf(aText, aArgs...);
                log_printf("\r\n");
            }
        }
    } /* namespace LoggerNS */

#else
    #define LOG(level, ...)
    #define LOG_LINE(level, ...)

    #define LOG_WITH_REF(level, ref_level, ...)
    #define LOG_LINE_WITH_REF(level, ref_level, ...)

#endif /* TRACE */

/** Defines to print a byte as binary (for logging)
 *
 * You can use the below macros like this:
 * LOG(LOG_DBG, "byte value: " PRINTF_BINARY_PATTERN_INT8, PRINTF_BYTE_TO_BINARY_INT8(value));
 */
#define PRINTF_BINARY_PATTERN_INT8          "%c%c%c%c%c%c%c%c"
#define PRINTF_BYTE_TO_BINARY_INT8(byte)     \
                ((byte & 0x80) ? '1' : '0'), \
                ((byte & 0x40) ? '1' : '0'), \
                ((byte & 0x20) ? '1' : '0'), \
                ((byte & 0x10) ? '1' : '0'), \
                ((byte & 0x08) ? '1' : '0'), \
                ((byte & 0x04) ? '1' : '0'), \
                ((byte & 0x02) ? '1' : '0'), \
                ((byte & 0x01) ? '1' : '0')

#endif /* LOGGER_H_ */
