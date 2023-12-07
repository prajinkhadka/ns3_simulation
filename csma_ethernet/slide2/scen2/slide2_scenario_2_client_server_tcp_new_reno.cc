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
    Config::SetDefault("ns3::OnOffApplication::PacketSize", UintegerValue(1400));
    Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue("2Mbps"));

    uint32_t nClients = 50; // Number of client nodes
    double channelDataRate = 2.0; // Total shared channel data rate in Mbps

    CommandLine cmd;
    cmd.AddValue("nClients", "Number of client nodes", nClients);
    cmd.AddValue("channelDataRate", "Total shared channel data rate (Mbps)", channelDataRate);
    cmd.Parse(argc, argv);

    // set TCP protocol
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpNewReno"));
    
    NS_LOG_INFO("Create nodes.");
    NodeContainer csmaNodes;
    csmaNodes.Create(nClients + 1); // +1 for the server

    NS_LOG_INFO("Create channel.");
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", DataRateValue(DataRate(channelDataRate * 1e6))); // Convert to bps
    csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));


    NetDeviceContainer csmaDevices;
    csmaDevices = csma.Install(csmaNodes);
    AsciiTraceHelper ascii;
    csma.EnableAsciiAll(ascii.CreateFileStream("Slide2_scen2_TcpNewReno.tr"));

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
    serverApp.Stop(Seconds(10.0));

    // Client applications
    OnOffHelper onOffHelper("ns3::TcpSocketFactory", serverAddress);
    onOffHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer clientApps;

for (uint32_t i = 0; i < nClients; ++i)
{
    ApplicationContainer onOffApp = onOffHelper.Install(csmaNodes.Get(i));
    std::set<uint32_t> selectedIndices = {0,1,2,3,4,5,6,7,8,9};
    clientApps.Add(onOffApp);

    if (selectedIndices.find(i) != selectedIndices.end()) {
	    onOffApp.Start(Seconds(1.0));
	    onOffApp.Stop(Seconds(2.0));
	    std::cout << "Condition is satisfied for i = " << i << std::endl;
   }

    std::set<uint32_t> selectedIndices_1 = {20,21,22,23,24,25,26,27,28,29};
    if (selectedIndices_1.find(i) != selectedIndices_1.end()) {
	    onOffApp.Start(Seconds(4.0));
	    onOffApp.Stop(Seconds(10.0));
	    std::cout << "Condition is satisfied for i = " << i << std::endl;
    }
    else{
	onOffApp.Stop(Seconds(10.0));
    }
}

    NS_LOG_INFO("Run Simulation.");
    Simulator::Run();

    Simulator::Destroy();
    NS_LOG_INFO("Done.");

    return 0;
}
