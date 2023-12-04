#include <string>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/tcp-dctcp.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpBulkSendExample");

int main(int argc, char *argv[]) {

    bool tracing = false;
    uint32_t maxBytes = 0;

    CommandLine cmd;
    cmd.AddValue("tracing", "Flag to enable/disable tracing", tracing);
    cmd.AddValue("maxBytes", "Total number of bytes for application to send", maxBytes);
    cmd.Parse(argc, argv);

    NS_LOG_INFO("Create nodes.");
    NodeContainer nodes;
    nodes.Create(2);

    NS_LOG_INFO("Create channels.");

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Kbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("5ms"));

    NetDeviceContainer devices;
    devices = pointToPoint.Install(nodes);
    InternetStackHelper internet;
    internet.Install(nodes);

    NS_LOG_INFO("Assign IP Addresses.");
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i = ipv4.Assign(devices);

    NS_LOG_INFO("Create Applications.");

    uint16_t port = 9;
    
    // set TCP protocol
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpDctcp"));

    BulkSendHelper source("ns3::TcpSocketFactory",
                          InetSocketAddress(i.GetAddress(1), port));
    source.SetAttribute("MaxBytes", UintegerValue(maxBytes));
    ApplicationContainer sourceApps = source.Install(nodes.Get(0));
    sourceApps.Start(Seconds(0.0));
    sourceApps.Stop(Seconds(50.0));

    PacketSinkHelper sink("ns3::TcpSocketFactory",
                          InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApps = sink.Install(nodes.Get(1));
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(50.0));

    if (tracing) {
        AsciiTraceHelper ascii;
        pointToPoint.EnableAsciiAll(ascii.CreateFileStream("tcp-dctcp-bulk-send.tr"));
        pointToPoint.EnablePcapAll("tcp-bulk-send", false);
    }

    FlowMonitorHelper flowMonitor;
    Ptr<FlowMonitor> monitor = flowMonitor.InstallAll();

    NS_LOG_INFO("Run Simulation.");
    Simulator::Stop(Seconds(50.0));
    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_INFO("Done.");

    Ptr<PacketSink> sink1 = DynamicCast<PacketSink>(sinkApps.Get(0));
    std::cout << "Total Bytes Received: " << sink1->GetTotalRx() << std::endl;

    // Print FlowMonitor metrics
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowMonitor.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    for (auto const& entry : stats) {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(entry.first);

        if (t.sourceAddress == "10.1.1.1" && t.destinationAddress == "10.1.1.2") {
            std::cout << "Flow " << entry.first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
            double duration = entry.second.timeLastRxPacket.GetSeconds() - entry.second.timeFirstTxPacket.GetSeconds();
            if (duration > 0) {
                std::cout << "  Throughput: " << entry.second.txBytes * 8.0 / duration / 1e6 << " Mbps\n";
            	std::cout << "  Total Data Sent: " << entry.second.txBytes << " bytes\n"; // Add this line
	    	std::cout << "  Packets Sent: " << entry.second.txPackets << " packets\n";  // Add this line

            } else {
                std::cout << "  Throughput: N/A (duration is zero or negative)\n";
            }
            std::cout << "  Average Delay: " << entry.second.delaySum / entry.second.rxPackets << " seconds\n";
            std::cout << "  Packet Loss: " << entry.second.lostPackets << " packets\n";
            std::cout << "Simulation Stop: " << Simulator::Now().GetSeconds() << "s\n";
        }
    }

    return 0;
}

