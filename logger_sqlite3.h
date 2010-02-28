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
 * @file logger_sqlite3.h
 * @brief This logger module only provides convenient wrapper method over
 *        logger::err() to log sqlite3 error messages.
 ****************************************************************************/

#ifndef LOGGER_SQLITE3_H
#define LOGGER_SQLITE3_H

#include "logger.h"
#include <sqlite3.h>

#ifdef __cpluplus
extern "C" {
#endif


/** Log the last sqlite3 error message from the DB. */
void 
sqlite3_err (const char *file, unsigned int line, sqlite3 *db,
	    const char *msg, ...);

/** Log an sqlite3 error message (there is _no_ need to free err_str). */
void
sqlite3_err_str (const char *file, unsigned int line, char **err_str,
		const char *msg, ...);

/** Convenient wrapper for calling sqlite_err. */
#define SQLITE3_ERR(db, msg, ...) sqlite3_err (__FILE__, __LINE__, db,	\
					       msg , ## __VA_ARGS__)

/** Convenient wrapper for calling sqlite_err_str (NO need to free err_str). */
#define SQLITE3_ERR_STR(err_str, msg, ...) sqlite3_err_str (__FILE__, __LINE__, \
							    &err_str,	\
							    msg , ## __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* LOGGER_SQLITE3_H */
