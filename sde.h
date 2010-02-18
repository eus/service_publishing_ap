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
 * @file sde.h
 * @brief The Service Description Exchange data and packet structures.
 * A service description exchange happens when a user connects to a particular
 * AP to retrieve the details of the service offered in the SSID. The data
 * exchange will be modeled after the latest Android ToDo protocol that
 * employs static packet structures and TLV chunks. Once the gadget gets an IP
 * address, it should connect to UDP port 30003 to start the following
 * communication sequence.
 * <ol>
 *    <li>GET_METADATA</li>
 *    <ul>
 *        <li>If there is no answer after a timeout, the gadget can notify
 *            the user that the AP is not responding. The user can then
 *            choose to retry.</li>
 *        <li>One use of metadata is to enable caching of information like
 *            what is used in HTML proxy.</li>
 *    </ul>
 *    <li>METADATA: METADATA_COUNT</li>
 *    <ul>
 *        <li>The METADATA_COUNT tells the gadget how many "struct metadata"
 *            is contained in the packet that follows. In other words, the AP
 *            will issue two UDP packets in response to GET_METADATA.</li>
 *        <li>The packet that follows contains the last modification
 *            timestamp of each of the published service in the SSID. The
 *            timestamps are ordered according to the service positions in the
 *            SSID. The timestamp is the number of seconds since the Unix
 *            epoch stored in 8 bytes.</li>
 *        <li>A TLV chunk is not employed here because, I think, metadata
 *            negotiation has to be fast.</li>
 *    </ul>
 *    <li>GET_SERVICE_DESCRIPTION: POSITION_COUNT
 *    <ul>
 *        <li>The SERVICE_ID_COUNT tells the AP how many "struct position"
 *            is contained in the packet that follows. In other words, the
 *            gadget will issue two UDP packets in response to METADATA.</li>
 *        <li>The packet that follows contains the positions of the services
 *            that are published in the SSID so that the AP can send only the
 *            needed data as the gadget wants. A position starts from 0 and
 *            is stored in 1 byte.</li>
 *    </ul>
 *    </li>
 *    <li>SERVICE_DESCRIPTION: TOTAL_BYTES
 *    <ul>
 *        <li>The TOTAL_BYTES tells the gadget how many bytes is contained in
 *            the packet that follows. In other words, the AP will issue two
 *            UDP packets in response to GET_SERVICE_DESCRIPTION.</li>
 *        <li>The packet that follows contains a TLV chunk for each of the
 *            requested position. Each chunk currently contains the following
 *            information represented as nested TLV chunks:
 *        <ul>
 *            <li>Last modification timestamp.</li>
 *            <li>Service long description.</li>
 *            <li>URI to obtain the service (e.g., rss://xxx).</li>
 *        </ul>
 *        </li>
 *        <li>TLV is used here since more information can be added in the
 *            future (e.g., a small picture) or the information can be
 *            tailored to the service category.</li>
 *    </ul>
 *    </li>
 * </ol>
 ****************************************************************************/

#ifndef SDE_H
#define SDE_H

#include <stdint.h>
#include "tlv.h"

#ifdef __cpluplus
extern "C" {
#endif

/** The metadata of the published services. */
struct metadata
{
  uint64_t ts; /**< The last-modification timestamp. */
} __attribute__ ((packed));

/** The positions of the desired services. */
struct position
{
  uint8_t pos; /**<
		* The position of a service starting from 0 as advertised in the
		* SSID.
		*/
} __attribute__ ((packed));

/** The type of a chunk contained in a service description data. */
enum service_desc_chunk_type
  {
    DESCRIPTION, /**<
		   * The service description of a service at a particular
		   * position. The value is chunks of ::SERVICE_POS,
		   * ::SERVICE_TS, ::SERVICE_LONG_DESC and ::SERVICE_URI.
		   */
    SERVICE_POS, /**<
		  * The position of a particular service as advertised in the
		  * SSID.
		  */
    SERVICE_TS, /**< The last-modification timestamp of a particular service. */
    SERVICE_LONG_DESC, /**< The long description of a particular service. */
    SERVICE_URI, /**< The URI to obtain a particular service. */
  };

/** Service Description Exchange packet types. */
enum sde_packet_type
  {
    GET_METADATA, /**< A metadata request. */
    METADATA, /**< A metadata response. */
    METADATA_DATA, /**< The metadata response data. */
    GET_SERVICE_DESC, /**< A service description request. */
    GET_SERVICE_DESC_DATA, /**< The service description request data. */
    SERVICE_DESC, /**< The service description response. */
    SERVICE_DESC_DATA, /**< The service description response data. */
  };

/** The common data of all Service Description Exchange packets. */
struct sde_packet
{
  uint32_t type; /**< The type of the SDE packet. */
  uint32_t seq; /**< The packet sequence number. */
} __attribute__ ((packed));

/**
 * The SDE GET_METADATA packet.
 * The sde_packet::type is ::GET_METADATA.
 * The sde_packet::seq should be set and tracked by the sender.
 */
struct sde_get_metadata
{
  struct sde_packet c; /**< The common part of an SDE packet. */
} __attribute__ ((packed));

/**
 * The SDE METADATA packet.
 * The sde_packet::type is ::METADATA.
 * The sde_packet::seq is as the same as the one in the sde_get_metadata that is
 * replied.
 */
struct sde_metadata
{
  struct sde_packet c; /**< The common part of an SDE packet. */
  uint8_t count; /**< The number of metadata in the data packet. */
} __attribute__ ((packed));

/**
 * The SDE METADATA data packet.
 * The sde_packet::type is ::METADATA_DATA.
 * The sde_packet::seq is as the same as the one in the sde_get_metadata that is
 * replied.
 */
struct sde_metadata_data
{
  struct sde_packet c; /**< The common part of an SDE packet. */
  struct metadata data[0]; /**< The metadata data. */
} __attribute__ ((packed));

/**
 * The SDE GET_SERVICE_DESCRIPTION packet.
 * The sde_packet::type is ::GET_SERVICE_DESC.
 * The sde_packet::seq should be set and tracked by the sender.
 */
struct sde_get_service_desc
{
  struct sde_packet c; /**< The common part of an SDE packet. */
  uint8_t count; /**< The number of positions in the data packet. */
} __attribute__ ((packed));

/**
 * The SDE GET_SERVICE_DESCRIPTION data packet.
 * The sde_packet::type is ::GET_SERVICE_DESC_DATA.
 * The sde_packet::seq is as the same as the one in the sde_get_service_desc
 * that has this data.
 */
struct sde_get_service_desc_data
{
  struct sde_packet c; /**< The common part of an SDE packet. */
  struct position data[0]; /**< The service descriptions to be retrieved. */
} __attribute__ ((packed));

/**
 * The SDE SERVICE_DESCRIPTION packet.
 * The sde_packet::type is ::SERVICE_DESC.
 * The sde_packet::seq is as the same as the one in the sde_get_service_desc
 * that is replied.
 */
struct sde_service_desc
{
  struct sde_packet c; /**< The common part of an SDE packet. */
  uint32_t size; /**< The size in bytes of the data packet. */
} __attribute__ ((packed));

/**
 * The SDE SERVICE_DESCRIPTION data packet.
 * The sde_packet::type is ::SERVICE_DESC_DATA.
 * The sde_packet::seq is as the same as the one in the sde_get_service_desc
 * that is replied.
 */
struct sde_service_desc_data
{
  struct sde_packet c; /**< The common part of an SDE packet. */
  struct tlv_chunk data[0]; /**< The size in bytes of the data packet. */
} __attribute__ ((packed));

#ifdef __cplusplus
}
#endif

#endif /* SDE_H */
