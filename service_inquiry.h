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
 * @file service_inquiry.h
 * @brief The core logic of service inquiry handler module.
 *        <strong>[CAUTION]</strong> When an application
 *        uses the APIs in this file, upon exit the application should call
 *        destroy_sde_handler_cache() as a part of its memory clean up.
 ****************************************************************************/

#ifndef SERVICE_INQUIRY_H
#define SERVICE_INQUIRY_H

#include <netinet/in.h>
#include "sde.h"

#ifdef __cpluplus
extern "C" {
#endif

/**
 * Destroyes the already cached data. The cache data are used to speed up SDE
 * sessions. Calling this function repeatedly is safe although the performance
 * of the SDE handler will degrade significantly.
 */
void
destroy_sde_handler_cache (void);

/**
 * Creates the response packets for an sde_get_metadata.
 * 
 * @param [in] seq the sequence number of the sde_get_metadata packet.
 * @param [out] p1 a pointer to a dynamically allocated memory space containing
 *                the response packet to be sent first.
 * @param [out] p1_size the size of p1 in bytes.
 * @param [out] p2 a pointer to a dynamically allocated memory space containing
 *                the response packet to be sent after p1.
 * @param [out] p2_size the size of p2 in bytes.
 *
 * @return 0 if there is no error or non-zero if there is an error.
 */
int
get_metadata_response (uint32_t seq,
		       struct sde_metadata **p1,
		       size_t *p1_size,
		       struct sde_metadata_data **p2,
		       size_t *p2_size);

/**
 * Creates the response packets for an sde_get_service_desc.
 * 
 * @param [in] seq the sequence number of the sde_get_service_desc packet.
 * @param [out] p1 a pointer to a dynamically allocated memory space containing
 *                the response packet to be sent first.
 * @param [out] p1_size the size of p1 in bytes.
 * @param [out] p2 a pointer to a dynamically allocated memory space containing
 *                the response packet to be sent after p1.
 * @param [out] p2_size the size of p2 in bytes.
 * @param [in] pos the position data contained in the corresponding
 *                 sde_get_service_desc_data packet. <strong>[CAUTION]</strong>
 *                 The caller must sort pos.
 * @param [in] pos_len the number of positions in pos.
 *
 * @return 0 if there is no error or non-zero if there is an error.
 */
int
get_service_desc_response (uint32_t seq,
			   struct sde_service_desc **p1,
			   size_t *p1_size,
			   struct sde_service_desc_data **p2,
			   size_t *p2_size,
			   const struct position *pos, uint32_t pos_len);

#ifdef __cplusplus
}
#endif

#endif /* SERVICE_INQUIRY_H */
