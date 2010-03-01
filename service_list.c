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

/**
 * Any write operation between load_service_list() and save_service_list()
 * are treated as temporary so as to permit the presence of many writers
 * because the main service list table may only change when the write
 * operations are deemed okay by save_service_list()
 * (i.e., the modified SSID is publishable).
 *
 * To do so, all write operations are done in
 * a temporary table that is unique per sqlite3 DB connection. When at least
 * one write operation has already been performed, any subsequent read
 * operation will read the temporary writing table instead of the main one
 * (this is logical since the modification is local).
 *
 * The rationale behind the temporary table is because the first uncommitted
 * write operation prevents another write operation through another DB
 * connection, and therefore, prevents another writer from tinkering with the
 * main table. Originally I would like to implement the data structure as a
 * double linked list. One main copy is kept for SDE and each writer is
 * assigned a copy of the whole link list. But, then I would reinvent the
 * wheel that sqlite3 has taken care of so well (e.g., the ACID properties).
 *
 * It turns out that each reader should also be allocated a temporary table.
 * Consider the following code (sl is a service list):
 * <pre>
 *  size_t service_count = count_service (sl);
 *  for (i = 0; i < service_count; i++)
 *    {
 *      struct service *s;
 *      
 *      get_service_at (sl, &s, i);
 *      // do something fun with s
 *      destroy_service (&s);
 *    }
 * </pre>
 * If a writer happens to write into the main table while the poor reader is
 * still in the loop, service_count may be too long and hence s may be NULL.
 * The reader will unexpectedly get a segmentation fault that is hard to debug
 * because the premise is that the number of iterated service should stay the
 * same over a course of iteration.
 */

#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>
#include "app_err.h"
#include "logger.h"
#include "logger_sqlite3.h"
#include "service_list.h"
#include "ssid.h"

#define TABLE_SERVICE_LIST "service_list"
#define TABLE_SERVICE_LIST_TMP "service_list_tmp"
#define COLUMN_POSITION "position"
#define COLPOS_POSITION 0
#define COLUMN_MOD_TIME "mod_time"
#define COLPOS_MOD_TIME 1
#define COLUMN_CAT_ID "cat_id"
#define COLPOS_CAT_ID 2
#define COLUMN_URI "uri"
#define COLPOS_URI 3
#define COLUMN_DESC "desc"
#define COLPOS_DESC 4
#define COLUMN_LONG_DESC "long_desc"
#define COLPOS_LONG_DESC 5

/** The implementation of service list. */
struct service_list_impl
{
  sqlite3 *db; /**< The published service DB. */
  int has_service_list_tmp_table; /**<
				   * Non-zero if a temporary table has been
				   * created as a duplicate of the service list
				   * table for temporary writing purpose.
				   */
  sqlite3_stmt *get_service_at; /**<
				 * The supporting DML of the stated
				 * operation.
				 */
  sqlite3_stmt *insert_service_at; /**<
				    * The supporting DML of the stated
				    * operation.
				    */
  sqlite3_stmt *replace_service_at; /**<
				     * The supporting DML of the stated
				     * operation.
				     */
  sqlite3_stmt *delete_service_at; /**<
				    * The supporting DML of the stated
				    * operation.
				    */
  sqlite3_stmt *inc_dec_pos; /**<
			      * Increment/decrement the position of a service.
			      */
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
  if (*s != NULL)
    {
      if ((*s)->desc != NULL)
	{
	  free ((*s)->desc);
	}
      if ((*s)->long_desc != NULL)
	{
	  free ((*s)->long_desc);
	}
      if ((*s)->uri != NULL)
	{
	  free ((*s)->uri);
	}
      free ((void *) *s);
      *s = NULL;
    }
}

/**
 * Since it is not possible to be waiting forever with the presence of temporary
 * writing table per DB connection, we will just wait forever.
 */
static int
patient_busy_handler (void *arg, int waiting_time)
{
  return 1;
}

int
load_service_list (service_list **sl)
{
  char *err_msg = NULL;
  struct service_list_impl *ptr_sl = NULL;

  ptr_sl = malloc (sizeof (*ptr_sl));
  if (ptr_sl == NULL)
    {
      return ERR_MEM;
    }
  memset (ptr_sl, 0, sizeof (*ptr_sl));

  if (sqlite3_open (SERVICE_LIST_DB, &ptr_sl->db))
    {
      SQLITE3_ERR (ptr_sl->db, "Cannot open DB");
      if (sqlite3_close (ptr_sl->db))
	{
	  SQLITE3_ERR (ptr_sl->db, "Cannot close DB");
	}
      return ERR_LOAD_SERVICE_LIST;
    }

  if (sqlite3_exec (ptr_sl->db, 
		    "create table if not exists " TABLE_SERVICE_LIST " ("
		    COLUMN_POSITION " integer primary key not null,"
		    COLUMN_MOD_TIME " integer not null,"
		    COLUMN_CAT_ID " integer not null,"
		    COLUMN_URI " text not null,"
		    COLUMN_DESC " text,"
		    COLUMN_LONG_DESC " text"
		    ")",
		    NULL, NULL, &err_msg))
    {
      SQLITE3_ERR_STR (err_msg, "Cannot create service list table");
      if (sqlite3_close (ptr_sl->db))
	{
	  SQLITE3_ERR (ptr_sl->db, "Cannot close DB");
	}
      return ERR_LOAD_SERVICE_LIST;
    }

  if (sqlite3_busy_handler(ptr_sl->db, patient_busy_handler, NULL))
    {
      SQLITE3_ERR (ptr_sl->db, "Cannot install DB busy handler");
      if (sqlite3_close (ptr_sl->db))
	{
	  SQLITE3_ERR (ptr_sl->db, "Cannot close DB");
	}
      return ERR_LOAD_SERVICE_LIST;
    }

  *sl = ptr_sl;

  return ERR_SUCCESS;
}

