/*******************************************************************************
 * Copyright (c) 2014, 2015 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution. 
 *
 * The Eclipse Public License is available at 
 *   http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at 
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial contribution
 *    Benjamin Cabe - adapt to IPStack, and add Yun instructions
 *    Ian Craggs - remove sprintfs to reduce sketch size
 *******************************************************************************/
#define SPRESENSE_LTE

#define WARN Serial.println

#define MQTTCLIENT_QOS2 1



#include <SPI.h>

#ifdef SPRESENSE_LTE
#include <LTE.h> // For Spresense LTE Board
// APN data for LTE-M
#define LTE_APN       "iijmio.jp"   // replace your APN
#define LTE_USER_NAME "mio@iij"     // replace with your APN username
#define LTE_PASSWORD  "iij"         // replace with your APN password
#elif
#include <Ethernet.h>
#endif

#include <IPStack.h>
#include <Countdown.h>
#include <MQTTClient.h>


int arrivedcount = 0;

void messageArrived(MQTT::MessageData& md)
{
  MQTT::Message &message = md.message;
  
  Serial.print("Message ");
  Serial.print(++arrivedcount);
  Serial.print(" arrived: qos ");
  Serial.print(message.qos);
  Serial.print(", retained ");
  Serial.print(message.retained);
  Serial.print(", dup ");
  Serial.print(message.dup);
  Serial.print(", packetid ");
  Serial.println(message.id);
  Serial.print("Payload ");
  Serial.println((char*)message.payload);
}

#ifdef SPRESENSE_LTE
LTE lteAccess;
LTEClient c;
#elif
EthernetClient c; // replace by a YunClient if running on a Yun
#endif

IPStack ipstack(c);
MQTT::Client<IPStack, Countdown, 50, 1> client = MQTT::Client<IPStack, Countdown, 50, 1>(ipstack);

#ifndef SPRESENSE_LTE 
byte mac[] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 };  // replace with your device's MAC
#endif
const char* topic = "arduino-sample";

void connect()
{
  //char hostname[] = "iot.eclipse.org"; // no longer available 
  char hostname[] = "test.mosquitto.org";
  int port = 1883;

  Serial.print("Connecting to ");
  Serial.print(hostname);
  Serial.print(":");
  Serial.println(port);
 
  int rc = ipstack.connect(hostname, port);
  if (rc != 1)
  {
    Serial.print("rc from TCP connect is ");
    Serial.println(rc);
  }
 
  Serial.println("MQTT connecting");
  MQTTPacket_connectData data = MQTTPacket_connectData_initializer;       
  data.MQTTVersion = 3;
  data.clientID.cstring = (char*)"arduino-sample";
  rc = client.connect(data);
  if (rc != 0)
  {
    Serial.print("rc from MQTT connect is ");
    Serial.println(rc);
  }
  Serial.println("MQTT connected");
  
  rc = client.subscribe(topic, MQTT::QOS2, messageArrived);   
  if (rc != 0)
  {
    Serial.print("rc from MQTT subscribe is ");
    Serial.println(rc);
  }
  Serial.println("MQTT subscribed");
}

void setup()
{
  Serial.begin(115200);

#ifdef SPRESENSE_LTE
  while (true) {
    if (lteAccess.begin() == LTE_SEARCHING) {
      if (lteAccess.attach(LTE_APN, LTE_USER_NAME, LTE_PASSWORD) == LTE_READY) {
        Serial.println("attach succeeded.");
        break;
      }
      Serial.println("An error occurred, shutdown and try again.");
      lteAccess.shutdown();
      sleep(1);
    }
  }
#elif
  Ethernet.begin(mac);
#endif  
  
  Serial.println("MQTT Hello example");
  connect();
}

MQTT::Message message;

void loop()
{ 
  if (!client.isConnected())
    connect();
    
  arrivedcount = 0;

  // Send and receive QoS 0 message
  char buf[100];
  strcpy(buf, "Hello World! QoS 0 message");
  message.qos = MQTT::QOS0;
  message.retained = false;
  message.dup = false;
  message.payload = (void*)buf;
  message.payloadlen = strlen(buf)+1;
  int rc = client.publish(topic, message);
  while (arrivedcount == 0)
  {
    Serial.println("Waiting for QoS 0 message");
    client.yield(1000);
  }
  
  // Send and receive QoS 1 message
  strcpy(buf, "Hello World! QoS 1 message");
  message.qos = MQTT::QOS1;
  message.payloadlen = strlen(buf)+1;
  rc = client.publish(topic, message);
  while (arrivedcount == 1)
  {
    Serial.println("Waiting for QoS 1 message");
    client.yield(1000);
  }

  // Send and receive QoS 2 message
  strcpy(buf, "Hello World! QoS 2 message");
  message.qos = MQTT::QOS2;
  message.payloadlen = strlen(buf)+1;
  rc = client.publish(topic, message);
  while (arrivedcount == 2)
  {
    Serial.println("Waiting for QoS 2 message");
    client.yield(1000);  
  }    
  delay(2000);
}
