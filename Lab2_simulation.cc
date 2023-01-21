#include <string.h>
#include <omnetpp.h>
#include "Lab2_simulation_m.h"
#include <map>
#include <queue>

using namespace omnetpp;
using namespace std;

class Ethernet : public cSimpleModule
{
private:
    uint8_t sourceAddress[6];//MAC address of a host.
    map<uint8_t,uint8_t*> ARP_table;//ARP cache table
    queue<IP_pck*> packet_queue[3];
  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};
Define_Module(Ethernet);

void Ethernet::initialize()
{
    cModule* parent = getParentModule();
    for(int i=0;i<5;i++) this->sourceAddress[i]=0x11;
    this->sourceAddress[5] = parent->par("HostNumber");
}

void Ethernet::handleMessage(cMessage *msg){
    if (msg->isPacket()){//handles 2 packet types: arp request and arp reply
        cMessage* dup = msg->dup();
        ARP* arp = dynamic_cast<ARP *>(dup);
        cModule* parent = getParentModule();
        uint8_t host = parent->par("HostNumber").intValue();
        if(arp){
            if (arp->getOperation()==1){//handle for arp request
                if (arp->getTargetProtocolAddress(3)==host){//the arp request is sent in layer 2 broadcast. the host needs to check if he is the destination.
                    for (int i=0;i<6;i++){//switches the hardware dest address and inputs his source address.
                        arp->setTargetHardwareAddress(i, arp->getSourceHardwareAddress(i));
                        arp->setSourceHardwareAddress(i, this->sourceAddress[i]);
                    }
                    arp->setOperation(2);//switch message type to reply.
                    EV<<"Send arp reply. im host "<<(char)host+'0'<<endl;
                    simtime_t delay = gate("Switch_gate$o")->getTransmissionChannel()->getTransmissionFinishTime();
                    if(!(delay<=simTime()))
                        sendDelayed(arp, delay-simTime(),"Switch_gate$o");
                    else send(arp,"Switch_gate$o");
                }
                else delete(arp);//this message is not directed to this host.
            }
            else{//handle for arp reply and set timestamp for the new arp line (optional).
                if (arp->getTargetHardwareAddress(5)==host){
                    int dest_temp=arp->getSourceHardwareAddress(5);
                    if (dest_temp>host)dest_temp--;
                    if ((this->ARP_table.find(arp->getSourceHardwareAddress(5)))==(this->ARP_table.end())){
                        uint8_t* target = new uint8_t[6];
                        for (int i=0;i<6;i++) target[i] = arp->getSourceHardwareAddress(i);
                        uint8_t target_name = arp->getTargetProtocolAddress(3);
                        this->ARP_table.insert(pair<uint8_t,uint8_t*>(target_name,target));//add the host's address you looked for to the arp cache.
                        EV<<"ARP table of host "<<(char)host+'0'<<" is: "<<this->ARP_table.size()<<endl;
                        delete(arp);
                        char* target_name_char = new char[1];
                        *target_name_char = target_name +'0';
                        cMessage* timeStamp = new cMessage(target_name_char);
                        simtime_t cancel = par("LifeOfArp");
                        scheduleAt(simTime()+cancel, timeStamp);
                    }
                    IP_pck* original = this->packet_queue[dest_temp].front();
                    this->packet_queue[dest_temp].pop();
                    Eth_pck* eth = new Eth_pck();
                    eth->setByteLength(18);
                    eth->encapsulate(original);
                    uint8_t dest_station = original->getDst_addr(3);
                    uint8_t* dest_mac = this->ARP_table[dest_station];
                    for (int i=0;i<6;i++){
                        eth->setSrc_addr(i, this->sourceAddress[i]);
                        eth->setDst_addr(i, dest_mac[i]);
                    }
                    EV<<(char)host+'0'<<" send ethernet to host "<<(char)(eth->getDst_addr(5))+'0'<<endl;
                    simtime_t delay = gate("Switch_gate$o")->getTransmissionChannel()->getTransmissionFinishTime();
                    if(!(delay<=simTime()))
                        sendDelayed(eth, delay-simTime(),"Switch_gate$o");
                    else send(eth,"Switch_gate$o");
                }
                else delete(arp);
            }
        }
        else{
            dup = msg->dup();
            IP_pck* ip = dynamic_cast<IP_pck *>(dup);
            if (ip){
                uint8_t dest_station = ip->getDst_addr(3);
                if ((this->ARP_table.find(dest_station))==(this->ARP_table.end())){//checks if the IP address is in the arp cache.
                //if it doesn't, creates ARP packet and sends it to switch.
                    int dest_temp=dest_station;
                    if (dest_station>host)
                        dest_temp--;
                    this->packet_queue[dest_temp].push(ip);
                    ARP* arp_ip = new ARP();
                    arp_ip->setByteLength(28);
                    arp_ip->setOperation(1);
                    for(int i=0;i<4;i++)arp_ip->setTargetProtocolAddress(i, ip->getDst_addr(i));
                    for(int i=0;i<3;i++) arp_ip->setSourceProtocolAddress(i, ip->getDst_addr(i));
                    arp_ip->setSourceProtocolAddress(3, host);
                    for(int i=0;i<6;i++) arp_ip->setSourceHardwareAddress(i, this->sourceAddress[i]);
                    EV<<(char)host+'0'<<" Send arp request to "<<(char)(arp_ip->getTargetProtocolAddress(3))+'0'<<endl;
                    simtime_t delay = gate("Switch_gate$o")->getTransmissionChannel()->getTransmissionFinishTime();
                    if(!(delay<=simTime()))
                        sendDelayed(arp_ip, delay-simTime(),"Switch_gate$o");
                    else send(arp_ip,"Switch_gate$o");
                }
                else{//if the IP address is in the arp cache
                    Eth_pck* eth = new Eth_pck();
                    eth->setByteLength(18);
                    eth->encapsulate(ip);
                    uint8_t* dest_mac = this->ARP_table[dest_station];
                    for (int i=0;i<6;i++){
                        eth->setSrc_addr(i, this->sourceAddress[i]);
                        eth->setDst_addr(i, dest_mac[i]);
                    }
                    EV<<(char)host+'0'<<" send ethernet to host "<<(char)(eth->getDst_addr(5))+'0'<<endl;
                    simtime_t delay = gate("Switch_gate$o")->getTransmissionChannel()->getTransmissionFinishTime();
                    if(!(delay<=simTime()))
                        sendDelayed(eth, delay-simTime(),"Switch_gate$o");
                    else send(eth,"Switch_gate$o");
                }
            }
            else{//if the packet is Ethernet
                Eth_pck* eth = dynamic_cast<Eth_pck *>(msg);
                EV<<"Deliver to the ip layer"<<endl;
                if (eth->getDst_addr(5)==host)
                    send(eth,"Ip_gate$o");
                else delete(eth);
            }
        }
    }
    else{//self message for deleting ARP line.
        uint8_t num = *(msg->getName());
        delete[](this->ARP_table[num]);
        this->ARP_table.erase(num);
    }
}

