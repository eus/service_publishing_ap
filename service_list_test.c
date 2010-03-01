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
#include <sqlite3.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "app_err.h"
#include "logger.h"
#include "logger_sqlite3.h"
#include "service_list.h"
#include "ssid.h"

#define TABLE_SERVICE_LIST "service_list"
#define COLUMN_POSITION "position"
#define COLUMN_MOD_TIME "mod_time"
#define COLUMN_CAT_ID "cat_id"
#define COLUMN_URI "uri"
#define COLUMN_DESC "desc"
#define COLUMN_LONG_DESC "long_desc"

GLOBAL_LOGGER;

static sqlite3 *db = NULL;
static char *err_msg = NULL;

static void
clean_up (void)
{
  if (err_msg != NULL)
    {
      sqlite3_free (err_msg);
      err_msg = NULL;
    }
  if (db != NULL)
    {
      if (sqlite3_close (db))
	SQLITE3_ERR (db, "Cannot close DB");
      db = NULL;
    }
}

static void
setup_test_db (void)
{
  if (sqlite3_open (SERVICE_LIST_DB, &db))
    {
      SQLITE3_ERR (db, "Cannot open %s", SERVICE_LIST_DB);
      exit (EXIT_FAILURE);
    }

  if (sqlite3_exec (db,
		    "drop table if exists " TABLE_SERVICE_LIST ";"
		    "create table " TABLE_SERVICE_LIST " ("
		    COLUMN_POSITION " integer primary key not null,"
		    COLUMN_MOD_TIME " integer not null,"
		    COLUMN_CAT_ID " integer not null,"
		    COLUMN_URI " text not null,"
		    COLUMN_DESC " text,"
		    COLUMN_LONG_DESC " text"
		    ")", NULL, NULL, &err_msg))
    {
      SQLITE3_ERR_STR (err_msg, "Cannot create test table");
      exit (EXIT_FAILURE);
    }
}

struct service *
service_factory (unsigned long cat_id,
		 char *desc,
		 char *long_desc,
		 char *uri)
{
  char *desc_ptr = desc ? malloc (strlen (desc) + 1) : NULL;
  char *long_desc_ptr = long_desc ? malloc (strlen (long_desc) + 1) : NULL;
  char *uri_ptr = uri ? malloc (strlen (uri) + 1) : NULL;
  struct service *s = NULL;

  if (desc_ptr) strcpy (desc_ptr, desc);
  if (long_desc_ptr) strcpy (long_desc_ptr, long_desc);
  if (uri_ptr) strcpy (uri_ptr, uri);

  assert (0 == create_service (&s, cat_id, desc_ptr, long_desc_ptr, uri_ptr));

  return s;
}

