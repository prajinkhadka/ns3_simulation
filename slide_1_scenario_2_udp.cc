#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/netanim-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"

// Network topology (default)
//
//        n2 n3 n4              .
//         \ | /                .
//          \|/                 .
//     n1--- n0---n5            .
//          /|\                 .
//         / | \                .
//        n8 n7 n6              .
//

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Star");

void SetInterfacesDown(PointToPointStarHelper& star) {
    for (uint32_t i = 0; i < star.SpokeCount (); ++i){
        Ptr<Node> node = star.GetSpokeNode(i);
        Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
        uint32_t ifIndex = 1; // Assuming the first point-to-point interface, adjust if needed
        ipv4->SetDown(ifIndex);
    }
}

void SetInterfacesUp(PointToPointStarHelper& star) {
    for (uint32_t i = 0; i < star.SpokeCount (); ++i) {
        Ptr<Node> node = star.GetSpokeNode(i);
        Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
        uint32_t ifIndex = 1; // Assuming the first point-to-point interface, adjust if needed
        ipv4->SetUp(ifIndex);
    }
}

int main (int argc, char *argv[])
{

  // Set up some default values for the simulation.
  Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (137));

  // ??? try and stick 15kb/s into the data rate
  // This means how fast the traffic is genereated.
  Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue ("14kb/s"));

  // Default number of nodes in the star.  Overridable by command line argument.
  uint32_t nSpokes = 8;

  CommandLine cmd;
  cmd.AddValue ("nSpokes", "Number of nodes to place in the star", nSpokes);
  cmd.Parse (argc, argv);

  NS_LOG_INFO ("Build star topology.");
  PointToPointHelper pointToPoint;
  // This is the data rate/bandwidth of the point to point link.
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
  PointToPointStarHelper star (nSpokes, pointToPoint);

  NS_LOG_INFO ("Install internet stack on all nodes.");
  InternetStackHelper internet;
  star.InstallStack (internet);

  NS_LOG_INFO ("Assign IP Addresses.");
  star.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0", "255.255.255.0"));

  NS_LOG_INFO ("Create applications.");
  //
  // Create a packet sink on the star "hub" to receive packets.
  //
  uint16_t port = 50000;
  Address hubLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", hubLocalAddress);
  ApplicationContainer hubApp = packetSinkHelper.Install (star.GetHub ());
  hubApp.Start (Seconds (1.0));
  hubApp.Stop (Seconds (10.0));

  //
  // Create OnOff applications to send UDP to the hub, one on each spoke node.
  //
  OnOffHelper onOffHelper ("ns3::UdpSocketFactory", Address ());
  onOffHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  onOffHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));

  ApplicationContainer spokeApps;

  for (uint32_t i = 0; i < star.SpokeCount (); ++i)
    {
      AddressValue remoteAddress (InetSocketAddress (star.GetHubIpv4Address (i), port));
      onOffHelper.SetAttribute ("Remote", remoteAddress);
      spokeApps.Add (onOffHelper.Install (star.GetSpokeNode (i)));
    }
  spokeApps.Start (Seconds (1.0));
  spokeApps.Stop (Seconds (10.0));

  NS_LOG_INFO ("Enable static global routing.");
  //
  // Turn on global static routing so we can actually be routed across the star.
  //
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  NS_LOG_INFO ("Enable pcap tracing.");
  //
  // Do pcap tracing on all point-to-point devices on all nodes.
  //
  pointToPoint.EnablePcapAll ("star");
  //For ascii trace
  AsciiTraceHelper ascii;
  pointToPoint.EnableAsciiAll(ascii.CreateFileStream("slide_1_scenario_2_udp.tr"));

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Schedule (Seconds (2), SetInterfacesDown, star);
  Simulator::Schedule (Seconds (4), SetInterfacesUp, star);


  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");

  return 0;
}
