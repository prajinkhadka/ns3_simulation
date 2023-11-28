#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

void DeleteNode(Ptr<Node> node) {
    // Delete the node
    node->Dispose();
    std::cout << "Node deleted at t = " << Simulator::Now() << std::endl;
}

int main() {
    // Initialize NS-3

    // Create nodes
    NodeContainer nodes;
    nodes.Create(2);

    Ptr<Node> nodeToDelete = nodes.Get(1); // Get the second node for deletion

    // Create a point-to-point link
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer devices = pointToPoint.Install(nodes);

    // Install the Internet stack on the nodes
    InternetStackHelper stack;
    stack.Install(nodes);    // Assign IP addresses to the devices
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // Create a simple UDP application to generate traffic
    UdpEchoClientHelper echoClient(interfaces.GetAddress(1), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(1));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps = echoClient.Install(nodes.Get(0));
    clientApps.Start(Seconds(1.0));
    clientApps.Stop(Seconds(1000.0)); // Stop the traffic generation at t = 5 seconds

    // Create a packet sink application to receive the traffic
    UdpEchoServerHelper echoServer(9);
    ApplicationContainer serverApps = echoServer.Install(nodes.Get(1));
    serverApps.Start(Seconds(0.0));
    serverApps.Stop(Seconds(1000.0)); // Stop the packet sink at t = 10 seconds

    // Schedule node deletion after 5 seconds
    //Simulator::Schedule(Seconds(5.0), &DeleteNode, nodeToDelete);


    // Create a packet trace file
    AsciiTraceHelper ascii;
    pointToPoint.EnableAsciiAll(ascii.CreateFileStream("point-to-point_no_dispose.tr"));

    // Stop the simulation after scheduling node deletion
    Simulator::Stop();
        // Print flow statistics


    // Run the simulation
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}

