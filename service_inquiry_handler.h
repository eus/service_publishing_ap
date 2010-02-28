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
 * @file service_inquiry_handler.h
 * @brief The service inquiry handler module.
 ****************************************************************************/

#ifndef SERVICE_INQUIRY_HANDLER_H
#define SERVICE_INQUIRY_HANDLER_H

#ifdef __cpluplus
extern "C" {
#endif

/**
 * Runs the SDE handler. This is a blocking operation.
 *
 * @param [in] is_stopped a callback function called by the handler to decide
 *                        whether or not to handler should return.
 *
 * @return 0 if there is no error or non-zero if there is an error.
 */
int
run_inquiry_handler (int (*is_stopped) (void));

#ifdef __cplusplus
}
#endif

#endif /* SERVICE_INQUIRY_HANDLER_H */
