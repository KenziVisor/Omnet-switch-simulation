#include <string.h>
#include <omnetpp.h>
#include "Lab2_simulation_m.h"
#include <map>

using namespace omnetpp;
using namespace std;

class Ethernet : public cSimpleModule
{
private:
    uint8_t sourceAddress[6];//MAC address of a host.
    map<uint8_t,uint8_t*> ARP_table;//ARP cache table
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
    cMessage* msg = new cMessage("self");
    scheduleAt(0.0, msg);
}

void Ethernet::handleMessage(cMessage *msg){
    if (msg->isPacket()){//handles 2 packet types: arp request and arp reply
    ARP* arp = check_and_cast<ARP *>(msg);
        if(arp){
            if (arp->getOperation()==1){//handle for arp request
                cModule* parent = getParentModule();
                uint8_t host = parent->par("HostNumber").intValue();
                if (arp->getTargetProtocolAddress(3)==host){//the arp request is sent in layer 2 broadcast. the host needs to check if he is the destination.
                    for (int i=0;i<6;i++){//switches the hardware dest address and inputs his source address.
                        arp->setTargetHardwareAddress(i, arp->getSourceHardwareAddress(i));
                        arp->setSourceHardwareAddress(i, this->sourceAddress[i]);
                    }
                    arp->setOperation(2);//switch message type to reply.
                    EV<<"Send arp reply"<<endl;
                    simtime_t delay = gate("Switch_gate$o")->getTransmissionChannel()->getTransmissionFinishTime();
                    if(!(delay<=simTime()))
                        sendDelayed(arp, delay-simTime(),"Switch_gate$o");
                    else send(arp,"Switch_gate$o");
                }
                else delete(arp);//this message is not directed to this host.
            }
            else{//handle for arp reply and set timestamp for the new arp line (optional).
                uint8_t* target = new uint8_t[6];
                for (int i=0;i<6;i++) target[i] = arp->getSourceHardwareAddress(i);
                uint8_t target_name = arp->getTargetProtocolAddress(3);
                this->ARP_table.insert(pair<uint8_t,uint8_t*>(target_name,target));//add the host's address you looked for to the arp cache.
                delete(arp);
                /*
                char* target_name_char = new char[1];
                *target_name_char = target_name +'0';
                cMessage* timeStamp = new cMessage(target_name_char);
                simtime_t cancel = par("LifeOfArp");
                scheduleAt(simTime()+cancel, timeStamp);
                */
            }
        }
    }
    else{//handles self messages and deletes arp line (optional)
        if (!strcmp(msg->getName(),"self")){//if the message is self
            cModule* parent = getParentModule();
            uint8_t host = parent->par("HostNumber").intValue();
            if ((this->ARP_table.size())==3){//checks if the arp table is full;
                EV<<(int)host<<" is done!"<<endl;
                delete(msg);
                return;
            }
            //draw host's IP address;
            uint8_t ip_dest[4] = {172, 168, 32, 0};
            uint8_t dest_station = intuniform(0,2);
            if (dest_station>=host)
                dest_station++;
            ip_dest[3] = dest_station;
            if ((this->ARP_table.find(dest_station))==(this->ARP_table.end())){//checks if the IP address is in the arp cache.
                //if it doesn't, creates ARP packet and sends it to switch.
                ARP* arp = new ARP();
                arp->setByteLength(18);
                arp->setOperation(1);
                for(int i=0;i<4;i++)arp->setTargetProtocolAddress(i, ip_dest[i]);
                for(int i=0;i<3;i++) arp->setSourceProtocolAddress(i, ip_dest[i]);
                arp->setSourceProtocolAddress(3, host);
                for(int i=0;i<6;i++) arp->setSourceHardwareAddress(i, this->sourceAddress[i]);
                EV<<"Send arp request"<<endl;
                simtime_t delay = gate("Switch_gate$o")->getTransmissionChannel()->getTransmissionFinishTime();
                if(!(delay<=simTime()))
                    sendDelayed(arp, delay-simTime(),"Switch_gate$o");
                else send(arp,"Switch_gate$o");
            }
            scheduleAt(simTime()+5, msg);
        }
        /*else{
            uint8_t num = *(msg->getName())-'0';
            delete(this->ARP_table[num]);
            this->ARP_table.erase(num);
            delete(msg);
        }*/
    }
}

class Application : public cSimpleModule
{
  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};
Define_Module(Application);

void Application::initialize()
{


}

void Application::handleMessage(cMessage *msg)
{

}

class IP : public cSimpleModule
{
  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};
Define_Module(IP);

void IP::initialize()
{


}

void IP::handleMessage(cMessage *msg)
{

}

class Switch : public cSimpleModule
{
  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};
Define_Module(Switch);

void Switch::initialize()
{

}

void Switch::handleMessage(cMessage *msg)
{
    ARP* arp = check_and_cast<ARP *>(msg);
    if(arp){
        if(arp->getOperation()==1){
            for(int i=0;i<gateSize("gate");i++){
                ARP* arp_dup = arp->dup();
                simtime_t delay = gate("gate$o",i)->getTransmissionChannel()->getTransmissionFinishTime();
                if(!(delay<=simTime()))
                    sendDelayed(arp_dup, delay-simTime(),"gate$o",i);
                else send(arp_dup,"gate$o",i);
            }
            delete(arp);
        }
        else{
            int target = arp->getTargetHardwareAddress(5);
            simtime_t delay = gate("gate$o",target)->getTransmissionChannel()->getTransmissionFinishTime();
            if(!(delay<=simTime()))
                sendDelayed(arp, delay-simTime(),"gate$o",target);
            else send(arp,"gate$o",target);
        }
    }
}