void
destroy_service_list (service_list **sl)
{
  if (*sl == NULL)
    {
      return;
    }

  if ((*sl)->get_service_at != NULL)
    {
      if (sqlite3_finalize ((*sl)->get_service_at))
	{
	  SQLITE3_ERR ((*sl)->db, "Cannot finalize statement "
			 "get service at");
	}
    }
  if ((*sl)->insert_service_at != NULL)
    {
      if (sqlite3_finalize ((*sl)->insert_service_at))
	{
	  SQLITE3_ERR ((*sl)->db, "Cannot finalize statement "
			 "insert service at");
	}
    }
  if ((*sl)->replace_service_at != NULL)
    {
      if (sqlite3_finalize ((*sl)->replace_service_at))
	{
	  SQLITE3_ERR ((*sl)->db, "Cannot finalize statement "
			 "replace service at");
	}
    }
  if ((*sl)->delete_service_at != NULL)
    {
      if (sqlite3_finalize ((*sl)->delete_service_at))
	{
	  SQLITE3_ERR ((*sl)->db, "Cannot finalize statement "
			 "del service at");
	}
    }
  if ((*sl)->inc_dec_pos != NULL)
    {
      if (sqlite3_finalize ((*sl)->inc_dec_pos))
	{
	  SQLITE3_ERR ((*sl)->db, "Cannot finalize statement inc dec pos");
	}
    }

  if (sqlite3_close ((*sl)->db))
    {
      SQLITE3_ERR ((*sl)->db, "Cannot close DB");
    }

  free ((void *) *sl);
  *sl = NULL;
}

/** The arguments to gen_ssid(). */
struct gen_ssid_arg
{
  char ssid[SSID_MAX_LEN]; /**< The resulting SSID. */
  size_t ssid_len; /**< The length of the resulting SSID. */
  int last_pos; /**<
		 * The last position of the advertised service (0-based).
		 * This must be initialized to -1 for checking purpose.
		 */
  int rc; /**< The error encountered during parsing. */
};

/**
 * The callback function used with sqlite3_exec() to get the SSID to be
 * advertised. The SQL select in sqlite3_exec() <strong>MUST</strong> be
 * correct in selecting only cat_id as the first column, desc as the second
 * one and position (for DB integrity check) as the third one,
 * and the services are ordered by their positions in ascending order.
 *
 * @param [out] result where the SSID should be put.
 * @param [in] col_count the number of columns in this record.
 * @param [in] cols the columns of this record.
 * @param [in] col_names the column names of this record.
 *
 * @return 0 if there is no error or non-zero if there is one.
 */
static int
gen_ssid (void *result, int col_count, char **cols, char **col_names)
{
  struct gen_ssid_arg *arg = result;
  size_t len;
  unsigned long position = strtoul (cols[2], NULL, 10);

  if (arg->last_pos + 1 != position)
    {
      l->ERR ("Wrong service position in the SSID (%d + 1 != %lu)",
	      arg->last_pos, position);
      arg->rc = ERR_INVALID_SERVICE_POS;
      return -1;
    }

  len = strlen (cols[0]); /* cat_id */
  if (arg->ssid_len + len + 1 > SSID_MAX_LEN)
    {
      arg->rc = ERR_SSID_TOO_LONG;
      return -1;
    }
  arg->ssid[arg->ssid_len] = '^';
  arg->ssid_len++;
  strncpy (arg->ssid + arg->ssid_len, cols[0], len);
  arg->ssid_len += len;

  if (cols[1] != NULL) /* desc */
    {
      len = strlen (cols[1]);
      if (arg->ssid_len + len + 1 > SSID_MAX_LEN)
	{
	  arg->rc = ERR_SSID_TOO_LONG;
	  return -1;
	}
      arg->ssid[arg->ssid_len] = ',';
      arg->ssid_len++;
      strncpy (arg->ssid + arg->ssid_len, cols[1], len);
      arg->ssid_len += len;
    }

  arg->last_pos = position;

  return 0;
}

/**
 * The callback function used with sqlite3_exec() to not change the modification
 * time of unmodified record. The SQL select in sqlite3_exec()
 * <strong>MUST</strong> select all columns with '*'.
 *
 * @param [in] stmt the prepared statement to adjust the modification time.
 * @param [in] col_count the number of columns in this record.
 * @param [in] cols the columns of this record.
 * @param [in] col_names the column names of this record.
 *
 * @return 0 if there is no error or non-zero if there is one.
 */
