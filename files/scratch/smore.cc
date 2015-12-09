/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 University of Washington
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
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/applications-module.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>

namespace std
{
  template <typename T> std::string to_string (const T& n)
    {
      std::ostringstream stm;
      stm << n;
      return stm.str ();
    }
}

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SmoreGridWithOlsr");

int main (int argc, char *argv[])
{
  std::string phyMode ("DsssRate1Mbps");
  double distance = 500;  // m
  uint32_t packetSize = 1000; // bytes
  uint32_t batchSize = 3;
  uint32_t dataSize = 3000;
  uint32_t numChecksums = 2;
  uint32_t sinkNode = 0;
  uint32_t sourceNode = 8;
  uint32_t gridWidth = 3;
  double interval = 1.0; // seconds

  CommandLine cmd;

  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("distance", "distance (m)", distance);
  cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue ("batchSize", "number of packets in a batch", batchSize);
  cmd.AddValue ("dataSize", "amount of data (bytes) to transfer", dataSize);
  cmd.AddValue ("interval", "interval (seconds) between coded packets", interval);
  cmd.AddValue ("numChecksums", "checksums to generate per coded packet", numChecksums);
  cmd.AddValue ("sinkNode", "Receiver node number", sinkNode);
  cmd.AddValue ("sourceNode", "Sender node number", sourceNode);
  cmd.AddValue ("gridWidth", "Width of the grid in nodes", gridWidth);

  cmd.Parse (argc, argv);
  Time interPacketInterval = Seconds (interval);

  // disable fragmentation for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold",
                      StringValue ("2200"));
  // turn off RTS/CTS for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold",
                      StringValue ("2200"));
  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", 
                      StringValue (phyMode));

  LogComponentEnable ("UdpSmorePeer", LOG_INFO);

  NodeContainer nodes;
  nodes.Create (gridWidth*gridWidth);

  WifiHelper wifi;

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  // set it to zero; otherwise, gain will be added
  wifiPhy.Set ("RxGain", DoubleValue (-10) ); 
  // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO); 

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add a non-QoS upper mac, and disable rate control
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));
  // Set it to adhoc mode
  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, nodes);

  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (distance),
                                 "DeltaY", DoubleValue (distance),
                                 "GridWidth", UintegerValue (gridWidth),
                                 "LayoutType", StringValue ("RowFirst"));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);

  // Enable OLSR
  OlsrHelper olsr;
  Ipv4StaticRoutingHelper staticRouting;

  Ipv4ListRoutingHelper list;
  list.Add (staticRouting, 0);
  list.Add (olsr, 10);

  InternetStackHelper internet;
  internet.SetRoutingHelper (list); // has effect on the next Install ()
  internet.Install (nodes);

  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = ipv4.Assign (devices);

  ApplicationContainer apps;
  uint32_t numNodes = nodes.GetN ();
  for (uint32_t i = 0; i < numNodes; ++i)
    {
      UdpSmorePeerHelper h (interfaces.GetAddress (i), 9001);
      h.SetAttribute ("BatchSize", UintegerValue (batchSize));
      h.SetAttribute ("PacketSize", UintegerValue (packetSize));
      h.SetAttribute ("NumChecksums", UintegerValue (numChecksums));
      h.SetAttribute ("Interval", TimeValue (Seconds (interval)));

      if (i == sourceNode)
        {
          h.SetAttribute ("RemoteAddress",
                          Ipv4AddressValue (interfaces.GetAddress (sinkNode)));
          h.SetAttribute ("DataSize", UintegerValue (dataSize));
        }

      ApplicationContainer a = h.Install (nodes.Get (i));
      a.Start (Seconds ((i == sourceNode) ? 31.0 : 30.0));
      a.Stop (Seconds (40.0));

      apps.Add(a);
    }

  // Output what we are doing
  NS_LOG_UNCOND ("Topology:");
  for (uint32_t y = 0; y < gridWidth; ++y)
    {
      std::string row ("\t");
      for (uint32_t x = 0; x < gridWidth; ++x)
        {
          uint32_t nodeNumber = (gridWidth-y-1)*gridWidth + x;
          row += std::to_string (nodeNumber);
          row += (nodeNumber == sourceNode) ? "s"
               : (nodeNumber == sinkNode) ? "d"
               : "";
          row += "\t";
        }
      NS_LOG_UNCOND (row);
    }
  NS_LOG_UNCOND ("Testing from node " << sourceNode << " to " << sinkNode
              << " with grid distance " << distance);

  Simulator::Stop (Seconds (40.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

