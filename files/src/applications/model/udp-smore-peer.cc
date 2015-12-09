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
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/ipv4-header.h"
#include "ns3/udp-header.h"
#include "ns3/uinteger.h"
#include "udp-smore-peer.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("UdpSmorePeer");

NS_OBJECT_ENSURE_REGISTERED (UdpSmorePeer);

static const uint8_t msg[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890";
static const uint32_t msglen = sizeof (msg) - 1;

TypeId
UdpSmorePeer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::UdpSmorePeer")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<UdpSmorePeer> ()
    .AddAttribute ("LocalAddress", 
                   "The address of this node",
                   Ipv4AddressValue (),
                   MakeIpv4AddressAccessor (&UdpSmorePeer::m_localAddress),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("RemoteAddress", 
                   "The destination address of the outbound file",
                   Ipv4AddressValue (),
                   MakeIpv4AddressAccessor (&UdpSmorePeer::m_remoteAddress),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("Port", 
                   "The port to listen on / send to",
                   UintegerValue (),
                   MakeUintegerAccessor (&UdpSmorePeer::m_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("DataSize", 
                   "The size of the file the application will send, if any",
                   UintegerValue (0),
                   MakeUintegerAccessor (&UdpSmorePeer::SetDataSize,
                                         &UdpSmorePeer::GetDataSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Interval", 
                   "The time to wait between sending linear combinations",
                   TimeValue (Seconds (0.01)),
                   MakeTimeAccessor (&UdpSmorePeer::m_interval),
                   MakeTimeChecker ())
    .AddAttribute ("PacketSize", "Size of a native packet",
                   UintegerValue (1500),
                   MakeUintegerAccessor (&UdpSmorePeer::m_packetSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("BatchSize", 
                   "The number of linear combinations in a batch",
                   UintegerValue (32),
                   MakeUintegerAccessor (&UdpSmorePeer::m_batchSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("NumChecksums", 
                   "The number of checksums to send with a coded packet"
                   " (not yet implemented)",
                   UintegerValue (2),
                   MakeUintegerAccessor (&UdpSmorePeer::m_numChecksums),
                   MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}

UdpSmorePeer::UdpSmorePeer ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_data = 0;
  m_canSend = false;
  m_batchIndex = 0;
  m_sendEvent = EventId ();
  m_rng.SetStream (Simulator::Now ().GetSeconds ());
  m_enc = 0;
  m_dec = new RlncDecoder (m_packetSize, m_batchSize);
  m_rec = new RlncRecoder (m_packetSize, m_batchSize);
}

UdpSmorePeer::~UdpSmorePeer ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  delete [] m_data;
  m_data = 0;
  m_canSend = false;
  m_dataSize = 0;
  m_batchIndex = 0;
  delete m_enc;
  delete m_dec;
  delete m_rec;
}

void
UdpSmorePeer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
UdpSmorePeer::SetPort (uint16_t port)
{
  NS_LOG_FUNCTION (this << port);
  m_port = port;
}

void
UdpSmorePeer::SetLocalAddress (Ipv4Address address)
{
  NS_LOG_FUNCTION (this << address);
  m_localAddress = address;
}

void
UdpSmorePeer::SetRemoteAddress (Ipv4Address address)
{
  NS_LOG_FUNCTION (this << address);
  m_remoteAddress = address;
}

void
UdpSmorePeer::SetDataSize (uint32_t dataSize)
{
  NS_LOG_FUNCTION (this << dataSize);
  m_dataSize = dataSize;
  m_data = new uint8_t[dataSize];
  for (uint32_t i = 0; i < dataSize; ++i)
    {
      m_canSend = true;
      m_data[i] = msg[i%msglen];
    }
}

uint32_t
UdpSmorePeer::GetDataSize (void) const
{
  NS_LOG_FUNCTION (this);
  return m_dataSize;
}

void
UdpSmorePeer::SetInterval (Time interval)
{
  NS_LOG_FUNCTION (this << interval);
  m_interval = interval;
}

void
UdpSmorePeer::SetPacketSize (uint32_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_packetSize = size;
}

void
UdpSmorePeer::SetBatchSize (uint32_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_batchSize = size;
}

void
UdpSmorePeer::SetNumChecksums (uint32_t count)
{
  NS_LOG_FUNCTION (this << count);
  m_numChecksums = count;
}

void
UdpSmorePeer::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      m_socket->SetAllowBroadcast (true);
      m_socket->Bind (InetSocketAddress (m_localAddress, m_port));
      m_socket->Connect (InetSocketAddress (Ipv4Address::GetBroadcast (), m_port));
      if (m_canSend)
        {
          ScheduleTransmit (Seconds (0.0));
        }
    }

  m_socket->SetRecvCallback (MakeCallback (&UdpSmorePeer::HandleRead, this));
}

void
UdpSmorePeer::StopApplication (void)
{
  NS_LOG_FUNCTION (this);
  m_socket->Close ();
  m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
}

void
UdpSmorePeer::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      Ipv4Address srcAddr = InetSocketAddress::ConvertFrom (from).GetIpv4 ();

      uint32_t pktSize = packet->GetSize ();
      uint8_t buf[pktSize];
      packet->CopyData(buf, pktSize);

      //sMORE header
      uint8_t version = buf[0];                         // Version number
      uint8_t type = buf[1];                            // Packet type
      uint16_t hopCount = ntohs(*((uint16_t*)(buf+2))); // Hop count
      uint32_t batchID = ntohl(*((uint32_t*)(buf+4)));  // Batch ID
      Ipv4Address smoreSrc = Ipv4Address::Deserialize(&buf[8]);   // Source Address
      Ipv4Address smoreDst = Ipv4Address::Deserialize(&buf[12]); // Destination Address

      RlncCodeword c = RlncCodeword::Deserialize (&buf[16], pktSize-16);
      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s peer " << m_localAddress
                << " received a packet from " << srcAddr << " on port " << m_port);
      if (type == 1)
        NS_LOG_INFO ("Packet is coded for forwarding from " << smoreSrc << " to " << smoreDst
                  << " using sMORE version " << (int)version << ". It is from batch "
                  << batchID << " and has travelled " << hopCount << " hops.");
      NS_LOG_DEBUG ("Packet contents: " << c);
    }
}

void
UdpSmorePeer::ScheduleTransmit (Time dt)
{
  NS_LOG_FUNCTION (this << dt);

  m_sendEvent = Simulator::Schedule (dt, &UdpSmorePeer::Send, this);
}

void
UdpSmorePeer::Send (void)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT (m_sendEvent.IsExpired ());

  uint32_t numBatches = m_dataSize / (m_batchSize*m_packetSize);
  if (m_canSend && m_batchIndex < numBatches)
    {
      if (m_enc == NULL)
        {
          std::vector<RlncWord> batch;
          for (uint32_t i = 0; i < m_batchSize; ++i)
            {
              batch.push_back (RlncWord (m_data + i*m_packetSize, m_packetSize));
              NS_LOG_DEBUG ("Node " << m_localAddress << " adding packet " << batch.back ());
            }
          m_enc = new RlncEncoder (batch, m_packetSize);
        }

      RlncCodeword c = m_enc->Encode ();
      uint32_t size = 16+c.GetSerializedSize ();
      uint8_t buf[size];

      //sMORE header
      buf[0] = 1;                                  // Version number
      buf[1] = 1;                                  // Packet type (forward)
      *((uint16_t*)(buf+2)) = htons(0);            // Hop count
      *((uint32_t*)(buf+4)) = htonl(m_batchIndex); // Batch ID
      m_localAddress.Serialize(&buf[8]);           // Source Address
      m_remoteAddress.Serialize(&buf[12]);         // Destnation Address

      c.Serialize (&buf[16], size-16);
      Ptr<Packet> p = Create<Packet> (buf, size);
      m_socket->Send (p);

      ScheduleTransmit (m_interval);

      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s peer " << m_localAddress
                << " broadcast " << p->GetSize () << " bytes from batch "
                << m_batchIndex << " of " << m_dataSize/(m_packetSize*m_batchSize) + 1
                << " to " << m_remoteAddress);
      NS_LOG_DEBUG ("Codeword: " << RlncCodeword::Deserialize (&buf[16], size-16));

      return;
    }

  NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << " peer " << m_localAddress
            << " ran out of packets to send");
}

} // Namespace ns3
