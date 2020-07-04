#include "mfrc522.h"
#define MFRC522_CS_LOW	nrf_gpio_pin_clear(SPI_SS_PIN);
#define MFRC522_CS_HIGH	nrf_gpio_pin_set(SPI_SS_PIN);

RFID tqt_rfid;
 bool isCard() 
 {
	unsigned char status;
	unsigned char str[MAX_LEN];

	status = MFRC522Request(PICC_REQIDL, str);	
    if (status == MI_OK) {
		return true;
	} else { 
		return false; 
	}
 }

 bool readCardSerial(){
	unsigned char status;
	unsigned char str[MAX_LEN];
	status = anticoll(str);
	memcpy(tqt_rfid.serNum, str, 5);

	if (status == MI_OK) {
		return true;
	} else {
		return false;
	}

 }
void init()
{
	tqt_init_spi_master();
	reset();
	//Timer: TPrescaler*TreloadVal/6.78MHz = 24ms
    writeMFRC522(TModeReg, 0x8D);		//Tauto=1; f(Timer) = 6.78MHz/TPreScaler
    writeMFRC522(TPrescalerReg, 0x3E);	//TModeReg[3..0] + TPrescalerReg
    writeMFRC522(TReloadRegL, 30);           
    writeMFRC522(TReloadRegH, 0);
	writeMFRC522(TxAutoReg, 0x40);		//100%ASK
	writeMFRC522(ModeReg, 0x3D);		// CRC valor inicial de 0x6363
	antennaOn();		//Abre  la antena
}
void reset()
{
	writeMFRC522(CommandReg, PCD_RESETPHASE);
}

void writeMFRC522(unsigned char addr, unsigned char val)
{
	MFRC522_CS_LOW;
	char tqt_var = (addr<<1)&0x7E;
	tqt_run_spi_master(&tqt_var);	
        nrf_delay_ms(300);
	tqt_run_spi_master(&val);

	MFRC522_CS_HIGH;
}

void antennaOn(void)
{
	unsigned char temp;

	temp = readMFRC522(TxControlReg);
	if (!(temp & 0x03))
	{
		setBitMask(TxControlReg, 0x03);
	}
}

unsigned char readMFRC522(unsigned char addr)
{
	unsigned char val;
	MFRC522_CS_LOW;
	char tqt_var = ((addr<<1)&0x7E) | 0x80;
	tqt_run_spi_master(&tqt_var);	
	
	val =tqt_run_spi_master(0x00);
	MFRC522_CS_HIGH;
	return val;	
}

void setBitMask(unsigned char reg, unsigned char mask)  
{ 
    unsigned char tmp;
    tmp = readMFRC522(reg);
    writeMFRC522(reg, tmp | mask);  // set bit mask
}

void clearBitMask(unsigned char reg, unsigned char mask)  
{
    unsigned char tmp;
    tmp = readMFRC522(reg);
    writeMFRC522(reg, tmp & (~mask));  // clear bit mask
} 

void calculateCRC(unsigned char *pIndata, unsigned char len, unsigned char *pOutData)
{
    unsigned char i, n;

    clearBitMask(DivIrqReg, 0x04);			//CRCIrq = 0
    setBitMask(FIFOLevelReg, 0x80);		
    for (i=0; i<len; i++)
    {   
		writeMFRC522(FIFODataReg, *(pIndata+i));   
	}
    writeMFRC522(CommandReg, PCD_CALCCRC);
    i = 0xFF;
    do 
    {
        n = readMFRC522(DivIrqReg);
        i--;
    }
    while ((i!=0) && !(n&0x04));			
    pOutData[0] = readMFRC522(CRCResultRegL);
    pOutData[1] = readMFRC522(CRCResultRegM);
}

