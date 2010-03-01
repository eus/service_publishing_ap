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

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "logger.h"
#include "sde.h"
#include "tlv.h"

GLOBAL_LOGGER;

/**
 * Converts the byte order of 64-bits data to a host byte order.
 *
 * @param [in] the 64-bits data to be converted.
 *
 * @return the 64-bits data in host byte order.
 */
static uint64_t
ntohll (uint64_t n)
{
  static int is_host_big_endian = -1;

  if (is_host_big_endian == -1)
    {
      is_host_big_endian = (0xFACE == htons (0xFACE) ? 1 : 0);
    }

  if (is_host_big_endian)
    {
      return n;
    }

  return (((n & 0x00000000000000FFULL) << 56)
	  | ((n & 0x000000000000FF00ULL) << 40)
	  | ((n & 0x0000000000FF0000ULL) << 24)
	  | ((n & 0x00000000FF000000ULL) << 8)
	  | ((n & 0x000000FF00000000ULL) >> 8)
	  | ((n & 0x0000FF0000000000ULL) >> 24)
	  | ((n & 0x00FF000000000000ULL) >> 40)
	  | ((n & 0xFF00000000000000ULL) >> 56));
}

static size_t
get_user_input (char *buffer, size_t len, const char **err_msg)
{
  char *newline;

  fgets (buffer, len, stdin);
  if ((newline = strchr (buffer, '\n')) == NULL)
    {
      int next_char = getchar ();

      if (next_char == '\n' || next_char == EOF)
	{
	  return len - 1;
	}
      else
	{
	  *err_msg = "input too long";
	  return 0;
	}
    }

  *newline = '\0';

  return newline - buffer;
}

static void
display_user_prompt (const char *prompt_msg, char *buffer, size_t len)
{
  const char *err_msg = NULL;

  do
    {
      if (err_msg != NULL)
	{
	  printf ("Error: %s\n", err_msg);
	  err_msg = NULL;
	}
      printf ("%s", prompt_msg);
    }
  while (get_user_input (buffer, len, &err_msg) == 0
	 || err_msg != NULL);
}

static unsigned char
display_user_choice_prompt (const char *prompt_msg, unsigned char last_option)
{
  const char *err_msg = NULL;
  char ans[4];
  int num = 1;

  do
    {
      if (err_msg != NULL)
	{
	  printf ("Error: %s\n", err_msg);
	  err_msg = NULL;
	}
      if (num < 1 || num > last_option)
	{
	  printf ("Error: Invalid choice (%d)\n", num);
	}
      printf ("%s", prompt_msg);
    }
  while (get_user_input (ans, sizeof (ans), &err_msg) == 0
	 || err_msg != NULL
	 || (num = atoi (ans)) < 1
	 || num > last_option);

  return (unsigned char) num;
}

static int sock = -1;
static void *packet_buffer = NULL;
static size_t packet_buffer_len = 1024 * 1024;

static void
clean_up (void)
{
  if (packet_buffer)
    {
      free (packet_buffer);
      packet_buffer = NULL;
    }
  if (sock != -1)
    {
      if (close (sock) == -1)
	{
	  l->SYS_ERR ("Cannot close socket");
	}
      sock = -1;
    }
}

static struct sockaddr *ap_addr = NULL;
static socklen_t ap_addr_len;

static uint32_t packet_seq = 0;

static void
get_metadata (void)
{
  struct sde_get_metadata m = {
    .c = {
      .type = GET_METADATA,
      .seq = packet_seq,
    },
  };

  if (sendto (sock, &m, sizeof (m), 0, ap_addr, ap_addr_len) == -1)
    {
      l->SYS_ERR ("Cannot send get_metadata");
      return;
    }

  l->INFO ("get_metadata sent");
}

static void
get_service_description (struct position *pos, size_t count)
{
  struct sde_get_service_desc s = {
    .c = {
      .type = GET_SERVICE_DESC,
      .seq = packet_seq,
    },
    .count = htonl (count),
  };
  size_t d_size = (sizeof (struct sde_get_service_desc_data)
		   + sizeof (*pos) * count);
  struct sde_get_service_desc_data *d = malloc (d_size);

  if (d == NULL)
    {
      return;
    }

  d->c.type = GET_SERVICE_DESC_DATA;
  d->c.seq = packet_seq;
  d->count = htonl (count);
  memcpy (d->data, pos, sizeof (*pos) * count);

  if (sendto (sock, &s, sizeof (s), 0, ap_addr, ap_addr_len) == -1)
    {
      l->SYS_ERR ("Cannot send get_service_desc");
      free (d);
      return;
    }  
  if (sendto (sock, &d, sizeof (d_size), 0, ap_addr, ap_addr_len) == -1)
    {
      l->SYS_ERR ("Cannot send get_service_desc_data");
    }
  free (d);
}

