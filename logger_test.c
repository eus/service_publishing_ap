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

#include <stdlib.h>
#include <errno.h>
#include "logger.h"

GLOBAL_LOGGER;

/** The possible errors. */
enum err
  {
    ERR_SUCCESS, /**< There is no error. */
    ERR_SOCK, /**< Socket error. */
    ERR_MEM, /**< Insufficient memory. */
  };

static const char *
errtostr (int err)
{
  static const char *errstr[] = {
    "Success",
    "Socket error",
    "Not enough memory",
  };

  return errstr[err];
}

int
main (int argc, char **argv, char **envp)
{
  SETUP_LOGGER ("/dev/null", errtostr);
  
  /* Ensuring no segmentation fault happens */
  errno = ENOMEM;
  l->SYS_ERR ("System error");

  l->APP_ERR (ERR_MEM, "Application error");

  l->ERR ("Custom error %s", "[ERROR]");

  exit (EXIT_SUCCESS);
}
