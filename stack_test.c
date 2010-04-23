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

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "app_err.h"
#include "logger.h"
#include "stack.h"

GLOBAL_LOGGER;

int
main (int argc, char **argv, char **envp)
{
  stack *s;
  int top_element;
  int an_integer;

  SETUP_LOGGER ("/dev/stderr", errtostr);

  assert (create_stack (&s, sizeof (an_integer)) == 0);

  assert (pop (NULL, s) == -1);
  assert (top (&top_element, s) == -1);
  assert (is_empty (s));

  an_integer = 5;
  assert (push (&an_integer, s) == 0);
  assert (top (&top_element, s) == 0);
  assert (top_element == 5);
  assert (!is_empty (s));
  assert (pop (NULL, s) == 0);
  assert (top (&top_element, s) == -1);
  assert (pop (&top_element, s) == -1);
  assert (is_empty (s));

  an_integer = 20;
  assert (push (&an_integer, s) == 0);
  assert (top (&top_element, s) == 0);
  assert (top_element == 20);
  assert (pop (&top_element, s) == 0);
  assert (top_element == 20);
  assert (is_empty (s));
  assert (top (&top_element, s) == -1);
  assert (pop (&top_element, s) == -1);

  int i;
  for (i = 0; i < 4096; i++)
    {
      assert (push (&i, s) == 0);
      assert (top (&top_element, s) == 0);
      assert (top_element == i);
      assert (!is_empty (s));
    }
  while (pop (&top_element, s) != -1)
    {
      assert (top_element == --i);
      assert (top_element == 0 ? is_empty (s) : !is_empty (s));
    }
  assert (i == 0);
  assert (is_empty (s));

  destroy_stack (&s);

  exit (EXIT_SUCCESS);
}