class Application : public cSimpleModule
{
  private:
    int numCreated;
    simtime_t TimelastEntrance;
    cHistogram EntrancesStats;
    cOutVector EntrancesVector;
    cHistogram TimeInSystemStats;
    cOutVector TimeInSystemVector;
  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
};
Define_Module(Application);

void Application::initialize()
{
    this->numCreated=0;
    this->TimelastEntrance=0;
    EntrancesStats.setName("EntrancesStats");
    EntrancesVector.setName("Entrances");
    TimeInSystemStats.setName("TimeInSystemStats");
    TimeInSystemVector.setName("TimeInSystem");
    cMessage* msg = new cMessage("self");
    scheduleAt(0.0, msg);
}

void Application::handleMessage(cMessage *msg)
{
    if(msg->isPacket()){//if this message is a packet from the IP layer
        IP_pck* ip_packet = dynamic_cast<IP_pck *>(msg);
        App_pck* data = dynamic_cast<App_pck *>(ip_packet->decapsulate());
        simtime_t lastEntrance=simTime()-(this->TimelastEntrance);
        this->TimelastEntrance=simTime();
        this->EntrancesVector.record(lastEntrance);
        this->EntrancesStats.collect(lastEntrance);
        simtime_t TimeInSystem = simTime()-(data->getCreationTime());
        this->TimeInSystemVector.record(TimeInSystem);
        this->TimeInSystemStats.collect(TimeInSystem);
        delete(data);
    }
    else{//if this message is a self message
        App_pck* packet = new App_pck();
        int MsgSize = 4*(int)par("MsgSizeDist").doubleValue();
        while (MsgSize<26 || MsgSize>1480) MsgSize = 4*par("MsgSizeDist").intValue();
        packet->setByteLength(MsgSize);
        this->numCreated++;
        send(packet,"Ip_gate$o");
        scheduleAt(simTime()+par("TimeBetweenPackets"), msg);
    }
}

