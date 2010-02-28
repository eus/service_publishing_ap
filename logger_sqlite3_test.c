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

#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "logger_sqlite3.h"

GLOBAL_LOGGER;

int
main (int argc, char **argv, char **envp)
{
  sqlite3 *db;
  char db_name[32] = "/tmp/test_";
  char *err_msg;
  time_t now = time (NULL);

  SETUP_LOGGER ("/dev/null", NULL);

  strftime (db_name + strlen (db_name), sizeof (db_name) - strlen (db_name),
	    "%s", gmtime (&now));
  strcpy (db_name + strlen (db_name), ".db");
  
  /* Ensuring no segmentation fault or memory leak happens */
  if (sqlite3_open_v2 (db_name, &db, SQLITE_OPEN_READWRITE, NULL))
    {
      SQLITE3_ERR (db, "Cannot open DB in read-write mode");
      if (sqlite3_close (db))
	{
	  assert (0);
	}
    }
  else
    {
      assert (0);
    }

  if (sqlite3_open (db_name, &db))
    {
      sqlite3_close (db);
      assert (0);
    }

  if (sqlite3_exec (db, "select * from eus", NULL, NULL, &err_msg))
    {
      SQLITE3_ERR_STR (err_msg, "Table eus does not exist");
    }

  if (sqlite3_close (db))
    {
      assert (0);
    }

  exit (EXIT_SUCCESS);
}
