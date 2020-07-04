#include "mfrc522.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "log.h"
#include "rtt_input.h"
#include "tqt_spi.h"
//extern MIFARE_Key key;
extern int block;

extern char blockcontent[16]; 
//byte blockcontent[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; //all zeros. This can be used to delete a block.
extern char readbackblock[18]; 							// Used by PICC_ReadCardSerial().
extern bool cardPresent;

void tqt_init_rfid_module();
void tqt_run_rfid_module();

