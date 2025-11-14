#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

int main(int argc, char *argv[]){
    uint32_t nWifi = 1; 
    uint32_t protocol = 0;
    uint32_t mobility = 0;
    std::string outputFile = "flow-monitor.xml";

    CommandLine cmd;
    cmd.AddValue("nWifi", "Numero de clientes WiFi", nWifi);
    cmd.AddValue("protocol", "0 = UDP, 1 = TCP, 2 = TCP/UDP", protocol);
    cmd.AddValue("mobility", "0 = Sem mobilidade, 1 = Com mobilidade", mobility);
    cmd.AddValue("outputFile", "Nome do arquivo de saida", outputFile);
    cmd.Parse(argc, argv);

    uint32_t nLanNodes = 10;

// Configuração da rede LAN
    NodeContainer lanNodes;
    lanNodes.Create(nLanNodes);

    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));
    NetDeviceContainer lanDevices = csma.Install(lanNodes);

// Configuração da rede WiFi
    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(nWifi);

    NodeContainer wifiApNode;
    wifiApNode.Add(lanNodes.Get(0)); // AP é o nó 0 da LAN

    YansWifiChannelHelper channel;
    channel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    channel.AddPropagationLoss("ns3::FriisPropagationLossModel");

    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    phy.Set("TxPowerStart", DoubleValue(16.0));
    phy.Set("TxPowerEnd", DoubleValue(16.0));
    phy.Set("TxPowerLevels", UintegerValue(1));
    phy.Set("RxSensitivity", DoubleValue(-80.0));
    phy.Set("CcaEdThreshold", DoubleValue(-78.0));

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211a);
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");

    WifiMacHelper mac;
    Ssid ssid = Ssid("Equipe8");

    mac.SetType("ns3::StaWifiMac",
                "Ssid", SsidValue(ssid),
                "ActiveProbing", BooleanValue(false));
    NetDeviceContainer staDevices = wifi.Install(phy, mac, wifiStaNodes);

    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer apDevices = wifi.Install(phy, mac, wifiApNode);

// Configuração de Mobilidade

    // Servidor em (0,0)
    MobilityHelper mobilityRouter;
    Ptr<ListPositionAllocator> routerPos = CreateObject<ListPositionAllocator> ();
    routerPos->Add(Vector(0.0, 0.0, 0.0));
    mobilityRouter.SetPositionAllocator(routerPos);
    mobilityRouter.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobilityRouter.Install(lanNodes.Get(nLanNodes - 1));

    // AP em (70,70)
    MobilityHelper mobilityAp;
    Ptr<ListPositionAllocator> apPos = CreateObject<ListPositionAllocator> ();
    apPos->Add(Vector(70.0, 70.0, 0.0));
    mobilityAp.SetPositionAllocator(apPos);
    mobilityAp.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobilityAp.Install(wifiApNode);

    // Clientes WiFi em grade 3 por linha
    MobilityHelper mobilitySta;
    mobilitySta.SetPositionAllocator(
        "ns3::GridPositionAllocator",
        "MinX", DoubleValue(70.0),
        "MinY", DoubleValue(70.0),
        "DeltaX", DoubleValue(5.0),
        "DeltaY", DoubleValue(5.0),
        "GridWidth", UintegerValue(3),
        "LayoutType", StringValue("RowFirst")
    );

    if (mobility == 0){
        mobilitySta.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        mobilitySta.Install(wifiStaNodes);
    } else if (mobility == 1){
        // ConstantVelocityMobilityModel gera movimento linear unidirecional se afastando do Ap
        mobilitySta.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                                     "Speed", StringValue("ns3::UniformRandomVariable[Min=1.0|Max=2.0]"),
                                     "Distance", DoubleValue(10.0));
        mobilitySta.Install(wifiStaNodes);
    } 

    // Restante dos nós da LAN ficam fixos
    MobilityHelper mobilityLanRest;
    mobilityLanRest.SetPositionAllocator(
        "ns3::RandomRectanglePositionAllocator",
        "X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=140.0]"),
        "Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=140.0]")
    );
    mobilityLanRest.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    for (uint32_t i = 1; i < nLanNodes - 1; i++) {
        mobilityLanRest.Install(lanNodes.Get(i));
    }

// Configuração da Pilha de Protocolos e Endereçamento
    InternetStackHelper stack;
    stack.Install(lanNodes);
    stack.Install(wifiStaNodes);

    // LAN
    Ipv4AddressHelper addressLan;
    addressLan.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer lanInterfaces = addressLan.Assign(lanDevices);

    // WiFi
    Ipv4AddressHelper addressWifi;
    addressWifi.SetBase("192.168.0.0", "255.255.255.0");
    Ipv4InterfaceContainer apInterfaces = addressWifi.Assign(apDevices);
    Ipv4InterfaceContainer staInterfaces = addressWifi.Assign(staDevices);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

