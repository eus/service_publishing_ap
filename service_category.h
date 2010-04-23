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
 *************************************************************************//**
 * @file service_category.h
 * @brief The service category module. This module is thread-safe and
 *        fork-safe as long as each thread and each child process load their
 *        own service list (i.e., the service list object must not be passed
 *        from one thread to another or from a parent process to its child).
 *        And, it is good to reset the category list before navigating it to
 *        get the latest update to the underlying DB. In the future, an event
 *        listener may be available to make this more efficient. The geometry
 *        of the hierarchical structure of a category list is a tree whose
 *        root is only logical and not present in the list. The children of
 *        the logical root are the top-level categories. Upon obtaining a
 *        service category list using either load_flat_cat_list() or
 *        load_cat_list(), or after resetting a service category list with
 *        reset(), the internal iterator is before the first category if the
 *        list is flat or before the first top-level category if the list is
 *        hierarchical that means that get_cat(), prev(), go_sup() and
 *        go_sub() will indicate that there is no category unless next() has
 *        been executed to move the internal iterator to the first
 *        (top-level) category.
 ****************************************************************************/

#ifndef SERVICE_CATEGORY_H
#define SERVICE_CATEGORY_H

#ifndef CATEGORY_LIST_DB
#define CATEGORY_LIST_DB "./category_list.db"
#endif

#ifdef __cpluplus
extern "C" {
#endif

/** The category under the internal iterator of the category list. */
struct cat
{
  unsigned long id; /**< The category ID. */
  const char *name; /**< The category name. */
};

/** A list of categories. */
typedef struct cat_list_impl cat_list;

/**
 * Loads the currently available categories to be read without considering the
 * hierarchical structure. The list is detached from the underlying DB so that
 * any change to the underlying DB will not be seen by the list unless the list
 * is reset.
 *
 * @param [out] cl the pointer pointing to a dynamically allocated memory that
 *                 should be freed with destroy_cat_list().
 *
 * @return 0 if there is no error or non-zero if there is an error.
 */
int
load_flat_cat_list (cat_list **cl);

/**
 * Loads the currently available categories to be read in a hierarchical manner.
 * The list is detached from the underlying DB so that any change to the
 * underlying DB will not be seen by the list unless the list is reset.
 *
 * @param [out] cl the pointer pointing to a dynamically allocated memory that
 *                 should be freed with destroy_cat_list().
 *
 * @return 0 if there is no error or non-zero if there is an error.
 */
int
load_cat_list (cat_list **cl);

/**
 * Frees the memory allocated through load_cat_list() or load_flat_cat_list()
 * and sets the pointer to NULL as a safe guard. Passing a pointer to NULL is
 * okay but not a NULL pointer.
 *
 * @param [in] cl the category list to be freed.
 */
void
destroy_cat_list (cat_list **cl);

/**
 * Moves the internal iterator to the next category in the same subcategory.
 * Or, if the category list is flat, the internal iterator will move to the
 * next available category.
 *
 * @param [in] cl the category list whose internal iterator is to be updated.
 *
 * @return 0 if there is no next category or the list is empty, -1 if there is
 *         a next category, or a positive integer if there is an error.
 */
int
next (cat_list *cl);

/**
 * Moves the internal iterator to the previous category in the same subcategory.
 * Or, if the category list is flat, the internal iterator will move to the
 * previous available category.
 *
 * @param [in] cl the category list whose internal iterator is to be updated.
 *
 * @return 0 if there is no previous category or the internal iterator is not
 *         over any category, -1 if there is a previous category, or a positive
 *         integer if there is an error.
 */
int
prev (cat_list *cl);

/**
 * Moves the internal iterator to the subcategory of the category under the
 * internal iterator.
 *
 * @param [in] cl the category list whose internal iterator is to be updated.
 *
 * @return 0 if there is no subcategory or the internal iterator is not over any
 *         category or the list is flat, -1 if there is a subcategory, or a
 *         positive integer if there is an error.
 */
int
go_sub (cat_list *cl);

/**
 * Moves the internal iterator to the parent category of the category under
 * the internal iterator.
 *
 * @param [in] cl the category list whose internal iterator is to be updated.
 *
 * @return 0 if there is no parent category or the internal iterator is not over
 *         any category or the list is flat, -1 if there is a parent category, or
 *         a positive integer if there is an error.
 */
int
go_sup (cat_list *cl);

/**
 * Resets the internal iterator of the given category list as well as refreshing
 * the list data by re-reading the underlying DB.
 *
 * @param [in] cl the category list whose internal iterator will be reset.
 *
 * @return 0 if there is no error or non-zero if there is an error.
 */
int
reset (cat_list *cl);

/**
 * Reads the category under the internal iterator.
 *
 * @param [in] cl the category list to be read.
 *
 * @return the category under the internal iterator or NULL if the internal
 *         iterator is not over any category.
 */
const struct cat *
get_cat (const cat_list *cl);

/**
 * Updates the category list database. The update will not be visible to
 * a category list that is created before the update and has not been reset.
 *
 * @param [in] sql_statements the update data as provided by the central category
 *                            database.
 *
 * @return 0 if there is no error or non-zero if there is one.
 */
int
update_cat_list (const char *sql_statements);

#ifdef __cplusplus
}
#endif

#endif /* SERVICE_CATEGORY_H */