static void
print_metadata (struct sde_metadata *m, size_t size)
{
  if (size < 0)
    {
      return;
    }

  printf ("Metadata (%u)", ntohl (m->c.seq));
  printf ("\tCount: %u", ntohl (m->count));
}

static void
print_metadata_data (struct sde_metadata_data *d, size_t size)
{
  int i;

  if (size < 0)
    {
      return;
    }

  printf ("Metadata data (%u)\n", ntohl (d->c.seq));
  printf ("\tCount: %u\n", ntohl (d->count));
  printf ("\tData:\n");
  for (i = 0; i < ntohl (d->count); i++)
    {
      printf ("\t\t[%d] %llu\n", i, ntohll (d->data[i].ts));
    }

  packet_seq++;
}

static void
print_service_desc (struct sde_service_desc *s, size_t size)
{
  if (size < 0)
    {
      return;
    }

  printf ("Service desc (%u)", ntohl (s->c.seq));
  printf ("\tSize: %u", ntohl (s->size));
}

static void
print_service_desc_data (struct sde_service_desc_data *d, size_t size)
{
  char buffer[1024];
  const struct tlv_chunk *itr = NULL;
  int i = -1;

  if (size < 0)
    {
      return;
    }

  printf ("Service desc data (%u)\n", ntohl (d->c.seq));
  printf ("\tSize: %u\n", ntohl (d->size));
  printf ("\tData:\n");
  while ((itr = read_chunk (d->data, ntohl (d->size), itr)))
    {
      const struct tlv_chunk *itr2 = NULL;
      ++i;

      if (ntohl (itr->type) != DESCRIPTION)
	{
	  l->ERR ("Not a description %d", i);
	  continue;
	}

      printf ("\t\t[Service %d]\n", i);
      while ((itr2 = read_chunk (itr->value, ntohl (itr->length), itr2)))
	{
	  switch (ntohl (itr2->type))
	    {
	    case SERVICE_POS:
	      printf ("\t\tPosition: %hhu\n", *((unsigned char *) itr2->value));
	      break;
	    case SERVICE_TS:
	      printf ("\t\tMod time: %llu\n",
		      ntohll (*((unsigned long long *) itr2->value)));
	      break;
	    case SERVICE_LONG_DESC:
	      memset (buffer, 0, sizeof (buffer));
	      memcpy (buffer, itr2->value, ntohl (itr2->length));
	      printf ("\t\tLong desc: %s\n", buffer);
	      break;
	    case SERVICE_URI:
	      memset (buffer, 0, sizeof (buffer));
	      memcpy (buffer, itr2->value, ntohl (itr2->length));
	      printf ("\t\tURI: %s\n", buffer);
	      break;
	    }
	}
    }

  packet_seq++;
}

static ssize_t
wait_for (enum sde_packet_type expected_type, uint32_t expected_seq,
	  struct sde_packet *p, size_t p_len)
{
  ssize_t bytes_rcvd;

  switch (expected_type)
    {
    case METADATA:
      printf ("Waiting for METADATA (%u)\n", expected_seq);
      break;
    case METADATA_DATA:
      printf ("Waiting for METADATA_DATA (%u)\n", expected_seq);
      break;
    case SERVICE_DESC:
      printf ("Waiting for SERVICE_DESC (%u)\n", expected_seq);
      break;
    case SERVICE_DESC_DATA:
      printf ("Waiting for SERVICE_DESC_DATA (%u)\n", expected_seq);
      break;
    default:
      l->ERR ("Invalid expected type");
      return -1;
    }

  do
    {
      int is_size_invalid = 0;
      struct sde_metadata_data *md = NULL;
      struct sde_service_desc_data *sd = NULL;

      if ((bytes_rcvd = recvfrom (sock,
				  p,
				  p_len,
				  0,
				  NULL,
				  NULL)) == -1)
	{
	  l->SYS_ERR ("Cannot retrieve packet");
	  continue;
	}

      switch (expected_type)
	{
	case METADATA:
	  is_size_invalid = (bytes_rcvd < sizeof (struct sde_metadata));
	  break;
	case METADATA_DATA:
	  md = (struct sde_metadata_data *) p;
	  is_size_invalid = (bytes_rcvd < sizeof (*md)
			     + sizeof (*md->data) * ntohl (md->count));
	  break;
	case SERVICE_DESC:
	  is_size_invalid = (bytes_rcvd < sizeof (struct sde_service_desc));
	  break;
	case SERVICE_DESC_DATA:
	  sd = (struct sde_service_desc_data *) p;
	  is_size_invalid = (bytes_rcvd < sizeof (*sd) + ntohl (sd->size));
	  break;
	default:
	  is_size_invalid = 1;
	  break;
	}
      if (is_size_invalid)
	{
	  l->ERR ("Invalid size for %u packet", expected_type);
	  continue;
	}
      if (ntohl (p->type) != expected_type)
	{
	  l->ERR ("ntohl (p->type): %lu != expected_type: %lu",
		  ntohl (p->type), expected_type);
	  continue;
	}
      if (ntohl (p->seq) != expected_seq)
	{
	  l->ERR ("ntohl (p->seq): %lu != expected_seq: %lu",
		  ntohl (p->seq), expected_seq);
	  continue;
	}

      break;
    }
  while (1);

