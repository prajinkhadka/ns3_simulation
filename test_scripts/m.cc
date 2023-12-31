#include <string>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/flow-monitor-module.h"

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
    nodes.Create(5); // 0 to 22 nodes

    NS_LOG_INFO("Create channels.");

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("10ms"));
pointToPoint.SetDeviceAttribute("Mtu", UintegerValue(1500)); // Set MTU to 1500 bytes
    //pointToPoint.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("500p"));
    pointToPoint.EnablePcapAll("output-file-prefix");

    NetDeviceContainer devices;

    // Connect nodes with point-to-point channels
    for (uint32_t i = 0; i < 4; ++i) {
	 
        std::cout << " I is " << i;
        devices.Add(pointToPoint.Install(nodes.Get(i), nodes.Get(i + 1)));
    }
    InternetStackHelper internet;
    internet.Install(nodes);
    Ipv4StaticRoutingHelper staticRouting;


    NS_LOG_INFO("Assign IP Addresses.");

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

for (uint32_t i = 0; i < nodes.GetN(); ++i) {
     std::cout << "  Node " << i << interfaces.GetAddress(i);
}
    NS_LOG_INFO("Create Applications.");

    uint16_t port = 9;

    // Source application on node 0
    BulkSendHelper source("ns3::TcpSocketFactory",
                          InetSocketAddress(interfaces.GetAddress(1), port));
    source.SetAttribute("MaxBytes", UintegerValue(maxBytes));
    ApplicationContainer sourceApps = source.Install(nodes.Get(0));
    sourceApps.Start(Seconds(0.0));
    sourceApps.Stop(Seconds(3000.0));

    // Log packet transmissions
for (uint32_t i = 0; i < 4; ++i) {
    NS_LOG_DEBUG("Packet sent from Node " << i << " to Node " << (i + 1) << " at time " << Simulator::Now());
}

    // Sink application on node 22
    PacketSinkHelper sink("ns3::TcpSocketFactory",
                          InetSocketAddress(interfaces.GetAddress(0), port));
    ApplicationContainer sinkApps = sink.Install(nodes.Get(1));
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(3000.0));


    if (tracing) {
        AsciiTraceHelper ascii;
        pointToPoint.EnableAsciiAll(ascii.CreateFileStream("tcp-bulk-send.tr"));
        pointToPoint.EnablePcapAll("tcp-bulk-send", false);
    }

    FlowMonitorHelper flowMonitor;
    Ptr<FlowMonitor> monitor = flowMonitor.InstallAll();

    NS_LOG_INFO("Run Simulation.");
    Simulator::Stop(Seconds(3000.0));
    Simulator::Run();
    // Check if the ping was successful

    NS_LOG_INFO("Done.");

    Ptr<PacketSink> sink1 = DynamicCast<PacketSink>(sinkApps.Get(0));
    std::cout << " \n Total Bytes Received: " << sink1->GetTotalRx() << std::endl;
    

    // Print FlowMonitor metrics
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowMonitor.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    for (auto const& entry : stats) {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(entry.first);

        std::cout << "Flow " << entry.first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
        if (t.sourceAddress == "10.1.1.1" && t.destinationAddress == "10.1.1.5") {
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

    Simulator::Destroy();
    return 0;
}

