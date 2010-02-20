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

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "app_err.h"
#include "logger.h"
#include "service_inquiry.h"

static int stop_signal = 0;

static int
is_stopped ()
{
  return stop_signal;
}

static void
signal_handler (int signum)
{
  stop_signal = 1;
}

GLOBAL_LOGGER;

int
main (int argc, char **argv, char **envp)
{
  struct sigaction act = {
    .sa_handler = signal_handler,
  };
  int rc;

  if (argc != 2)
    {
      fprintf (stderr, "Usage: %s LOG_FILE\n", argv[0]);
      exit (EXIT_FAILURE);
    }

  SETUP_LOGGER (argv[1], errtostr);

  if (sigaction (SIGTERM, &act, NULL)
      || sigaction (SIGINT, &act, NULL))
    {
      l->SYS_ERR ("Cannot install signal handler");
      exit (EXIT_FAILURE);
    }

  if ((rc = run_inquiry_handler (is_stopped, &l)))
    {
      l->APP_ERR (rc, "Error in inquiry handler");
      rc = EXIT_FAILURE;
    }
  else
    {
      rc = EXIT_SUCCESS;
    }

  exit (rc);
}
