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

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "tlv.h"

enum tlv_type
  {
    TYPE_1 = 1,
    TYPE_2,
    TYPE_3,
    TYPE_123,
  };

int
main (int argc, char **argv, char **envp)
{
  void *data;
  uint32_t data_len;
  const struct tlv_chunk *itr = NULL;

  int value1 = 0xBABE;
  char value2[] = "I am here, ain't I?";
  double value3 = 0.4;

  /* Padding */
  assert (get_padded_length (4, VALUE_ALIGNMENT) == 4);
  assert (get_padded_length (7, VALUE_ALIGNMENT) == 8);

  /* Creating */
  itr = create_chunk (TYPE_1, sizeof (value1), &value1, itr, &data, &data_len);
  assert (itr != NULL);

  itr = create_chunk (TYPE_2, strlen (value2) + 1, value2, itr, &data, &data_len);
  assert (itr != NULL);

  itr = create_chunk (TYPE_3, sizeof (value3), &value3, itr, &data, &data_len);
  assert (itr != NULL);

  /* Parsing */
  itr = NULL;

  itr = read_chunk (data, data_len, itr);
  assert (itr != NULL);
  assert (ntohl (itr->type) == TYPE_1);
  assert (ntohl (itr->length) == sizeof (value1));
  assert (*((int *) itr->value) == value1);

  itr = read_chunk (data, data_len, itr);
  assert (itr != NULL);
  assert (ntohl (itr->type) == TYPE_2);
  assert (ntohl (itr->length) == sizeof (value2));
  assert (strcmp (itr->value, value2) == 0);

  itr = read_chunk (data, data_len, itr);
  assert (itr != NULL);
  assert (ntohl (itr->type) == TYPE_3);
  assert (ntohl (itr->length) == sizeof (value3));
  assert (*((double *) itr->value) == value3);

  itr = read_chunk (data, data_len, itr);
  assert (itr == NULL);

  /* Creating a nested TLV packet */
  void *larger_data;
  uint32_t larger_data_len;
  itr = NULL;

  itr = create_chunk (TYPE_123, data_len, data, itr, &larger_data,
		      &larger_data_len);
  assert (itr != NULL);

  itr = create_chunk (TYPE_123, data_len, data, itr, &larger_data,
		      &larger_data_len);
  assert (itr != NULL);

  free (data);

  /* Parsing a nested TLV packet */
  const struct tlv_chunk *larger_itr = NULL;

  while ((larger_itr = read_chunk (larger_data, larger_data_len, larger_itr))
	  != NULL)
    {
      const void *data = larger_itr->value;
      uint32_t data_len = ntohl (larger_itr->length);
      const struct tlv_chunk *itr = NULL;

      assert (ntohl (larger_itr->type) == TYPE_123);
      assert (ntohl (larger_itr->length) == data_len);

      itr = read_chunk (data, data_len, itr);
      assert (itr != NULL);
      assert (ntohl (itr->type) == TYPE_1);
      assert (ntohl (itr->length) == sizeof (value1));
      assert (*((int *) itr->value) == value1);

      itr = read_chunk (data, data_len, itr);
      assert (itr != NULL);
      assert (ntohl (itr->type) == TYPE_2);
      assert (ntohl (itr->length) == sizeof (value2));
      assert (strcmp (itr->value, value2) == 0);

      itr = read_chunk (data, data_len, itr);
      assert (itr != NULL);
      assert (ntohl (itr->type) == TYPE_3);
      assert (ntohl (itr->length) == sizeof (value3));
      assert (*((double *) itr->value) == value3);

      itr = read_chunk (data, data_len, itr);
      assert (itr == NULL);
    }

  free (larger_data);

  exit (EXIT_SUCCESS);
}
