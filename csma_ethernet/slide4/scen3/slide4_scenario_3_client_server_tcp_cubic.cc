#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ipv4-interface.h"

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("CsmaExample");

int main(int argc, char *argv[])
{
    // Set up some default values for the simulation.
    // These can also be changed if needed to create congestion in the network.
    Config::SetDefault("ns3::OnOffApplication::PacketSize", UintegerValue(1400));
    Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue("2Mbps"));

    uint32_t nClients = 60; // These are the base values.
    double channelDataRate = 2.0; //These are the base values.
    CommandLine cmd;
    cmd.AddValue("nClients", "Number of client nodes", nClients);
    cmd.AddValue("channelDataRate", "Total shared channel data rate (Mbps)", channelDataRate);
    cmd.Parse(argc, argv);

    NS_LOG_INFO("Create nodes.");
    NodeContainer csmaNodes;
    csmaNodes.Create(nClients + 1); // +1 for the server

    NS_LOG_INFO("Create channel.");
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", DataRateValue(DataRate(channelDataRate * 1e6))); // Convert to bps
    csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));

    // set TCP protocol
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpCubic"));

    NetDeviceContainer csmaDevices;
    csmaDevices = csma.Install(csmaNodes);
    AsciiTraceHelper ascii;
    csma.EnableAsciiAll(ascii.CreateFileStream("Slide4_scen3_TcpCubic.tr"));

    csma.EnablePcapAll("csma-example-prajin");

    NS_LOG_INFO("Install internet stack on all nodes.");
    InternetStackHelper internet;
    internet.Install(csmaNodes);

    NS_LOG_INFO("Assign IP Addresses.");
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(csmaDevices);

    NS_LOG_INFO("Create applications.");

    // Server application
    uint16_t serverPort = 50000;
    Address serverAddress(InetSocketAddress(interfaces.GetAddress(nClients), serverPort));
    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", serverAddress);
    ApplicationContainer serverApp = packetSinkHelper.Install(csmaNodes.Get(nClients));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(100.0));

    // Client applications
    OnOffHelper onOffHelper("ns3::TcpSocketFactory", serverAddress);
    onOffHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer clientApps;

for (uint32_t i = 0; i < nClients; ++i)
    {
    ApplicationContainer onOffApp = onOffHelper.Install(csmaNodes.Get(i));
    clientApps.Add(onOffApp);
        
    std::set<uint32_t> selectedIndices = {20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39};
    if (selectedIndices.find(i) != selectedIndices.end()) {
	    onOffApp.Start(Seconds(20.0));
	    onOffApp.Stop(Seconds(100.0));
	    std::cout << "Condition is satisfied for i = " << i << std::endl;
   }

    std::set<uint32_t> selectedIndices_1 = {40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59}; 
    if (selectedIndices_1.find(i) != selectedIndices_1.end()) {
	    onOffApp.Start(Seconds(40.0));
	    onOffApp.Stop(Seconds(100.0));
	    std::cout << "Condition is satisfied for i = " << i << std::endl;
    }
    else{
	onOffApp.Stop(Seconds(100.0));
    }

    }

    NS_LOG_INFO("Run Simulation.");
    Simulator::Run();

    Simulator::Destroy();
    NS_LOG_INFO("Done.");

    return 0;
}

