/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Ben Hamlin
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Ben Hamlin <protob3n@gmail.com>
 */

#ifndef UDP_SMORE_PEER_H
#define UDP_SMORE_PEER_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/random-variable-stream.h"
#include "udp-smore-rlnc.h"

namespace ns3 {

class Socket;
class Packet;

/**
 * \ingroup smore
 * \brief Secure rlnc file transfer
 *
 * Data is batchified, RLNC coded, and forwarded.
 */
class UdpSmorePeer : public Application 
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  UdpSmorePeer ();
  virtual ~UdpSmorePeer ();

  /**
   * \brief Set the port the peer will listen / send on.
   * \param port the port
   */
  void SetPort (uint16_t port);

  /**
   * \brief Set the local IPv4 address.
   * \param address the local address
   */
  void SetLocalAddress (Ipv4Address address);

  /**
   * \brief Set the IPv4 address to send to, if any.
   * \param address the local address
   */
  void SetRemoteAddress (Ipv4Address address);

  /**
   * \brief Set the size of the file to send, if any.
   * \param size the size of the file to send
   */
  void SetDataSize (uint32_t size);

  /**
   * \brief Get the size of the file to send, if any.
   */
  uint32_t GetDataSize (void) const;

  /**
   * \brief Set the interval between sending linear combinations.
   * \param size the size of a native packet
   */
  void SetInterval (Time interval);

  /**
   * Set the size of a native packet. The file will be fragmented into packets
   * of this size.
   *
   * \param size the size of a native packet
   */
  void SetPacketSize (uint32_t size);

  /**
   * \brief Set the number of native packets to code into a batch.
   * \param size the number of native packets in a batch
   */
  void SetBatchSize (uint32_t size);

  /**
   * \brief Set the number of checksums to send per packet.
   * \param count the number of checksums to send per packet
   */
  void SetNumChecksums (uint32_t count);

protected:
  virtual void DoDispose (void);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  /**
   * \brief Handle a packet reception.
   *
   * This function is called by lower layers.
   *
   * \param socket the socket the packet was received to.
   */
  void HandleRead (Ptr<Socket> socket);

  /**
   * \brief Schedule the next packet transmission
   * \param dt time interval between packets.
   */
  void ScheduleTransmit (Time dt);

  /**
   * \brief Send a linear combination
   */
  void Send (void);

  uint16_t m_port; //!< port on which we listen for / send packets
  Ptr<Socket> m_socket; //!< IPv4 Socket
  Ipv4Address m_localAddress; //!< local address

  Ipv4Address m_remoteAddress; //!< address to send to, if any
  uint32_t m_dataSize; //!< size of file to send
  uint8_t *m_data; //!< payload to send
  bool m_canSend;
  uint32_t m_batchIndex; //!< start index of the current batch

  Time m_interval; //!< time between sending linear combinations
  EventId m_sendEvent; //!< event to send the next packet
  uint32_t m_packetSize; //!< sizeof a native packet
  uint32_t m_batchSize; //!< number of packets in a batch
  uint32_t m_numChecksums; //!< number of matrix checksums to send

  UniformRandomVariable m_rng;
  RlncEncoder *m_enc;
  RlncDecoder *m_dec;
  RlncRecoder *m_rec;
};

} // namespace ns3

#endif /* UDP_SMORE_PEER_H */
