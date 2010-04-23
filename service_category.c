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

#include <stdlib.h>
#include <sqlite3.h>
#include <string.h>
#include "stack.h"
#include "app_err.h"
#include "logger.h"
#include "logger_sqlite3.h"
#include "tlv.h"
#include "service_category.h"

#define TABLE_CATEGORY_LIST "category_list"
#define TABLE_CATEGORY_LIST_TMP "category_list_tmp"
#define COLUMN_CAT_ID "cat_id"
#define COLPOS_CAT_ID 0
#define COLUMN_CAT_NAME "cat_name"
#define COLPOS_CAT_NAME 1

#define TABLE_CATEGORY_STRUCTURE "category_structure"
#define TABLE_CATEGORY_STRUCTURE_TMP "category_structure_tmp"
#define COLUMN_CAT_ID "cat_id"
#define COLPOS_CAT_ID 0
#define COLUMN_SUBCAT_ID "subcat_id"
#define COLPOS_SUBCAT_ID 1

/** The alignment of cat_list_impl::cur_cat.name size in bytes. */
#define CUR_CAT_NAME_SIZE_ALIGNMENT 32

/**
 * The implementation of a list of categories. I don't build a linked-list from
 * the DB to be iterated. Instead, I directly iterate the categories using SQL
 * queries. Although the outside world sees the list as containing a tree like
 * the following one:
 * +-- NULL (0)
 * |-- Restaurant (1)
 * |-- Schools (2)
 * |   |-- Private schools (4)
 * |   |   |-- Kindergarden (6)
 * |   |   |-- Elementary school (7)
 * |   |
 * |   |-- Public schools (5)
 * |   |   |-- Kindergarden (6)
 * |   |   |-- Elementary school (7)
 * |
 * |-- Games (3)
 *     |-- Logic games (8)
 *     |-- Card games (9)
 *
 * the implementation is not since one subcategory may be reused by some parents
 * as illustrated below:
 * +-------+----------+
 * |cat_id | subcat_id|
 * +-------+----------+
 * |...    |...       |
 * | 2     | 4        |
 * | 2     | 5        |
 * | 4     | 6        |
 * | 4     | 7        |
 * | 5     | 6        |
 * | 5     | 7        |
 * |...    |...       |
 * +-------+----------+
 * Doing `select subcat_id from t where cat_id = 4' returns a subtree rooted
 * at "Private schools". But, doing `select cat_id from t where subcat_id = 6'
 * returns two parents: "Private schools" and "Public schools". This means that
 * navigation of the tree must never start from a subcategory. Therefore,
 * going to a subcategory has to record both the current category ID and its
 * iteration offset so that it is possible to go back to the parent categories.
 * For example, iterating "Schools" -> "Private schools" -> "Public schools" ->
 * "Kindergarden" causes the following vertical navigation information to be
 * stored in a stack:
 * +--------+-------------+
 * | cat_id | next_offset |
 * +--------+-------------+
 * | 2      | 3           |
 * | 7      | 2           |
 * +--------+-------------+
 *
 * In order to go back to the list containing the previous parent, the following
 * SQL should be used:
 * select cat_id, cat_name from category_list where cat_id in (
 *  select subcat_id from category_structure where cat_id = ?) limit 1 offset ?
 *
 * The first "?" should be initialized with cat_id 2 that has cat_id 7 as one of
 * its children so that the second "?" can be initialized to next_offset 2.
 *
 * If the content of the stack above is only the first entry, going to a parent
 * category is simply retrieving the list of top-level categories and selecting
 * the recorded next_offset.
 */
struct cat_list_impl
{
  /* Common */
  sqlite3 *db; /**< The category DB. */
  struct cat cur_cat; /**< The category under iterator. */
  size_t cur_cat_name_size; /**< The current capacity of cur_cat.name */
  unsigned long next_offset; /**< 
			      * The SQL offset value starting from 0 used to
			      * fetch the next category. 0 means that the
			      * internal iterator is not over any category.
			      */