static int
adjust_mod_time (void *stmt, int col_count, char **cols, char **col_names)
{
  sqlite3_stmt *updt_stmt = stmt;
  sqlite3 *db = sqlite3_db_handle (updt_stmt);

  if (sqlite3_bind_text (stmt, 1, cols[COLPOS_MOD_TIME], -1, SQLITE_STATIC))
    {
      SQLITE3_ERR (db, "Cannot bind mod_time to updt stmt");
      return -1;
    }
  if (sqlite3_bind_text (stmt, 2, cols[COLPOS_POSITION], -1, SQLITE_STATIC))
    {
      SQLITE3_ERR (db, "Cannot bind position to updt stmt");
      return -1;
    }
  if (sqlite3_bind_text (stmt, 3, cols[COLPOS_CAT_ID], -1, SQLITE_STATIC))
    {
      SQLITE3_ERR (db, "Cannot bind cat_id to updt stmt");
      return -1;
    }
  if (sqlite3_bind_text (stmt, 4, cols[COLPOS_DESC], -1, SQLITE_STATIC))
    {
      SQLITE3_ERR (db, "Cannot bind desc to updt stmt");
      return -1;
    }
  if (sqlite3_bind_text (stmt, 5, cols[COLPOS_LONG_DESC], -1, SQLITE_STATIC))
    {
      SQLITE3_ERR (db, "Cannot bind long_desc to updt stmt");
      return -1;
    }
  if (sqlite3_bind_text (stmt, 6, cols[COLPOS_URI], -1, SQLITE_STATIC))
    {
      SQLITE3_ERR (db, "Cannot bind uri to updt stmt");
      return -1;
    }

  if (sqlite3_step (stmt) != SQLITE_DONE)
    {
      SQLITE3_ERR (db, "Cannot execute updt stmt");
      return -1;
    }

  if (sqlite3_reset (stmt))
    {
      SQLITE3_ERR (db, "Cannot reset updt stmt");
      return -1;
    }
  if (sqlite3_clear_bindings (stmt))
    {
      SQLITE3_ERR (db, "Cannot clear updt stmt bindings");
      return -1;
    }

  return 0;
}

int
save_service_list (const service_list *sl)
{
  struct gen_ssid_arg arg = {
    .ssid = "##",
    .ssid_len = 2,
    .last_pos = -1,
    .rc = ERR_SUCCESS,
  };
  char *err_msg;
  int rc;
  char old_ssid[SSID_MAX_LEN];
  ssize_t old_ssid_len;
  sqlite3_stmt *updt_stmt;

  if (!sl->has_service_list_tmp_table)
    {
      return ERR_SUCCESS;
    }

  /* Constructing the SSID */
  if ((rc = sqlite3_exec (sl->db,
			  "select "
			  COLUMN_CAT_ID ", " COLUMN_DESC ", " COLUMN_POSITION
			  " from " TABLE_SERVICE_LIST_TMP
			  " order by " COLUMN_POSITION " asc",
			  gen_ssid, &arg, &err_msg)) != SQLITE_ABORT
      && rc != SQLITE_OK)
    {
      SQLITE3_ERR_STR (err_msg, "Cannot select services to publish");
      return ERR_SAVE_SERVICE_LIST;
    }
  if (rc == SQLITE_ABORT)
    {
      l->APP_ERR (arg.rc, "Cannot publish services");
      return arg.rc;
    }

  /* Preparing for reverting to the old SSID */
  if ((old_ssid_len = get_ssid (old_ssid, sizeof (old_ssid))) == -1)
    {
      l->ERR ("Cannot retrieve old SSID");
      return ERR_SAVE_SERVICE_LIST;
    }

  /* Give modification time to all records in the temporary table */
  if (sqlite3_exec (sl->db,
		    "update " TABLE_SERVICE_LIST_TMP
		    " set " COLUMN_MOD_TIME " = strftime ('%s', 'now')",
		    NULL, NULL, &err_msg))
    {
      SQLITE3_ERR_STR (err_msg, "Cannot set service list new mod time");
      return ERR_SAVE_SERVICE_LIST;
    }
  
  /* If there old and new records are the same, don't change the mod time */
  if (sqlite3_prepare_v2 (sl->db,
			  "update " TABLE_SERVICE_LIST_TMP " set "
			  COLUMN_MOD_TIME " = ?"
			  " where " COLUMN_POSITION " = ?"
			  " and (" COLUMN_CAT_ID " != ?"
			  " or " COLUMN_DESC " != ?"
			  " or " COLUMN_LONG_DESC " != ?"
			  " or " COLUMN_URI " != ?)",
			  -1,
			  &updt_stmt,
			  NULL))
    {
      SQLITE3_ERR (sl->db, "Cannot prepare mod time adjustment update statement");
      return ERR_SAVE_SERVICE_LIST;
    }
  if ((rc = sqlite3_exec (sl->db, "select * from " TABLE_SERVICE_LIST_TMP,
			  adjust_mod_time, updt_stmt, &err_msg)))
    {
      SQLITE3_ERR_STR (err_msg, "Cannot adjust mod time");
    }
  if (sqlite3_finalize (updt_stmt))
    {
      SQLITE3_ERR_STR (err_msg, "Cannot finalize mod time adjustment stmt");
      return ERR_SAVE_SERVICE_LIST;
    }
  if (rc)
    {
      return ERR_SAVE_SERVICE_LIST;
    }

  /* The real saving process */
  if (sqlite3_exec (sl->db, "begin exclusive", NULL, NULL, &err_msg))
    {
      SQLITE3_ERR_STR (err_msg, "Cannot lock service list DB");
      return ERR_SAVE_SERVICE_LIST;
    }
  if ((rc = set_ssid (arg.ssid, arg.ssid_len)))
    {
      l->APP_ERR (rc, "Cannot set SSID");
      if (sqlite3_exec (sl->db, "rollback", NULL, NULL, &err_msg))
	{
	  SQLITE3_ERR_STR (err_msg,
			      "Cannot unlock (rollback) service list DB");
	}
      return rc;
    }
  if (sqlite3_exec (sl->db,
		    "delete from " TABLE_SERVICE_LIST ";"
		    "insert into " TABLE_SERVICE_LIST
		    " select *"
		    " from " TABLE_SERVICE_LIST_TMP ";"
		    "commit",
		    NULL, NULL, &err_msg))
    {
      SQLITE3_ERR_STR (err_msg, "Cannot save services");
      if ((rc = set_ssid (old_ssid, old_ssid_len)))
	{
	  l->APP_ERR (rc, "Cannot revert back to the old SSID");
	}
      if (sqlite3_exec (sl->db, "rollback", NULL, NULL, &err_msg))
	{
	  SQLITE3_ERR_STR (err_msg,
			      "Cannot unlock (rollback) service list DB");
	}
      return ERR_SAVE_SERVICE_LIST;
    }
  
  return ERR_SUCCESS;
}

