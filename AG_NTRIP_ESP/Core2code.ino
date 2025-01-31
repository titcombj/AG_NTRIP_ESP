//Core2: this task serves the Webpage and handles the GPS NMEAs

void Core2code( void * pvParameters ){
  
  DBG("\nTask2 running on core ");
  DBG((int)xPortGetCoreID(), 1);

  
// Start WiFi Client
 while (!EE_done){  // wait for eeprom data
  delay(10);
 }
 WiFi_Start_STA();
 if (my_WiFi_Mode == 0) WiFi_Start_AP(); // if failed start AP

 repeat_ser = millis();

  udpRoof.listen(portMy);
  UDPReceiveNtrip();
  
  for(;;){ //main loop core2
		WiFi_Traffic();
		Serial_Traffic();

    //no data for more than 2 secs = blink
    if (millis() > (Ntrip_data_time + 2000)) {
      if (!LED_WIFI_ON) {
        if (millis() > (LED_WIFI_time + LED_WIFI_pause)) {
          LED_WIFI_time = millis();
          LED_WIFI_ON = true;
          digitalWrite(LED_PIN_WIFI, HIGH);
        }
      }
      if (LED_WIFI_ON) {
        if (millis() > (LED_WIFI_time + LED_WIFI_pulse)) {
          LED_WIFI_time = millis();
          LED_WIFI_ON = false;
          digitalWrite(LED_PIN_WIFI, LOW);
        }
      }
    }
    else
    {
      digitalWrite(LED_PIN_WIFI, HIGH);
    }
   }//end main loop core 2
}
//------------------------------------------------------------------------------------------
// Subs --------------------------------------
void udpNtripRecv()
{ //callback when received packets
  udpNtrip.onPacket([](AsyncUDPPacket packet) 
   {Serial.print("."); 
    for (int i = 0; i < packet.length(); i++) 
        {
          //JTI- We just want to forward all Ntrip data all the time in our application

          //if(NtripSettings.enableNtrip == 2) {
            //Serial.print(packet.data()[i],HEX); 
          Serial1.write(packet.data()[i]); 
          //}
        }
   });  // end of onPacket call
}

//------------------------------------------------------------------------------------------
//Read serial GPS data
//This is where we need to deal with our pod data 
//-------------------------------------------------------------------------------------------
void Serial_Traffic(){

while (Serial1.available()) 
  { 
   c = Serial1.read();
   //Serial2.print(c);
   if (c == '$')      //if character is $=0x24 start new sentence
      {
        newSentence = true; // only store sentence, if time is over
        gpsBuffer[0] = '/0';
        i = 0;
      }
    
   if (c == 0x0D && newSentence)   //if character is CR, build UDP send buffer
      {
        char Sent_Buffer[]="???";
        gpsBuffer[i++] = 0x0D;  //last-1 byte =CR
        gpsBuffer[i++] = 0x0A;  //last-1 byte =CR
        //gpsBuffer[i++] = "/0";  //
        Sent_Buffer[0] = (char)gpsBuffer[3];
        Sent_Buffer[1] = (char)gpsBuffer[4];
        Sent_Buffer[2] = (char)gpsBuffer[5];

        
        if (strcmp(Sent_Buffer, "RMC")==0 || strcmp(Sent_Buffer, "GGA")==0 || strcmp(Sent_Buffer, "VTG")==0 || strcmp(Sent_Buffer, "ZDA")==0){
            switch (NtripSettings.send_UDP_AOG){
              case 1:
                 udpRoof.writeTo(gpsBuffer, i, ipDestination, portDestination );    
               break;
              case 2:          
              break;
            }          
               
            if (NtripSettings.sendGGAsentence == 2 ){
               if (strcmp(Sent_Buffer, "GGA") == 0){
                  for (byte n = 0; n <= i; n++){
                    lastSentence[n] = gpsBuffer[n];
                   }
                  repeat_ser = millis(); //Reset timer
                }
              } 
          }
          i = 0;
          newSentence = false;
     }
   
   if (newSentence && i < 100) 
     {
       gpsBuffer[i++] = c;
	   if (debugmode) DBG(gpsBuffer[i]);
     }
  }

}  
