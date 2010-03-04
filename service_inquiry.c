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

#include <sqlite3.h>
#include <stdint.h>
#include <netinet/in.h>
#include <string.h>
#include "tlv.h"
#include "sde.h"
#include "app_err.h"
#include "logger.h"
#include "service_list.h"
#include "service_inquiry.h"

/**
 * Converts the byte order of 64-bits data to a network byte order.
 *
 * @param [in] the 64-bits data to be converted.
 *
 * @return the 64-bits data in network byte order.
 */
static uint64_t
htonll (uint64_t h)
{
  static int is_host_big_endian = -1;

  if (is_host_big_endian == -1)
    {
      is_host_big_endian = (0xFACE == htons (0xFACE) ? 1 : 0);
    }

  if (is_host_big_endian)
    {
      return h;
    }

  return (((h & 0x00000000000000FFULL) << 56)
	  | ((h & 0x000000000000FF00ULL) << 40)
	  | ((h & 0x0000000000FF0000ULL) << 24)
	  | ((h & 0x00000000FF000000ULL) << 8)
	  | ((h & 0x000000FF00000000ULL) >> 8)
	  | ((h & 0x0000FF0000000000ULL) >> 24)
	  | ((h & 0x00FF000000000000ULL) >> 40)
	  | ((h & 0xFF00000000000000ULL) >> 56));
}

/**< The cached service list. */
static service_list *sl = NULL;

/**
 * Retrieves a service list from the published service DB if the DB has been
 * updated since a particular time in the past. The returned service list should
 * be freed properly.
 *
 * @param [in] last_mod_time a particular time in the past.
 * @param [out] curr_mod_time the time of the latest DB update (equals to
 *                            last_mod_time unless the DB has since been
 *                            updated)
 *
 * @return NULL if the DB has since not been updated or the updated service list.
 */
static service_list *
get_service_list (uint64_t last_mod_time, uint64_t *curr_mod_time)
{
  int rc;

  if (sl == NULL)
    {
      if ((rc = load_service_list (&sl)))
	{
	  l->APP_ERR (rc, "Cannot load service list");
	  return NULL;
	}
    }

  *curr_mod_time = get_last_modification_time (sl);

  if (last_mod_time != *curr_mod_time || last_mod_time == 0)
    {
      last_mod_time = *curr_mod_time;

      if ((rc = reload_service_list (sl)))
	{
	  l->APP_ERR (rc, "Cannot reload service list");
	  return NULL;
	}

      return sl;
    }

  return NULL;
}

/**
 * Creates a ready-to-be-send list of metadata from the given service list.
 *
 * @param [in] sl the service list to be extracted.
 * @param [out] metadata a pointer to a dynamically allocated memory containing
 *                       the metadata list.
 * @param [out] metadata_size the size of the allocated memory in bytes.
 *
 * @return 0 if there is no error or non-zero if there is an error.
 */
static int
get_metadata_from_service_list (service_list *sl,
				struct metadata **metadata,
				size_t *metadata_size)
{
  struct metadata *ptr_m;
  size_t len;
  size_t i, service_count;

  service_count = count_service (sl);
  len = sizeof (*ptr_m) * service_count;
  ptr_m = malloc (len);
  if (ptr_m == NULL)
    {
      return ERR_MEM;
    }

  for (i = 0; i < service_count; i++)
    {
      int rc;
      struct service *s;

      if ((rc = get_service_at (sl, &s, i)))
	{
	  l->APP_ERR (rc, "Cannot get service[%d] from service list", i);
	  free (ptr_m);
	  return ERR_EXTRACTING_METADATA;
	}

      ptr_m[i].ts = htonll (s->ro.mod_time);

      destroy_service (&s);
    }

  *metadata = ptr_m;
  *metadata_size = len;

  return ERR_SUCCESS;
}

/**< The cached metadata. */
static struct metadata *metadata = NULL;

