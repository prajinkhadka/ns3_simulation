#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

int main() {
  ns3::NodeContainer nodes;
  nodes.Create(4); // Create 4 nodes

  // Create point-to-point links
  ns3::PointToPointHelper pointToPoint1;
  pointToPoint1.SetDeviceAttribute("DataRate", ns3::DataRateValue(ns3::DataRate("100Mbps")));
  pointToPoint1.SetChannelAttribute("Delay", ns3::TimeValue(ns3::MilliSeconds(1)));
  ns3::NetDeviceContainer devices1 = pointToPoint1.Install(nodes.Get(0), nodes.Get(1));

  ns3::PointToPointHelper pointToPoint2;
  pointToPoint2.SetDeviceAttribute("DataRate", ns3::DataRateValue(ns3::DataRate("100Mbps")));
  pointToPoint2.SetChannelAttribute("Delay", ns3::TimeValue(ns3::MilliSeconds(1)));
  ns3::NetDeviceContainer devices2 = pointToPoint2.Install(nodes.Get(2), nodes.Get(3));

  ns3::InternetStackHelper stack;
  stack.Install(nodes);

  ns3::Ipv4AddressHelper address;
  address.SetBase("10.1.1.0", "255.255.255.0");
  ns3::Ipv4InterfaceContainer interfaces1 = address.Assign(devices1);
  ns3::Ipv4InterfaceContainer interfaces2 = address.Assign(devices2);

  // Real-time traffic pattern (Constant Bit Rate - CBR)
  ns3::OnOffHelper onOffHelper1("ns3::UdpSocketFactory", ns3::InetSocketAddress(interfaces1.GetAddress(1), 9));
  onOffHelper1.SetAttribute("DataRate", ns3::StringValue("100Mbps"));
  onOffHelper1.SetAttribute("PacketSize", ns3::UintegerValue(1500));
  onOffHelper1.SetAttribute("OnTime", ns3::StringValue("ns3::ConstantRandomVariable[Constant=1]"));
  onOffHelper1.SetAttribute("OffTime", ns3::StringValue("ns3::ConstantRandomVariable[Constant=0]"));
  ns3::ApplicationContainer apps1 = onOffHelper1.Install(nodes.Get(0));
  apps1.Start(ns3::Seconds(1.0));
  apps1.Stop(ns3::Seconds(2.0));

  ns3::OnOffHelper onOffHelper2("ns3::UdpSocketFactory", ns3::InetSocketAddress(interfaces2.GetAddress(1), 9));
  onOffHelper2.SetAttribute("DataRate", ns3::StringValue("100Mbps"));
  onOffHelper2.SetAttribute("PacketSize", ns3::UintegerValue(1500));
  onOffHelper2.SetAttribute("OnTime", ns3::StringValue("ns3::ConstantRandomVariable[Constant=1]"));
  onOffHelper2.SetAttribute("OffTime", ns3::StringValue("ns3::ConstantRandomVariable[Constant=0]"));
  ns3::ApplicationContainer apps2 = onOffHelper2.Install(nodes.Get(2));
  apps2.Start(ns3::Seconds(1.0));
  apps2.Stop(ns3::Seconds(2.0));

  ns3::Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  ns3::FlowMonitorHelper flowMonitor;
  ns3::Ptr<ns3::FlowMonitor> monitor = flowMonitor.InstallAll();

  ns3::Simulator::Stop(ns3::Seconds(2.0));
  ns3::Simulator::Run();

  monitor->CheckForLostPackets();

  ns3::Ptr<ns3::Ipv4FlowClassifier> classifier = ns3::DynamicCast<ns3::Ipv4FlowClassifier>(flowMonitor.GetClassifier());
  ns3::FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

  for (auto it = stats.begin(); it != stats.end(); ++it) {
    ns3::Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(it->first);
    std::cout << "Flow (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
    std::cout << "  Tx Packets: " << it->second.txPackets << "\n";
    std::cout << "  Rx Packets: " << it->second.rxPackets << "\n";
    std::cout << "  Throughput: " << it->second.rxBytes * 8.0 / 2.0 / 1000 / 1000 << " Mbps\n"; // Divide by 2 since the simulation time is 2 seconds
    std::cout << "  Packet Loss: " << it->second.lostPackets << "\n";
    std::cout << "  Packet Loss Ratio: " << it->second.lostPackets / static_cast<double>(it->second.txPackets) << "\n";
    std::cout << "  Delay: " << it->second.delaySum / it->second.rxPackets << " seconds\n";
  }

  ns3::Simulator::Destroy();

  return 0;
}