void Application::finish(){
    cModule* parent = getParentModule();
    uint8_t host = parent->par("HostNumber").intValue();
    EV <<"numCreated = "<<this->numCreated<<endl;
    EV << "Entrance stats of host "<<(char)host+'0'-48<<", min:    " << this->EntrancesStats.getMin() << endl;
    EV << "Entrance stats of host "<<(char)host+'0'-48<<" , max:    " << this->EntrancesStats.getMax() << endl;
    EV << "Entrance stats of host "<<(char)host+'0'-48<<" , mean:   " << this->EntrancesStats.getMean() << endl;
    EV << "Entrance stats of host "<<(char)host+'0'-48<<" , variance:   " << this->EntrancesStats.getVariance() << endl;
    EV << "Time in system stats of host "<<(char)host+'0'-48<<" , min:    " << this->TimeInSystemStats.getMin() << endl;
    EV << "Time in system stats of host "<<(char)host+'0'-48<<" , max:    " << this->TimeInSystemStats.getMax() << endl;
    EV << "Time in system stats of host "<<(char)host+'0'-48<<" , mean:   " << this->TimeInSystemStats.getMean() << endl;
    EV << "Time in system stats of host "<<(char)host+'0'-48<<" , variance: " << this->TimeInSystemStats.getVariance() << endl;
}

class IP : public cSimpleModule
{
private:
    uint8_t sourceAddress[4];//IP address of a host.
  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};
Define_Module(IP);

void IP::initialize()
{
    cModule* parent = getParentModule();
    this->sourceAddress[0]=172;
    this->sourceAddress[1]=168;
    this->sourceAddress[2]=32;
    this->sourceAddress[3]=parent->par("HostNumber").intValue();
}

void IP::handleMessage(cMessage *msg)
{
    cModule* parent = getParentModule();
    uint8_t host = parent->par("HostNumber");
    cPacket* packet = dynamic_cast<cPacket *>(msg);
    IP_pck* ip_packet;
    if((packet->getArrivalGate())==gate("App_gate$i")){
        ip_packet = new IP_pck();
        ip_packet->setByteLength(20);
        ip_packet->encapsulate(packet);
        ip_packet->setTotalLength(ip_packet->getByteLength());
        for (int i=0;i<4;i++)ip_packet->setSrc_addr(i, this->sourceAddress[i]);
        for (int i=0;i<3;i++)ip_packet->setDst_addr(i, this->sourceAddress[i]);
        int dest = par("DestNumber");
        if (dest>=host)dest++;
        ip_packet->setDst_addr(3, dest);
        send(ip_packet,"Eth_gate$o");
    }
    else{
        Eth_pck* eth = dynamic_cast<Eth_pck *>(packet);
        ip_packet = dynamic_cast<IP_pck *>(eth->decapsulate());
        send(ip_packet,"App_gate$o");
    }
}

class Switch : public cSimpleModule
{
private:
    typedef struct Gate_Data{
        int GateIndex = -1;
        cMessage* self = NULL;
    }Gate_data;
    map<uint8_t,Gate_data*> Filtering_Database;
  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};
Define_Module(Switch);

void Switch::initialize() {}

