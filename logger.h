/*****************************************************************************
 * Copyright (C) 2010  Tadeus Prastowo (eus@member.fsf.org)                  *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU General Public License as published by      *
 * the Free Software Foundation, either version 3 of the License, or         *
 * (at your option) any later version.                                       *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU General Public License for more details.                              *
 *                                                                           *
 * You should have received a copy of the GNU General Public License         *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *************************************************************************//**
 * @file logger.h
 * @brief The logger module to be used globally within an application.
 *        The file of the main function of an application should define
 *        USE_GLOBAL_LOGGER, initializes it with init_logger() and register
 *        destroy_logger() with atexit() so that destroy_logger() will be
 *        called last by atexit().
 ****************************************************************************/

#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

#ifdef __cpluplus
extern "C" {
#endif

struct logger_data;

/** The logger object used to log messages. */
struct logger
{
  struct logger_data *private; /**< Logger's private data. */
  void (*sys_err) (const char *file, unsigned int line,
		   const char *msg, ...); /**< Log a system error message. */
  void (*app_err) (const char *file, unsigned int line, int error_num,
		   const char *msg, ...); /**<
					   * Log an application error message.
					   */
  void (*err) (const char *file, unsigned int line,
	       const char *msg, ...); /**< Log a custom error message. */
};

/** Convenient wrapper for calling logger::sys_err. */
#define SYS_ERR(msg, ...) sys_err (__FILE__, __LINE__, msg , ## __VA_ARGS__)

/** Convenient wrapper for calling logger::app_err. */
#define APP_ERR(error_num, msg, ...) app_err (__FILE__, __LINE__, error_num, \
					      msg , ## __VA_ARGS__)

/** Convenient wrapper for calling logger::err. */
#define ERR(msg, ...) err (__FILE__, __LINE__, msg , ## __VA_ARGS__)

/** The global pointer to the global logger of an application. */
extern struct logger *l;

/** Convenient way to define a global logger object. */
#define GLOBAL_LOGGER struct logger logger = {0}; struct logger *l = &logger

/**
 * Initializes the global logger.
 *
 * @param [in] log_output the file where the message should be logged.
 * @param [in] err2str the function used to translate application error code
 *             to its meaning. If this is NULL, the application-specific error
 *             code will be logged as it is.
 *
 * @return zero if the installation is successful or non-zero if it is not.
 */
int
init_logger (const char *log_output, const char *(*err2str) (int));

/**
 * Destroys the global logger by freeing up its resources. It is convenient to
 * register this with atexit().
 */
void
destroy_logger (void);

/**
 * Convenient way to setup an application to use and destroy the global
 * logger. This should be put as the first executable line in main() and must
 * only be called once. If the logger needs to be modified, use
 * destroy_logger() and then init_logger() instead of this macro.
 */
#define SETUP_LOGGER(log_output, err2str) do {				\
    if (!atexit (destroy_logger) && !init_logger (log_output, err2str))	\
      break;								\
    fprintf (stderr, "Cannot setup logger\n");				\
    exit (EXIT_FAILURE);						\
  } while (0)

#ifdef __cplusplus
}
#endif

#endif /* LOGGER_H */
