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

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include "service_inquiry_handler.h"
#include "app_err.h"
#include "logger.h"
#include "sde.h"
#include "service_inquiry.h"

/** The socket through which SDE packets are exchanged. */
static int s = -1;

/**
 * Removes the next pending packet from the SDE socket receive buffer.
 *
 * @return 0 if there is no error or non-zero if there is an error.
 */
static int
remove_packet (void)
{
  char buffer;

  if (recvfrom (s, &buffer, sizeof (buffer), 0, NULL, NULL) == -1)
    {
      l->SYS_ERR ("Cannot remove the next packet from the receive buffer");
      return ERR_SOCK;
    }

  return ERR_SUCCESS;
}

/**
 * Gets information on the next pending SDE packet from the SDE socket.
 *
 * @param [out] packet_type the type of the next SDE packet.
 * @param [out] packet_size the size of the next SDE packet.
 *
 * @return 0 if no packet pending, -1 if there is an error or 1 if there is an
 *         SDE packet to be fetched.
 */
static int
next_sde_packet_info (enum sde_packet_type *packet_type, int *packet_size)
{
  ssize_t bytes_rcvd;
  struct sde_packet packet;

  bytes_rcvd = recvfrom (s, &packet, sizeof (packet), MSG_PEEK, NULL, NULL);
  if (bytes_rcvd == -1)
    {
      if (errno == EINTR)
	{
	  return 0;
	}

      l->SYS_ERR ("Cannot peek at the next SDE packet");
      return -1;
    }

  if (bytes_rcvd < sizeof (packet))
    {
      if (remove_packet ())
	{
	  return -1;
	}

      return 0;
    }

  *packet_type = ntohl (packet.type);

  if (ioctl (s, FIONREAD, packet_size) == -1)
    {
      l->SYS_ERR ("Cannot get the size of the next pending packet");
      return -1;
    }

  return 1;
}

/**
 * Checks whether or not a packet size is correct with regard to a
 * recognized type.
 *
 * @param [in] packet the SDE packet to be checked.
 * @param [in] packet_size the size of the packet.
 *
 * @return 0 if the size is incorrect or type is unrecognized, or non-zero
 *         if the size is correct and the type is recognized.
 */
static int
is_sde_packet_sane (struct sde_packet *packet, int packet_size)
{
  switch (ntohl (packet->type))
    {
    case GET_METADATA:
      return packet_size >= sizeof (struct sde_get_metadata);
    case GET_SERVICE_DESC:
      return packet_size >= sizeof (struct sde_get_service_desc);
    case GET_SERVICE_DESC_DATA:
      if (packet_size >= sizeof (struct sde_get_service_desc_data))
	{
	  struct sde_get_service_desc_data *d =
	    (struct sde_get_service_desc_data *) packet;

	  return (packet_size - ntohl (d->count) * sizeof (*d->data)
		  >= sizeof (*d));
	}
      else
	{
	  return 0;
	}
    default:
      return 0;
    }
}

/**
 * Takes the next pending packet in the SDE socket.
 *
 * @param [out] packet the taken packet in a dynamically allocated memory.
 * @param [in] packet_size the size of the packet to be taken.
 * @param [out] sender_addr the sender of the packet.
 *
 * @return 0 if there is no error or non-zero if there is an error.
 */
static int
take_sde_packet (struct sde_packet **packet, int packet_size,
		 struct sockaddr_in **sender_addr)
{
  socklen_t sender_addr_len = sizeof (*sender_addr);

  *packet = malloc (packet_size);
  if (*packet == NULL)
    {
      return ERR_MEM;
    }

  if (recvfrom (s, packet, packet_size, 0, (struct sockaddr *) sender_addr,
		&sender_addr_len) == -1)
    {
      l->SYS_ERR ("Cannot retrieve an SDE packet from the SDE socket");
      free (*packet);
      *packet = NULL;
      return ERR_SOCK;
    }

  if (sender_addr_len != sizeof (*sender_addr))
    {
      l->ERR ("Socket returns incorrect sender address");
      free (*packet);
      *packet = NULL;
      return ERR_SOCK;
    }

  return ERR_SUCCESS;
}

/**
 * Sends back sde_metadata and sde_metadata_data to the sender through the SDE
 * socket.
 *
 * @param [in] socket the socket used to send the packets.
 * @param [in] sender_addr the sender of the replied sde_get_service_desc.
 * @param [in] seq the sequence number of the replied sde_get_service_desc.
 */
static void
send_metadata (struct sockaddr_in *sender_addr, uint32_t seq)
{
  int rc;
  struct sde_metadata *metadata;
  size_t metadata_len;
  struct sde_metadata_data *metadata_data;
  size_t metadata_data_len;
  ssize_t bytes_sent;

  if ((rc = get_metadata_response (seq, &metadata, &metadata_len,
				   &metadata_data, &metadata_data_len)))
    {
      l->APP_ERR (rc, "Cannot get metadata packets");
    }

  bytes_sent = sendto (s, metadata, metadata_len, 0,
		       (struct sockaddr *) sender_addr, sizeof (*sender_addr));
  if (bytes_sent == -1)
    {
      l->SYS_ERR ("Cannot send metadata packet");
    }

  bytes_sent = sendto (s, metadata_data, metadata_data_len, 0,
		       (struct sockaddr *) sender_addr, sizeof (*sender_addr));
  if (bytes_sent == -1)
    {
      l->SYS_ERR ("Cannot send metadata data packet");
    }

  free (metadata);
  free (metadata_data);
}