void Switch::handleMessage(cMessage *msg)
{
    cMessage* dup = msg->dup();
    ARP* arp = dynamic_cast<ARP *>(dup);
    if(arp){
        if((this->Filtering_Database.find(arp->getSourceHardwareAddress(5)))==(this->Filtering_Database.end())){
        //checks if the MAC address is in the switch cache.
            Gate_data* myGate = new Gate_data;
            myGate->GateIndex = arp->getArrivalGate()->getIndex();
            char* index = new char[1];
            *index = (arp->getSourceHardwareAddress(5)) +'0';
            myGate->self = new cMessage(index);
            this->Filtering_Database.insert(pair<uint8_t,Gate_data*>(arp->getSourceHardwareAddress(5),myGate));
            scheduleAt(simTime()+par("AgeingTime"), myGate->self);
        }
        else{//if the MAC address is in the switch cache
            cMessage* myself = this->Filtering_Database[arp->getSourceHardwareAddress(5)]->self;
            if (myself->isScheduled())
                cancelEvent(myself);
            scheduleAt(simTime() + par("AgeingTime"), myself);
        }
        if(arp->getOperation()==1){//if the arp message is from type 1:
            for(int i=0;i<gateSize("gate");i++){
                if (!(arp->arrivedOn("gate$i", i))){
                    ARP* arp_dup = arp->dup();
                    double latency = par("SwitchLatincyConstant").doubleValue()*(arp_dup->getByteLength());
                    simtime_t delay = gate("gate$o",i)->getTransmissionChannel()->getTransmissionFinishTime();
                    if(!(delay<=simTime()))
                        sendDelayed(arp_dup, delay+latency-simTime(),"gate$o",i);
                    else sendDelayed(arp_dup, latency,"gate$o",i);
                }
            }
            delete(arp);
        }
        else{//if the arp message is from type 2:
            if((this->Filtering_Database.find(arp->getTargetHardwareAddress(5)))==(this->Filtering_Database.end())){
                for(int i=0;i<gateSize("gate");i++){
                    if (!(arp->arrivedOn("gate$i", i))){
                        ARP* arp_dup = arp->dup();
                        double latency = par("SwitchLatincyConstant").doubleValue()*(arp_dup->getByteLength());
                        simtime_t delay = gate("gate$o",i)->getTransmissionChannel()->getTransmissionFinishTime();
                        if(!(delay<=simTime()))
                            sendDelayed(arp_dup, delay+latency-simTime(),"gate$o",i);
                        else sendDelayed(arp_dup, latency,"gate$o",i);
                    }
                }
                delete(arp);
            }
            else{
                int index = this->Filtering_Database[arp->getTargetHardwareAddress(5)]->GateIndex;
                double latency = par("SwitchLatincyConstant").doubleValue()*(arp->getByteLength());
                simtime_t delay = gate("gate$o",index)->getTransmissionChannel()->getTransmissionFinishTime();
                if(!(delay<=simTime()))
                    sendDelayed(arp, delay+latency-simTime(),"gate$o",index);
                else sendDelayed(arp, latency,"gate$o",index);
            }
        }
    }
    else{//the message is not an arp packet.
        dup = msg->dup();
        Eth_pck* eth = dynamic_cast<Eth_pck *>(dup);
        if (eth){
            if((this->Filtering_Database.find(eth->getSrc_addr(5)))==(this->Filtering_Database.end())){
            //checks if the MAC address is in the switch cache.
                Gate_data* myGate = new Gate_data;
                myGate->GateIndex = eth->getArrivalGate()->getIndex();
                char* index = new char[1];
                *index = (eth->getSrc_addr(5)) +'0';
                myGate->self = new cMessage(index);
                this->Filtering_Database.insert(pair<uint8_t,Gate_data*>(eth->getSrc_addr(5),myGate));
                scheduleAt(simTime()+par("AgeingTime"), myGate->self);
            }
            else{//if the MAC address is in the switch cache.
                cMessage* myself = this->Filtering_Database[eth->getSrc_addr(5)]->self;
                if (myself->isScheduled())
                    cancelEvent(myself);
                scheduleAt(simTime() + par("AgeingTime"), myself);
            }
            if((this->Filtering_Database.find(eth->getDst_addr(5)))==(this->Filtering_Database.end())){
            //checks if the MAC address of the destination is in the switch cache.
                for(int i=0;i<gateSize("gate");i++){
                    if (!(eth->arrivedOn("gate$i", i))){
                        Eth_pck* eth_dup = eth->dup();
                        double latency = par("SwitchLatincyConstant").doubleValue()*(eth_dup->getByteLength());
                        simtime_t delay = gate("gate$o",i)->getTransmissionChannel()->getTransmissionFinishTime();
                        if(!(delay<=simTime()))
                            sendDelayed(eth_dup, delay+latency-simTime(),"gate$o",i);
                        else sendDelayed(eth_dup, latency,"gate$o",i);
                    }
                }
                delete(eth);
            }
            else{//if the MAC address of the destination is in the switch cache.
                int index = this->Filtering_Database[eth->getDst_addr(5)]->GateIndex;
                double latency = par("SwitchLatincyConstant").doubleValue()*(eth->getByteLength());
                EV<<"send ethernet to dest "<<index<<endl;
                simtime_t delay = gate("gate$o",index)->getTransmissionChannel()->getTransmissionFinishTime();
                if(!(delay<=simTime()))
                    sendDelayed(eth, delay+latency-simTime(),"gate$o",index);
                else sendDelayed(eth, latency,"gate$o",index);
            }
        }
        else{////self message for deleting SWITCH TABLE line.
            uint8_t num = *(msg->getName());
            delete(this->Filtering_Database[num]);
            this->Filtering_Database.erase(num);
        }
    }
}
