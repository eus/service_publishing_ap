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
 * @file app_err.h
 * @brief The error module.
 ****************************************************************************/

#ifndef APP_ERR_H
#define APP_ERR_H

#ifdef __cpluplus
extern "C" {
#endif

/** The possible errors. */
enum err
  {
    ERR_SUCCESS, /**< There is no error. */
    ERR_SERVICE_SET_FULL, /**< The service set ads will not fit in the SSID. */
    ERR_SOCK, /**< Socket error. */
    ERR_MEM, /**< Insufficient memory. */
    ERR_SSID_TOO_LONG, /**< The SSID is too long. */
    ERR_SET_SSID, /**< Cannot set the SSID. */
    ERR_GET_SDE_INFO, /**< Error in getting the next SDE packet's info. */
    ERR_HANDLE_SDE_PACKET, /**< Error in handling an SDE packet. */
    ERR_TAKE_SDE_PACKET, /**< Error in taking an SDE packet. */
    ERR_REMOVE_SDE_PACKET, /**< Error in taking an SDE packet. */
    ERR_SEND_METADATA, /**< Error in sending SDE metadata response. */
    ERR_GET_METADATA_PACKETS, /**< Error in creating metadata packets. */
    ERR_EXTRACTING_METADATA, /**<
			      * Error in extracting metadata from a service
			      * list.
			      */
    ERR_GET_SERVICE_DESC_PACKETS, /**<
				   * Error in creating service description
				   * packets.
				   */
    ERR_GET_SERVICE_DESC, /**<
			   * Error in extracting a service description from a
			   * service list.
			   */
    ERR_LOAD_SERVICE_LIST, /**< Error in loading published service DB. */
    ERR_ADD_SERVICE_FIRST, /**< Error in adding a service at the front. */
    ERR_SAVE_SERVICE_LIST, /**< Error in saving a service list. */
    ERR_COUNT_SERVICES, /**< Error in counting services in a service list. */
    ERR_INVALID_SERVICE_POS, /**< Invalid service position in the SSID. */
    ERR_COUNT_ENABLED_SERVICES, /**<
				 * Error in counting enabled services in a
				 * service list.
				 */
    ERR_CREATE_SERVICE_URI_NULL, /**< URI is mandatory in creating a service. */
    ERR_CREATE_TMP_SERVICE_LIST, /**< Cannot create a tmp service list table. */
    ERR_INC_POS, /**< Cannot increment service positions. */
    ERR_DEC_POS, /**< Cannot decrement service positions. */
    ERR_ADD_SERVICE_LAST, /**< Error in adding a service at the back. */
    ERR_INSERT_SERVICE, /**< Error in inserting a service. */
    ERR_REPLACE_SERVICE, /**< Error in replacing a service. */
    ERR_DELETE_SERVICE, /**< Error in deleting a service. */
    ERR_DELETE_ALL_SERVICE, /**< Error in deleting all services. */
    ERR_RANGE, /**< Index is not within range. */
    ERR_INIT_INC_DEC_POS, /**< Cannot ensure inc_dec_pos. */
    ERR_GET_SERVICE, /**< Error in retrieving a service. */
    ERR_RELOAD_SERVICE_LIST, /**< Error in reloading service list. */
    ERR_LOAD_CATEGORY_LIST, /**< Error in loading category DB. */
    ERR_PARSE_DATA, /**< Error in parsing data. */
  };

/**
 * Translates an error code to a human-readable string.
 *
 * @param [in] err the error code to be translated.
 *
 * @return a read-only string explaining the error code.
 */
const char *
errtostr (int err);

#ifdef __cplusplus
}
#endif

#endif /* APP_ERR_H */
