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
 *****************************************************************************/

#include <sqlite3.h>
#include "logger.h"

void
sqlite3_err (const char *file, unsigned int line, sqlite3 *db,
	     const char *msg, ...)
{
  l->err (file, line, "[SQLITE3] %s (%s)", msg, sqlite3_errmsg (db));
}

void
sqlite3_err_str (const char *file, unsigned int line, char **err_str,
		 const char *msg, ...)
{
  l->err (file, line, "[SQLITE3] %s (%s)\n", msg, *err_str);
  sqlite3_free (*err_str);
  *err_str = NULL;
}