  /* Flat iterator only */
  int is_flat; /**<
		* 0 if the iterator is a hierarchical iterator or non-zero if
		* the iterator is flat.
		*/
  sqlite3_stmt *flat_next_prev_stmt; /**<
				      * SQL to go to the next/prev category in
				      * a flat manner.
				      */

  /* Hierarchical iterator only */
  stack *parents_id_and_next_offset; /**<
				      * The ID and value of next_offset at
				      * each parent level.
				      */
  sqlite3_stmt *go_sub_sup_stmt; /**<
				  * SQL to go to the subcategory/super category
				  */
  sqlite3_stmt *top_level_next_prev_stmt; /**<
					   * SQL to go to the next/prev top-level
					   * category.
					   */
  sqlite3_stmt *next_prev_stmt; /**<
				 * SQL to go to the next/prev non-top-level
				 * category at a particular depth.
				 */
};

/** The structure of the object contained in parents_id_and_next_offset stack. */
struct id_and_next_offset
{
  unsigned long id;
  unsigned long next_offset;
};

static int
is_at_top_level (const cat_list *cl)
{
  return (cl->parents_id_and_next_offset == NULL
	  || is_empty (cl->parents_id_and_next_offset));
}

static int
open_cat_list_db (sqlite3 **db)
{
  char *err_msg;

  if (sqlite3_open (CATEGORY_LIST_DB, db))
    {
      SQLITE3_ERR (*db, "Cannot open category list DB");
      if (sqlite3_close (*db))
	{
	  SQLITE3_ERR (*db, "Cannot close category list DB");
	}
      return ERR_DB;
    }

  if (sqlite3_exec (*db,

		    "create table if not exists " TABLE_CATEGORY_LIST " ("
		    COLUMN_CAT_ID " integer primary key not null,"
		    COLUMN_CAT_NAME " text not null);"

		    "create table if not exists " TABLE_CATEGORY_STRUCTURE " ("
		    COLUMN_CAT_ID " integer not null,"
		    COLUMN_SUBCAT_ID " integer not null,"
		    "primary key (" COLUMN_CAT_ID ", " COLUMN_SUBCAT_ID "),"
		    "foreign key (" COLUMN_CAT_ID ") references "
		    TABLE_CATEGORY_LIST " (" COLUMN_CAT_ID "),"
		    "foreign key (" COLUMN_SUBCAT_ID ") references "
		    TABLE_CATEGORY_LIST " (" COLUMN_CAT_ID "));",

		    NULL, NULL, &err_msg))
    {
      SQLITE3_ERR_STR (err_msg, "Cannot create category tables");
      if (sqlite3_close (*db))
	{
	  SQLITE3_ERR (*db, "Cannot close category list DB");
	}
      return ERR_DB;
    }

  return ERR_SUCCESS;
}

int
load_flat_cat_list (cat_list **cl)
{
  int rc;
  struct cat_list_impl *o;

  if ((rc = load_cat_list (&o)))
    {
      l->APP_ERR (rc, "Cannot load flat category list");
      return ERR_LOAD_FLAT_CATEGORY_LIST;
    }
  o->is_flat = 1;

  *cl = o;

  return ERR_SUCCESS;
}

int
load_cat_list (cat_list **cl)
{
  struct cat_list_impl *o;
  int rc;

  o = malloc (sizeof (*o));
  if (o == NULL)
    {
      l->APP_ERR (ERR_MEM, "Cannot allocate category list");
      return ERR_LOAD_CATEGORY_LIST;
    }
  memset (o, 0, sizeof (*o));

  if ((rc = open_cat_list_db (&o->db)))
    {
      l->APP_ERR (rc, "Cannot open category list DB");
      free (o);
      return ERR_LOAD_CATEGORY_LIST;
    }
  if ((rc = reset (o)))
    {
      l->APP_ERR (rc, "Cannot reset category list DB");
      if (sqlite3_close (o->db))
	{
	  SQLITE3_ERR (o->db, "Cannot close category list DB");
	}
      free (o);
      return ERR_LOAD_CATEGORY_LIST;
    }

  *cl = o;

  return ERR_SUCCESS;
}

