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
    Config::SetDefault("ns3::OnOffApplication::PacketSize", UintegerValue(137)); // Keep like scenario 2.a
    Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue("14kb/s")); // Keep like scenario 2.a

    uint32_t nClients = 30; // Keep like scenarion 2.a
    double channelDataRate = 2.5; // Data rate is halaved as comapred to scenario 2.a so that we can see how much decrease in throughput/other metrics in multiple tcp varaint when data rate is havled. Does through put also halves ? or by how much it decreases ? 

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


    NetDeviceContainer csmaDevices;
    csmaDevices = csma.Install(csmaNodes);
    AsciiTraceHelper ascii;
    csma.EnableAsciiAll(ascii.CreateFileStream("test/csma-trace_prajin.tr"));

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
    clientApps.Add(onOffApp);
    onOffApp.Start(Seconds(1.0));
    onOffApp.Stop(Seconds(10.0));

    }

    NS_LOG_INFO("Run Simulation.");
    Simulator::Run();

    Simulator::Destroy();
    NS_LOG_INFO("Done.");

    return 0;
}

