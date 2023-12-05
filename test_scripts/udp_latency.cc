#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

int main() {
  ns3::NodeContainer nodes;
  nodes.Create(2);

  // Adjust delay for latency simulation
  ns3::PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute("DataRate", ns3::DataRateValue(ns3::DataRate("100Mbps")));
  pointToPoint.SetChannelAttribute("Delay", ns3::TimeValue(ns3::MilliSeconds(50))); // Increased delay to simulate latency

  ns3::NetDeviceContainer devices;
  devices = pointToPoint.Install(nodes);

  ns3::InternetStackHelper stack;
  stack.Install(nodes);

  ns3::Ipv4AddressHelper address;
  address.SetBase("10.1.1.0", "255.255.255.0");
  ns3::Ipv4InterfaceContainer interfaces = address.Assign(devices);

  // UDP application
  ns3::OnOffHelper onOffHelper("ns3::UdpSocketFactory", ns3::InetSocketAddress(interfaces.GetAddress(1), 9));
  onOffHelper.SetAttribute("DataRate", ns3::StringValue("100Mbps"));
  onOffHelper.SetAttribute("PacketSize", ns3::UintegerValue(1500));

  ns3::ApplicationContainer apps = onOffHelper.Install(nodes.Get(0));
  apps.Start(ns3::Seconds(1.0));
  apps.Stop(ns3::Seconds(20.0));

  ns3::Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  ns3::FlowMonitorHelper flowMonitor;
  ns3::Ptr<ns3::FlowMonitor> monitor = flowMonitor.InstallAll();

  ns3::Simulator::Stop(ns3::Seconds(20.0));
  ns3::Simulator::Run();

  monitor->CheckForLostPackets();

  ns3::Ptr<ns3::Ipv4FlowClassifier> classifier = ns3::DynamicCast<ns3::Ipv4FlowClassifier>(flowMonitor.GetClassifier());
  ns3::FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

  for (auto it = stats.begin(); it != stats.end(); ++it) {
    ns3::Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(it->first);
    std::cout << "Flow (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
    std::cout << "  Tx Packets: " << it->second.txPackets << "\n";
    std::cout << "  Rx Packets: " << it->second.rxPackets << "\n";
    std::cout << "  Throughput: " << it->second.rxBytes * 8.0 / 2.0 / 1000 / 1000 << " Mbps\n";
    std::cout << "  Packet Loss: " << it->second.lostPackets << "\n";
    std::cout << "  Packet Loss Ratio: " << it->second.lostPackets / static_cast<double>(it->second.txPackets) << "\n";
    std::cout << "  Delay: " << it->second.delaySum / it->second.rxPackets << " seconds\n";
  }

  ns3::Simulator::Destroy();

  return 0;
}

