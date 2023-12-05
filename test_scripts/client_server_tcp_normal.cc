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

int main(int argc, char *argv[])
{
    // Set up some default values for the simulation.
    Config::SetDefault("ns3::OnOffApplication::PacketSize", UintegerValue(137));
    Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue("14kb/s"));

    // Default number of nodes in the star.  Overridable by command line argument.
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


    // Create a packet sink on the star "hub" to receive packets.
    uint16_t port = 50000;
    Address hubLocalAddress(InetSocketAddress(Ipv4Address::GetAny(), port));
    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", hubLocalAddress);
    ApplicationContainer hubApp = packetSinkHelper.Install(star.GetHub());
    hubApp.Start(Seconds(1.0));
    hubApp.Stop(Seconds(10.0));

    // Create OnOff applications to send TCP to the hub, one on each spoke node.
    OnOffHelper onOffHelper("ns3::TcpSocketFactory", Address());
    onOffHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer spokeApps;

    for (uint32_t i = 0; i < star.SpokeCount(); ++i)
    {
        AddressValue remoteAddress(InetSocketAddress(star.GetHubIpv4Address(i), port));
        onOffHelper.SetAttribute("Remote", remoteAddress);
        spokeApps.Add(onOffHelper.Install(star.GetSpokeNode(i)));
    }

    spokeApps.Start(Seconds(1.0));
    spokeApps.Stop(Seconds(10.0));

    NS_LOG_INFO("Enable static global routing.");
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    NS_LOG_INFO("Enable pcap tracing.");
    pointToPoint.EnablePcapAll("star");

    // Add Flow Monitor
    NS_LOG_INFO("Enable Flow Monitor.");
    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();

    NS_LOG_INFO("Run Simulation.");
    Simulator::Run();

    // Print Flow Monitor statistics
    flowMonitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats();
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
        std::cout << "Flow ID: " << i->first << " Src Addr " << t.sourceAddress << " Dst Addr " << t.destinationAddress << "\n";
        std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
        std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
        std::cout << "  Lost Packets: " << i->second.lostPackets << "\n";
        std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / 10.0 / 1024 / 1024 << " Mbps\n";
        std::cout << "  Delay: " << i->second.delaySum / i->second.rxPackets * 1000 << " ms\n";
        std::cout << "  Jitter: " << i->second.jitterSum / (i->second.rxPackets - 1) * 1000 << " ms\n";
    }


    Simulator::Destroy();
    NS_LOG_INFO("Done.");

    return 0;
}

