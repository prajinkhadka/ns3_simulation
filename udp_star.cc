#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/netanim-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Star");

int main(int argc, char *argv[]) {
  //
  // Set up some default values for the simulation.
  //
  Config::SetDefault("ns3::OnOffApplication::PacketSize", UintegerValue(137));
  Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue("14kb/s"));

  //
  // Default number of nodes in the star.  Overridable by command line argument.
  //
  uint32_t nSpokes = 8;

  CommandLine cmd;
  cmd.AddValue("nSpokes", "Number of nodes to place in the star", nSpokes);
  cmd.Parse(argc, argv);

  NS_LOG_INFO("Build star topology.");
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
  pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));
  PointToPointStarHelper star(nSpokes, pointToPoint);

  NS_LOG_INFO("Install internet stack on all nodes.");
  InternetStackHelper internet;
  star.InstallStack(internet);

  NS_LOG_INFO("Assign IP Addresses.");
  star.AssignIpv4Addresses(Ipv4AddressHelper("10.1.1.0", "255.255.255.0"));

  NS_LOG_INFO("Create applications.");
  //
  // Create a packet sink on the star "hub" to receive packets.
  //
  uint16_t port = 50000;
  Address hubLocalAddress(InetSocketAddress(Ipv4Address::GetAny(), port));
  PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", hubLocalAddress);
  ApplicationContainer hubApp = packetSinkHelper.Install(star.GetHub());
  hubApp.Start(Seconds(1.0));
  hubApp.Stop(Seconds(10.0));

  //
  // Create OnOff applications to send UDP to the hub, one on each spoke node.
  //
  OnOffHelper onOffHelper("ns3::UdpSocketFactory", Address());
  onOffHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
  onOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

  ApplicationContainer spokeApps;

  for (uint32_t i = 0; i < star.SpokeCount(); ++i) {
    AddressValue remoteAddress(InetSocketAddress(star.GetHubIpv4Address(i), port));
    onOffHelper.SetAttribute("Remote", remoteAddress);
    spokeApps.Add(onOffHelper.Install(star.GetSpokeNode(i)));
  }
  spokeApps.Start(Seconds(1.0));
  spokeApps.Stop(Seconds(10.0));

  NS_LOG_INFO("Enable static global routing.");
  //
  // Turn on global static routing so we can actually be routed across the star.
  //
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  NS_LOG_INFO("Enable Flow Monitor.");
  FlowMonitorHelper flowMonitor;
  Ptr<FlowMonitor> monitor = flowMonitor.InstallAll();

  NS_LOG_INFO("Enable pcap tracing.");
  //
  // Do pcap tracing on all point-to-point devices on all nodes.
  //
  pointToPoint.EnablePcapAll("star");

  NS_LOG_INFO("Run Simulation.");
  Simulator::Stop(Seconds(10.0));
  Simulator::Run();

  monitor->CheckForLostPackets();

  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowMonitor.GetClassifier());
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

  for (auto it = stats.begin(); it != stats.end(); ++it) {
    Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(it->first);
    std::cout << "Flow (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
    std::cout << "  Tx Packets: " << it->second.txPackets << "\n";
    std::cout << "  Rx Packets: " << it->second.rxPackets << "\n";
    std::cout << "  Throughput: " << it->second.rxBytes * 8.0 / 10.0 / 1000 / 1000 << " Mbps\n";
    std::cout << "  Packet Loss: " << it->second.lostPackets << "\n";
    std::cout << "  Packet Loss Ratio: " << it->second.lostPackets / static_cast<double>(it->second.txPackets) << "\n";
    std::cout << "  Delay: " << it->second.delaySum / it->second.rxPackets << " seconds\n";
  }

  NS_LOG_INFO("Destroy Simulator.");
  Simulator::Destroy();

  return 0;
}