/**
 * The callback function used with sqlite3_exec() to count the rows
 * of a table. The SQL select in sqlite3_exec() <strong>MUST</strong>
 * correct in returning only one row having only one column containing
 * the count to be extracted.
 *
 * @param [out] result where the counting result should be put.
 * @param [in] col_count the number of columns in this record.
 * @param [in] cols the columns of this record.
 * @param [in] col_names the column names of this record.
 *
 * @return 0 if there is no error or non-zero if there is one.
 */
static int
get_row_count (void *result, int col_count, char **cols, char **col_names)
{
  unsigned long *count = result;

  *count = strtoul (cols[0], NULL, 10);

  return 0;
}

size_t
count_service (const service_list *sl)
{
  char *err_msg;
  unsigned long result = 0;
  int rc;

  if (sl->has_service_list_tmp_table)
    {
      rc = sqlite3_exec (sl->db, "select count (*) from " TABLE_SERVICE_LIST_TMP,
			 get_row_count, &result, &err_msg);
    }
  else
    {
      rc = sqlite3_exec (sl->db, "select count (*) from " TABLE_SERVICE_LIST,
			 get_row_count, &result, &err_msg);
    }
   
  if (rc)
    {
      SQLITE3_ERR_STR (err_msg, "Cannot count services");
      return 0;
    }

  return result;
}

/**
 * Since writing to the service table must not succeed until
 * save_service_list() can check the sanity of the new set of published
 * services and a reader's table must not be changed while being iterated,
 * a temporary table unique to each sqlite3 DB connection is
 * created when a read or write operation is performed for the very first time.
 *
 * @param [in] sl the service list where the read or write operation is to be
 *                performed.
 *
 * @return 0 if there is no error or non-zero if there is an error.
 */
static int
ensure_tmp_table (service_list *sl)
{
  char *err_msg;

  if (sl->has_service_list_tmp_table)
    {
      return ERR_SUCCESS;
    }

  if (sqlite3_exec (sl->db,
		    "create temporary table if not exists "
		    TABLE_SERVICE_LIST_TMP " ("
		    COLUMN_POSITION " integer primary key not null,"
		    COLUMN_MOD_TIME " integer not null default 0,"
		    COLUMN_CAT_ID " integer not null,"
		    COLUMN_URI " text not null,"
		    COLUMN_DESC " text,"
		    COLUMN_LONG_DESC " text);"
		    "insert into " TABLE_SERVICE_LIST_TMP
		    " select * from " TABLE_SERVICE_LIST ";",
		    NULL, NULL, &err_msg))
    {
      SQLITE3_ERR_STR (err_msg, "Cannot create writable service list");
      return ERR_CREATE_TMP_SERVICE_LIST;
    }

  sl->has_service_list_tmp_table = 1;

  return ERR_SUCCESS;
}

/**
 * Since not all writing operations need to use service_list_impl::inc_dec_pos,
 * the statement is only prepared when there is really a need for one.
 *
 * @param [in] sl the service list where a write operation needing
 *                service_list_impl::inc_dec_pos is to be performed.
 *
 * @return 0 if there is no error or non-zero if there is an error.
 */
static int
ensure_inc_dec_pos (service_list *sl)
{
  int rc;
  sqlite3_stmt *stmt;

  if (sl->inc_dec_pos)
    {
      return ERR_SUCCESS;
    }

  if ((rc = ensure_tmp_table (sl)))
    {
      l->APP_ERR (rc, "Service list is not writable");
      return ERR_INIT_INC_DEC_POS;
    }

  if (sqlite3_prepare_v2 (sl->db,
			  "update " TABLE_SERVICE_LIST_TMP
			  " set " COLUMN_POSITION " = " COLUMN_POSITION " + ?"
			  " where " COLUMN_POSITION " = ?",
			  -1, &stmt, NULL))
    {
      SQLITE3_ERR (sl->db, "Cannot prepare inc_dec_pos statement");
      return ERR_INIT_INC_DEC_POS;
    }

  sl->inc_dec_pos = stmt;

  return ERR_SUCCESS;
}

/**
 * Increments the positions of contiguous services by one.
 *
 * @param [in] sl the service list to be modified.
 * @param [in] from the first position in the range.
 * @param [in] to the last position in the range.
 *
 * @return 0 if there is no error or non-zero if there is an error.
 */
