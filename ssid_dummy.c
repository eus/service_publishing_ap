/*****************************************************************************
 * Copyright (C) 2010  Tadeus Prastowo (eus@member.fsf.org)                  *
 * Copyright (C) 1997-2007  Jean Tourrilhes <jt@hpl.hp.com>                  *
 *                                                                           *
 * This file is based on wireless-tools by Jean Tourrilhes.                  *
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

#include <string.h>
#include <stdlib.h>
#include "app_err.h"
#include "logger.h"
#include "ssid.h"

static int ssid[SSID_MAX_LEN];
static size_t ssid_len = 0;

int
set_ssid (const void *new_ssid, size_t len)
{
  if (len > SSID_MAX_LEN)
    {
      l->APP_ERR (ERR_SSID_TOO_LONG, "Cannot set SSID");
      return ERR_SSID_TOO_LONG;
    }

  memcpy (ssid, new_ssid, len);
  ssid_len = len;

  return ERR_SUCCESS;
}

ssize_t
get_ssid (void *buffer, size_t len)
{
  if (ssid_len > len)
    {
      return -1;
    }

  memcpy (buffer, ssid, ssid_len);

  return ssid_len;
}
