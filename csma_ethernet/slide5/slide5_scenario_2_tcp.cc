#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TcpExample");

class MyApp : public Application
{
public:
  MyApp ();
  virtual ~MyApp ();

  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
};

MyApp::MyApp ()
  : m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0)
{
}

MyApp::~MyApp ()
{
  m_socket = 0;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendPacket ();
}

void
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

void
MyApp::SendPacket (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  m_socket->Send (packet);

  if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }
}

void
MyApp::ScheduleTx (void)
{
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
      m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
    }
}

static void
CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
  //NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << newCwnd);
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldCwnd << "\t" << newCwnd << std::endl;
}

void UpdateDataRate(Ptr<NetDevice> device, DataRate newRate)
{
  Ptr<PointToPointNetDevice> p2pDevice = DynamicCast<PointToPointNetDevice>(device);
  if (p2pDevice)
  {
    p2pDevice->SetDataRate(newRate);
  }
  else
  {
    NS_LOG_ERROR("Invalid NetDevice type for UpdateDataRate");
  }
}


int
main (int argc, char *argv[])
{
  // THe bandwidth of point ot point link is setup as 2Mbps.
  std::string bandwidth = "2Mbps";
  std::string delay = "5ms";
  std::string queuesize = "10p";
  double error_rate = 0.000001;

  int simulation_time = 10; //seconds

  // Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpCubic"));


  NodeContainer n0n1;
  n0n1.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue (bandwidth));
  pointToPoint.SetChannelAttribute ("Delay", StringValue (delay));

  pointToPoint.SetQueue ("ns3::DropTailQueue",
              "MaxSize", StringValue (queuesize));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (n0n1);

  NodeContainer n1n2;
  n1n2.Add (n0n1.Get (1));
  n1n2.Create (1);

  PointToPointHelper pointToPoint2;
  pointToPoint2.SetDeviceAttribute ("DataRate", StringValue (bandwidth));
  pointToPoint2.SetChannelAttribute ("Delay", StringValue (delay));

  pointToPoint2.SetQueue ("ns3::DropTailQueue",
             "MaxSize", StringValue (queuesize));

  NetDeviceContainer devices2;
  devices2= pointToPoint2.Install (n1n2);

  Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  em->SetAttribute ("ErrorRate", DoubleValue (error_rate));
  devices.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

  InternetStackHelper stack;
   stack.InstallAll ();
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.252");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  Ipv4AddressHelper address2;
  address2.SetBase ("10.1.2.0", "255.255.255.252");
  Ipv4InterfaceContainer interfaces2 = address2.Assign (devices2);
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();


  uint16_t sinkPort = 8080;
  Address sinkAddress (InetSocketAddress (interfaces2.GetAddress (1), sinkPort));
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
  ApplicationContainer sinkApps = packetSinkHelper.Install (n1n2.Get (1));
  sinkApps.Start (Seconds (0.));
  sinkApps.Stop (Seconds (simulation_time));

  Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (n0n1.Get (0), TcpSocketFactory::GetTypeId ());

  Ptr<MyApp> app = CreateObject<MyApp> ();
  // The data rate is senin the speed of 100Mbps. This remains constant.
  app->Setup (ns3TcpSocket, sinkAddress, 1460, 1000000, DataRate ("100Mbps"));
  n0n1.Get (0)->AddApplication (app);
  app->SetStartTime (Seconds (1.));
  app->SetStopTime (Seconds (simulation_time));

  //trace cwnd
  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream ("test/tcp-example_data_rate_changed.cwnd");
  ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream));


  AsciiTraceHelper ascii;
  pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("test/tcp-example.tr"));

  // Schedule to update the data rate at t=2s, For now i have used , t=2 and t=4 since total simualtion time is 10s.
  // Need to change this 
  
  Simulator::Schedule(Seconds(2.0), &UpdateDataRate, devices.Get(1), DataRate("1Mbps"));
  Simulator::Schedule(Seconds(2.0), &UpdateDataRate, devices2.Get(1), DataRate("1Mbps"));

  Simulator::Schedule(Seconds(4.0), &UpdateDataRate, devices.Get(1), DataRate("0.5Mbps"));
  Simulator::Schedule(Seconds(4.0), &UpdateDataRate, devices2.Get(1), DataRate("0.5Mbps"));

  Simulator::Stop (Seconds (simulation_time));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