static int
inc_positions (service_list *sl, int from, int to)
{
  int i;
  int rc;

  if ((rc = ensure_inc_dec_pos (sl)))
    {
      l->APP_ERR (rc, "inc_dec_pos is unavailable");
      return ERR_INC_POS;
    }

  if (sqlite3_reset (sl->inc_dec_pos))
    {
      SQLITE3_ERR (sl->db, "Cannot reset inc_dec_pos");
      return ERR_INC_POS;
    }
  if (sqlite3_clear_bindings (sl->inc_dec_pos))
    {
      SQLITE3_ERR (sl->db, "Cannot clear inc_dec_pos bindings");
      return ERR_INC_POS;
    }

  if (sqlite3_bind_int (sl->inc_dec_pos, 1, 1))
    {
      SQLITE3_ERR (sl->db, "Cannot bind +1 to inc_dec_pos");
      return ERR_INC_POS;
    }
  for (i = to; i >= from; i--)
    {
      if (sqlite3_bind_int (sl->inc_dec_pos, 2, i))
	{
	  SQLITE3_ERR (sl->db, "Cannot bind pos #%d to inc_dec_pos", i);
	  return ERR_INC_POS;
	}
      if (sqlite3_step (sl->inc_dec_pos) != SQLITE_DONE)
	{
	  SQLITE3_ERR (sl->db, "Cannot execute inc_dec_pos #%d", i);
	  return ERR_INC_POS;
	}
      if (sqlite3_reset (sl->inc_dec_pos))
	{
	  SQLITE3_ERR (sl->db, "Cannot reset inc_dec_pos #%d", i);
	  return ERR_INC_POS;
	}
    }

  return ERR_SUCCESS;
}

/**
 * Decrements the positions of contiguous services by one.
 *
 * @param [in] sl the service list to be modified.
 * @param [in] from the first position in the range.
 * @param [in] to the last position in the range.
 *
 * @return 0 if there is no error or non-zero if there is an error.
 */
static int
dec_positions (service_list *sl, int from, int to)
{
  int i;
  int rc;

  if ((rc = ensure_inc_dec_pos (sl)))
    {
      l->APP_ERR (rc, "inc_dec_pos is unavailable");
      return ERR_DEC_POS;
    }

  if (sqlite3_reset (sl->inc_dec_pos))
    {
      SQLITE3_ERR (sl->db, "Cannot reset inc_dec_pos");
      return ERR_DEC_POS;
    }
  if (sqlite3_clear_bindings (sl->inc_dec_pos))
    {
      SQLITE3_ERR (sl->db, "Cannot clear inc_dec_pos bindings");
      return ERR_DEC_POS;
    }

  if (sqlite3_bind_int (sl->inc_dec_pos, 1, -1))
    {
      SQLITE3_ERR (sl->db, "Cannot bind -1 to inc_dec_pos");
      return ERR_DEC_POS;
    }

  for (i = from; i <= to; i++)
    {
      if (sqlite3_bind_int (sl->inc_dec_pos, 2, i))
	{
	  SQLITE3_ERR (sl->db, "Cannot bind pos #%d to inc_dec_pos", i);
	  return ERR_DEC_POS;
	}
      if (sqlite3_step (sl->inc_dec_pos) != SQLITE_DONE)
	{
	  SQLITE3_ERR (sl->db, "Cannot execute inc_dec_pos #%d", i);
	  return ERR_DEC_POS;
	}
      if (sqlite3_reset (sl->inc_dec_pos))
	{
	  SQLITE3_ERR (sl->db, "Cannot reset inc_dec_pos #%d", i);
	  return ERR_DEC_POS;
	}
    }

  return ERR_SUCCESS;
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
  size_t last_pos = count_service (sl);

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
  struct service *ptr_s;
  const char *retrieved_desc;
  char *desc = NULL;
  const char *retrieved_long_desc;
  char *long_desc = NULL;
  const char *retrieved_uri;
  char *uri = NULL;

  int rc;

  /* Preparation */
  if ((rc = ensure_tmp_table (sl)))
    {
      l->APP_ERR (rc, "Service list is not readable");
      return ERR_GET_SERVICE;
    }

  if (sl->get_service_at == NULL)
    {
      if (sqlite3_prepare_v2 (sl->db,
			      "select * from " TABLE_SERVICE_LIST_TMP
			      " where " COLUMN_POSITION " = ?",
			      -1, &sl->get_service_at, NULL))
	{
	  SQLITE3_ERR (sl->db, "Cannot prepare statement"
			 " get service at");
	  return ERR_GET_SERVICE;
	}
    }
  else
    {
      if (sqlite3_clear_bindings (sl->get_service_at))
	{
	  SQLITE3_ERR (sl->db, "Cannot clear bindings of"
			 " get service at");
	  return ERR_GET_SERVICE;
	}
    }

  /* Bindings */
  if (sqlite3_bind_int (sl->get_service_at, 1, idx))
    {
      SQLITE3_ERR (sl->db, "Cannot bind position to"
		      " get service at");
      return ERR_GET_SERVICE;
    }

  /* Obtaining */
  rc = sqlite3_step (sl->get_service_at);
  if (rc != SQLITE_ROW && rc != SQLITE_DONE)
    {
      SQLITE3_ERR (sl->db, "Cannot execute get service at");
      if (sqlite3_reset (sl->get_service_at))
	{
	  SQLITE3_ERR (sl->db, "Cannot reset statement"
			 " get service at");
	}
      return ERR_GET_SERVICE;
    }
  
  if (rc == SQLITE_ROW)
    {
      retrieved_desc = (const char *) sqlite3_column_text (sl->get_service_at,
							   COLPOS_DESC);
      if (retrieved_desc != NULL)
	{
	  desc = malloc (strlen (retrieved_desc) + 1);
	  if (desc != NULL)
	    {
	      strcpy (desc, retrieved_desc);
	    }
	}
      retrieved_long_desc = ((const char *)
			     sqlite3_column_text (sl->get_service_at,
						  COLPOS_LONG_DESC));
      if (retrieved_long_desc != NULL)
	{
	  long_desc = malloc (strlen (retrieved_long_desc) + 1);
	  if (long_desc != NULL)
	    {
	      strcpy (long_desc, retrieved_long_desc);
	    }
	}
      retrieved_uri = (const char *) sqlite3_column_text (sl->get_service_at,
							  COLPOS_URI);
      if (retrieved_uri != NULL)
	{
	  uri = malloc (strlen (retrieved_uri) + 1);
	  if (uri != NULL)
	    {
	      strcpy (uri, retrieved_uri);
	    }
	}

      ptr_s = malloc (sizeof (*ptr_s));
      if (ptr_s == NULL || (retrieved_desc != NULL && desc == NULL)
	  || (retrieved_long_desc != NULL && long_desc == NULL)
	  || (retrieved_uri != NULL && uri == NULL))
	{
	  if (sqlite3_reset (sl->get_service_at))
	    {
	      SQLITE3_ERR (sl->db, "Cannot reset statement get service at");
	    }

	  return ERR_MEM;
	}

      ptr_s->ro.pos = idx;
      ptr_s->ro.mod_time = strtoull ((const char *)
				     sqlite3_column_text (sl->get_service_at,
							  COLPOS_MOD_TIME),
				     NULL, 10);
      ptr_s->cat_id = strtoul ((const char *)
			       sqlite3_column_text (sl->get_service_at,
						    COLPOS_CAT_ID),
			       NULL, 10);
      ptr_s->desc = desc;
      ptr_s->long_desc = long_desc;
      ptr_s->uri = uri;
    }
  else
    {
      ptr_s = NULL;
    }
  if (sqlite3_reset (sl->get_service_at))
    {
      SQLITE3_ERR (sl->db, "Cannot reset statement get service at");
      if (ptr_s != NULL)
	{
	  free (ptr_s->desc);
	  free (ptr_s->long_desc);
	  free (ptr_s->uri);
	  free (ptr_s);
	}
      return ERR_GET_SERVICE;
    }

  *s = ptr_s;

  return ERR_SUCCESS;
}

