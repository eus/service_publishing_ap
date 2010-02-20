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

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include "service_inquiry.h"
#include "app_err.h"
#include "logger.h"

static int s = -1;

static int
destroy_socket (void)
{
  if (s != -1)
    {
      if (close (s) == -1)
	{
	  l->SYS_ERR ("Cannot close inquiry handler socket");
	  return ERR_SOCK;
	}
    }

  return ERR_SUCCESS;
}

int
run_inquiry_handler (int (*is_stopped) (void))
{
  struct sockaddr_in handler_addr = {
    .sin_family = AF_INET,
    .sin_addr = INADDR_ANY,
    .sin_port = SERVICE_INQUIRY_HANDLER_PORT,
  };

  s = socket (AF_INET, SOCK_DGRAM, 0);
  if (s == -1)
    {
      l->SYS_ERR ("Cannot create inquiry handler socket");
      return ERR_SOCK;
    }

  if (bind (s, (struct sockaddr *) &handler_addr, sizeof (handler_addr)) == -1)
    {
      l->SYS_ERR ("Cannot name inquiry handler socket");
      destroy_socket ();
      return ERR_SOCK;
    }

  while (!is_stopped ())
    {
    }

  return ERR_SUCCESS;
}