int
close_all_prepared_statements (cat_list *cl)
{
  int rc = ERR_SUCCESS;

  if (cl->flat_next_prev_stmt != NULL
      && sqlite3_finalize (cl->flat_next_prev_stmt))
    {
      SQLITE3_ERR (cl->db, "Cannot finalize category list flat_next_prev_stmt");
      rc = ERR_DB;
    }
  if (cl->go_sub_sup_stmt != NULL && sqlite3_finalize (cl->go_sub_sup_stmt))
    {
      SQLITE3_ERR (cl->db, "Cannot finalize category list go_sub_sup_stmt");
      rc = ERR_DB;
    }
  if (cl->top_level_next_prev_stmt != NULL
      && sqlite3_finalize (cl->top_level_next_prev_stmt))
    {
      SQLITE3_ERR (cl->db,
		   "Cannot finalize category list top_level_next_prev_stmt");
      rc = ERR_DB;
    }
  if (cl->next_prev_stmt != NULL && sqlite3_finalize (cl->next_prev_stmt))
    {
      SQLITE3_ERR (cl->db, "Cannot finalize category list next_prev_stmt");
      rc = ERR_DB;
    }

  cl->flat_next_prev_stmt = NULL;
  cl->go_sub_sup_stmt = NULL;
  cl->top_level_next_prev_stmt = NULL;
  cl->next_prev_stmt = NULL;

  return rc;
}

void
destroy_cat_list (cat_list **cl)
{
  int rc;

  if (*cl == NULL)
    {
      return;
    }

  if ((rc = close_all_prepared_statements (*cl)))
    {
      l->APP_ERR (rc, "Cannot close all prepared statements to destroy cat_list");
    }
  if (sqlite3_close ((*cl)->db))
    {
      SQLITE3_ERR ((*cl)->db, "Cannot close category list DB");
    }
  destroy_stack (&(*cl)->parents_id_and_next_offset);
  if ((*cl)->cur_cat.name != NULL)
    {
      free ((void *) (*cl)->cur_cat.name);
    }

  free ((void *) *cl);
  *cl = NULL;
}

static int
ensure_cur_cat_name (cat_list *cl, size_t needed_capacity)
{
  if (cl->cur_cat.name == NULL)
    {
      size_t padded_length = get_padded_length (needed_capacity,
						CUR_CAT_NAME_SIZE_ALIGNMENT);

      cl->cur_cat.name = malloc (padded_length);
      if (cl->cur_cat.name == NULL)
	{
	  l->ERR ("Not enough memory to allocate cur_cat.name");
	  return ERR_MEM;
	}

      cl->cur_cat_name_size = padded_length;
    }
  else if (cl->cur_cat_name_size < needed_capacity)
    {
      size_t padded_length = get_padded_length (needed_capacity,
						CUR_CAT_NAME_SIZE_ALIGNMENT);
      void *new_name = realloc (&cl->cur_cat.name, padded_length);

      if (new_name == NULL)
	{
	  l->ERR ("Not enough memory to enlarge cur_cat.name");
	  return ERR_MEM;
	}

      cl->cur_cat.name = new_name;
      cl->cur_cat_name_size = padded_length;
    }

  return ERR_SUCCESS;
}

static int
ensure_flat_next_prev_stmt (cat_list *cl)
{
  sqlite3_stmt *o;

  if (cl->flat_next_prev_stmt != NULL)
    {
      return ERR_SUCCESS;
    }

  if (sqlite3_prepare_v2 (cl->db,
			  "select " COLUMN_CAT_ID ", " COLUMN_CAT_NAME
			  " from " TABLE_CATEGORY_LIST_TMP
			  " limit 1 offset ?",
			  -1, &o, NULL))
    {
      SQLITE3_ERR (cl->db, "Cannot prepare flat_next_prev_stmt");
      return ERR_DB;
    }

  cl->flat_next_prev_stmt = o;

  return ERR_SUCCESS;
}