int
insert_service_at (service_list *sl, const struct service *s, unsigned int idx)
{
  char number_cat_id[32];
  char *err_msg;
  int rc;

  if (idx > count_service (sl))
    {
      return ERR_RANGE;
    }

  /* Preparation */
  if ((rc = ensure_tmp_table (sl)))
    {
      l->APP_ERR (rc, "Service list is not writable");
      return ERR_INSERT_SERVICE;
    }

  if (sl->insert_service_at == NULL)
    {
      if (sqlite3_prepare_v2 (sl->db,
			      "insert into " TABLE_SERVICE_LIST_TMP
			      " ("
			      COLUMN_POSITION ","
			      COLUMN_CAT_ID ","
			      COLUMN_URI ","
			      COLUMN_DESC ","
			      COLUMN_LONG_DESC
			      ") values ("
			      " ?, ?, ?, ?, ?)",
			      -1, &sl->insert_service_at, NULL))
	{
	  SQLITE3_ERR (sl->db, "Cannot prepare statement"
			 " insert service at");
	  return ERR_INSERT_SERVICE;
	}
    }
  else
    {
      if (sqlite3_reset (sl->insert_service_at))
	{
	  SQLITE3_ERR (sl->db, "Cannot reset statement"
			 " insert service at");
	  return ERR_INSERT_SERVICE;
	}
      if (sqlite3_clear_bindings (sl->insert_service_at))
	{
	  SQLITE3_ERR (sl->db, "Cannot clear bindings of"
			 " insert service at");
	  return ERR_INSERT_SERVICE;
	}
    }

  /* Bindings */
  if (sqlite3_bind_int (sl->insert_service_at, 1, idx))
    {
      SQLITE3_ERR (sl->db, "Cannot bind position to"
		      " insert service at");
      return ERR_INSERT_SERVICE;
    }
  snprintf (number_cat_id, sizeof (number_cat_id), "%lu", s->cat_id);
  if (sqlite3_bind_text (sl->insert_service_at, 2, number_cat_id, -1,
			 SQLITE_STATIC))
    {
      SQLITE3_ERR (sl->db, "Cannot bind cat_id to"
		      " insert service at");
      return ERR_INSERT_SERVICE;
    }
  if (sqlite3_bind_text (sl->insert_service_at, 3, s->uri, -1, SQLITE_STATIC))
    {
      SQLITE3_ERR (sl->db, "Cannot bind uri to"
		      " insert service at");
      return ERR_INSERT_SERVICE;
    }
  if (s->desc)
    {
      if (sqlite3_bind_text (sl->insert_service_at, 4, s->desc, -1,
			     SQLITE_STATIC))
	{
	  SQLITE3_ERR (sl->db, "Cannot bind desc to"
			  " insert service at");
	  return ERR_INSERT_SERVICE;
	}
    }
  else
    {
      if (sqlite3_bind_null (sl->insert_service_at, 4))
	{
	  SQLITE3_ERR (sl->db, "Cannot bind null desc to"
			  " insert service at");
	  return ERR_INSERT_SERVICE;
	}
    }
  if (s->long_desc)
    {
      if (sqlite3_bind_text (sl->insert_service_at, 5, s->long_desc, -1,
			     SQLITE_STATIC))
	{
	  SQLITE3_ERR (sl->db, "Cannot bind long desc to"
			  " insert service at");
	  return ERR_INSERT_SERVICE;
	}
    }
  else
    {
      if (sqlite3_bind_null (sl->insert_service_at, 5))
	{
	  SQLITE3_ERR (sl->db, "Cannot bind null long desc to"
			  " insert service at");
	  return ERR_INSERT_SERVICE;
	}
    }

  /* Inserting */
  if (sqlite3_exec (sl->db, "begin", NULL, NULL, &err_msg))
    {
      SQLITE3_ERR_STR (err_msg, "Cannot lock tmp service list");
      return ERR_INSERT_SERVICE;
    }
  if ((rc = inc_positions (sl, idx, count_service (sl) - 1)))
    {
      l->APP_ERR (rc, "Cannot increment positions");
      if (sqlite3_exec (sl->db, "rollback", NULL, NULL, &err_msg))
	{
	  SQLITE3_ERR_STR (err_msg,
			      "Cannot unlock (rollback) tmp service list");
	}
      return ERR_INSERT_SERVICE;
    }
  if (sqlite3_step (sl->insert_service_at) != SQLITE_DONE)
    {
      SQLITE3_ERR (sl->db, "Cannot execute insert service at");
      if (sqlite3_exec (sl->db, "rollback", NULL, NULL, &err_msg))
	{
	  SQLITE3_ERR_STR (err_msg,
			      "Cannot unlock (rollback) tmp service list");
	}
      return ERR_INSERT_SERVICE;
    }
  if (sqlite3_exec (sl->db, "commit", NULL, NULL, &err_msg))
    {
      SQLITE3_ERR_STR (err_msg, "Cannot unlock (commit) tmp service list");
      return ERR_INSERT_SERVICE;
    }

  return ERR_SUCCESS;
}

