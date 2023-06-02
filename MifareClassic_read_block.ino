/**
 * Example to read a Mifare Classic block 4 and show its information 
 * Authors: 
 *        Salvador Mendoza - @Netxing - salmg.net
 *        For Electronic Cats - electroniccats.com
 * 
 *  March 2020
 * 
 * This code is beerware; if you see me (or any other collaborator 
 * member) at the local, and you've found our code helpful, 
 * please buy us a round!
 * Distributed as-is; no warranty is given.
 */
 
#include "Electroniccats_PN7150.h"          
#define PN7150_IRQ   (3)
#define PN7150_VEN   (2)
#define PN7150_ADDR  (0x28)

#define BLK_NB_MFC      4                                          // Block tat wants to be read
#define KEY_MFC         0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF         // Default Mifare Classic key

Electroniccats_PN7150 nfc(PN7150_IRQ, PN7150_VEN, PN7150_ADDR);    // creates a global NFC device interface object, attached to pins 7 (IRQ) and 8 (VEN) and using the default I2C address 0x28
RfIntf_t RfInterface;                                              //Intarface to save data for multiple tags

uint8_t mode = 1; 
                                                 // modes: 1 = Reader/ Writer, 2 = Emulation
void ResetMode(){                                      //Reset the configuration mode after each reading
  Serial.println("Re-initializing...");
  nfc.ConfigMode(mode);                               
  nfc.StartDiscovery(mode);
}

void PrintBuf(const byte * data, const uint32_t numBytes){ //Print hex data buffer in format
  uint32_t szPos;
  for (szPos=0; szPos < numBytes; szPos++)
  {
    Serial.print(F("0x"));
    // Append leading 0 for small values
    if (data[szPos] <= 0xF)
      Serial.print(F("0"));
    Serial.print(data[szPos]&0xff, HEX);
    if ((numBytes > 1) && (szPos != numBytes - 1))
    {
      Serial.print(F(" "));
    }
  }
  Serial.println();
}

void PCD_MIFARE_scenario (void){
    Serial.println("Start reading process...");
    bool status;
    unsigned char Resp[256];
    unsigned char RespSize;
    /* Authenticate sector 1 with generic keys */
    unsigned char Auth[] = {0x40, BLK_NB_MFC/4, 0x10, KEY_MFC};
    /* Read block 4 */
    unsigned char Read[] = {0x10, 0x30, BLK_NB_MFC};

    /* Authenticate */
    status = nfc.ReaderTagCmd(Auth, sizeof(Auth), Resp, &RespSize);
    if((status == NFC_ERROR) || (Resp[RespSize-1] != 0))
        Serial.println("Auth error!");
    
    /* Read block */
    status = nfc.ReaderTagCmd(Read, sizeof(Read), Resp, &RespSize);
    if((status == NFC_ERROR) || (Resp[RespSize-1] != 0))
        Serial.print("Error reading sector!");
    
    Serial.print("------------------------Sector ");
    Serial.print(BLK_NB_MFC/4, DEC);
    Serial.println("-------------------------");
    
    PrintBuf(Resp+1, RespSize-2);
}

void setup(){
  Serial.begin(9600);
  while(!Serial);
  Serial.println("Read mifare classic data block 4 with PN7150");
  
  Serial.println("Initializing...");                
  if (nfc.connectNCI()) { //Wake up the board
    Serial.println("Error while setting up the mode, check connections!");
    while (1);
  }
  
  if (nfc.ConfigureSettings()) {
    Serial.println("The Configure Settings is failed!");
    while (1);
  }
  
  if(nfc.ConfigMode(mode)){ //Set up the configuration mode
    Serial.println("The Configure Mode is failed!!");
    while (1);
  }
  nfc.StartDiscovery(mode); //NCI Discovery mode
  Serial.println("Waiting for an Mifare Classic Card ...");
}

void loop(){
  if(!nfc.WaitForDiscoveryNotification(&RfInterface)){ // Waiting to detect cards
    switch(RfInterface.Protocol) {
      case PROT_MIFARE:
        Serial.println(" - Found MIFARE card");
        switch(RfInterface.ModeTech) { //Indetify card technology
            case (MODE_POLL | TECH_PASSIVE_NFCA):
                char tmp[16];
                Serial.print("\tSENS_RES = ");
                sprintf(tmp, "0x%.2X",RfInterface.Info.NFC_APP.SensRes[0]);
                Serial.print(tmp); Serial.print(" ");
                sprintf(tmp, "0x%.2X",RfInterface.Info.NFC_APP.SensRes[1]);
                Serial.print(tmp); Serial.println(" ");
                
                Serial.print("\tNFCID = ");
                PrintBuf(RfInterface.Info.NFC_APP.NfcId, RfInterface.Info.NFC_APP.NfcIdLen);
                
                if(RfInterface.Info.NFC_APP.SelResLen != 0) {
                    Serial.print("\tSEL_RES = ");
                    sprintf(tmp, "0x%.2X",RfInterface.Info.NFC_APP.SelRes[0]);
                    Serial.print(tmp); Serial.println(" ");
                }
            break;
          }
          PCD_MIFARE_scenario();
          break;
      
      default:
          Serial.println(" - Found a card, but it is not Mifare");
          break;
    }
    
    //* It can detect multiple cards at the same time if they use the same protocol 
    if(RfInterface.MoreTags) {
        Serial.println("detect multiple cards");
        nfc.ReaderActivateNext(&RfInterface);
        Serial.print("\tNFCID = ");
        PrintBuf(RfInterface.Info.NFC_APP.NfcId, RfInterface.Info.NFC_APP.NfcIdLen);
        if(RfInterface.MoreTags)
        {
          Serial.println("il y a un autre carte");
        }
    }
    //* Wait for card removal 
    nfc.ProcessReaderMode(RfInterface, PRESENCE_CHECK);
    Serial.println("CARD REMOVED!");
    
    nfc.StopDiscovery();
    nfc.StartDiscovery(mode);
  }
  ResetMode();
  delay(500);
}