static int
ensure_parents_id_and_next_offset_stack (cat_list *cl)
{
  int rc;

  if (cl->parents_id_and_next_offset != NULL)
    {
      return ERR_SUCCESS;
    }

  if ((rc = create_stack (&cl->parents_id_and_next_offset,
			  sizeof (struct id_and_next_offset))))
    {
      l->APP_ERR (rc, "Cannot create parents ID and next offset stack");
      return rc;
    }

  return ERR_SUCCESS;
}

static int
ensure_go_sub_sup_stmt (cat_list *cl)
{
  sqlite3_stmt *o;

  if (cl->go_sub_sup_stmt != NULL)
    {
      return ERR_SUCCESS;
    }

  if (sqlite3_prepare_v2 (cl->db,
			  "select " COLUMN_CAT_ID ", " COLUMN_CAT_NAME
			  " from " TABLE_CATEGORY_LIST_TMP
			  " where " COLUMN_CAT_ID " in ("
			  "  select " COLUMN_SUBCAT_ID
			  "  from " TABLE_CATEGORY_STRUCTURE_TMP
			  "  where " COLUMN_CAT_ID " = ? )"
			  " limit 1 offset ?",
			  -1, &o, NULL))
    {
      SQLITE3_ERR (cl->db, "Cannot prepare go_sub_sup_stmt");
      return ERR_DB;
    }

  cl->go_sub_sup_stmt = o;

  return ERR_SUCCESS;
}

static int
ensure_top_level_next_prev_stmt (cat_list *cl)
{
  sqlite3_stmt *o;

  if (cl->top_level_next_prev_stmt != NULL)
    {
      return ERR_SUCCESS;
    }

  if (sqlite3_prepare_v2 (cl->db,
			  "select " COLUMN_CAT_ID ", " COLUMN_CAT_NAME
			  " from " TABLE_CATEGORY_LIST_TMP
			  " where " COLUMN_CAT_ID " not in ("
			  "  select " COLUMN_SUBCAT_ID
			  "  from " TABLE_CATEGORY_STRUCTURE_TMP ")"
			  " limit 1 offset ?",
			  -1, &o, NULL))
    {
      SQLITE3_ERR (cl->db, "Cannot prepare top_level_next_prev_stmt");
      return ERR_DB;
    }

  cl->top_level_next_prev_stmt = o;

  return ERR_SUCCESS;
}

static int
ensure_next_prev_stmt (cat_list *cl)
{
  sqlite3_stmt *o;

  if (cl->next_prev_stmt != NULL)
    {
      return ERR_SUCCESS;
    }

  if (sqlite3_prepare_v2 (cl->db,
			  "select " COLUMN_CAT_ID ", " COLUMN_CAT_NAME
			  " from " TABLE_CATEGORY_LIST_TMP
			  " where " COLUMN_CAT_ID " in ("
			  "  select " COLUMN_SUBCAT_ID
			  "  from " TABLE_CATEGORY_STRUCTURE_TMP
			  "  where " COLUMN_CAT_ID " = ?)"
			  " limit 1 offset ?",
			  -1, &o, NULL))
    {
      SQLITE3_ERR (cl->db, "Cannot prepare next_prev_stmt");
      return ERR_DB;
    }

  cl->next_prev_stmt = o;

  return ERR_SUCCESS;
}

static int
update_cur_cat (cat_list *cl, sqlite3_stmt *stmt)
{
  int rc;
  const char *cat_name = (char *) sqlite3_column_text (stmt, 1);

  if ((rc = ensure_cur_cat_name (cl, strlen (cat_name) + 1)))
    {
      l->APP_ERR (rc, "Cannot update cur_cat");
      return rc;
    }

  cl->cur_cat.id = sqlite3_column_int (stmt, 0);
  strcpy ((char *) cl->cur_cat.name, cat_name);

  return ERR_SUCCESS;
}

