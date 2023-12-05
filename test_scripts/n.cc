#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

int main(int argc, char *argv[])
{
    // Create nodes
    NodeContainer nodes;
    nodes.Create(6);

    // Create point-to-point channels
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer devices01, devices12, devices23, devices34, devices45;
    devices01 = p2p.Install(nodes.Get(0), nodes.Get(1));
    devices12 = p2p.Install(nodes.Get(1), nodes.Get(2));
    devices23 = p2p.Install(nodes.Get(2), nodes.Get(3));
    devices34 = p2p.Install(nodes.Get(3), nodes.Get(4));
    devices45 = p2p.Install(nodes.Get(4), nodes.Get(5));

    // Install the internet stack on the nodes
    InternetStackHelper stack;
    stack.Install(nodes);

    // Assign IP addresses
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces01 = address.Assign(devices01);
    Ipv4InterfaceContainer interfaces12 = address.Assign(devices12);
    Ipv4InterfaceContainer interfaces23 = address.Assign(devices23);
    Ipv4InterfaceContainer interfaces34 = address.Assign(devices34);
    Ipv4InterfaceContainer interfaces45 = address.Assign(devices45);

    // Create a TCP sink application on the last node (node 5)
    uint16_t port = 9; // well-known echo port number
    Address sinkAddress(InetSocketAddress(interfaces45.GetAddress(0), port));
    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", sinkAddress);
    ApplicationContainer sinkApps = packetSinkHelper.Install(nodes.Get(5));
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(100.0)); // simulation duration

    // Create a TCP source application on the first node (node 0)
    Address sourceAddress(InetSocketAddress(interfaces01.GetAddress(1), port));
    OnOffHelper onoff("ns3::TcpSocketFactory", sourceAddress);
    onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    onoff.SetAttribute("PacketSize", UintegerValue(1024));
    onoff.SetAttribute("DataRate", StringValue("5Mbps"));

    ApplicationContainer apps = onoff.Install(nodes.Get(0));
    apps.Start(Seconds(1.0));
    apps.Stop(Seconds(100.0)); // simulation duration

    // Flow monitor
    FlowMonitorHelper flowMonitor;
    Ptr<FlowMonitor> monitor = flowMonitor.InstallAll();

    // Run the simulation
    Simulator::Stop(Seconds(100.0)); // stop simulation after 1000 seconds
    Simulator::Run();

    // Print total bytes received by the last node (node 5)
    Ptr<PacketSink> sink5 = DynamicCast<PacketSink>(sinkApps.Get(0));
    std::cout << "Total Bytes Received by Node 5: " << sink5->GetTotalRx() << std::endl;

    // Cleanup
    Simulator::Destroy();

    return 0;
}

