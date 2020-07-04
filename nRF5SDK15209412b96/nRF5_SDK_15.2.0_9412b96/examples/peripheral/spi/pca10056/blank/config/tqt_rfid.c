#include "tqt_rfid.h"
//Present = false;
//uint8_t CardID[5];

//MIFARE_Key key;
int block=2;

char blockcontent[16] = "1234567890123456"; 
//byte blockcontent[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; //all zeros. This can be used to delete a block.
char readbackblock[18]; 							// Used by PICC_ReadCardSerial().
bool cardPresent = false;
unsigned char reading_card[5]; //for reading card
void tqt_init_rfid_module() {
    //TM_MFRC522_Init();
    init();
}

void tqt_run_rfid_module() {
    //uint8_t CardID[5];
//    do {
//            cardPresent = PICC_IsNewCardPresent(); 	// sets successRead to 1 when we get read from reader otherwise 0
//    } while (!cardPresent); 	//the program will not go further while you not get a successful read
//    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "TQT DETECTED CARD\n");
//    if (PICC_ReadCardSerial()) {
//            printf("TQT: %02x%02x%02x%02x\r\n", rfid_uid.uidByte[0], rfid_uid.uidByte[1], rfid_uid.uidByte[2], rfid_uid.uidByte[3]);
//    }

//    if (TM_MFRC522_Check(CardID) == MI_OK) {
//        __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "TQT Tag ID: %d %d %d %d %d\r\n", CardID[0], CardID[1], 
//                                                                            CardID[2], CardID[3], CardID[4]);
//    }


//Test arduino code
//bool tqt_check = isCard();
//__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "TQT RUN: %d\n", tqt_check);
    if (isCard())
    {
        if (readCardSerial())
        {
        __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "TQT Tag ID: %d %d %d %d %d \n", tqt_rfid.serNum[0], tqt_rfid.serNum[1], 
                                                                            tqt_rfid.serNum[2], tqt_rfid.serNum[3], tqt_rfid.serNum[4]);
         }
    }
    halt();
}