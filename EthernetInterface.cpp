/*
 *  © 2021 Fred Decker
 *  © 2020-2021 Harald Barth
 *  © 2020-2021 Chris Harlow
 *  © 2020 Gregor Baues
 *  All rights reserved.
 *  
 *  This file is part of DCC-EX/CommandStation-EX
 *
 *  This is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  It is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with CommandStation.  If not, see <https://www.gnu.org/licenses/>.
 * 
 *  Modifications
 *  22-04-02 Metkom, modidx:1, added support for personal mac address
 *  22-04-08 Metkom, modidx:2, added delay due to incomming data loss
 *  
 ***********************************************************************/

#include "defines.h" 
#if ETHERNET_ON == true
  #include "EthernetInterface.h"
  #include "DIAG.h"
  #include "CommandDistributor.h"
  #include "DCCTimer.h"

  EthernetInterface *EthernetInterface::singleton=NULL;
  /**
  * @brief Setup Ethernet Connection
  * 
  */
  void EthernetInterface::setup()
  {
    singleton = new EthernetInterface();
    if (!singleton->connected) singleton = NULL; 
  };

/**
 * @brief Aquire IP Address from DHCP and start server
 * 
 * @return true 
 * @return false 
 */
EthernetInterface::EthernetInterface()
{
// modidx:1, if a mac address is configured, take this one
  #ifdef MAC_ADDRESS
    byte mac[] = MAC_ADDRESS;
  #else
    byte mac[6];
    DCCTimer::getSimulatedMacAddress(mac);
  #endif
    connected = false;

  #ifdef IP_ADDRESS
    Ethernet.begin(mac, IP_ADDRESS);
  #else
    if (Ethernet.begin(mac) == 0)
    {
        DIAG(F("Ethernet.begin FAILED"));
        return;
    } 
  #endif
    DIAG(F("begin OK."));
    if (Ethernet.hardwareStatus() == EthernetNoHardware)
    {
      DIAG(F("Ethernet shield not found"));
      return;
    }
    unsigned long startmilli = millis();
    while ((millis() - startmilli) < 5500) // Loop to give time to check for cable connection
    {
      if (Ethernet.linkStatus() == LinkON) break;
      DIAG(F("Ethernet waiting for link (1sec) "));
      //      delay(1000); replaced by millis
      unsigned long previousMillis = millis();
      while (millis() - previousMillis < 1000) ;
    }
    if (Ethernet.linkStatus() == LinkOFF) {
      DIAG(F("Ethernet cable not connected"));
      return;
    }
    connected=true;
    IPAddress ip = Ethernet.localIP(); // reassign the obtained ip address
    server = new EthernetServer(IP_PORT); // Server listening on port: IP_PORT (2560)
    server->begin();
    LCD(4,F("IP: %d.%d.%d.%d"), ip[0], ip[1], ip[2], ip[3]);
    LCD(5,F("Port:%d"), IP_PORT);
    outboundRing = new RingStream(OUTBOUND_RING_SIZE);    // 2048
}

/**
 * @brief Main loop for the EthernetInterface
 * 
 */
void EthernetInterface::loop()
{
  if (!singleton) return;
  switch (Ethernet.maintain())
  {
  case 1:
    //renew failed
    DIAG(F("Ethernet Error: renew failed"));
    singleton = NULL;
    return;
  case 3:
    //rebind failed
    DIAG(F("Ethernet Error: rebind failed"));
    singleton = NULL;
    return;
  default:
    //nothing happened
    break;
  }
  singleton->loop2();
}

void EthernetInterface::loop2()
{
  // get a new client from the server
  EthernetClient newClient = server->accept();

  // check for new client
  if (newClient)
  {
    if (Diag::ETHERNET) DIAG(F("Ethernet: New client "));
    byte socket;
    for (socket = 0; socket < MAX_SOCK_NUM; socket++)
    {
      if (!clients[socket])
      {
        // On accept() the EthernetServer doesn't track the new client anymore
        // so we store it in our client array
        if (Diag::ETHERNET) DIAG(F("Socket %d"), socket);
        clients[socket] = newClient;
        break;
      }
    }
    if (socket == MAX_SOCK_NUM) DIAG(F("new Ethernet OVERFLOW")); 
  }

  // check for incoming data from all possible clients
  for (byte socket = 0; socket < MAX_SOCK_NUM; socket++)
  {
    if (clients[socket])
    {
      int avail = clients[socket].available();  // at least 1 byte to be read
      // modidx:2, added delay due to incomming data loss
      unsigned long previousMillis = millis();
      while (millis() - previousMillis < 300) ;
      if (avail > 0)
      {
        if (Diag::ETHERNET)  DIAG(F("EI 162: current socket: 0x%x, #bytes available: 0x%x"), socket, avail);
        // read bytes from a client
        int count = clients[socket].read(buffer, MAX_ETH_BUFFER);  //  get max. 512 bytes from client
        buffer[count] = '\0'; // terminate the string properly after last data byte
        if (Diag::ETHERNET) DIAG(F("EI 166: count= %d, Buffer: %s"), count, buffer);
        // execute with data going directly back
        outboundRing->mark(socket); 
        CommandDistributor::parse(socket, buffer, outboundRing);
        outboundRing->commit();
        return; // limit the amount of processing that takes place within 1 loop() cycle. 
      }
    }
  }

  // stop any clients which disconnect
  for (int socket = 0; socket<MAX_SOCK_NUM; socket++)
  {
    if (clients[socket] && !clients[socket].connected())
    {
      clients[socket].stop();
      CommandDistributor::forget(socket);          
      if (Diag::ETHERNET)  DIAG(F("Ethernet: disconnect %d "), socket);             
    }
  }
    
  // handle at most 1 outbound transmission 
  int socketOut = outboundRing->read();
  if (socketOut >= 0)
  {
    int count=outboundRing->count();
    if (Diag::ETHERNET) DIAG(F("Ethernet reply socket=%d, count=:%d"), socketOut,count);
    for(;count>0;count--)  clients[socketOut].write(outboundRing->read());
    clients[socketOut].flush(); //maybe 
  }
}
#endif
