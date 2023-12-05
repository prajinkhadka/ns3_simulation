#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("LargeFileTransfer");

int main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  // Set up nodes
  NodeContainer nodes;
  nodes.Create (2);

  // Set up point-to-point link
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  // Set up internet stack
  InternetStackHelper stack;
  stack.Install (nodes);

  // Assign IP addresses
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  // Set up UDP client and server
  UdpServerHelper server (9);
  ApplicationContainer serverApps = server.Install (nodes.Get (1));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpClientHelper client (interfaces.GetAddress (1), 9);
  client.SetAttribute ("MaxPackets", UintegerValue (1));
  client.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  client.SetAttribute ("PacketSize", UintegerValue (2048)); // Adjust packet size as needed
  ApplicationContainer clientApps = client.Install (nodes.Get (0));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  // Enable packet capturing on the server
  pointToPoint.EnablePcapAll ("large_file_transfer");

  // Run the simulation
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

