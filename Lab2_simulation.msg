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
// TODO generated message class
//
packet App_pck {}

packet IP_pck{
    uint8_t version_ihl;
    uint8_t dscp_ecn;
    uint16_t totalLength;
    uint16_t identification;
    uint16_t flags_fragmentOffset;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint8_t src_addr[4];
    uint8_t dst_addr[4];
}

packet Eth_pck{
    uint8_t dst_addr[6];
    uint8_t src_addr[6];
    uint16_t eth_type;
}

packet ARP{
    uint16_t hardwareType = 1;
    uint16_t protocolType = 0x0800;
    uint8_t HLen = 48;
    uint8_t PLen = 32;
    uint16_t operation;
    uint8_t sourceHardwareAddress[6];
    uint8_t sourceProtocolAddress[4];
    uint8_t targetHardwareAddress[6];
    uint8_t targetProtocolAddress[4];
}








