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

#include <string.h>
#include <stdlib.h>
#include "app_err.h"
#include "logger.h"
#include "stack.h"

/** The number of new elements to be accommodated when the stack is enlarged. */
#define STACK_INCREMENT 10

/** The implementation of stack of objects. */
struct stack_impl
{
  void *base; /**< The pointer to the stack. */
  void *end; /**< The end of the stack. */
  unsigned long next_idx; /**< The push index of the next element. */
  int is_empty; /**< 0 if the stack is not empty but 1 if it is. */
  unsigned long element_size; /**< The size in bytes of an element. */
};

int
create_stack (stack **s, unsigned long element_size)
{
  stack *p;

  p = malloc (sizeof (*p));
  if (p == NULL)
    {
      l->ERR ("No memory to create stack");
      return ERR_MEM;
    }
  memset (p, 0, sizeof (*p));
  p->is_empty = 1;
  p->element_size = element_size;

  p->base = malloc (STACK_INCREMENT * element_size);
  if (p->base == NULL)
    {
      l->ERR ("No memory to store data in stack");
      free (p);
      return ERR_MEM;
    }
  p->end = p->base + STACK_INCREMENT * element_size;

  *s = p;

  return ERR_SUCCESS;
}

void
destroy_stack (stack **s)
{
  if (*s == NULL)
    {
      return;
    }

  free ((*s)->base);
  free ((void *) *s);

  *s = NULL;
}

static int
enlarge_stack (stack *s)
{
  size_t cur_element_count = s->next_idx;
  size_t new_element_count = cur_element_count + STACK_INCREMENT;
  void *resized_stack = realloc (s->base, new_element_count * s->element_size);

  if (resized_stack == NULL)
    {
      l->ERR ("Cannot enlarge stack");
      return ERR_MEM;
    }

  s->base = resized_stack;
  s->end = s->base + new_element_count * s->element_size;

  return ERR_SUCCESS;
}

int
push (void *e, stack *s)
{
  if (s->end - s->base == s->next_idx * s->element_size)
    {
      int rc;

      if ((rc = enlarge_stack (s)))
	{
	  l->APP_ERR (rc, "Cannot push into stack");
	  return rc;
	}
    }

  memcpy (s->base + s->next_idx * s->element_size, e, s->element_size);
  s->next_idx++;

  if (s->is_empty)
    {
      s->is_empty = 0;
    }

  return ERR_SUCCESS;
}

int
pop (void *e, stack *s)
{
  if (s->is_empty)
    {
      return -1;
    }

  s->next_idx--;

  if (e != NULL)
    {
      memcpy (e, s->base + s->next_idx * s->element_size, s->element_size);
    }

  if (s->next_idx == 0)
    {
      s->is_empty = 1;
    }

  return ERR_SUCCESS;
}

int
top (void *e, const stack *s)
{
  if (s->is_empty)
    {
      return -1;
    }

  memcpy (e, s->base + (s->next_idx - 1) * s->element_size, s->element_size);

  return ERR_SUCCESS;
}

int
is_empty (const stack *s)
{
  return s->is_empty;
}
