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
 * @file ssid.h
 * @brief The SSID advertisement module.
 ****************************************************************************/

#ifndef SSID_H
#define SSID_H

#include <stdlib.h>

#ifdef __cpluplus
extern "C" {
#endif

/**
 * Sets the SSID.
 * 
 * @param [in] new_ssid the new SSID to set.
 * @param [in] len the length of the new SSID.
 *
 * @return 0 if it is successful, ERR_SSID_TOO_LONG if the new SSID is too long,
 *         or non-zero for other errors.
 */
int
set_ssid (const void *new_ssid, size_t len);

/**
 * Gets the SSID.
 *
 * @param [out] buffer the buffer to hold the returned SSID.
 * @param [in] len the length of the buffer.
 *
 * @return the length of the SSID contained in the buffer or -1 if there is an
 *         error.
 */
ssize_t
get_ssid (void *buffer, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* SSID_H */
