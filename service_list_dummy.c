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
 ****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "app_err.h"
#include "logger.h"
#include "service_list.h"

/** The implementation of service list. */
struct service_list_impl
{
  int unused;
};

int
create_service (struct service **s,
		unsigned long cat_id,
		char *desc,
		char *long_desc,
		char *uri)
{
  struct service *ptr_s;

  if (uri == NULL)
    {
      return ERR_CREATE_SERVICE_URI_NULL;
    }

  ptr_s = malloc (sizeof (*ptr_s));
  if (ptr_s == NULL)
    {
      return ERR_MEM;
    }

  ptr_s->ro.pos = cat_id;
  ptr_s->ro.mod_time = 888999777;

  ptr_s->cat_id = cat_id;
  ptr_s->desc = desc;
  ptr_s->long_desc = long_desc;
  ptr_s->uri = uri;

  *s = ptr_s;

  return ERR_SUCCESS;
}

void
destroy_service (struct service **s)
{
  if (*s == NULL)
    {
      return;
    }
  free ((void *) *s);
  *s = NULL;
}

int
load_service_list (service_list **sl)
{
  static struct service_list_impl dt;

  *sl = &dt;
  l->INFO ("Service list loaded");

  return ERR_SUCCESS;
}

int
reload_service_list (service_list *sl)
{
  return ERR_SUCCESS;
}

void
destroy_service_list (service_list **sl)
{
  l->INFO ("Service list destroyed");
}

void
publish_services (void)
{
  l->INFO ("Service list published");
}

int
save_service_list (const service_list *sl)
{
  l->INFO ("Service list saved"); 
  
  return ERR_SUCCESS;
}

size_t
count_service (const service_list *sl)
{
  l->INFO ("Services counted"); 

  return 3;
}

int
add_service_first (service_list *sl, const struct service *s)
{
  int rc;

  if ((rc = insert_service_at (sl, s, 0)))
    {
      l->APP_ERR (rc, "Cannot insert service at position 0");
      return ERR_ADD_SERVICE_FIRST;
    }

  return ERR_SUCCESS;
}

int
add_service_last (service_list *sl, const struct service *s)
{
  int rc;
  size_t last_pos = count_service (sl) - 1;

  if ((rc = insert_service_at (sl, s, last_pos)))
    {
      l->APP_ERR (rc, "Cannot insert service at position %d (last)", last_pos);
      return ERR_ADD_SERVICE_LAST;
    }

  return ERR_SUCCESS;
}

int
get_service_at (service_list *sl, struct service **s, unsigned int idx)
{
  static char desc1[] = "Desc 1";
  static char uri1[] = "URI 1";
  static char desc2[] = "Desc 2";
  static char longdesc2[] = "Long Desc 2";
  static char uri2[] = "URI 2";
  static char desc3[] = "Desc 3";
  static char uri3[] = "URI 3";

  l->INFO ("Get service at %u", idx);

  switch (idx)
    {
    case 0:
      if (create_service (s,
			  1,
			  desc1,
			  NULL,
			  uri1))
	{
	  l->ERR ("Cannot create service");
	  return ERR_GET_SERVICE;
	}
      break;
    case 1:
      if (create_service (s,
			  2,
			  desc2,
			  longdesc2,
			  uri2))
	{
	  l->ERR ("Cannot create service");
	  return ERR_GET_SERVICE;
	}
      break;
    default:
      if (create_service (s,
			  3,
			  desc3,
			  NULL,
			  uri3))
	{
	  l->ERR ("Cannot create service");
	  return ERR_GET_SERVICE;
	}
      break;
    }

  return ERR_SUCCESS;
}

int
insert_service_at (service_list *sl, const struct service *s, unsigned int idx)
{
  l->INFO ("Service inserted at %u", idx);

  return ERR_SUCCESS;
}

int
replace_service_at (service_list *sl, const struct service *s, unsigned int idx)
{
  l->INFO ("Service replaced at %u", idx);

  return ERR_SUCCESS;
}

int
delete_service_at (service_list *sl, const struct service *s, unsigned int idx)
{
  l->INFO ("Service deleted at %u", idx);

  return ERR_SUCCESS;
}

int
del_service_all (service_list *sl)
{
  l->INFO ("All services deleted");

  return ERR_SUCCESS;
}

uint64_t
get_last_modification_time (service_list *sl)
{
  uint64_t result = 83828299ULL;

  l->INFO ("Last mod time calculated");

  return result;
}
