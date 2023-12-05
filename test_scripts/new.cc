#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

int main() {
    NodeContainer nodes;
    nodes.Create(22); // 1 source, 1 Destination, 20 intermediate hops

    // Setting up the links
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", DataRateValue(DataRate("1000Mbps")));
    pointToPoint.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));

    // Install point-to-point links on the devices.
    NetDeviceContainer devices;
    for (uint32_t i = 0; i < 21; ++i) {
        devices.Add(pointToPoint.Install(nodes.Get(i), nodes.Get(i + 1)));
    }

    // Installs internet stack like IP.
    InternetStackHelper internet;
    internet.Install(nodes);

    // It assigns IP address to the devices in the network.
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");    
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // Set up a TCP Server
    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", InetSocketAddress(interfaces.GetAddress(21), 9));
    ApplicationContainer serverApps = packetSinkHelper.Install(nodes.Get(21));
    serverApps.Start(Seconds(0.0));
    serverApps.Stop(Seconds(100));

    // Set up a TCP client on the first node, i.e., node0.
    OnOffHelper onoff("ns3::TcpSocketFactory", InetSocketAddress(interfaces.GetAddress(21), 9));
    onoff.SetAttribute("DataRate", DataRateValue(DataRate("1000Mbps")));
    onoff.SetAttribute("PacketSize", UintegerValue(1400));

    // It sets up a TCP client on the first node i.e. node0. The client application
    // starts at t=1s and stops at t=100s.
    ApplicationContainer clientApps = onoff.Install(nodes.Get(0));
    clientApps.Start(Seconds(1.0));
    clientApps.Stop(Seconds(100));

    // Install FlowMonitor on all nodes
    FlowMonitorHelper flowMonitor;
    Ptr<FlowMonitor> monitor = flowMonitor.InstallAll();

    // Run the simulation
    Simulator::Stop(Seconds(100));
    Simulator::Run();

    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowMonitor.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();
    uint64_t totalBytesSent = 0; // Variable to store the total bytes sent

    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i) {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);

        if (t.sourceAddress == "10.1.1.1" && t.destinationAddress == "10.1.1.22") {
            // Only print metrics for the flow from source (10.1.1.1) to destination (10.1.1.22)
            std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
            double duration = i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds();
            if (duration > 0) {
                std::cout << "  Throughput: " << i->second.txBytes * 8.0 / duration / 1e6 << " Mbps\n";
                totalBytesSent += i->second.txBytes;
            } else {
                std::cout << "  Throughput: N/A (duration is zero or negative)\n";
            }
            std::cout << "  Average Delay: " << i->second.delaySum / i->second.rxPackets << " seconds\n";
            //std::cout << "  Jitter: " << i->second.jitterSum / (i->second.rxPackets - 1) << " seconds\n";
            std::cout << "  Packet Loss: " << i->second.lostPackets << " packets\n";
            std::cout << "Simulation Stop: " << Simulator::Now().GetSeconds() << "s\n";
	    Ptr<PacketSink> sink21 = DynamicCast<PacketSink>(serverApps.Get(0));
	    std::cout << "Total Bytes Received by Node 21: " << sink21->GetTotalRx() << std::endl;
        }
    }
    std::cout << "Total Bytes Sent: " << totalBytesSent << " bytes\n";
    // Cleanup
    Simulator::Destroy();

    return 0;
}

