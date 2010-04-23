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
#include <string.h>
#include "app_err.h"
#include "logger.h"
#include "service_category.h"

/* Category list underlying DB structure as specified in
 * "./doc/Central Database Category.txt"
 */
#define TABLE_CATEGORY_LIST "category_list"
#define COLUMN_CAT_ID "cat_id"
#define COLUMN_CAT_NAME "cat_name"
#define TABLE_CATEGORY_STRUCTURE "category_structure"
#define COLUMN_CAT_ID "cat_id"
#define COLUMN_SUBCAT_ID "subcat_id"

GLOBAL_LOGGER;

int
main (int argc, char **argv, char **envp)
{
  int i;
  cat_list *flat_cl;
  cat_list *cl; /* hierarchical list */
  const struct cat *cur_cat;

  SETUP_LOGGER ("/dev/stderr", errtostr);

  /* test for emptiness */
  update_cat_list ("delete from " TABLE_CATEGORY_LIST ";"
		   "delete from " TABLE_CATEGORY_STRUCTURE ";");
  assert (load_flat_cat_list (&flat_cl) == 0);
  assert (!next (flat_cl));
  assert (!prev (flat_cl));
  assert (!go_sub (flat_cl));
  assert (!go_sup (flat_cl));
  assert (get_cat (flat_cl) == NULL);
  assert (reset (flat_cl) == 0);

  assert (load_cat_list (&cl) == 0);
  assert (!next (cl));
  assert (!prev (cl));
  assert (!go_sub (cl));
  assert (!go_sup (cl));
  assert (get_cat (cl) == NULL);
  assert (reset (cl) == 0);

  /* test for update not affecting existing list */
  update_cat_list ("insert into " TABLE_CATEGORY_LIST
		   " (" COLUMN_CAT_ID ", " COLUMN_CAT_NAME ")"
		   " values (0, 'NULL');"
		   "insert into " TABLE_CATEGORY_LIST
		   " (" COLUMN_CAT_ID ", " COLUMN_CAT_NAME ")"
		   " values (1, 'Restaurants');"
		   "insert into " TABLE_CATEGORY_LIST
		   " (" COLUMN_CAT_ID ", " COLUMN_CAT_NAME ")"
		   " values (2, 'Schools');"
		   "insert into " TABLE_CATEGORY_LIST
		   " (" COLUMN_CAT_ID ", " COLUMN_CAT_NAME ")"
		   " values (3, 'Games');"
		   "insert into " TABLE_CATEGORY_LIST
		   " (" COLUMN_CAT_ID ", " COLUMN_CAT_NAME ")"
		   " values (4, 'Kindergarden');"
		   "insert into " TABLE_CATEGORY_LIST
		   " (" COLUMN_CAT_ID ", " COLUMN_CAT_NAME ")"
		   " values (5, 'Elementary School');"
		   "insert into " TABLE_CATEGORY_LIST
		   " (" COLUMN_CAT_ID ", " COLUMN_CAT_NAME ")"
		   " values (6, 'Private School');"
		   "insert into " TABLE_CATEGORY_LIST
		   " (" COLUMN_CAT_ID ", " COLUMN_CAT_NAME ")"
		   " values (7, 'Public School');"
		   "insert into " TABLE_CATEGORY_LIST
		   " (" COLUMN_CAT_ID ", " COLUMN_CAT_NAME ")"
		   " values (8, 'Logic Games');"
		   "insert into " TABLE_CATEGORY_LIST
		   " (" COLUMN_CAT_ID ", " COLUMN_CAT_NAME ")"
		   " values (9, 'Card Games');"

		   "insert into " TABLE_CATEGORY_STRUCTURE
		   " (" COLUMN_CAT_ID ", " COLUMN_SUBCAT_ID ")"
		   " values (2, 6);"
		   "insert into " TABLE_CATEGORY_STRUCTURE
		   " (" COLUMN_CAT_ID ", " COLUMN_SUBCAT_ID ")"
		   " values (2, 7);"
		   "insert into " TABLE_CATEGORY_STRUCTURE
		   " (" COLUMN_CAT_ID ", " COLUMN_SUBCAT_ID ")"
		   " values (6, 4);"
		   "insert into " TABLE_CATEGORY_STRUCTURE
		   " (" COLUMN_CAT_ID ", " COLUMN_SUBCAT_ID ")"
		   " values (6, 5);"
		   "insert into " TABLE_CATEGORY_STRUCTURE
		   " (" COLUMN_CAT_ID ", " COLUMN_SUBCAT_ID ")"
		   " values (7, 4);"
		   "insert into " TABLE_CATEGORY_STRUCTURE
		   " (" COLUMN_CAT_ID ", " COLUMN_SUBCAT_ID ")"
		   " values (7, 5);"
		   "insert into " TABLE_CATEGORY_STRUCTURE
		   " (" COLUMN_CAT_ID ", " COLUMN_SUBCAT_ID ")"
		   " values (3, 8);"
		   "insert into " TABLE_CATEGORY_STRUCTURE
		   " (" COLUMN_CAT_ID ", " COLUMN_SUBCAT_ID ")"
		   " values (3, 9);");

  /* not affected list */
  assert (!next (cl));
  assert (!prev (cl));
  assert (!go_sub (cl));
  assert (!go_sup (cl));
  assert (get_cat (cl) == NULL);

  /* affected list */
  assert (reset (flat_cl) == 0);
  assert (!prev (flat_cl));
  assert (!go_sub (flat_cl));
  assert (!go_sup (flat_cl));
  assert (get_cat (flat_cl) == NULL);
  char *expected_categories[] = {
    "NULL", "Restaurants", "Schools", "Games", "Kindergarden",
    "Elementary School", "Private School", "Public School", "Logic Games",
    "Card Games",
  };
  for (i = 0; i < 10; i++)
    {
      assert (next (flat_cl) == -1);
      assert (!go_sub (flat_cl));
      assert (!go_sup (flat_cl));
      cur_cat = get_cat (flat_cl);
      assert (cur_cat != NULL);
      assert (cur_cat->id == i);
      assert (strcmp (cur_cat->name, expected_categories[i]) == 0);
    }

  /* cur_cat stays at the last reading when next cannot advance to the next cat */
  assert (!next (flat_cl));
  --i;
  assert (cur_cat->id == i);
  assert (strcmp (cur_cat->name, expected_categories[i]) == 0);

  for (--i; i >= 0; i--)
    {
      assert (prev (flat_cl) == -1);
      assert (!go_sub (flat_cl));
      assert (!go_sup (flat_cl));
      cur_cat = get_cat (flat_cl);
      assert (cur_cat != NULL);
      assert (cur_cat->id == i);
      assert (strcmp (cur_cat->name, expected_categories[i]) == 0);
    }

  /* cur_cat stays at the last reading when prev cannot advance to the prev cat */
  assert (!prev (flat_cl));
  ++i;
  assert (cur_cat->id == i);
  assert (strcmp (cur_cat->name, expected_categories[i]) == 0);

  /* Now try prev in the middle of the list */
  assert (reset (flat_cl) == 0);
  assert (next (flat_cl) == -1);
  assert (next (flat_cl) == -1);
  assert (next (flat_cl) == -1);
  assert (next (flat_cl) == -1);
  assert (next (flat_cl) == -1);
  assert (next (flat_cl) == -1);
  assert (prev (flat_cl) == -1);
  assert (prev (flat_cl) == -1);
  i = 3;
  assert (cur_cat->id == i);
  assert (strcmp (cur_cat->name, expected_categories[i]) == 0);

  /* Try the hierarchy iteration out */
  assert (reset (cl) == 0);
  assert (!prev (cl));
  assert (!go_sub (cl));
  assert (!go_sup (cl));
  assert (get_cat (cl) == NULL);

  for (i = 0; i < 4; i++)
    {
      assert (next (cl) == -1);
      cur_cat = get_cat (cl);
      assert (cur_cat->id == i);
      assert (strcmp (cur_cat->name, expected_categories[i]) == 0);
    }

  assert (next (cl) == 0);
  cur_cat = get_cat (cl);
  i = 3;
  assert (cur_cat->id == i);
  assert (strcmp (cur_cat->name, expected_categories[i]) == 0);

  /* Go back to schools */
  assert (prev (cl) == -1);
  cur_cat = get_cat (cl);
  i = 2;
  assert (cur_cat->id == i);
  assert (strcmp (cur_cat->name, expected_categories[i]) == 0);

  /* go down to private schools */
  assert (!go_sup (cl));
  assert (go_sub (cl) == -1);
  cur_cat = get_cat (cl);
  i = 6;
  assert (cur_cat->id == i);
  assert (strcmp (cur_cat->name, expected_categories[i]) == 0);

  /* go next to public schools */
  assert (!prev (cl));
  assert (next (cl) == -1);
  cur_cat = get_cat (cl);
  i = 7;
  assert (cur_cat->id == i);
  assert (strcmp (cur_cat->name, expected_categories[i]) == 0);

  /* go down to kindergarden */
  assert (go_sub (cl) == -1);
  cur_cat = get_cat (cl);
  i = 4;
  assert (cur_cat->id == i);
  assert (strcmp (cur_cat->name, expected_categories[i]) == 0);

  /* go up to public schools */
  assert (!prev (cl));
  assert (!go_sub (cl));
  assert (go_sup (cl) == -1);
  cur_cat = get_cat (cl);
  i = 7;
  assert (cur_cat->id == i);
  assert (strcmp (cur_cat->name, expected_categories[i]) == 0);

  /* go back to private schools */
  assert (!next (cl));
  assert (prev (cl) == -1);
  cur_cat = get_cat (cl);
  i = 6;
  assert (cur_cat->id == i);
  assert (strcmp (cur_cat->name, expected_categories[i]) == 0);

  /* go down to kindergarden */
  assert (!prev (cl));
  assert (go_sub (cl) == -1);
  cur_cat = get_cat (cl);
  i = 4;
  assert (cur_cat->id == i);
  assert (strcmp (cur_cat->name, expected_categories[i]) == 0);

  /* go next to elementary school */
  assert (!prev (cl));
  assert (!go_sub (cl));
  assert (next (cl) == -1);
  assert (!next (cl));
  assert (!go_sub (cl));
  cur_cat = get_cat (cl);
  i = 5;
  assert (cur_cat->id == i);
  assert (strcmp (cur_cat->name, expected_categories[i]) == 0);

  /* go up to private shools */
  assert (go_sup (cl) == -1);
  assert (!prev (cl));
  cur_cat = get_cat (cl);
  i = 6;
  assert (cur_cat->id == i);
  assert (strcmp (cur_cat->name, expected_categories[i]) == 0);

  /* go up to shools */
  assert (go_sup (cl) == -1);
  cur_cat = get_cat (cl);
  i = 2;
  assert (cur_cat->id == i);
  assert (strcmp (cur_cat->name, expected_categories[i]) == 0);

  /* go next to games */
  assert (!go_sup (cl));
  assert (next (cl) == -1);
  assert (!next (cl));
  cur_cat = get_cat (cl);
  i = 3;
  assert (cur_cat->id == i);
  assert (strcmp (cur_cat->name, expected_categories[i]) == 0);

  destroy_cat_list (&flat_cl);
  destroy_cat_list (&cl);

  exit (EXIT_SUCCESS);
}
