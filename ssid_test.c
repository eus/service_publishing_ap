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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ssid.h"

int
main (int argc, char **argv, char **envp)
{
  int i;
  ssize_t len;

  /* Binary SSID is okay */
  char ssid1 [] = {0x32, 0x1, 0x0, 0xf4, 0xf8, 0x32};
  char retrieved_ssid1[sizeof (ssid1)];

  assert (set_ssid (ssid1, sizeof (ssid1)) == 0);
  assert ((len = get_ssid (retrieved_ssid1, sizeof (retrieved_ssid1))) == sizeof (ssid1));
  for (i = 0; i < len; i++)
    {
      assert (ssid1[i] == retrieved_ssid1[i]);
    }

  /* CAUTION: A trailing NULL character cannot be put into the SSID */
  char ssid2 [] = "0123456789012345678901234567890"; /* 32 bytes in total */
  char retrieved_ssid2[sizeof (ssid2)];

  assert (set_ssid (ssid2, sizeof (ssid2)) == 0);
  assert ((len = get_ssid (retrieved_ssid2, sizeof (retrieved_ssid2)))
	  == sizeof (ssid2) - 1);
  for (i = 0; i < len; i++)
    {
      assert (ssid2[i] == retrieved_ssid2[i]);
    }
  char ssid3 [22]; /* provides two NULL trailing characters */
  char retrieved_ssid3[sizeof (ssid3)];
  memset (ssid3, 0, sizeof (ssid3));
  strcpy (ssid3, "01234567890123456789"); /* only 20 non-NULL characters */
  assert (set_ssid (ssid3, sizeof (ssid3)) == 0);
  assert ((len = get_ssid (retrieved_ssid3, sizeof (retrieved_ssid3)))
	  == sizeof (ssid3) - 2);
  for (i = 0; i < len; i++)
    {
      assert (ssid3[i] == retrieved_ssid3[i]);
    }

  /* Setting a zero-length SSID is okay. */
  char ssid4;
  assert (set_ssid (&ssid4, 0) == 0);
  assert ((len = get_ssid (&ssid4, sizeof (ssid4)))
	  == sizeof (ssid4) - 1);

  exit (EXIT_SUCCESS);
}