static int
execute_next_stmt (cat_list *cl, sqlite3_stmt **stmt,
		   int (*ensure_stmt_fn) (cat_list *cl),
		   int (*binding_fn) (cat_list *cl, sqlite3_stmt *stmt),
		   const char *stmt_name)
{
  int rc;
  int step_result;

  if ((rc = ensure_stmt_fn (cl)))
    {
      l->APP_ERR (rc, "Cannot ensure %s", stmt_name);
      return rc;
    }

  if (sqlite3_reset (*stmt))
    {
      SQLITE3_ERR (cl->db, "Cannot reset %s", stmt_name);
      return ERR_DB;
    }

  if (sqlite3_clear_bindings (*stmt))
    {
      SQLITE3_ERR (cl->db, "Cannot clear bindings of %s", stmt_name);
      return ERR_DB;
    }

  if ((rc = binding_fn (cl, *stmt)))
    {
      return rc;
    }

  step_result = sqlite3_step (*stmt);
  if (step_result == SQLITE_ROW)
    {
      if ((rc = update_cur_cat (cl, *stmt)))
	{
	  l->APP_ERR (rc, "Cannot advance %s", stmt_name);
	  return rc;
	}

      cl->next_offset++;

      return -1;
    }
  else if (step_result == SQLITE_DONE)
    {
      return 0;
    }
  else
    {
      SQLITE3_ERR (cl->db, "Cannot execute %s", stmt_name);
      return ERR_DB;
    }
}

static int
flat_next_prev_stmt_binder (cat_list *cl, sqlite3_stmt *stmt)
{
  if (sqlite3_bind_int (stmt, 1, cl->next_offset))
    {
      SQLITE3_ERR (cl->db, "Cannot bind next_offset to next_prev_stmt");
      return ERR_DB;
    }

  return ERR_SUCCESS;
}

static int
top_level_next_prev_stmt_binder (cat_list *cl, sqlite3_stmt *stmt)
{
  if (sqlite3_bind_int (stmt, 1, cl->next_offset))
    {
      SQLITE3_ERR (cl->db, "Cannot bind next_offset to top_level_next_prev_stmt");
      return ERR_DB;
    }

  return ERR_SUCCESS;
}

static int
next_prev_stmt_binder (cat_list *cl, sqlite3_stmt *stmt)
{
  int rc;
  struct id_and_next_offset o;

  if ((rc = top (&o, cl->parents_id_and_next_offset)))
    {
      l->APP_ERR (rc, "Cannot read top element of parents_id_and_next_offset");
      return rc;
    }

  if (sqlite3_bind_int (cl->next_prev_stmt, 1, o.id))
    {
      SQLITE3_ERR (cl->db, "Cannot bind id_and_next_offset.id to next_prev_stmt");
      return ERR_DB;
    }
  if (sqlite3_bind_int (cl->next_prev_stmt, 2, cl->next_offset))
    {
      SQLITE3_ERR (cl->db, "Cannot bind next_offset to next_prev_stmt");
      return ERR_DB;
    }

  return ERR_SUCCESS;
}

int
next (cat_list *cl)
{
  if (cl->is_flat)
    {
      return execute_next_stmt (cl,
				&cl->flat_next_prev_stmt,
				ensure_flat_next_prev_stmt,
				flat_next_prev_stmt_binder,
				"flat_next_prev_stmt");
    }
  else
    {
      if (is_at_top_level (cl))
	{
	  return execute_next_stmt (cl,
				    &cl->top_level_next_prev_stmt,
				    ensure_top_level_next_prev_stmt,
				    top_level_next_prev_stmt_binder,
				    "top_level_next_prev_stmt");
	}
      else
	{
	  return execute_next_stmt (cl,
				    &cl->next_prev_stmt,
				    ensure_next_prev_stmt,
				    next_prev_stmt_binder,
				    "next_prev_stmt");
	}
    }

  return ERR_INVALID_STATE;
}

