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
      fprintf(stderr, "Cannot read %s\n", get_proc_net_wireless ());
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
      fprintf(stderr, "Cannot parse %s\n", get_proc_net_wireless ());
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
      perror ("Cannot close socket");
    }

  *s = -1;
}

int
set_ssid (const char *new_ssid)
{
  int s;
  struct iwreq wrq;
  char ssid[IW_ESSID_MAX_SIZE + 1];
  size_t ssid_len;
  int kernel_we_version = iw_get_kernel_we_version();

  if (kernel_we_version < 0)
    {
      return -1;
    }

  if ((s = open_socket_to_kernel ()) == -1)
    {
      return -1;
    }

  if ((ssid_len = strlen (new_ssid)) > IW_ESSID_MAX_SIZE)
    {
      fprintf (stderr, "Cannot set SSID: too long\n");
      close_socket_to_kernel (&s);
      return -1;
    }

  strncpy (ssid, new_ssid, ssid_len);
  ssid[ssid_len] = '\0';

  wrq.u.essid.length = ssid_len;
  wrq.u.essid.flags = 1;
  wrq.u.essid.pointer = (caddr_t) ssid;
  if (kernel_we_version < 21)
    {
      wrq.u.essid.length++;
    }

  strncpy (wrq.ifr_name, get_wlan_ifname (), IFNAMSIZ);
  if (ioctl (s, SIOCSIWESSID, &wrq) < 0)
    {
      perror ("Cannot set SSID");
      close_socket_to_kernel (&s);
      return -1;
    }

  close_socket_to_kernel (&s);

  return 0;
}

ssize_t
get_ssid (void *buffer, size_t len)
{
  struct iwreq wrq;
  int s;

  if ((s = open_socket_to_kernel ()) == -1)
    {
      return -1;
    }

  memset (buffer, 0, len);

  wrq.u.essid.pointer = (caddr_t) buffer;
  wrq.u.essid.length = len - 1;
  wrq.u.essid.flags = 0;

  strncpy (wrq.ifr_name, get_wlan_ifname (), IFNAMSIZ);
  if (ioctl (s, SIOCGIWESSID, &wrq) < 0)
    {
      perror ("Cannot retrieve SSID");
      close_socket_to_kernel (&s);
      return -1;
    }

  close_socket_to_kernel (&s);

  return strlen (buffer);
}

int
main (int argc, char **argv, char **envp)
{
  ssize_t len;
  char ssid [33];

  if (argc == 2 && set_ssid (argv[1]) < 0)
    {
      exit (EXIT_FAILURE);
    }

  if ((len = get_ssid (ssid, sizeof (ssid))) < 0)
    {
      exit (EXIT_FAILURE);
    }

  printf ("ssid (%d) = %s\n", len, ssid);

  exit (EXIT_SUCCESS);
}