int
replace_service_at (service_list *sl, const struct service *s, unsigned int idx)
{
  char number_cat_id[32];
  char *err_msg;
  int rc;

  /* Preparation */
  if ((rc = ensure_tmp_table (sl)))
    {
      l->APP_ERR (rc, "Service list is not writable");
      return ERR_REPLACE_SERVICE;
    }

  if (sl->replace_service_at == NULL)
    {
      if (sqlite3_prepare_v2 (sl->db,
			      "update " TABLE_SERVICE_LIST_TMP " set "
			      COLUMN_CAT_ID " = ?,"
			      COLUMN_URI " = ?,"
			      COLUMN_DESC " = ?,"
			      COLUMN_LONG_DESC " = ?"
			      " where " COLUMN_POSITION " = ?",
			      -1, &sl->replace_service_at, NULL))
	{
	  SQLITE3_ERR (sl->db, "Cannot prepare statement"
			 " replace service at");
	  return ERR_REPLACE_SERVICE;
	}
    }
  else
    {
      if (sqlite3_reset (sl->replace_service_at))
	{
	  SQLITE3_ERR (sl->db, "Cannot reset statement"
			 " replace service at");
	  return ERR_REPLACE_SERVICE;
	}
      if (sqlite3_clear_bindings (sl->replace_service_at))
	{
	  SQLITE3_ERR (sl->db, "Cannot clear bindings of"
			 " replace service at");
	  return ERR_REPLACE_SERVICE;
	}
    }

  /* Bindings */
  if (sqlite3_bind_int (sl->replace_service_at, 5, idx))
    {
      SQLITE3_ERR (sl->db, "Cannot bind position to"
		      " replace service at");
      return ERR_REPLACE_SERVICE;
    }
  snprintf (number_cat_id, sizeof (number_cat_id), "%lu", s->cat_id);
  if (sqlite3_bind_text (sl->replace_service_at, 1, number_cat_id, -1,
			 SQLITE_STATIC))
    {
      SQLITE3_ERR (sl->db, "Cannot bind cat_id to"
		      " replace service at");
      return ERR_REPLACE_SERVICE;
    }
  if (sqlite3_bind_text (sl->replace_service_at, 2, s->uri, -1, SQLITE_STATIC))
    {
      SQLITE3_ERR (sl->db, "Cannot bind uri to"
		      " replace service at");
      return ERR_REPLACE_SERVICE;
    }
  if (s->desc)
    {
      if (sqlite3_bind_text (sl->replace_service_at, 3, s->desc, -1,
			     SQLITE_STATIC))
	{
	  SQLITE3_ERR (sl->db, "Cannot bind desc to"
			  " replace service at");
	  return ERR_REPLACE_SERVICE;
	}
    }
  else
    {
      if (sqlite3_bind_null (sl->replace_service_at, 3))
	{
	  SQLITE3_ERR (sl->db, "Cannot bind null desc to"
			  " replace service at");
	  return ERR_REPLACE_SERVICE;
	}
    }
  if (s->long_desc)
    {
      if (sqlite3_bind_text (sl->replace_service_at, 4, s->long_desc, -1,
			     SQLITE_STATIC))
	{
	  SQLITE3_ERR (sl->db, "Cannot bind long desc to"
			  " replace service at");
	  return ERR_REPLACE_SERVICE;
	}
    }
  else
    {
      if (sqlite3_bind_null (sl->replace_service_at, 4))
	{
	  SQLITE3_ERR (sl->db, "Cannot bind null long desc to"
			  " replace service at");
	  return ERR_REPLACE_SERVICE;
	}
    }

  /* Replacing */
  if (sqlite3_exec (sl->db, "begin", NULL, NULL, &err_msg))
    {
      SQLITE3_ERR_STR (err_msg, "Cannot lock tmp service list");
      return ERR_REPLACE_SERVICE;
    }
  if (sqlite3_step (sl->replace_service_at) != SQLITE_DONE)
    {
      SQLITE3_ERR (sl->db, "Cannot execute replace service at");
      if (sqlite3_exec (sl->db, "rollback", NULL, NULL, &err_msg))
	{
	  SQLITE3_ERR_STR (err_msg,
			      "Cannot unlock (rollback) tmp service list");
	}
      return ERR_REPLACE_SERVICE;
    }
  if (sqlite3_exec (sl->db, "commit", NULL, NULL, &err_msg))
    {
      SQLITE3_ERR_STR (err_msg, "Cannot unlock (commit) to tmp service list");
      return ERR_REPLACE_SERVICE;
    }

  return ERR_SUCCESS;  
}

