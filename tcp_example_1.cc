#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

#include "ns3/tcp-cubic.h"
#include "ns3/tcp-bic.h"
#include "ns3/tcp-dctcp.h"
#include "ns3/tcp-highspeed.h"
#include "ns3/tcp-linux-reno.h"
#include "ns3/tcp-scalable.h"
#include "ns3/tcp-hybla.h"
#include "ns3/tcp-illinois.h"
#include "ns3/tcp-ledbat.h"
#include "ns3/tcp-vegas.h"
#include "ns3/tcp-lp.h"
#include "ns3/tcp-veno.h"
#include "ns3/tcp-westwood.h"
#include "ns3/tcp-yeah.h"
#include "ns3/tcp-bbr.h"







int main() {
  // Creates a container to hold ns3 nodes.
  ns3::NodeContainer nodes;
  // create two nodes
  nodes.Create(2);

  // It creates a point-to-point link between the nodes.
  // It creates a config link with a data rate of 5Mbps and a channel delay of 2 milliseconds.
  ns3::PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute("DataRate", ns3::DataRateValue(ns3::DataRate("5Mbps")));
  pointToPoint.SetChannelAttribute("Delay", ns3::TimeValue(ns3::MilliSeconds(2)));

  // Installs the point-to-point connection on the nodes.
  ns3::NetDeviceContainer devices;
  devices = pointToPoint.Install(nodes);

  // Installs the Internet stack on the nodes. Necessary for higher layer protocols (e.g., IP) to function.
  ns3::InternetStackHelper stack;
  stack.Install(nodes);

  // This installs the Internet stack on the nodes. It is necessary for higher layer protocols (IP) to function.
  ns3::Ipv4AddressHelper address;
  // Base IP is set to 10.1.1.0 and subnet mask with 255.255.255.0
  address.SetBase("10.1.1.0", "255.255.255.0");
  ns3::Ipv4InterfaceContainer interfaces = address.Assign(devices);

  // Set the TCP variant to Cubic
  //ns3::Config::Set("/NodeList/0/$ns3::TcpL4Protocol/SocketType", ns3::TypeIdValue(ns3::TcpLinuxReno::GetTypeId()));
  //ns3::Config::Set("/NodeList/0/$ns3::TcpL4Protocol/SocketType", ns3::TypeIdValue(ns3::TcpDctcp::GetTypeId()));
  //ns3::Config::Set("/NodeList/0/$ns3::TcpL4Protocol/SocketType", ns3::TypeIdValue(ns3::TcpHighSpeed::GetTypeId()));

  //ns3::Config::Set("/NodeList/0/$ns3::TcpL4Protocol/SocketType", ns3::TypeIdValue(ns3::TcpBic::GetTypeId()));
  //ns3::Config::Set("/NodeList/0/$ns3::TcpL4Protocol/SocketType", ns3::TypeIdValue(ns3::TcpCubic::GetTypeId()));
  // ns3::Config::Set("/NodeList/0/$ns3::TcpL4Protocol/SocketType", ns3::TypeIdValue(ns3::TcpScalable::GetTypeId()));
  // ns3::Config::Set("/NodeList/0/$ns3::TcpL4Protocol/SocketType", ns3::TypeIdValue(ns3::TcpHybla::GetTypeId()));
  // ns3::Config::Set("/NodeList/0/$ns3::TcpL4Protocol/SocketType", ns3::TypeIdValue(ns3::TcpIllinois::GetTypeId()));
 // ns3::Config::Set("/NodeList/0/$ns3::TcpL4Protocol/SocketType", ns3::TypeIdValue(ns3::TcpLedbat::GetTypeId()));
 // ns3::Config::Set("/NodeList/0/$ns3::TcpL4Protocol/SocketType", ns3::TypeIdValue(ns3::TcpVegas::GetTypeId()));
 //ns3::Config::Set("/NodeList/0/$ns3::TcpL4Protocol/SocketType", ns3::TypeIdValue(ns3::TcpLp::GetTypeId()));
 //ns3::Config::Set("/NodeList/0/$ns3::TcpL4Protocol/SocketType", ns3::TypeIdValue(ns3::TcpVeno::GetTypeId()));
 //ns3::Config::Set("/NodeList/0/$ns3::TcpL4Protocol/SocketType", ns3::TypeIdValue(ns3::TcpWestwood::GetTypeId()));
 ns3::Config::Set("/NodeList/0/$ns3::TcpL4Protocol/SocketType", ns3::TypeIdValue(ns3::TcpYeah::GetTypeId()));









  // a BulkSend application using TCP Cubic
  ns3::BulkSendHelper source("ns3::TcpSocketFactory", ns3::InetSocketAddress(interfaces.GetAddress(1), 9));
  source.SetAttribute("MaxBytes", ns3::UintegerValue(0)); // Send as much as possible

  // Install the BulkSend application on the first node and set start/stop time
  ns3::ApplicationContainer apps = source.Install(nodes.Get(0));
  apps.Start(ns3::Seconds(1.0));
  apps.Stop(ns3::Seconds(20.0));

  // It populates the routing table for IP forwarding.
  ns3::Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  // Install and configure the flow-level monitor to collect flow-level statistics.
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
    std::cout << "  Throughput: " << it->second.rxBytes * 8.0 / 20.0 / 1000 / 1000 << " Mbps\n";
    std::cout << "  Packet Loss: " << it->second.lostPackets << "\n";
    std::cout << "  Packet Loss Ratio: " << it->second.lostPackets / static_cast<double>(it->second.txPackets) << "\n";
    std::cout << "  Delay: " << it->second.delaySum / it->second.rxPackets << " seconds\n";
  }

  ns3::Simulator::Destroy();
  return 0;
}

