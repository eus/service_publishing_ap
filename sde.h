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
 * A Service description exchange (<strong>SDE</strong>) happens when a user
 * connects to a particular AP to retrieve the details of the service offered
 * in the SSID. The data exchange will be modeled after the latest Android
 * ToDo protocol that employs static packet structures and TLV chunks. The
 * data are aligned properly as can be seen in detail in the C implementation
 * at the end of this document. Once the gadget gets an IP address, it should
 * connect to UDP port 30003 to start the following communication sequence:
 * <ol>
 *     <li>GET_METADATA (can be skipped)
 *     <ul>
 *         <li>If there is no answer after a timeout, the gadget can notify
 *         the user that the AP is not responding. The user can then
 *         choose to retry.</li>
 *         <li>One use of metadata is to enable caching of information like
 *         what is used in HTML proxy.</li>
 *     </ul>
 *     <dl>
 *         <dt><strong>Response</strong></dt>
 *         <dd>
 *         <ol>
 *             <li>METADATA: METADATA_COUNT
 *             <ul>
 *                 <li>The METADATA_COUNT tells the gadget how many
 *                 "struct metadata" is contained in the packet that
 *                 follows.</li>
 *             </ul>
 *             </li>
 *             <li>METADATA_DATA: METADATA_COUNT, TSES
 *             <ul>
 *                 <li>METADATA_COUNT is the corresponding METADATA's
 *                 METADATA_COUNT.</li>
 *                 <li>TSES are the last modification
 *                 timestamps of the published services in the SSID. The
 *                 timestamps are ordered according to the service positions
 *                 in the SSID. A timestamp is the number of seconds since
 *                 the Unix epoch stored in 8 bytes.</li>
 *                 <li>A TLV chunk is not employed here because, I think,
 *                 metadata negotiation has to be fast.</li>
 *             </ul>
 *             </li>
 *         </ol>
 *         </dd>
 *     </dl>
 *     </li>
 *     <li>GET_SERVICE_DESCRIPTION: POSITION_COUNT
 *     <ul>
 *         <li>The SERVICE_ID_COUNT tells the AP how many "struct position"
 *         is contained in the packet that follows.</li>
 *     </ul>
 *     </li>
 *     <li>GET_SERVICE_DESCRIPTION_DATA: POSITION_COUNT, POSITIONS
 *     <ul>
 *         <li>POSITION_COUNT is the corresponding GET_SERVICE_DESCRIPTION's
 *         POSITION_COUNT.</li>
 *         <li>POSITIONS are the positions of the services
 *         that are published in the SSID so that the AP can send only the
 *         needed data as the gadget wants. A position starts from 0 and
 *         is stored in 1 byte.</li>
 *     </ul>
 *     <dl>
 *         <dt><strong>Response</strong></dt>
 *         <dd>
 *         <ol>
 *             <li>SERVICE_DESCRIPTION: TOTAL_BYTES
 *             <ul>
 *                 <li>The TOTAL_BYTES tells the gadget how many bytes is
 *                 contained in the packet that follows.</li>
 *             </ul>
 *             </li>
 *             <li>SERVICE_DESCRIPTION_DATA: TOTAL_BYTES, TLV_DATA
 *             <ul>
 *                 <li>TOTAL_BYTES is the corresponding
 *                 SERVICE_DESCRIPTION's TOTAL_BYTES.</li>
 *                 <li>TLV_DATA are TLV chunks for each of the requested
 *                 position. <strong>[CAUTION]</strong> Not all requested
 *                 service descriptions may be present (e.g., a service
 *                 can be disabled by the AP owner). Each chunk currently
 *                 contains the following information represented as nested
 *                 TLV chunks:
 *                 <ul>
 *                     <li>Last modification timestamp.</li>
 *                     <li>Service long description.</li>
 *                     <li>URI to obtain the service (e.g., rss://xxx).</li>
 *                 </ul>
 *                 </li>
 *                 <li>TLV is used here since more information can be added in
 *                 the future (e.g., a small picture) or the information can
 *                 be tailored to the service category.</li>
 *             </ul>
 *             </li>
 *         </ol>
 *         </dd>
 *     </dl>
 *     </li>
 * </ol>
 * <strong>[CAUTION]</strong> GET_METADATA and the corresponding METADATA
 * reply communication session can be skipped (i.e., GET_SERVICE_DESCRIPTION
 * can be issued without issuing GET_METADATA first).
 * <h1>Why duplicated "count" and "size" fields in sde_metadata_data,
 * sde_get_service_desc_data and sde_service_desc_data?</h1>
 * The duplicated count and size fields are there so that an AP or a gadget
 * that are capable of using ioctl (..., FIONREAD, ...) to look the size of
 * the next pending UDP datagram without retrieving the UDP datagram from
 * the socket can save memory by not remembering the previously received
 * announcement.
 * The announcement scheme (preceding the variable length data packet with
 * an announcement packet) is used because Android's Java requires a prior
 * knowledge about the length of the next UDP datagram before you can
 * retrieve the UDP datagram without truncation (i.e., there is no
 * equivalent of FIONREAD in Java, but CMIIW).
 * <h1>Beware of outdated data</h1>
 * Consider the following sequence of communication with an AP:
 * <table border="1">
 *         <tr>
 *             <th>Gadget</th>
 *             <th>AP</th>
 *         </tr>
 *         <tr>
 *             <td>get_metadata</td>
 *             <td></td>
 *         </tr>
 *         <tr>
 *             <td></td>
 *             <td>metadata (count = 2)</td>
 *         </tr>
 *         <tr>
 *             <td>get_service_description<br>
 *             (count = 1, position = 0)</td>
 *             <td></td>
 *         </tr>
 *         <tr>
 *             <td></td>
 *             <td>Owner deletes service #1, service #2 has position = 0</td>
 *         </tr>
 *         <tr>
 *             <td></td>
 *             <td>service_description<br>
 *             (carries service description #2 instead of #1 as the gadget
 *             requested)</td>
 *         </tr>
 * </table>
 * As illustrated above, the gadget software should anticipate for such a
 * discrepancy between the previous and current information by checking the
 * last modification timestamp.
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
  uint32_t count; /**< The number of metadata in the data packet. */
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
  uint32_t count; /**< A copy of the corresponding sde_metadata::count. */
  uint32_t unused1; /**< Padding for aligning the data at 64-bits. */
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
  uint32_t count; /**< The number of positions in the data packet. */
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
  uint32_t count; /**<
		   * A copy of the corresponding sde_get_service_desc::count.
		   */
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
  uint32_t size; /**< A copy of the corresponding sde_service_desc::size. */
  struct tlv_chunk data[0]; /**< The service description data. */
} __attribute__ ((packed));

#ifdef __cplusplus
}
#endif

#endif /* SDE_H */
