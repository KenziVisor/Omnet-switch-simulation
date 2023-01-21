//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

//
// TODO auto-generated type
//
simple Application
{
    parameters:
        volatile double TimeBetweenPackets @unit(s);
        volatile int MsgSize @unit(byte);
    gates:
        inout Ip_gate;
}
simple IP
{
    parameters:
        volatile int DestNumber;
    gates:
        inout App_gate;
        inout Eth_gate;
}
simple Ethernet
{
    parameters:
        volatile double LifeOfArp @unit(s);
    gates:
        inout Ip_gate;
        inout Switch_gate;
}
module Host
{
    parameters:
        int HostNumber;
        volatile int x;
        volatile int y;
        volatile int Radios;
    gates:
        inout Switch_gate;
    submodules:
        app: Application;
        ip: IP;
        eth: Ethernet;
    connections:
        app.Ip_gate <--> ip.App_gate;
        ip.Eth_gate <--> eth.Ip_gate;
        Switch_gate <--> eth.Switch_gate;
}

simple Switch
{
    parameters:
        volatile double AgeingTime @unit(s);
        volatile double SwitchLatincyTime @unit(s);
    gates:
        inout gate[];
}
network Lab2_simulation
{
    @display("bgb=406,159");
    types:
        channel Ethernet100 extends ned.DatarateChannel
		{
    		datarate = 100Mbps;
    		delay = 0s;
    		ber = 0;
		}
    submodules:
        hosts[4]: Host;
        switch: Switch {
            @display("p=0,0");
        }
    connections:
        hosts[0].Switch_gate <--> Ethernet100 <--> switch.gate++;
        hosts[1].Switch_gate <--> Ethernet100 <--> switch.gate++;
        hosts[2].Switch_gate <--> Ethernet100 <--> switch.gate++;
        hosts[3].Switch_gate <--> Ethernet100 <--> switch.gate++;
}