/**
 * Compares two positions.
 *
 * @param [in] p1 the first position to be compared.
 * @param [in] p2 the second position to be compared.
 *
 * @return an integer less than, equal to, or greater than zero if p1 is
 *         less than, equal to, or greater than p2.
 */
static int
compare_service_position (const void *p1, const void *p2)
{
  const struct position *pos1 = p1;
  const struct position *pos2 = p2;

  return pos1->pos - pos2->pos;
}

/**
 * Sends back sde_service_desc and sde_service_desc_data to the sender
 * through the SDE socket.
 *
 * @param [in] sender_addr the sender of the replied sde_get_service_desc.
 * @param [in] seq the sequence number of the replied sde_get_service_desc.
 * @param [in] pos the position data in the replied sde_get_service_desc_data.
 * @param [in] pos_len position data count in the replied
 *                     sde_get_service_desc_data.
 */
static void
send_service_desc (struct sockaddr_in *sender_addr, uint32_t seq,
		   struct position *pos, uint32_t pos_len)
{
  int rc;
  struct sde_service_desc *service_desc;
  size_t service_desc_len;
  struct sde_service_desc_data *service_desc_data;
  size_t service_desc_data_len;
  ssize_t bytes_sent;

  qsort (pos, pos_len, sizeof (*pos), compare_service_position);

  if ((rc = get_service_desc_response (seq, &service_desc, &service_desc_len,
				       &service_desc_data, &service_desc_data_len,
				       pos, pos_len)))
    {
      l->APP_ERR (rc, "Cannot get service description packets");
    }

  bytes_sent = sendto (s, service_desc, service_desc_len, 0,
		       (struct sockaddr *) sender_addr, sizeof (*sender_addr));
  if (bytes_sent == -1)
    {
      l->SYS_ERR ("Cannot send service description packet");
    }

  bytes_sent = sendto (s, service_desc_data, service_desc_data_len, 0,
		       (struct sockaddr *) sender_addr, sizeof (*sender_addr));
  if (bytes_sent == -1)
    {
      l->SYS_ERR ("Cannot send service description data packet");
    }

  free (service_desc);
  free (service_desc_data);
}

/**
 * Handles the given packet by doing some actions like sending back to the
 * sender particular information.
 * 
 * @param [in] packet the packet to handle.
 * @param [in] packet_size the size of the given packet.
 * @param [in] sender_addr the address of the packet sender.
 */
static void
handle_sde_packet (struct sde_packet *packet, int packet_size,
		   struct sockaddr_in *sender_addr)
{
  struct sde_get_service_desc_data *d;

  switch (ntohl (packet->type))
    {
    case GET_METADATA:
      send_metadata (sender_addr, ntohl (packet->seq));
      break;
    case GET_SERVICE_DESC_DATA:
      d = (struct sde_get_service_desc_data *) packet;

      send_service_desc (sender_addr, ntohl (d->c.seq),
			 d->data, ntohl (d->count));
      break;
    default:
      break;
    }
}

/**
 * Destroys the SDE socket.
 */
static void
destroy_socket (void)
{
  if (s != -1)
    {
      if (close (s) == -1)
	{
	  l->SYS_ERR ("Cannot close inquiry handler socket");
	}
    }
}

int
run_inquiry_handler (int (*is_stopped) (void))
{
#define cleanly destroy_socket (),

  struct sockaddr_in handler_addr = {
    .sin_family = AF_INET,
    .sin_addr = {INADDR_ANY},
    .sin_port = SERVICE_INQUIRY_HANDLER_PORT,
  };

  s = socket (AF_INET, SOCK_DGRAM, 0);
  if (s == -1)
    {
      l->SYS_ERR ("Cannot create inquiry handler socket");
      return cleanly ERR_SOCK;
    }

  if (bind (s, (struct sockaddr *) &handler_addr, sizeof (handler_addr)) == -1)
    {
      l->SYS_ERR ("Cannot name inquiry handler socket");
      return cleanly ERR_SOCK;
    }

  while (!is_stopped ())
    {
      struct sde_packet *packet;
      enum sde_packet_type packet_type;
      struct sockaddr_in *sender_addr;
      int packet_size;
      int rc;

      rc = next_sde_packet_info (&packet_type, &packet_size);
      if (rc == -1)
	{
	  return cleanly ERR_GET_SDE_INFO;
	}

      if (rc != 0)
	{
	  continue;
	}

      if (is_sde_packet_sane (packet, packet_size))
	{
	  if ((rc = take_sde_packet (&packet, packet_size, &sender_addr)))
	    {
	      l->APP_ERR (rc, "Cannot take an SDE packet");
	      return cleanly ERR_TAKE_SDE_PACKET;
	    }
	}
      else
	{
	  if ((rc = remove_packet ()))
	    {
	      l->APP_ERR (rc, "Cannot remove an SDE packet");
	      return cleanly ERR_REMOVE_SDE_PACKET;
	    }

	  continue;
	}

      handle_sde_packet (packet, packet_size, sender_addr);
      free (packet);
    }

  return cleanly ERR_SUCCESS;

#undef cleanly
}