int
prev (cat_list *cl)
{
  int rc;

  /* next_offset == 0, internal iterator is not over any category.
   * next_offset == 1, internal iterator is over the first category.
   */
  if (cl->next_offset < 2)
    {
      return 0;
    }

  cl->next_offset -= 2;

  rc = next (cl);
  if (rc != 0 && rc != -1)
    {
      l->APP_ERR (rc, "Cannot move internal iterator to the previous location");
    }

  return rc;
}

static int
go_sub_sup (cat_list *cl, unsigned long from_id,
	    unsigned long from_id_next_offset, unsigned long to_next_offset)
{
  int rc;
  int step_result;

  if ((rc = ensure_parents_id_and_next_offset_stack (cl)))
    {
      l->APP_ERR (rc, "Cannot ensure parents_id_and_next_offset");
      return rc;
    }

  if ((rc = ensure_go_sub_sup_stmt (cl)))
    {
      l->APP_ERR (rc, "Cannot ensure go_sub_sup_stmt");
      return rc;
    }

  if (sqlite3_reset (cl->go_sub_sup_stmt))
    {
      SQLITE3_ERR (cl->db, "Cannot reset go_sub_sup_stmt");
      return ERR_DB;
    }

  if (sqlite3_clear_bindings (cl->go_sub_sup_stmt))
    {
      SQLITE3_ERR (cl->db, "Cannot clear bindings of go_sub_sup_stmt");
      return ERR_DB;
    }

  if (sqlite3_bind_int (cl->go_sub_sup_stmt, 1, from_id))
    {
      SQLITE3_ERR (cl->db, "Cannot bind from_id to go_sub_sup_stmt");
      return ERR_DB;
    }
  if (sqlite3_bind_int (cl->go_sub_sup_stmt, 2, to_next_offset - 1))
    {
      SQLITE3_ERR (cl->db, "Cannot bind next_offset to go_sub_sup_stmt");
      return ERR_DB;
    }

  step_result = sqlite3_step (cl->go_sub_sup_stmt);
  if (step_result == SQLITE_ROW)
    {
      int rc;
      struct id_and_next_offset o;

      o.id = from_id;
      o.next_offset = from_id_next_offset;

      if ((rc = push (&o, cl->parents_id_and_next_offset)))
	{
	  l->APP_ERR (rc, "Cannot go the subcategory");
	  return rc;
	}

      if ((rc = update_cur_cat (cl, cl->go_sub_sup_stmt)))
	{
	  int pop_rc;

	  if ((pop_rc = pop (NULL, cl->parents_id_and_next_offset)))
	    {
	      if (pop_rc == -1)
		{
		  l->APP_ERR (ERR_INVALID_STATE,
			      "parents_id_and_next_offset must not be empty");
		}
	      else
		{
		  l->APP_ERR (pop_rc, "Cannot pop parents_id_and_next_offset");
		}
	    }

	  return rc;
	}

      cl->next_offset = to_next_offset;

      return -1;
    }
  else if (step_result == SQLITE_DONE)
    {
      return 0;
    }
  else
    {
      SQLITE3_ERR (cl->db, "Cannot execute go_sub_sup_stmt");
      return ERR_DB;
    }
}

int
go_sub (cat_list *cl)
{
  int rc;

  if (cl->is_flat
      || cl->next_offset == 0) /* internal iterator is not over any category. */
    {
      return 0;
    }

  rc = go_sub_sup (cl, cl->cur_cat.id, cl->next_offset, 1);
  if (rc != 0 && rc != -1)
    {
      l->APP_ERR (rc, "Cannot move internal iterator to the subcategory");
    }

  return rc;
}