int
get_metadata_response (uint32_t seq,
		       struct sde_metadata **p1,
		       size_t *p1_size,
		       struct sde_metadata_data **p2,
		       size_t *p2_size)
{
  static uint64_t last_mod_time = 0;
  static size_t metadata_size = 0;

  struct sde_metadata *ptr_m;
  struct sde_metadata_data *ptr_d;
  size_t ptr_d_size;
  uint64_t curr_mod_time;
  service_list *sl = get_service_list (last_mod_time, &curr_mod_time);
  if (sl != NULL)
    {
      int rc;

      l->INFO ("Cache miss");

      last_mod_time = curr_mod_time;
      if (metadata != NULL)
	{
	  free (metadata);
	  metadata = NULL;
	}
      if ((rc = get_metadata_from_service_list (sl, &metadata, &metadata_size)))
	{
	  l->APP_ERR (rc, "Cannot get metadata from service list");
	  return ERR_GET_METADATA_PACKETS;
	}
    }
  else
    {
      l->INFO ("Cache hit");
    }

  ptr_m = malloc (sizeof (*ptr_m));
  if (ptr_m == NULL)
    {
      return ERR_MEM;
    }
  ptr_d_size = sizeof (*ptr_d) + metadata_size;
  ptr_d = malloc (ptr_d_size);
  if (ptr_d == NULL)
    {
      return ERR_MEM;
    }

  ptr_m->c.type = htonl (METADATA);
  ptr_m->c.seq = htonl (seq);
  ptr_m->count = htonl (metadata_size / sizeof (*metadata));
  l->INFO ("METADATA #%u packet crafted announcing %u metadata",
	   ntohl (ptr_m->c.seq), ntohl (ptr_m->count));

  ptr_d->c.type = htonl (METADATA_DATA);
  ptr_d->c.seq = ptr_m->c.seq;
  ptr_d->count = ptr_m->count;
  ptr_d->unused1 = 0;
  memcpy (ptr_d->data, metadata, metadata_size);
  l->INFO ("METADATA_DATA #%u packet crafted containing %u bytes",
	   ntohl (ptr_d->c.seq), metadata_size);

  *p1 = ptr_m;
  *p1_size = sizeof (*ptr_m);
  *p2 = ptr_d;
  *p2_size = ptr_d_size;

  return ERR_SUCCESS;
}

/**
 * Creates a ready-to-be-sent TLV chunks of service description from the given
 * service list.
 *
 * @param [in] sl the service list to be extracted.
 * @param [out] service_desc a pointer to a dynamically allocated memory
 *                           containing the service description TLV chunks.
 * @param [out] service_desc_size the size of the allocated memory in bytes.
 *
 * @return 0 if there is no error or non-zero if there is an error.
 */
static int
get_service_desc_from_service_list (service_list *sl,
				    struct tlv_chunk **service_desc,
				    size_t *service_desc_size)
{
  size_t service_count = count_service (sl);
  int i;
  const struct tlv_chunk *itr2 = NULL;
  void *descs = NULL;
  uint32_t descs_size = 0;

  for (i = 0; i < service_count; i++)
    {
#define return_cleanly(rc) do {			\
	if (descs != NULL)			\
	  free (descs);				\
	if (service_data != NULL)		\
	  free (service_data);			\
	destroy_service (&s);			\
	return rc;				\
      } while (0)

      int rc;
      const struct tlv_chunk *itr = NULL;
      struct service *s = NULL;
      void *service_data = NULL;
      uint32_t service_data_size;
      uint64_t mod_time;
      uint32_t cat_id;

      if ((rc = get_service_at (sl, &s, i)))
	{
	  l->APP_ERR (rc, "Cannot get service[%d] from service list", i);
	  return_cleanly (ERR_GET_SERVICE_DESC);
	}

      if ((itr = create_chunk (SERVICE_POS, sizeof (uint8_t), &i, itr,
			       &service_data, &service_data_size)) == NULL)
	{
	  return_cleanly (ERR_MEM);
	}
      mod_time = htonll (s->ro.mod_time);
      if ((itr = create_chunk (SERVICE_TS, sizeof (uint64_t), &mod_time, itr,
			       &service_data, &service_data_size)) == NULL)
	{
	  return_cleanly (ERR_MEM);
	}
      cat_id = htonl (s->cat_id);
      if ((itr = create_chunk (SERVICE_CAT_ID, sizeof (cat_id), &cat_id, itr,
			       &service_data, &service_data_size)) == NULL)
	{
	  return_cleanly (ERR_MEM);
	}
      if (s->desc != NULL
	  && (itr = create_chunk (SERVICE_SHORT_DESC, strlen (s->desc),
				  s->desc, itr,
				  &service_data, &service_data_size)) == NULL)
	{
	  return_cleanly (ERR_MEM);
	}
      if (s->long_desc != NULL
	  && (itr = create_chunk (SERVICE_LONG_DESC, strlen (s->long_desc),
				  s->long_desc, itr,
				  &service_data, &service_data_size)) == NULL)
	{
	  return_cleanly (ERR_MEM);
	}
      if ((itr = create_chunk (SERVICE_URI, strlen (s->uri), s->uri, itr,
			       &service_data, &service_data_size)) == NULL)
	{
	  return_cleanly (ERR_MEM);
	}

      if ((itr2 = create_chunk (DESCRIPTION, service_data_size, service_data, itr2,
			       &descs, &descs_size)) == NULL)
	{
	  return_cleanly (ERR_MEM);
	}

      free (service_data);

      destroy_service (&s);

#undef return_cleanly
    }

  *service_desc = descs;
  *service_desc_size = descs_size;

  return ERR_SUCCESS;
}

/**
 * Calculates the size of the selected TLV chunks.
 *
 * @param [in] service_desc the complete TLV chunks from which the selections
 *                          will be made.
 * @param [in] service_desc_size the size in bytes of service_desc.
 * @param [in] pos the selected positions.
 * @param [in] pos_len the number of positions to be selected.
 *
 * @return the size in bytes of the selected TLV chunks.
 */
