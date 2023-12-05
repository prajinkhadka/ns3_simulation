#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

int main() {
  // Create nodes
  NodeContainer nodes;
  nodes.Create(3);

  // Create point-to-point links
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
  p2p.SetChannelAttribute("Delay", StringValue("2ms"));

  NetDeviceContainer devices;
  devices = p2p.Install(nodes);

  // Install internet stack on nodes
  InternetStackHelper internet;
  internet.Install(nodes);

  // Assign IP addresses to devices
  Ipv4AddressHelper ipv4;
  ipv4.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

  // Add TCP application on Node 0 (source) to Node 2 (destination)
  BulkSendHelper source("ns3::TcpSocketFactory", InetSocketAddress(interfaces.GetAddress(2), 9));
  source.SetAttribute("MaxBytes", UintegerValue(1000000));
  ApplicationContainer sourceApps = source.Install(nodes.Get(0));
  sourceApps.Start(Seconds(1.0));
  sourceApps.Stop(Seconds(10.0));

  // Add PacketSink application on Node 2
  PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(interfaces.GetAddress(2), 9));
  ApplicationContainer sinkApps = sink.Install(nodes.Get(2));
  sinkApps.Start(Seconds(0.0));
  sinkApps.Stop(Seconds(10.0));

  // Add static routing from Node 0 to Node 2 via Node 1
  Ipv4StaticRoutingHelper staticRouting;
  Ptr<Ipv4StaticRouting> staticRoutingTable = staticRouting.GetStaticRouting(nodes.Get(0)->GetObject<Ipv4>());
  staticRoutingTable->AddHostRouteTo(interfaces.GetAddress(2), 1); // Route to Node 2 via Node 1

  // Set up tracing
  AsciiTraceHelper ascii;
  p2p.EnableAsciiAll(ascii.CreateFileStream("multi_hop_tcp.tr"));

  // Run the simulation
  Simulator::Run();

  // Print total received bytes at the sink
  Ptr<PacketSink> sinkApp = DynamicCast<PacketSink>(sinkApps.Get(0));
  std::cout << "Total Bytes Received at Node 2: " << sinkApp->GetTotalRx() << std::endl;

  Simulator::Destroy();

  return 0;
}

