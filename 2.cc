#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

int main() {
    NodeContainer nodes;
    nodes.Create(11); // 1 source, 1 Destination, 20 intermediate hops

    // Setting up the links
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", DataRateValue(DataRate("1Gbps")));
    pointToPoint.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));

    // Install point to point links on the devices.
    NetDeviceContainer devices;
    for (uint32_t i = 0; i < 10; ++i) {
        devices.Add(pointToPoint.Install(nodes.Get(i), nodes.Get(i + 1)));
    }

    // Installs internet stack like IP.
    InternetStackHelper internet;
    internet.Install(nodes);

    // It assigns IP address to the devices in the network.
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // Set up a BulkSend application on the destination node (Node 21)
    uint16_t port = 9;
    Address serverAddress(InetSocketAddress(interfaces.GetAddress(10), port));
    BulkSendHelper serverHelper("ns3::TcpSocketFactory", serverAddress);
    ApplicationContainer serverApps = serverHelper.Install(nodes.Get(10));
    serverApps.Start(Seconds(0.0));
    serverApps.Stop(Seconds(10000));

    // Set up a BulkSend application on the first node, i.e., node0.
    BulkSendHelper clientHelper("ns3::TcpSocketFactory", serverAddress);
    clientHelper.SetAttribute("MaxBytes", UintegerValue(100000000000)); // Set MaxBytes to 0 for unlimited bytes

    ApplicationContainer clientApps = clientHelper.Install(nodes.Get(0));
    clientApps.Start(Seconds(1.0));
    clientApps.Stop(Seconds(10000));

    // Install FlowMonitor on all nodes
    FlowMonitorHelper flowMonitor;
    Ptr<FlowMonitor> monitor = flowMonitor.InstallAll();

    // Run the simulation
    Simulator::Stop(Seconds(10000));
    Simulator::Run();

    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowMonitor.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();
    uint64_t totalBytesSent = 0; // Variable to store the total bytes sent

    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i) {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);

        std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
        if (t.sourceAddress == "10.1.1.1" && t.destinationAddress == "10.1.1.11") {
            // Only print metrics for the flow from source (10.1.1.1) to destination (10.1.1.22)
            std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
            double duration = i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds();
            if (duration > 0) {
                std::cout << "  Throughput: " << i->second.txBytes * 8.0 / duration / 1e6 << " Mbps\n";
                totalBytesSent += i->second.txBytes;
                std::cout << "  Packets Sent: " << i->second.txPackets << " packets\n";
            } else {
                std::cout << "  Throughput: N/A (duration is zero or negative)\n";
            }
            std::cout << "  Average Delay: " << i->second.delaySum / i->second.rxPackets << " seconds\n";
            std::cout << "  Packet Loss: " << i->second.lostPackets << " packets\n";
            std::cout << "Simulation Stop: " << Simulator::Now().GetSeconds() << "s\n";
        }
    }
    std::cout << "Total Bytes Sent: " << totalBytesSent << " bytes\n";
    // Cleanup
    Simulator::Destroy();

    return 0;
}