static size_t
get_req_service_desc_size (const struct tlv_chunk *service_desc,
			   size_t service_desc_size,
			   const struct position *pos, uint32_t pos_len)
{
  uint32_t i = 0, j = 0;
  size_t result = 0;
  const struct tlv_chunk *itr = NULL;

  while ((itr = read_chunk (service_desc, service_desc_size, itr)) != NULL)
    {
      if (i == pos[j].pos)
	{
	  result += sizeof (*itr) + get_padded_length (ntohl (itr->length),
						       VALUE_ALIGNMENT);

	  j++;
	  if (j == pos_len)
	    {
	      break;
	    }
	}

      i++;
    }

  return result;
}

/**
 * Copies selected TLV chunks.
 *
 * @param [out] dst the copy destination.
 * @param [in] service_desc the complete TLV chunks from which selected
 *                          copies will be made.
 * @param [in] service_desc_size the size in bytes of service_desc.
 * @param [in] pos the selected positions.
 * @param [in] pos_len the number of positions to be selected.
 */
static void
copy_req_service_desc (void *dst, const struct tlv_chunk *service_desc,
		       size_t service_desc_size, const struct position *pos,
		       uint32_t pos_len)
{
  uint32_t i = 0, j = 0;
  const struct tlv_chunk *itr = NULL;

  while ((itr = read_chunk (service_desc, service_desc_size, itr)) != NULL)
    {
      if (i == pos[j].pos)
	{
	  size_t tot_len = (sizeof (*itr)
			    + get_padded_length (ntohl (itr->length),
						 VALUE_ALIGNMENT));
	  memcpy (dst, itr, tot_len);
	  dst = (char *) dst + tot_len;

	  j++;
	  if (j == pos_len)
	    {
	      break;
	    }
	}

      i++;
    }
}

/**< The cached service description. */
static struct tlv_chunk *service_desc = NULL;

int
get_service_desc_response (uint32_t seq,
			   struct sde_service_desc **p1,
			   size_t *p1_size,
			   struct sde_service_desc_data **p2,
			   size_t *p2_size,
			   const struct position *pos, uint32_t pos_len)
{
  static uint64_t last_mod_time = 0;
  static size_t service_desc_size = 0;

  struct sde_service_desc *ptr_s;
  struct sde_service_desc_data *ptr_d;
  size_t req_service_desc_size;
  size_t ptr_d_size;
  uint64_t curr_mod_time;
  service_list *sl = get_service_list (last_mod_time, &curr_mod_time);
  if (sl != NULL)
    {
      int rc;

      l->INFO ("Cache miss");

      last_mod_time = curr_mod_time;
      if (service_desc != NULL)
	{
	  free (service_desc);
	  service_desc = NULL;
	}
      if ((rc = get_service_desc_from_service_list (sl, &service_desc,
						    &service_desc_size)))
	{
	  l->APP_ERR (rc, "Cannot get service description from service list");
	  return ERR_GET_SERVICE_DESC_PACKETS;
	}
    }
  else
    {
      l->INFO ("Cache hit");
    }

  ptr_s = malloc (sizeof (*ptr_s));
  if (ptr_s == NULL)
    {
      return ERR_MEM;
    }
  req_service_desc_size = get_req_service_desc_size (service_desc,
						     service_desc_size,
						     pos, pos_len);
  ptr_d_size = sizeof (*ptr_d) + req_service_desc_size;
  ptr_d = malloc (ptr_d_size);
  if (ptr_d == NULL)
    {
      return ERR_MEM;
    }

  ptr_s->c.type = htonl (SERVICE_DESC);
  ptr_s->c.seq = htonl (seq);
  ptr_s->size = htonl (ptr_d_size);
  l->INFO ("SERVICE_DESC #%u packet crafted announcing %u bytes of data packet",
	   ntohl (ptr_s->c.seq), ntohl (ptr_s->size));

  ptr_d->c.type = htonl (SERVICE_DESC_DATA);
  ptr_d->c.seq = ptr_s->c.seq;
  ptr_d->size = ptr_s->size;
  copy_req_service_desc (ptr_d->data, service_desc, service_desc_size, pos,
			 pos_len);
  l->INFO ("SERVICE_DESC_DATA #%u packet crafted having %u bytes of data",
	   ntohl (ptr_d->c.seq), ntohl (ptr_d->size));

  *p1 = ptr_s;
  *p1_size = sizeof (*ptr_s);
  *p2 = ptr_d;
  *p2_size = ptr_d_size;

  return ERR_SUCCESS;
}

void
destroy_sde_handler_cache (void)
{
  if (metadata != NULL)
    {
      free (metadata);
      metadata = NULL;
    }

  if (service_desc != NULL)
    {
      free (service_desc);
      service_desc = NULL;
    }

  if (sl != NULL)
    {
      destroy_service_list (&sl);
      sl = NULL;
    }
}