int
del_service_at (service_list *sl, unsigned int idx)
{
  char *err_msg;
  int rc;

  /* Preparation */
  if ((rc = ensure_tmp_table (sl)))
    {
      l->APP_ERR (rc, "Service list is not writable");
      return ERR_DELETE_SERVICE;
    }

  if (sl->delete_service_at == NULL)
    {
      if (sqlite3_prepare_v2 (sl->db,
			      "delete from " TABLE_SERVICE_LIST_TMP
			      " where " COLUMN_POSITION " = ?",
			      -1, &sl->delete_service_at, NULL))
	{
	  SQLITE3_ERR (sl->db, "Cannot prepare statement"
			 " delete service at");
	  return ERR_DELETE_SERVICE;
	}
    }
  else
    {
      if (sqlite3_reset (sl->delete_service_at))
	{
	  SQLITE3_ERR (sl->db, "Cannot reset statement"
			 " delete service at");
	  return ERR_DELETE_SERVICE;
	}
      if (sqlite3_clear_bindings (sl->delete_service_at))
	{
	  SQLITE3_ERR (sl->db, "Cannot clear bindings of"
			 " delete service at");
	  return ERR_DELETE_SERVICE;
	}
    }

  /* Bindings */
  if (sqlite3_bind_int (sl->delete_service_at, 1, idx))
    {
      SQLITE3_ERR (sl->db, "Cannot bind position to"
		      " delete service at");
      return ERR_DELETE_SERVICE;
    }

  /* Deleting */
  if (sqlite3_exec (sl->db, "begin", NULL, NULL, &err_msg))
    {
      SQLITE3_ERR_STR (err_msg, "Cannot lock tmp service list");
      return ERR_DELETE_SERVICE;
    }
  if (sqlite3_step (sl->delete_service_at) != SQLITE_DONE)
    {
      SQLITE3_ERR (sl->db, "Cannot execute delete service at");
      if (sqlite3_exec (sl->db, "rollback", NULL, NULL, &err_msg))
	{
	  SQLITE3_ERR_STR (err_msg,
			      "Cannot unlock (rollback) tmp service list");
	}
      return ERR_DELETE_SERVICE;
    }
  if ((rc = dec_positions (sl, idx + 1, count_service (sl))))
    {
      l->APP_ERR (rc, "Cannot increment positions");
      if (sqlite3_exec (sl->db, "rollback", NULL, NULL, &err_msg))
	{
	  SQLITE3_ERR_STR (err_msg,
			      "Cannot unlock (rollback) tmp service list");
	}
      return ERR_DELETE_SERVICE;
    }
  if (sqlite3_exec (sl->db, "commit", NULL, NULL, &err_msg))
    {
      SQLITE3_ERR_STR (err_msg, "Cannot unlock (commit) tmp service list");
      return ERR_DELETE_SERVICE;
    }

  return ERR_SUCCESS;
}

int
del_service_all (service_list *sl)
{
  char *err_msg;
  int rc;

  if ((rc = ensure_tmp_table (sl)))
    {
      l->APP_ERR (rc, "Service list is not writable");
      return ERR_DELETE_ALL_SERVICE;
    }

  if (sqlite3_exec (sl->db, "delete from " TABLE_SERVICE_LIST_TMP,
		    NULL, NULL, &err_msg))
    {
      SQLITE3_ERR_STR (err_msg, "Cannot delete tmp service list records");
      return ERR_DELETE_ALL_SERVICE;
    }

  return ERR_SUCCESS;
}

/**
 * The callback function used with sqlite3_exec() to get the last
 * modification time of the service list table.
 * The SQL select in sqlite3_exec() <strong>MUST</strong> be correct in
 * selecting only the maximum mod_time of the
 * <strong>already published</strong> services.
 *
 * @param [out] result where the last modification time should be put.
 * @param [in] col_count the number of columns in this record.
 * @param [in] cols the columns of this record.
 * @param [in] col_names the column names of this record.
 *
 * @return 0 if there is no error or non-zero if there is one.
 */
static int
get_mod_time (void *result, int col_count, char **cols, char **col_names)
{
  uint64_t *mod_time = result;

  if (cols[0] == NULL)
    {
      *mod_time = 0ULL;
      return 0;
    }

  *mod_time = strtoull (cols[0], NULL, 10);

  return 0;
}

uint64_t
get_last_modification_time (service_list *sl)
{
  uint64_t result;
  char *err_msg;

  if (sqlite3_exec (sl->db,
		    "select max (" COLUMN_MOD_TIME ")"
		    " from " TABLE_SERVICE_LIST,
		    get_mod_time, &result, &err_msg))
    {
      SQLITE3_ERR_STR (err_msg, "Cannot get last mod time");
      return 0;
    }

  return result;
}
