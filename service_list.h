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
 *        advertised in the SSID.
 ****************************************************************************/

#ifndef SERVICE_LIST_H
#define SERVICE_LIST_H

#include <stdlib.h>

#ifdef __cpluplus
extern "C" {
#endif

/** The possible errors. */
enum err
  {
    SUCCESS, /**< There is no error. */
    SERVICE_SET_FULL, /**< The service set ads will not fit in the SSID. */
  };

/** A list of services advertised in the SSID. */
typedef struct service_list_impl service_list;

/** A service to be included in a service list. */
typedef struct service_impl service;

/**
 * Creates a service having the specified attributes. The created service should
 * be freed later with destroy_service().
 *
 * @param s [out] the resulting service object.
 * @param cat_id [in] the mandatory category ID of the service.
 * @param desc [in] the optional description of the service for the SSID ads.
 * @param long_desc [in] the optional long description of the service.
 * @param uri [in] the mandatory URI to obtain the service.
 *
 * @return 0 if there is no error or non-zero if there is an error.
 */
int
create_service (service *s,
		unsigned long cat_id,
		const char *desc,
		const char *long_desc,
		const char *uri);

/**
 * Frees the memory allocated through create_service() or get_service_at() and
 * sets the pointer to NULL as a safe guard. Passing a pointer to NULL is okay
 * but not a NULL pointer.
 *
 * @param s [in] the service to be freed.
 */
void
destroy_service (service **s);

/**
 * Translates an error code to a human-readable string.
 *
 * @param err [in] the error code to be translated.
 *
 * @return a read-only string explaining the error code.
 */
const char *
errtostr (int err);

/**
 * Loads the currently published service list.
 *
 * @param sl [out] the pointer pointing to dynamically allocated memory that
 *                 should be freed with destroy_service_list().
 */
int
load_service_list (service_list **sl);

/**
 * Frees the memory allocated through load_service_list() and sets the pointer
 * to NULL as a safe guard. Passing a pointer to NULL is okay but not a NULL
 * pointer.
 *
 * @param sl [in] the service list to be freed.
 */
void
destroy_service_list (service_list **sl);

/**
 * Saves the service list in the published service database and advertises the
 * service list in the SSID accordingly.
 *
 * @param sl [in] the service list to be saved.
 *
 * @return 0 if there is no error or non-zero if there is an error.
 */
int
save_service_list (const service_list *sl);

/**
 * Count the number of elements in a service list.
 *
 * @param sl [in] the service list whose elements are to be counted.
 *
 * @return the number of services contained in the service list.
 */
size_t
count_service (const service_list *sl);

/** 
 * Adds a new service as the first member of the service list.
 * 
 * @param sl [in] the service list that will contain the new service.
 * @param s [in] the service to be added.
 * 
 * @return 0 if there is no error or non-zero if there is an error.
 */
int
add_service_first (service_list *sl, const service *s);

/** 
 * Adds a new service as the last member of the service list.
 * 
 * @param sl [in] the service list that will contain the new service.
 * @param s [in] the service to be added.
 * 
 * @return 0 if there is no error or non-zero if there is an error.
 */
int
add_service_last (service_list *sl, const service *s);

/** 
 * Gets a copy of the service at the specified index in the service list.
 * The copy should later be freed with destroy_service().
 * 
 * @param sl [in] the service list that has the service.
 * @param s [out] a pointer to the dynamically allocated copy of the service.
 * @param idx [in] the 0-based position index.
 * 
 * @return 0 if there is no error or non-zero if there is an error.
 */
int
get_service_at (const service_list *sl, service *s, unsigned int idx);

/** 
 * Inserts a new service at the specified index in the service list.
 * 
 * @param sl [in] the service list that will contain the new service.
 * @param s [in] the service to be added.
 * @param idx [in] the 0-based position index.
 * 
 * @return 0 if there is no error or non-zero if there is an error.
 */
int
insert_service_at (service_list *sl, const service *s, unsigned int idx);

/** 
 * Replaces a service at the specified index in the service list.
 * 
 * @param sl [in] the service list that contains the service.
 * @param s [in] the new service that will replace the old one.
 * @param idx [in] the 0-based position index.
 * 
 * @return 0 if there is no error or non-zero if there is an error.
 */
int
replace_service_at (service_list *sl, const service *s, unsigned int idx);

/** 
 * Deletes a service at the specified index in the service list.
 * 
 * @param sl [in] the service list that contains the service.
 * @param idx [in] the 0-based position index.
 * 
 * @return 0 if there is no error or non-zero if there is an error.
 */
int
del_service_at (service_list *sl, unsigned int idx);

/** 
 * Deletes all services.
 * 
 * @param sl [in] the service list that to be emptied.
 * 
 * @return 0 if there is no error or non-zero if there is an error.
 */
int
del_service_all (service_list *sl);

/** 
 * Disables a service at the specified index in the service list.
 * 
 * @param sl [in] the service list that contains the service.
 * @param idx [in] the 0-based position index.
 * 
 * @return 0 if there is no error or non-zero if there is an error.
 */
int
disable_service_at (service_list *sl, unsigned int idx);

/** 
 * Disables all services.
 * 
 * @param sl [in] the service list that to be disabled.
 * 
 * @return 0 if there is no error or non-zero if there is an error.
 */
int
disable_service_all (service_list *sl);

#ifdef __cplusplus
}
#endif

#endif /* SERVICE_LIST_H */