// Configuração das Aplicações

    if (protocol == 0){    
        // Servidor UDP
        uint16_t port = 9;
        Address serverAddress(InetSocketAddress(lanInterfaces.GetAddress(nLanNodes - 1), port));

        PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", serverAddress);
        ApplicationContainer serverApps = packetSinkHelper.Install(lanNodes.Get(nLanNodes - 1));
        serverApps.Start(Seconds(0));
        serverApps.Stop(Seconds(60.0));

        // Cliente WiFi envia para servidor LAN
        OnOffHelper onOffHelper("ns3::UdpSocketFactory", serverAddress);
        onOffHelper.SetAttribute("DataRate", StringValue("5Mbps"));
        onOffHelper.SetAttribute("PacketSize", UintegerValue(1024));
        onOffHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1.0]"));
        onOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.0]"));

        for (uint32_t i = 0; i < nWifi; i++){
            ApplicationContainer clientApps = onOffHelper.Install(wifiStaNodes.Get(i));
            clientApps.Start(Seconds(1.0));
            clientApps.Stop(Seconds(60.0));
        }

    } else if (protocol == 1){
        Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1500));

        uint16_t port = 9;
        Address serverAddress(InetSocketAddress(lanInterfaces.GetAddress(nLanNodes - 1), port));

        // Servidor TCP
        PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", serverAddress);
        ApplicationContainer serverApps = packetSinkHelper.Install(lanNodes.Get(nLanNodes - 1));
        serverApps.Start(Seconds(0));
        serverApps.Stop(Seconds(60.0));

        // Cliente TCP
        BulkSendHelper bulkSender("ns3::TcpSocketFactory", serverAddress);
        bulkSender.SetAttribute("MaxBytes", UintegerValue(0));

        for (uint32_t i = 0; i < nWifi; i++){
            ApplicationContainer clientApps = bulkSender.Install(wifiStaNodes.Get(i));
            clientApps.Start(Seconds(1.0));
            clientApps.Stop(Seconds(60.0));
        }
    } else if(protocol == 2){
        uint32_t nUdp = nWifi / 2;
        uint32_t nTcp = nWifi - nUdp;

        // Servidor UDP
        uint16_t udpPort = 9;
        Address udpServerAddress(InetSocketAddress(lanInterfaces.GetAddress(nLanNodes - 1), udpPort));

        PacketSinkHelper udpServer("ns3::UdpSocketFactory", udpServerAddress);
        ApplicationContainer udpServerApps = udpServer.Install(lanNodes.Get(nLanNodes - 1));
        udpServerApps.Start(Seconds(0));
        udpServerApps.Stop(Seconds(60.0));

        // Servidor TCP
        Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1500));

        Address tcpServerAddress(InetSocketAddress(lanInterfaces.GetAddress(nLanNodes - 1), 10));
        PacketSinkHelper tcpServer("ns3::TcpSocketFactory", tcpServerAddress);
        ApplicationContainer tcpServerApps = tcpServer.Install(lanNodes.Get(nLanNodes - 1));
        tcpServerApps.Start(Seconds(0));
        tcpServerApps.Stop(Seconds(60.0));

        // Clientes UDP
        OnOffHelper udpClient("ns3::UdpSocketFactory", udpServerAddress);
        udpClient.SetAttribute("DataRate", StringValue("5Mbps"));
        udpClient.SetAttribute("PacketSize", UintegerValue(1024));
        udpClient.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1.0]"));
        udpClient.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.0]"));

        for (uint32_t i = 0; i < nUdp; i++){
            ApplicationContainer apps = udpClient.Install(wifiStaNodes.Get(i));
            apps.Start(Seconds(1.0));
            apps.Stop(Seconds(60.0));
        }

        // Clientes TCP
        BulkSendHelper tcpClient("ns3::TcpSocketFactory", tcpServerAddress);
        tcpClient.SetAttribute("MaxBytes", UintegerValue(0));

        for (uint32_t i = 0; i < nTcp; i++){
            ApplicationContainer apps = tcpClient.Install(wifiStaNodes.Get(nUdp + i));
            apps.Start(Seconds(1.0));
            apps.Stop(Seconds(60.0));
        }
    }

// NetAnim 
    //AnimationInterface anim("netanim.xml");

// FlowMonitor
    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> monitor = flowHelper.InstallAll();

    Simulator::Stop(Seconds(61.0));
    Simulator::Run();

    monitor->SerializeToXmlFile(outputFile, true, true);

    Simulator::Destroy();
    return 0;
}
