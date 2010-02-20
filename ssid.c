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

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <linux/wireless.h>
#include <stdio.h>
#include <stdlib.h>
#include "app_err.h"
#include "logger.h"
#include "ssid.h"

static const char *
get_wlan_ifname (void)
{
  return "wlan0";
}

static const char *
get_proc_net_wireless (void)
{
  return "/proc/net/wireless";
}

static int
iw_get_kernel_we_version(void)
{
  char          buff[1024];
  FILE *        fh;
  char *        p;
  int           v;

  /* Check if /proc/net/wireless is available */
  fh = fopen(get_proc_net_wireless (), "r");

  if(fh == NULL)
    {
      l->SYS_ERR ("Cannot read %s", get_proc_net_wireless ());
      return(-1);
    }

  /* Read the first line of buffer */
  fgets(buff, sizeof(buff), fh);

  if(strstr(buff, "| WE") == NULL)
    {
      /* Prior to WE16, so explicit version not present */

      /* Black magic */
      if(strstr(buff, "| Missed") == NULL)
        v = 11;
      else
        v = 15;
      fclose(fh);
      return(v);
    }

  /* Read the second line of buffer */
  fgets(buff, sizeof(buff), fh);

  /* Get to the last separator, to get the version */
  p = strrchr(buff, '|');
  if((p == NULL) || (sscanf(p + 1, "%d", &v) != 1))
    {
      l->ERR ("Cannot parse %s", get_proc_net_wireless ());
      fclose(fh);
      return(-1);
    }

  fclose(fh);
  return(v);
}

static int
open_socket_to_kernel (void)
{
  return socket (AF_INET, SOCK_DGRAM, 0);
}

static void
close_socket_to_kernel (int *s)
{
  if (*s == -1)
    {
      return;
    }

  if (close (*s) == -1)
    {
      l->SYS_ERR ("Cannot close socket");
    }

  *s = -1;
}

int
set_ssid (const void *new_ssid, size_t len)
{
  int s;
  struct iwreq wrq;
  int kernel_we_version;

  if (len > IW_ESSID_MAX_SIZE)
    {
      l->APP_ERR (ERR_SSID_TOO_LONG, "Cannot set SSID");
      return ERR_SSID_TOO_LONG;
    }

  if ((kernel_we_version = iw_get_kernel_we_version()) < 0)
    {
      return ERR_SET_SSID;
    }

  if ((s = open_socket_to_kernel ()) == -1)
    {
      l->SYS_ERR ("Cannot create SSID manipulating socket");
      return ERR_SET_SSID;
    }

  wrq.u.essid.length = len;
  wrq.u.essid.flags = 1;
  wrq.u.essid.pointer = (caddr_t) new_ssid;
  if (kernel_we_version < 21)
    {
      wrq.u.essid.length++;
    }

  strncpy (wrq.ifr_name, get_wlan_ifname (), IFNAMSIZ);
  if (ioctl (s, SIOCSIWESSID, &wrq) < 0)
    {
      l->SYS_ERR ("Cannot set SSID");
      close_socket_to_kernel (&s);
      return ERR_SET_SSID;
    }

  close_socket_to_kernel (&s);

  return ERR_SUCCESS;
}

ssize_t
get_ssid (void *buffer, size_t len)
{
  struct iwreq wrq;
  int s;

  if ((s = open_socket_to_kernel ()) == -1)
    {
      l->SYS_ERR ("Cannot create SSID manipulating socket");
      return -1;
    }

  memset (buffer, 0, len);

  wrq.u.essid.pointer = (caddr_t) buffer;
  wrq.u.essid.length = len;
  wrq.u.essid.flags = 0;

  strncpy (wrq.ifr_name, get_wlan_ifname (), IFNAMSIZ);
  if (ioctl (s, SIOCGIWESSID, &wrq) < 0)
    {
      l->SYS_ERR ("Cannot retrieve SSID");
      close_socket_to_kernel (&s);
      return -1;
    }

  close_socket_to_kernel (&s);

  return wrq.u.essid.length;
}
