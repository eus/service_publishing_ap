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