int
main (int argc, char **argv, char **envp)
{
  char ssid[SSID_MAX_LEN];
  ssize_t ssid_len;
  char *expected_ssid;
  service_list *sl;
  struct service *s;
  uint64_t last_mod_time;

  SETUP_LOGGER ("/dev/stderr", errtostr);

  if (atexit (clean_up))
    {
      l->SYS_ERR ("Cannot register clean up");
      exit (EXIT_FAILURE);
    }

  setup_test_db ();

  /* test for emptiness */
  assert (0 == load_service_list (&sl));
  assert (0 == save_service_list (sl));
  assert (0 == count_service (sl));

  /* test for insert last in an empty list */
  s = service_factory (1, NULL, "service1", "uri1");
  assert (s != NULL);
  assert (0 == add_service_last (sl, s));
  destroy_service (&s);

  /* test the element */
  assert (1 == count_service (sl));
  assert (0 == get_service_at (sl, &s, 0));
  assert (s->cat_id == 1);
  assert (s->desc == NULL);
  assert (strcmp (s->long_desc, "service1") == 0);
  assert (strcmp (s->uri, "uri1") == 0);
  destroy_service (&s);

  /* test a non-existent element */
  assert (0 == get_service_at (sl, &s, 3));
  assert (s == NULL);

  /* test another insert at the end */
  s = service_factory (2, "short2", NULL, "uri2");
  assert (s != NULL);
  assert (0 == add_service_last (sl, s));
  destroy_service (&s);

  /* test the elements */
  assert (2 == count_service (sl));
  assert (0 == get_service_at (sl, &s, 0));
  assert (s->cat_id == 1);
  destroy_service (&s);
  assert (0 == get_service_at (sl, &s, 1));
  assert (s->cat_id == 2);
  destroy_service (&s);

  /* test insert at the front */
  s = service_factory (3, "short3", NULL, "uri3");
  assert (s != NULL);
  assert (0 == add_service_first (sl, s));
  destroy_service (&s);

  /* test the elements */
  assert (3 == count_service (sl));
  assert (0 == get_service_at (sl, &s, 0));
  assert (s->cat_id == 3);
  destroy_service (&s);
  assert (0 == get_service_at (sl, &s, 1));
  assert (s->cat_id == 1);
  destroy_service (&s);
  assert (0 == get_service_at (sl, &s, 2));
  assert (s->cat_id == 2);
  destroy_service (&s);

  /* test insert at the middle */
  s = service_factory (4, "short4", NULL, "uri4");
  assert (s != NULL);
  assert (0 == insert_service_at (sl, s, 1));
  destroy_service (&s);

  /* test the elements */
  assert (4 == count_service (sl));
  assert (0 == get_service_at (sl, &s, 0));
  assert (s->cat_id == 3);
  destroy_service (&s);
  assert (0 == get_service_at (sl, &s, 1));
  assert (s->cat_id == 4);
  destroy_service (&s);
  assert (0 == get_service_at (sl, &s, 2));
  assert (s->cat_id == 1);
  destroy_service (&s);
  assert (0 == get_service_at (sl, &s, 3));
  assert (s->cat_id == 2);
  destroy_service (&s);

  /* test insert at an invalid pos */
  s = service_factory (5, "short5", NULL, "uri5");
  assert (s != NULL);
  assert (insert_service_at (sl, s, 5));
  destroy_service (&s);

  /* test delete service at the middle */
  assert (0 == del_service_at (sl, 1));

  /* test the elements */
  assert (3 == count_service (sl));
  assert (0 == get_service_at (sl, &s, 0));
  assert (s->cat_id == 3);
  destroy_service (&s);
  assert (0 == get_service_at (sl, &s, 1));
  assert (s->cat_id == 1);
  destroy_service (&s);
  assert (0 == get_service_at (sl, &s, 2));
  assert (s->cat_id == 2);
  destroy_service (&s);

  /* test get mod time */
  assert (0 == get_last_modification_time (sl));

  /* test delete service at the end */
  assert (0 == del_service_at (sl, 2));

  /* test the elements */
  assert (2 == count_service (sl));
  assert (0 == get_service_at (sl, &s, 0));
  assert (s->cat_id == 3);
  destroy_service (&s);
  assert (0 == get_service_at (sl, &s, 1));
  assert (s->cat_id == 1);
  destroy_service (&s);
  assert (0 == get_service_at (sl, &s, 2));
  assert (s == NULL);

  /* test replace service */
  s = service_factory (6, "short6", NULL, "uri6");
  assert (s != NULL);
  assert (0 == replace_service_at (sl, s, 0));
  destroy_service (&s);

  /* test the elements */
  assert (2 == count_service (sl));
  assert (0 == get_service_at (sl, &s, 0));
  assert (s->cat_id == 6);
  assert (strcmp (s->desc, "short6") == 0);
  assert (s->long_desc == NULL);
  assert (strcmp (s->uri, "uri6") == 0);
  destroy_service (&s);
  assert (0 == get_service_at (sl, &s, 1));
  assert (s->cat_id == 1);
  destroy_service (&s);

  /* test delete all */
  assert (0 == del_service_all (sl));
  assert (0 == get_service_at (sl, &s, 0));
  assert (s == NULL);

  /* test for emptiness */
  assert (0 == save_service_list (sl));
  assert (0 == count_service (sl));

  /* test for SSID publication */
  s = service_factory (1, NULL, "service1", "uri1");
  assert (s != NULL);
  assert (0 == add_service_first (sl, s));
  destroy_service (&s);
  s = service_factory (2, "service2", "service2", "uri2");
  assert (s != NULL);
  assert (0 == add_service_last (sl, s));
  destroy_service (&s);
  s = service_factory (3, NULL, NULL, "uri3");
  assert (s != NULL);
  assert (0 == add_service_last (sl, s));
  destroy_service (&s);
  assert (0 == save_service_list (sl));
  expected_ssid = "##^1^2,service2^3";
  ssid_len = get_ssid (ssid, sizeof (ssid));
  assert (strlen (expected_ssid) == ssid_len);
  assert (memcmp (ssid, expected_ssid, ssid_len) == 0);

  /* test get mod time */
  last_mod_time = get_last_modification_time (sl);
  assert (0 != last_mod_time);

  /* test mod time permanency when no data actually changes */
  s = service_factory (4, "desc4", "Our Daily Bread", "http://odb.org");
  assert (s != NULL);
  assert (0 == replace_service_at (sl, s, 2));
  destroy_service (&s);
  s = service_factory (3, NULL, NULL, "uri3");
  assert (s != NULL);
  assert (0 == replace_service_at (sl, s, 2));
  destroy_service (&s);
  assert (0 == del_service_at (sl, 0));
  s = service_factory (1, NULL, "service1", "uri1");
  assert (s != NULL);
  assert (0 == add_service_first (sl, s));
  destroy_service (&s);
  assert (0 == save_service_list (sl));
  expected_ssid = "##^1^2,service2^3";
  ssid_len = get_ssid (ssid, sizeof (ssid));
  assert (strlen (expected_ssid) == ssid_len);
  assert (memcmp (ssid, expected_ssid, ssid_len) == 0);
  assert (last_mod_time == get_last_modification_time (sl));
  
  destroy_service_list (&sl);
  
  exit (EXIT_SUCCESS);
}
