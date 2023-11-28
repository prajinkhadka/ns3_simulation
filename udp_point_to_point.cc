#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/gnuplot.h"

int main() {
  // Creates a container to hold ns3 nodes.
  ns3::NodeContainer nodes;
  // create two nodes
  nodes.Create(2);

  // It creates a point to point link between the nodes.
  // It cteates a config link with data rate of 5Mbps and a channel delay of 2 miiliseconds.
  // Channel delay represnets how fast the signal is transmited. It may be higher for point to pont link with longer wires.

  ns3::PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute("DataRate", ns3::DataRateValue(ns3::DataRate("5Mbps")));
  pointToPoint.SetChannelAttribute("Delay", ns3::TimeValue(ns3::MilliSeconds(2)));

  // Installs the the point to point connection in the nodes.
  ns3::NetDeviceContainer devices;
  devices = pointToPoint.Install(nodes);

  // Installs the point to point devices on nodes.
  ns3::InternetStackHelper stack;
  stack.Install(nodes);

  // This install the internect stack on the nodes. It is necheasry for highr layer protocols ( IP ) to function
  ns3::Ipv4AddressHelper address;
  // base ip is set to 10.1.1.0 and subnet mask with 255.255.255.0
  address.SetBase("10.1.1.0", "255.255.255.0");
  ns3::Ipv4InterfaceContainer interfaces = address.Assign(devices);


  // a on off application that uses UDP for communcation. It will generate UDP traffic.
  ns3::OnOffHelper onOffHelper("ns3::UdpSocketFactory", ns3::InetSocketAddress(interfaces.GetAddress(1), 9));

  onOffHelper.SetAttribute("DataRate", ns3::StringValue("4Mbps"));
  // packet size is set to 1500 bytes.
  onOffHelper.SetAttribute("PacketSize", ns3::UintegerValue(1500));

  // Installs the on-off application on first node and soecifies the start and stop time for the application.
  ns3::ApplicationContainer apps = onOffHelper.Install(nodes.Get(0));
  apps.Start(ns3::Seconds(1.0));
  apps.Stop(ns3::Seconds(20.0));

  // It populates the routing table for IP for warding.
  ns3::Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  // Install and congure the folw level monitor to collect flow level statisitics.
  ns3::FlowMonitorHelper flowMonitor;
  ns3::Ptr<ns3::FlowMonitor> monitor = flowMonitor.InstallAll();

  ns3::Simulator::Stop(ns3::Seconds(20.0));
  ns3::Simulator::Run();

  // Checks for the Lost packets.
  monitor->CheckForLostPackets();

  // Retrieves flow statistics from Flow Monitor
  ns3::Ptr<ns3::Ipv4FlowClassifier> classifier = ns3::DynamicCast<ns3::Ipv4FlowClassifier>(flowMonitor.GetClassifier());
  ns3::FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

  for (auto it = stats.begin(); it != stats.end(); ++it) {
    ns3::Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(it->first);
    std::cout << "Flow (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
    std::cout << "  Tx Packets: " << it->second.txPackets << "\n";
    std::cout << "  Rx Packets: " << it->second.rxPackets << "\n";
    std::cout << "  Throughput: " << it->second.rxBytes * 8.0 / 20.0 / 1000 / 1000 << " Mbps\n"; // changing from bytes  to convert to magabits percsecons.
    // *8 to convert to t bits /20 to account for the fact that sumulation runs for 20 secinds
    // /1000 /1000 converting bits per seconds to megabits per second.
    //
    std::cout << "  Packet Loss: " << it->second.lostPackets << "\n";
    std::cout << "  Packet Loss Ratio: " << it->second.lostPackets / static_cast<double>(it->second.txPackets) << "\n";
    std::cout << "  Delay: " << it->second.delaySum / it->second.rxPackets << " seconds\n";
  }

  ns3::Simulator::Destroy();

  return 0;
}

