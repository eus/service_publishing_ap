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
 * @file tlv.h
 * @brief This helps you to create and parse a nested type-length-value
 *        packet. The value is aligned at 4 octets (32 bits) boundary.
 *        When creating a nested TLV packet, the inner part has to
 *        be created before the enclosing part. Once the enclosing part has
 *        been created, the created inner part can be freed since the chunks
 *        have been copied into the enclosing part. See tlv_test.c for a
 *        demonstration on how to create a nested TLV packet.
 * @example tlv_test.c
 ****************************************************************************/

#ifndef TLV_H
#define TLV_H

#include <stdint.h>

#ifdef __cpluplus
extern "C" {
#endif

/** The alignment of the value of a TLV chunk. */
#define VALUE_ALIGNMENT (sizeof (uint32_t))

/** A TLV chunk. */
struct tlv_chunk
{
  uint32_t type; /**< The type of a TLV chunk in network byte order. */
  uint32_t length; /**<
		    * The length of a TLV chunk in bytes in network byte
		    * order.
		    */
  char value[0]; /**< The value of a TLV chunk. */
} __attribute__ ((packed));

/**
 * This function wraps the pointer arithmetic, the dynamic memory allocation
 * and byte-order conversions for tlv_chunk fields.
 * At the start of creating a chunk, prev_chunk must be NULL while the
 * other parameters are set properly. 
 * The returned object then is kept using a pointer and supplied as prev_chunk
 * for the next chunk creation.
 *
 * @param [in] type the type of the TLV chunk in host byte order.
 * @param [in] length the length of the value in bytes in host byte order.
 * @param [in] value the value of the TLV chunk.
 * @param [in] prev_chunk the previously created chunk.
 * @param [in,out] data a pointer to the dynamically allocated memory space
 *                      containing the whole created chunks (the created chunks
 *                      can then be freed by freeing this pointer).
 * @param [in,out] data_len The total size in bytes of the created chunks will
 *                          be stored here.
 *
 * @return the created chunk or NULL if there is an insufficient memory.
 */
const struct tlv_chunk *
create_chunk (uint32_t type, uint32_t length, const void *value,
	      const struct tlv_chunk *prev_chunk,
	      void **data, uint32_t *data_len);

/**
 * This function wraps the pointer arithmetic needed to read the chunks in
 * a TLV packet.
 * At the start of parsing a chunk, prev_chunk must be NULL while chunk and
 * chunk_len are set properly.
 * For parsing the next chunk, prev_chunk must be to the last parsed chunk.
 * In all cases, the data and len should be the same so that the parser
 * knows when to stop parsing.
 * 
 * @param [in] data the start of the data containing TLV chunks.
 * @param [in] len the length of the data.
 * @param [in] prev_chunk the last chunk read.
 *
 * @return a parsed chunk or NULL if the end of the data containing the chunks
 *         has been reached.
 */
const struct tlv_chunk *
read_chunk (const void *data,
	    uint32_t len,
	    const struct tlv_chunk *prev_chunk);

/**
 * Calculates the padded length of unaligned data.
 *
 * @param [in] length the length of unaligned data.
 * @param [in] aligned_at the desired alignment.
 *
 * @return the padded length of the unaligned data that is divisible by the
 *         alignment.
 */
uint32_t
get_padded_length (uint32_t length, uint32_t aligned_at);

#ifdef __cplusplus
}
#endif

#endif /* TLV_H */
