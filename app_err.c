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

#include "app_err.h"

const char *
errtostr (int err)
{
  static const char *errstr[] = {
    "Success",
    "SSID cannot accommodate the service set",
    "Socket error",
    "Not enough memory",
    "SSID too long",
    "Set SSID error",
    "Error in getting the next SDE packet info",
    "Error in handling an SDE packet",
    "Error in taking an SDE packet",
    "Error in removing an SDE packet",
    "Error in sending SDE metadata response",
    "Error in creating metadata packets",
    "Error in extracting metadata from service list",
    "Error in creating service description packets",
    "Error in extracting service description from service list",
  };

  return errstr[err];
}