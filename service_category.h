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
 * @brief The service category module. First, the complete category list is
 *        loaded from the DB. Then, one or more iterators are created for the
 *        category list to navigate the list. The category list DB can be
 *        updated.
 ****************************************************************************/

#ifndef SERVICE_CATEGORY_H
#define SERVICE_CATEGORY_H

#ifdef __cpluplus
extern "C" {
#endif

/** A category. */
struct cat
{
  unsigned long id; /**< The category ID. */
  char *name; /**< The category name. */
};

/** A list of categories. */
typedef struct cat_list_impl cat_list;

/** An iterator to traverse a category list. */
typedef struct cat_list_itr_impl cat_list_itr;

/**
 * Loads the currently available categories.
 *
 * @param [out] cl the pointer pointing to a dynamically allocated memory that
 *                 should be freed with destroy_cat_list().
 *
 * @return 0 if there is no error or non-zero if there is an error.
 */
int
load_cat_list (cat_list **cl);

/**
 * Frees the memory allocated through load_cat_list() and sets the pointer
 * to NULL as a safe guard. Passing a pointer to NULL is okay but not a NULL
 * pointer.
 *
 * @param [in] cl the category list to be freed.
 */
void
destroy_cat_list (cat_list **cl);

/**
 * Creates an iterator to traverse the given category list.
 *
 * @param [out] itr the pointer pointing to a dynamically allocated memory that
 *                  should be freed with destroy_cat_list_itr().
 * @param [in] cl the category list to be traversed.
 *
 * @return 0 if there is no error or non-zero if there is an error.
 */
int
create_cat_list_itr (cat_list_itr **itr, cat_list *cl);

/**
 * Frees the memory allocated through create_cat_list_itr() and sets the pointer
 * to NULL as a safe guard. Passing a pointer to NULL is okay but not a NULL
 * pointer.
 *
 * @param [in] itr the iterator to be freed.
 */
void
destroy_cat_list_itr (cat_list_itr **itr);

/**
 * Moves the iterator to the next category in the same subcategory.
 *
 * @param [in] itr the category list iterator.
 *
 * @return 0 if there is no next category or non-zero if there is one.
 */
int
next (cat_list_itr *itr);

/**
 * Moves the iterator to the previous category in the same subcategory.
 *
 * @param [in] itr the category list iterator.
 *
 * @return 0 if there is no previous category or non-zero if there is one.
 */
int
prev (cat_list_itr *itr);

/**
 * Moves the iterator to the subcategory of the category under the iterator.
 *
 * @param [in] itr the category list iterator.
 *
 * @return 0 if there is no subcategory or non-zero if there is one.
 */
int
go_sub (cat_list_itr *itr);

/**
 * Moves the iterator to the parent category of the category under the iterator.
 *
 * @param [in] itr the category list iterator.
 *
 * @return 0 if there is no parent category or non-zero if there is one.
 */
int
go_sup (cat_list_itr *itr);

/**
 * Reads the category under the iterator.
 *
 * @param [out] c the pointer pointing to a dynamically allocated memory that
 *                should be freed with free_cat().
 * @param [in] itr the category list iterator.
 *
 * @return 0 if there is no error or non-zero if there is one.
 */
int
get_cat (struct cat **c, const cat_list_itr *itr);

/**
 * Frees the memory allocated through get_cat() and sets the pointer
 * to NULL as a safe guard. Passing a pointer to NULL is okay but not a NULL
 * pointer.
 *
 * @param [in] c the category to be freed.
 */
void
free_cat (struct cat **c);

/**
 * Updates the category list database.
 *
 * @param [in] data the update data as provided by the central category database.
 *
 * @return 0 if there is no error or non-zero if there is one.
 */
int
update_cat_list (const void *data);

#ifdef __cplusplus
}
#endif

#endif /* SERVICE_CATEGORY_H */