int
go_sup (cat_list *cl)
{
  struct id_and_next_offset parent;
  struct id_and_next_offset grandparent;
  int rc;

  if (cl->is_flat
      || cl->next_offset == 0 /* internal iterator is not over any category. */
      || is_at_top_level (cl))
    {
      return 0;
    }

  if ((rc = pop (&parent, cl->parents_id_and_next_offset)))
    {
      l->APP_ERR (rc, "Cannot pop parent id and next offset");
      return rc;
    }
  rc = pop (&grandparent, cl->parents_id_and_next_offset);
  if (rc == -1)
    {
      cl->next_offset = parent.next_offset - 1;
      rc = next (cl);
      if (rc != 0 && rc != -1)
	{
	  l->APP_ERR (rc, "Cannot move internal iterator to the parent category");
	}
      return rc;
    }
  else if (rc == 0)
    {
      rc = go_sub_sup (cl, grandparent.id, grandparent.next_offset,
		       parent.next_offset);
      if (rc != 0 && rc != -1)
	{
	  l->APP_ERR (rc, "Cannot move internal iterator to the parent category");
	}
      return rc;
    }
  else
    {
      l->APP_ERR (rc, "Cannot pop grandparent id and next offset");
      return rc;
    }
}

const struct cat *
get_cat (const cat_list *cl)
{
  if (cl->next_offset == 0)
    {
      return NULL;
    }

  return &cl->cur_cat;
}

int
update_cat_list (const char *sql_statements)
{
  char *err_msg;
  sqlite3 *db;
  int rc;

  if ((rc = open_cat_list_db (&db)))
    {
      l->APP_ERR (rc, "Cannot open category list DB for update");
      return ERR_UPDATE_CATEGORY_LIST;
    }

  rc = ERR_SUCCESS;
  if (sqlite3_exec (db,
		    sql_statements,
		    NULL, NULL, &err_msg))
    {
      SQLITE3_ERR_STR (err_msg, "Cannot execute update");
      rc = ERR_UPDATE_CATEGORY_LIST;
    }

  /* Data should have been safely written to the file so that a failing DB close
   * should not affect the already stored data.
   */
  if (sqlite3_close (db))
    {
      SQLITE3_ERR (db, "Cannot close category list DB");
    }

  return rc;
}

int
reset (cat_list *cl)
{
  int rc;
  char *err_msg;

  if ((rc = close_all_prepared_statements (cl)))
    {
      l->APP_ERR (rc, "Cannot close all prepared statements for reset");
      return rc;
    }

  if (sqlite3_exec (cl->db,

		    "drop table if exists " TABLE_CATEGORY_LIST_TMP ";"
		    "drop table if exists " TABLE_CATEGORY_STRUCTURE_TMP ";"

		    "create temporary table " TABLE_CATEGORY_LIST_TMP " ("
		    COLUMN_CAT_ID " integer primary key not null,"
		    COLUMN_CAT_NAME " text not null);"

		    "create temporary table " TABLE_CATEGORY_STRUCTURE_TMP " ("
		    COLUMN_CAT_ID " integer not null,"
		    COLUMN_SUBCAT_ID " integer not null,"
		    "primary key (" COLUMN_CAT_ID ", " COLUMN_SUBCAT_ID "),"
		    "foreign key (" COLUMN_CAT_ID ") references "
		    TABLE_CATEGORY_LIST_TMP " (" COLUMN_CAT_ID "),"
		    "foreign key (" COLUMN_SUBCAT_ID ") references "
		    TABLE_CATEGORY_LIST_TMP " (" COLUMN_CAT_ID "));"

		    "insert into " TABLE_CATEGORY_LIST_TMP
		    " select * from " TABLE_CATEGORY_LIST ";"

		    "insert into " TABLE_CATEGORY_STRUCTURE_TMP
		    " select * from " TABLE_CATEGORY_STRUCTURE ";",

		    NULL, NULL, &err_msg))
    {
      SQLITE3_ERR_STR (err_msg, "Cannot reset category list");
      return ERR_DB;
    }

  cl->next_offset = 0;

  if (cl->parents_id_and_next_offset != NULL)
    {
      while ((rc = pop (NULL, cl->parents_id_and_next_offset)));
      if (rc != -1)
	{
	  l->APP_ERR (rc, "Cannot empty stack during reset of cat_list");
	  return rc;
	}
    }

  return ERR_SUCCESS;
}
