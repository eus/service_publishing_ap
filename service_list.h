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
 * @file service_list.h
 * @brief The service list module. Everything starts by getting the
 *        currently available service list. The service list can then be
 *        manipulated before being saved in the DB and automatically
 *        advertised in the SSID. This module is thread-safe and fork-safe as
 *        long as each thread and each child process load their own service
 *        list (i.e., the service list object must not be passed from one
 *        thread to another or from a parent process to its child).
 ****************************************************************************/

#ifndef SERVICE_LIST_H
#define SERVICE_LIST_H

#include <stdlib.h>
#include <stdint.h>

/** The service list Sqlite3 DB file. */
#define SERVICE_LIST_DB "./service_list.dat"

#ifdef __cpluplus
extern "C" {
#endif

/** A list of services advertised in the SSID. */
typedef struct service_list_impl service_list;

/**
 * Data that are only valid after performing get_service_at() and can be
 * outdated by a subsequent call to another function performing a write
 * operation like replace_service_at().
 */
struct service_read_only_data
{
  unsigned long pos; /**< The service position in the SSID starting from 0. */
  uint64_t mod_time; /**<
		      * The last modification time of an already saved service
		      * (i.e., this will always be 0 for a new unsaved service).
		      */
};

/** A service to be included in a service list. */
struct service
{
  struct service_read_only_data ro; /**< The read-only data of the service. */
  unsigned long cat_id; /**< The service category ID. */
  char *desc; /**< The optional (can be NULL) service short description. */
  char *long_desc; /**< The optional (can be NULL) service long description. */
  char *uri; /**< The service URI. */
};

/**
 * Creates a service having the specified attributes. The created service should
 * be freed later with destroy_service(). <strong>[CAUTION]</strong> No string
 * copy operation will be performed on desc, long_desc and uri (i.e., each of
 * the corresponding field in the resulting struct will point to the given
 * string) and the fields will later be <strong>freed</strong> in
 * destroy_service() (i.e., the caller must perform the needed copy operation
 * if the strings pointed by the fields will still be used outside the data
 * structure).
 *
 * @param [out] s the resulting service object.
 * @param [in] cat_id the mandatory category ID of the service.
 * @param [in] desc the optional description of the service for the SSID ads.
 *                  NULL will omit this.
 * @param [in] long_desc the optional long description of the service.
 *                       NULL will omit this.
 * @param [in] uri the mandatory URI to obtain the service.
 *
 * @return 0 if there is no error or non-zero if there is an error.
 */
int
create_service (struct service **s,
		unsigned long cat_id,
		char *desc,
		char *long_desc,
		char *uri);

/**
 * Frees the memory allocated through create_service() or get_service_at() and
 * sets the pointer to NULL as a safe guard. Passing a pointer to NULL is okay
 * but not a NULL pointer.
 *
 * @param [in] s the service to be freed.
 */
void
destroy_service (struct service **s);

/**
 * Loads the currently published service list.
 *
 * @param [out] sl the pointer pointing to dynamically allocated memory that
 *                 should be freed with destroy_service_list().
 *
 * @return 0 if there is no error or non-zero if there is an error.
 */
int
load_service_list (service_list **sl);

/**
 * Frees the memory allocated through load_service_list() and sets the pointer
 * to NULL as a safe guard. Passing a pointer to NULL is okay but not a NULL
 * pointer.
 *
 * @param [in] sl the service list to be freed.
 */
void
destroy_service_list (service_list **sl);

/**
 * Saves the service list in the published service database and advertises the
 * service list in the SSID accordingly.
 *
 * @param [in] sl the service list to be saved.
 *
 * @return 0 if there is no error or non-zero if there is an error.
 */
int
save_service_list (const service_list *sl);

/**
 * Count the number of elements in a service list.
 *
 * @param [in] sl the service list whose elements are to be counted.
 *
 * @return the number of services contained in the service list. If an error
 *         occured, 0 is returned.
 */
size_t
count_service (const service_list *sl);

/** 
 * Adds a new service as the first member of the service list to be published.
 * 
 * @param [in] sl the service list that will contain the new service.
 * @param [in] s the service to be added.
 * 
 * @return 0 if there is no error or non-zero if there is an error.
 */
int
add_service_first (service_list *sl, const struct service *s);

/** 
 * Adds a new service as the last member of the service list to be published.
 * 
 * @param [in] sl the service list that will contain the new service.
 * @param [in] s the service to be added.
 * 
 * @return 0 if there is no error or non-zero if there is an error.
 */
int
add_service_last (service_list *sl, const struct service *s);

/** 
 * Gets a copy of the service at the specified index in the service list.
 * The copy should later be freed with destroy_service().
 * 
 * @param [in] sl the service list that has the service.
 * @param [out] s a pointer to the dynamically allocated copy of the service or
 *                a pointer to NULL if no service found at the specified index.
 * @param [in] idx the 0-based position index.
 * 
 * @return 0 if there is no error or non-zero if there is an error.
 */
int
get_service_at (service_list *sl, struct service **s, unsigned int idx);

/** 
 * Inserts a new service at the specified index in the service list.
 * It is an error to insert a service outside the range [0, count_service() - 1].
 * 
 * @param [in] sl the service list that will contain the new service.
 * @param [in] s the service to be added.
 * @param [in] idx the 0-based position index.
 * 
 * @return 0 if there is no error or non-zero if there is an error.
 */
int
insert_service_at (service_list *sl, const struct service *s, unsigned int idx);

/** 
 * Replaces a service at the specified index in the service list.
 * 
 * @param [in] sl the service list that contains the service.
 * @param [in] s the new service that will replace the old one.
 * @param [in] idx the 0-based position index.
 * 
 * @return 0 if there is no error or non-zero if there is an error.
 */
int
replace_service_at (service_list *sl, const struct service *s, unsigned int idx);

/** 
 * Deletes a service at the specified index in the service list.
 * 
 * @param [in] sl the service list that contains the service.
 * @param [in] idx the 0-based position index.
 * 
 * @return 0 if there is no error or non-zero if there is an error.
 */
int
del_service_at (service_list *sl, unsigned int idx);

/** 
 * Deletes all services.
 * 
 * @param [in] sl the service list that to be emptied.
 * 
 * @return 0 if there is no error or non-zero if there is an error.
 */
int
del_service_all (service_list *sl);

/**
 * Returns the time since Unix epoch the already published service list was
 * last modified (this is <strong>not</strong> the last modification time
 * of the service list that is modified but not yet saved).
 *
 * @param [in] sl the service list through which the already published
 *                services are to be checked.
 *
 * @return the already published service list's last modification time.
 *         When an error occurs, 0 is returned.
 */
uint64_t
get_last_modification_time (service_list *sl);

#ifdef __cplusplus
}
#endif

#endif /* SERVICE_LIST_H */
