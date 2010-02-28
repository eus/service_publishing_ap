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
    "Error in loading published service DB",
    "Error in adding a service at the front",
    "Error in saving a service list",
    "Error in counting services in a service list",
    "Invalid service position in the SSID",
    "Error in counting enabled services in a service list",
    "URI is mandatory in creating a service",
    "Error in creating a tmp service list table",
    "Cannot increment service positions",
    "Cannot decrement service positions",
    "Error in adding a service at the back",
    "Error in inserting a service",
    "Error in replacing a service",
    "Error in deleting a service",
    "Error in deleting all services",
    "Index is outside range",
    "Cannot ensure inc_dec_pos",
    "Error in retrieving a service",
  };

  return errstr[err];
}
