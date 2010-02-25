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

#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "tlv.h"

uint32_t
get_padded_length (uint32_t length, uint32_t aligned_at)
{
  uint32_t padding = aligned_at - (length - (aligned_at
					     * (length / aligned_at)));

  return length + ((padding == aligned_at) ? 0 : padding);
}

const struct tlv_chunk *
create_chunk (uint32_t type, uint32_t length, const void *value,
	      const struct tlv_chunk *prev_chunk,
	      void **data, uint32_t *data_len)
{
  struct tlv_chunk *ptr;
  uint32_t chunk_len = (sizeof (struct tlv_chunk)
			+ get_padded_length (length, VALUE_ALIGNMENT));

  if (prev_chunk == NULL)
    {
      *data_len = 0;

      ptr = malloc (chunk_len);
      if (ptr == NULL)
	{
	  return NULL;
	}
      *data = ptr;
    }
  else
    {
      unsigned long prev_chunk_offset = (char *) prev_chunk - (char *) *data;

      ptr = realloc (*data, *data_len + chunk_len);
      if (ptr == NULL)
	{
	  return NULL;
	}
      *data = ptr;
      prev_chunk = (struct tlv_chunk *) (((char *) *data) + prev_chunk_offset);

      ptr = (struct tlv_chunk *) ((char *) prev_chunk
				  + sizeof (struct tlv_chunk)
				  + get_padded_length (ntohl (prev_chunk->length), VALUE_ALIGNMENT));
    }

  memset (ptr, 0, chunk_len);
  ptr->type = htonl (type);
  ptr->length = htonl (length);
  memcpy (ptr->value, value, length);

  *data_len += chunk_len;

  return ptr;
}

const struct tlv_chunk *
read_chunk (const void *data,
	    uint32_t len,
	    const struct tlv_chunk *prev_chunk)
{
  const struct tlv_chunk *ptr;
  const struct tlv_chunk *end = (struct tlv_chunk *) ((char *) data + len);

  if (prev_chunk == NULL)
    {
      ptr = data;
    }
  else
    {
      ptr = (struct tlv_chunk *) ((char *) prev_chunk
				  + sizeof (struct tlv_chunk)
				  + get_padded_length (ntohl (prev_chunk->length), VALUE_ALIGNMENT));
    }

  if (ptr >= end
      || (((char *) ptr + sizeof (struct tlv_chunk)
	   + get_padded_length (ntohl (ptr->length), VALUE_ALIGNMENT))
	  > (char *) end))
    {
      return NULL;
    }

  return ptr;
}
