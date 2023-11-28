#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

int main() {
    NodeContainer nodes;
    nodes.Create(2); // 2 nodes: source and destination

    // Setting up the link
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", DataRateValue(DataRate("1000Mbps")));
    pointToPoint.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));

    // Install point-to-point link on the devices.
    NetDeviceContainer devices;
    devices.Add(pointToPoint.Install(nodes.Get(0), nodes.Get(1)));

    // Installs internet stack like IP.
    InternetStackHelper internet;
    internet.Install(nodes);

    // It assigns IP address to the devices in the network.
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // Set up a UDP Server
    UdpServerHelper udpServer(9);
    ApplicationContainer serverApps = udpServer.Install(nodes.Get(1));
    serverApps.Start(Seconds(0.0));
    serverApps.Stop(Seconds(500));

    // Set up a UDP client on the first node, i.e., node0.
    UdpClientHelper udpClient(interfaces.GetAddress(1), 9);
    udpClient.SetAttribute("MaxPackets", UintegerValue(10000)); // Set MaxPackets to 0 for unlimited packets
    udpClient.SetAttribute("PacketSize", UintegerValue(1400));

    // It sets up a UDP client on the first node i.e. node0. The client application
    // starts at t=1s and stops at t=100s.
    ApplicationContainer clientApps = udpClient.Install(nodes.Get(0));
    std::cout << "Client starting to send packets at t= " << Simulator::Now().GetSeconds() << "s\n";
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(500));

    // Install FlowMonitor on all nodes
    FlowMonitorHelper flowMonitor;
    Ptr<FlowMonitor> monitor = flowMonitor.InstallAll();

    // Run the simulation
    Simulator::Stop(Seconds(500));
    Simulator::Run();
    std::cout << "Client stopped sending packets at t= " << Simulator::Now().GetSeconds() << "s\n";
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowMonitor.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    uint64_t totalBytesSent = 0; // Variable to store the total bytes sent

    for (auto const& entry : stats) {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(entry.first);

        if (t.sourceAddress == "10.1.1.1" && t.destinationAddress == "10.1.1.2") {
            // Only print metrics for the flow from source (10.1.1.1) to destination (10.1.1.2)
            std::cout << "Flow " << entry.first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
            double duration = entry.second.timeLastRxPacket.GetSeconds() - entry.second.timeFirstTxPacket.GetSeconds();
            if (duration > 0) {
                std::cout << "  Throughput: " << entry.second.txBytes * 8.0 / duration / 1e6 << " Mbps\n";
		std::cout << "  Packets Sent: " << entry.second.txPackets << " packets\n";  // Add this line
                totalBytesSent += entry.second.txBytes;
            } else {
                std::cout << "  Throughput: N/A (duration is zero or negative)\n";
            }
            std::cout << "  Average Delay: " << entry.second.delaySum / entry.second.rxPackets << " seconds\n";
            std::cout << "  Packet Loss: " << entry.second.lostPackets << " packets\n";
            std::cout << "Simulation Stop: " << Simulator::Now().GetSeconds() << "s\n";
        }
    }

    std::cout << "Total Bytes Sent: " << totalBytesSent << " bytes\n";

    // Cleanup
    Simulator::Destroy();

    return 0;
}