unsigned char MFRC522ToCard(unsigned char command, unsigned char *sendData, unsigned char sendLen, unsigned char *backData, unsigned int *backLen)
{
    unsigned char status = MI_ERR;
    unsigned char irqEn = 0x00;
    unsigned char waitIRq = 0x00;
	unsigned char lastBits;
    unsigned char n;
    unsigned int i;

    switch (command)
    {
        case PCD_AUTHENT:		// Tarjetas de certificaci?n cerca
		{
			irqEn = 0x12;
			waitIRq = 0x10;
			break;
		}
		case PCD_TRANSCEIVE:	//La transmisi?n de datos FIFO
		{
			irqEn = 0x77;
			waitIRq = 0x30;
			break;
		}
		default:
			break;
    }

    writeMFRC522(CommIEnReg, irqEn|0x80);	//De solicitud de interrupci?n
    clearBitMask(CommIrqReg, 0x80);			// Borrar todos los bits de petici?n de interrupci?n
    setBitMask(FIFOLevelReg, 0x80);	
	writeMFRC522(CommandReg, PCD_IDLE);	
    for (i=0; i<sendLen; i++)
    {   
		writeMFRC522(FIFODataReg, sendData[i]);    
	}
	writeMFRC522(CommandReg, command);
    if (command == PCD_TRANSCEIVE)
    {    
		setBitMask(BitFramingReg, 0x80);		
	}   
	i = 2000;	
    do 
    {
        n = readMFRC522(CommIrqReg);
        i--;
    }
    while ((i!=0) && !(n&0x01) && !(n&waitIRq));

    clearBitMask(BitFramingReg, 0x80);			//StartSend=0

    if (i != 0)
    {    
      if(!(readMFRC522(ErrorReg) & 0x1B))	//BufferOvfl Collerr CRCErr ProtecolErr
    {
        status = MI_OK;
        if (n & irqEn & 0x01)
        {   
          status = MI_NOTAGERR;			//??   
        }

        if (command == PCD_TRANSCEIVE)
        {
            n = readMFRC522(FIFOLevelReg);
            lastBits = readMFRC522(ControlReg) & 0x07;
            if (lastBits)
            {   
                *backLen = (n-1)*8 + lastBits;   
            }
            else
            {   
                *backLen = n*8;   
            }

            if (n == 0)
            {   
                n = 1;    
            }
            if (n > MAX_LEN)
            {   
                n = MAX_LEN;   
            }

            //??FIFO??????? Lea los datos recibidos en el FIFO
            for (i=0; i<n; i++)
            {   
                backData[i] = readMFRC522(FIFODataReg);    
            }
        }
    }
    else
    {   
        status = MI_ERR;  
    }

    }
    return status;
}
unsigned char  MFRC522Request(unsigned char reqMode, unsigned char *TagType)
{
	unsigned char status;  
	unsigned int backBits;			//   Recibi? bits de datos

	writeMFRC522(BitFramingReg, 0x07);		//TxLastBists = BitFramingReg[2..0]	???

	TagType[0] = reqMode;
	status = MFRC522ToCard(PCD_TRANSCEIVE, TagType, 1, TagType, &backBits);

	if ((status != MI_OK) || (backBits != 0x10))
	{    
		status = MI_ERR;
	}

	return status;
}
unsigned char anticoll(unsigned char *serNum)
{
    unsigned char status;
    unsigned char i;
	unsigned char serNumCheck=0;
    unsigned int unLen;
	writeMFRC522(BitFramingReg, 0x00);		//TxLastBists = BitFramingReg[2..0]

    serNum[0] = PICC_ANTICOLL;
    serNum[1] = 0x20;
    status = MFRC522ToCard(PCD_TRANSCEIVE, serNum, 2, serNum, &unLen);

    if (status == MI_OK)
	{
		for (i=0; i<4; i++)
		{   
		 	serNumCheck ^= serNum[i];
		}
		if (serNumCheck != serNum[i])
		{   
			status = MI_ERR;    
		}
    }
    return status;
}
unsigned char auth(unsigned char authMode, unsigned char BlockAddr, unsigned char *Sectorkey, unsigned char *serNum)
{
    unsigned char status;
    unsigned int recvBits;
    unsigned char i;
	unsigned char buff[12]; 
    buff[0] = authMode;
    buff[1] = BlockAddr;
    for (i=0; i<6; i++)
    {    
		buff[i+2] = *(Sectorkey+i);   
	}
    for (i=0; i<4; i++)
    {    
		buff[i+8] = *(serNum+i);   
	}
    status = MFRC522ToCard(PCD_AUTHENT, buff, 12, buff, &recvBits);

    if ((status != MI_OK) || (!(readMFRC522(Status2Reg) & 0x08)))
    {   
		status = MI_ERR;   
	}

    return status;
}
unsigned char read(unsigned char blockAddr, unsigned char *recvData)
{
    unsigned char status;
    unsigned int unLen;

    recvData[0] = PICC_READ;
    recvData[1] = blockAddr;
    calculateCRC(recvData,2, &recvData[2]);
    status = MFRC522ToCard(PCD_TRANSCEIVE, recvData, 4, recvData, &unLen);

    if ((status != MI_OK) || (unLen != 0x90))
    {
        status = MI_ERR;
    }

    return status;
}
unsigned char write(unsigned char blockAddr, unsigned char *writeData)
{
    unsigned char status;
    unsigned int recvBits;
    unsigned char i;
	unsigned char buff[18]; 

    buff[0] = PICC_WRITE;
    buff[1] = blockAddr;
    calculateCRC(buff, 2, &buff[2]);
    status = MFRC522ToCard(PCD_TRANSCEIVE, buff, 4, buff, &recvBits);

    if ((status != MI_OK) || (recvBits != 4) || ((buff[0] & 0x0F) != 0x0A))
    {   
		status = MI_ERR;   
	}

    if (status == MI_OK)
    {
        for (i=0; i<16; i++)		//?FIFO?16Byte?? Datos a la FIFO 16Byte escribir
        {    
        	buff[i] = *(writeData+i);   
        }
        calculateCRC(buff, 16, &buff[16]);
        status = MFRC522ToCard(PCD_TRANSCEIVE, buff, 18, buff, &recvBits);

		if ((status != MI_OK) || (recvBits != 4) || ((buff[0] & 0x0F) != 0x0A))
        {   
			status = MI_ERR;   
		}
    }

    return status;
}
void halt()
{
    unsigned char status;
    unsigned int unLen;
    unsigned char buff[4];

    buff[0] = PICC_HALT;
    buff[1] = 0;
    calculateCRC(buff, 2, &buff[2]);

    clearBitMask(Status2Reg, 0x08); // turn off encryption

    status = MFRC522ToCard(PCD_TRANSCEIVE, buff, 4, buff,&unLen);
}