  return bytes_rcvd;
}

int
main (int argc, char **argv, char **envp)
{
  int is_terminated = 0;
  char buffer[32];
  struct position *pos;
  size_t pos_count;
  struct sockaddr_in ap_ip_addr = {
    .sin_family = AF_INET,
    .sin_port = htons (SDE_PORT),
  };
  ssize_t bytes_rcvd;

  ap_addr = (struct sockaddr *) &ap_ip_addr;
  ap_addr_len = sizeof (ap_ip_addr);

  if (argc != 2)
    {
      fprintf (stderr, "Usage: %s AP_IP_ADDR\n", argv[0]);
      exit (EXIT_FAILURE);
    }

  SETUP_LOGGER ("/dev/stderr", NULL);

  sock = socket (AF_INET, SOCK_DGRAM, 0);
  if (sock == -1)
    {
      l->SYS_ERR ("Cannot open socket");
      exit (EXIT_FAILURE);
    }

  packet_buffer = malloc (packet_buffer_len);
  if (packet_buffer == NULL)
    {
      l->ERR ("Not enough memory");
      exit (EXIT_FAILURE);
    }

  if (inet_pton (AF_INET, argv[1], &ap_ip_addr.sin_addr) != 1)
    {
      l->SYS_ERR ("Cannot resolve AP IP address");
      exit (EXIT_FAILURE);
    }

  if (atexit (clean_up))
    {
      l->ERR ("Cannot register clean_up");
      exit (EXIT_FAILURE);
    }
  
  do
    {
      printf ("[Menu]\n"
	      "1. Get metadata\n"
	      "2. Get service description\n"
	      "\n"
	      "3. Quit\n"
	      "\n");

      switch (display_user_choice_prompt ("Choice (1-3)? ", 3))
	{
	case 1:
	  get_metadata ();
	  print_metadata (packet_buffer,
			  wait_for (METADATA, packet_seq,
				    packet_buffer, packet_buffer_len));
	  print_metadata_data (packet_buffer,
			       wait_for (METADATA_DATA, packet_seq,
					 packet_buffer, packet_buffer_len));
	  break;
	case 2:
	  pos = NULL;
	  pos_count = 0;
	  display_user_prompt ("Position (0-based) to retrieve"
			       " (-1 to stop): ",
			       buffer, sizeof (buffer));
	  while (atoi (buffer) != -1)
	    {
	      pos_count++;
	      pos = realloc (pos, sizeof (*pos) * pos_count);
	      if (pos == NULL)
		{
		  l->ERR ("Not enough memory");
		  break;
		}
	      pos[pos_count - 1].pos = atoi (buffer);
	      display_user_prompt ("Position (0-based) to retrieve"
				   " (-1 to stop): ",
				   buffer, sizeof (buffer));
	    }
	  if (pos == NULL)
	    {
	      break;
	    }
	  get_service_description (pos, pos_count);
	  free (pos);
	  bytes_rcvd = recvfrom (sock, packet_buffer, packet_buffer_len,
				 0, NULL, NULL);
	  if (bytes_rcvd == -1)
	    {
	      l->SYS_ERR ("Cannot receive service_desc");
	      break;
	    }
	  print_service_desc (packet_buffer, bytes_rcvd);
	  bytes_rcvd = recvfrom (sock, packet_buffer, packet_buffer_len,
				 0, NULL, NULL);
	  if (bytes_rcvd == -1)
	    {
	      l->SYS_ERR ("Cannot receive service_desc_data");
	      break;
	    }
	  print_service_desc_data (packet_buffer, bytes_rcvd);
	  break;
	case 3:
	  is_terminated = 1;
	  break;
	default:
	  l->ERR ("Unexpected choice");
	  exit (EXIT_FAILURE);
	}
    }
  while (!is_terminated);

  exit (EXIT_SUCCESS);
}

