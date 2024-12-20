#define MNINIT
#include "section_sys.h"			/* @032wxOOM[10/03/02] */
#include "sr103sfe4k.h"		/* sr103sa5.h -> sr103sfe4k.h 	*/
#include "dec.h"
#include "cu_head.h"

#define VREFER085 43
#define VREFER115 59
#define VREFER365 186
#define VREFER435 222
#define COMPEEPROMEND_check	0x0180


UCHAR ucCPU_registertest(void);								/*register test function	*/
UCHAR ucCPU_Ex_registertest(void);							/*register test function	*/
UCHAR ucMemory_rammarchtest(volatile UCHAR *,volatile USHORT);				/*RAM march test function*/
unsigned short getromcrc(unsigned int,unsigned short ,unsigned short);
unsigned short u16CRC_Calc16_eep( const unsigned char *, unsigned short,unsigned short);
unsigned char ep_read_exec_eep(UCHAR *,USHORT,USHORT);
UCHAR ucCPU_pctest(void);								/*CPU program counter test function*/
void ramcheck(void);
void romcheck(void);
void eepcheck(void);
void ADC05check(void);
void ADC15check(void);
void ADC16check(void);
void del4u_boot(void);
void overcurrent(void);
void overcmpha(void);

volatile USHORT ulfuctionsel = 0;
volatile UCHAR ucResult;						/* It stores the result of each test */
volatile USHORT uramaddress_num;
volatile USHORT uramaddress = 0x0000;
volatile USHORT ulChecksum = 0;						/* It stores the value of the Checksum in total */
volatile USHORT eepChecksum = 0;	
volatile unsigned int ucromAddress = 0x40000000;		/*  variable to store away top address */
volatile unsigned int ucramAddress = 0;
volatile USHORT ulLength,ulromLength;					/* intput the length of the test area */
volatile UCHAR *ucAddress = 0;						/* A pointer variable to store away top address */
UCHAR r_interrupttm;
volatile USHORT cnt_interrupt_ad0;				/* Interrupt count*/
volatile USHORT cnt_interrupt_ad01;	
volatile USHORT cnt_interrupt_ad1;				/* Interrupt count*/
volatile USHORT jishu;
extern unsigned short pwm_fc;	/* carrier */
extern  UCHAR	acc05_buf[8],acc13_buf[8],acc14_buf[8],acc20_buf[8],acc26_buf[8],acc05,acc13,acc14,acc20,acc26,acc06,acc07;	/* AD saving 		*/

volatile USHORT r_adcselect;
USHORT caleepromCRC_check(USHORT strtaddress,USHORT endaddress);

/*********************************************************

MCU self check fuction

***********************************************************/
void overcurrent(void)
{
	if(ERR_CMPPHAPK != 1)
	{
		//IO_TEST  = 1;
		//PAOUT |= 0x02;
	}
	if((!F_DIAG||(r_diagsts==5))&&((Iu_data >=r_cmpphaipeak[2])||(Iv_data >=r_cmpphaipeak[2])||(Iw_data >=r_cmpphaipeak[2]))){
		overcurrent_cnt++;
		if(overcurrent_cnt>=3)
		{
			overcurrent_cnt = 0;
			//IO_TEST  = 0;
			//PAOUT &= ~0x02;
			r_cmpdrerr.BIT.f_phaipk = 1;
			sts.err |= 0x40;
			ERR_CMPPHAPK = 1;
			pre_trip();
		}
	}else{
		overcurrent_cnt = 0;
	}


	
}
void overcmpha(void)
{
#if 0
	if(ERR_CMPPHA!=1)
	{
		IO_TEST  = 1;
		PAOUT |= 0x02;
	}
#endif
	{
			int max,min;

			max = min = 0;
			if(COMP&&(r_cmpstrtm>=12)){
				if(Iu_max_dis_sys>=Iv_max_dis_sys){
					max = Iu_max_dis_sys;
					min = Iv_max_dis_sys;
				}else{
					max = Iv_max_dis_sys;
					min = Iu_max_dis_sys;
				}
				if(max >= Iw_max_dis_sys){
					if(min >= Iw_max_dis_sys)
						min = Iw_max_dis_sys;
				}else{
					max = Iw_max_dis_sys;
				}
			}else{
				r_phasetm = 0;
			}
			if(max == 0){
				r_phasetm =0;
			}else if(((min*100/max) >55)||(max<=30)){
				r_phasetm =0;
				IO_TEST  = 1;
		}else{
				
				IO_TEST=0;
				if(r_phasetm >= 2)
					{
						ERR_CMPPHA = 1;
						//IO_TEST=0;
					}else{
							
						}
			}
	}
}
void mcu_self_check(void)
{
	disable_irq();	/* disenable Interrupt*/
	ucResult = 1;	/* initialize ucResult */

	ulfuctionsel++;
	if(ulfuctionsel == 0){
		
	}else if(ulfuctionsel == 1){						/* CPU Register Test */						
		ucResult = ucCPU_registertest();		/* call ucCPU_registertest function */
		if(!ucResult)	ERR_REGISTER =1;
	}else if(ulfuctionsel ==2){
		ucResult = ucCPU_Ex_registertest();	
		if(!ucResult)	ERR_REGISTER =1;
	}else if(ulfuctionsel ==3){					/* CPU Program Counter Test */
		ucResult = ucCPU_pctest();				/* call ucCPU_pctest function */
		if(!ucResult)	ERR_REGISTER =1;
	}else if(ulfuctionsel <=4){			/* 3-10 8times 11*/
		enable_irq();
		ADC05check();						/* ADC0 chanel5*/
	}else if(ulfuctionsel <=5){			/* 11-18 8times*/
		enable_irq();
		ADC15check();						/* ADC1 chanel5*/		
	}else if(ulfuctionsel <=27){			/* 19-26 8times*/
			////enable_irq();
			////ADC16check();						/* ADC1 chanel6*/		
	}else{
		ramcheck();			/* RAM Test*/		
	}


	enable_irq();	/* enable Interrupt*/	
}

/****************************
Variable memory  (RAM Test)
*****************************/

void ramcheck(void)
{	
	ucAddress = (UCHAR *)uramaddress; 					/* intput top address */
	ulLength = 0x0001;								/* intput the length of the test area */
	ucResult = ucMemory_rammarchtest(ucAddress,ulLength);
													/* call ucMemory_rammarchtest function */
	if(!ucResult)	
	{
		ERR_REGISTER =1;
		uramaddress_num = uramaddress;
	}
	uramaddress++;
	
	if(uramaddress >= 0x2000){
		uramaddress = 0;
		ulfuctionsel = 0;
	}
}

/****************************
	ROM check
*****************************/

void romcheck(void)
{
	ulromLength = 0x100;

	ulChecksum = getromcrc(ucromAddress,ulromLength,ulChecksum);
													/* call ulMemory_romchecksum function */
	ucromAddress += ulromLength;
	if(ucromAddress >=  0x40040000){
		if(ulChecksum !=r_firewarecrc) ERR_ROM =1;
		ulChecksum = 0x0000;							/* initialize ulChecksum */
		ucromAddress = 0x40000000;							/* intput top address */
	}else{}		
}
void eepcheck(void)
{	
	//P9OUT |= 0x80;
	eepChecksum = caleepromCRC_check(0,COMPEEPROMEND_check);
	//P9OUT &= ~0x80;
	if(eepChecksum !=r_configcrc) 
	{
		ERR_EEP =1;
	}
	eepChecksum = 0x0000;	
}

/****************************
	ADC check
*****************************/
void adcheck(void)
{
	if(DC_ONE){
		////acc05 = (UCHAR)ad_heikin(acc05_buf);
		////if((acc05 < VREFER085)||(acc05 > VREFER115))	ERR_ADC = 1;			/* 0.85V ~ 1.15V*/
		
		////acc13 = (UCHAR)ad_heikin(acc13_buf);
		////if((acc13 < VREFER085)||(acc13 > VREFER115))	ERR_ADC = 1;			/* 0.85V ~ 1.15V*/
		
/*		acc14 = (UCHAR)ad_heikin(acc14_buf);
		if((acc14 < VREFER085)||(acc14 > VREFER115))	ERR_ADC = 1;	*/		/* 3.65 ~ 4.35*/

/*		acc20 = (UCHAR)ad_heikin(acc20_buf);
		if((acc20 < VREFER085)||(acc20 > VREFER115))	ERR_ADC = 1;	*/		/* 0.85V ~ 1.15V*/
		
		acc26 = (UCHAR)ad_heikin(acc26_buf);
		if((acc26 < VREFER365)||(acc26 > VREFER435)) 	
		{
			ERR_ADC = 1;			/* 3.65 ~ 4.35*/	
		}
	}else{}
}



void ADC05check(void)
{
	static UCHAR acc05cnt=0;

	AN0CTR0 = 0x0548;	/* ADIN05 IOCLOCK/3*/
	
	/* wait for 333ns*/
	{
	volatile unsigned int i;
	for (i = 0 ; i < 1 ; i++) {}		/* wait (5 + 5*i)*33.3333ns *//* 333.3ns*/
	}
	disable_irq();
	G26ICR_C = 0x01;
	enable_irq();
	
	AN0CTR0 |= 0x0080;		/* ADC start*/
	while((G26ICR & 0x0010)== 0){;}
	AN0CTR0 &= ~0x0080;		/* ADC stop*/
	//acc05_buf[acc05cnt++] = (UCHAR)(AN0BUF05 >>2);
	acc06=(UCHAR)(AN0BUF05>>2);
	if((acc06 < VREFER085)||(acc06 > VREFER115))	
	{
		ERR_ADC = 1;
	}
	//if(acc05cnt >= 8){
	//	acc05cnt = 0;		
	//}
	AN0CTR0 = AD00_TSET_1;
}

void ADC15check(void)
{
	static UCHAR acc13cnt =0;

	AN1CTR0 = 0x0348;	/* ADIN05 IOCLOCK/3 */

	/* wait for 333ns*/
	{
	volatile unsigned int i;
	for (i = 0 ; i < 1 ; i++) {}		/* wait (5 + 5*i)*33.3333ns *//* 333.3ns*/		
	}
	disable_irq();
	G27ICR_C = 0x01;
	enable_irq();
	
	AN1CTR0 |= 0x0080;		/* ADC start*/
	while((G27ICR & 0x0010)== 0){;}
	AN1CTR0 &= ~0x0080;		/* ADC stop*/

	//acc13_buf[acc13cnt++] = (UCHAR)(AN1BUF05 >>2);
	acc07=(UCHAR)(AN0BUF05>>2);
	if((acc07 < VREFER085)||(acc07 > VREFER115))	
	{
		ERR_ADC = 1;
	}
	//if(acc13cnt >= 8){
	//	acc13cnt = 0;		
	//}	
	AN1CTR0 = AD10_TSET_3;
}

void ADC16check(void)
{
	static UCHAR acc14cnt =0;
	
	AN1CTR0 = 0x0448;	/* ADIN06 IOCLOCK/3 */

	/* wait for 333ns*/
	{
	volatile unsigned int i;
	for (i = 0 ; i < 1 ; i++) {}		/* wait (5 + 5*i)*33.3333ns *//* 333.3ns*/		
	}
	disable_irq();
	G27ICR_C = 0x01;
	enable_irq();
	
	AN1CTR0 |= 0x0080;		/* ADC start*/
	while((G27ICR & 0x0010)== 0){;}
	AN1CTR0 &= ~0x0080;		/* ADC stop*/

	acc14_buf[acc14cnt++] = (UCHAR)(AN1BUF06 >>2);
	if(acc14cnt >= 8){
		acc14cnt = 0;
	}	
}


void AD2check(void)
{
	static UCHAR acc2cnt = 0;

	//exec_ad2chg(0);
	//acc20_buf[acc2cnt] = (UCHAR)(AN2BUF06 >>2);  /* ADIN06 IOCLOCK/3 */

	exec_ad2chg(6);
	acc26_buf[acc2cnt] = (UCHAR)(AN2BUF12 >>2);  /* ADIN12 IOCLOCK/3 */

	if(++acc2cnt >= 8){
		acc2cnt = 0;
	}
	AN2CTR0 = ( 0x0054 | (acc2cnt << 8) );
	r_adcselect = ( 0x0054 | (acc2cnt << 8) );
//	if(AN2CTR0!= r_adcselect) 
//		{
//			ERR_ADC = 1;
//		}			
}

void interrupt_check(void)
{
	static unsigned short pwm_fcpre;
	//P9OUT = ~(0x80&P9OUT);
	if(pwm_fcpre != pwm_fc){  /*what is pwm_fc zaibo?eeprom?*/
		r_interrupttm = 0;
		cnt_interrupt_ad0 = 0;
		cnt_interrupt_ad1 = 1;		/*why = 1*/
	}else{}
		pwm_fcpre = pwm_fc;
#if 0
	if(r_interrupttm == 8){			/* wait for 4ms *250 = 1sec*/		
		acc26 = (UCHAR)ad_heikin(acc26_buf);
		if((acc26 < VREFER365)||(acc26 > VREFER435)) 
		{
			ERR_ADC = 1;
		}			/* 3.65 ~ 4.35*/
		
	}
#endif
	if(r_interrupttm++ >= 250){			/* wait for 4ms *250 = 1sec*/
		if((cnt_interrupt_ad0 < (pwm_fc*9/10))||(cnt_interrupt_ad0 >= (pwm_fc*11/10)))   /*where cnt_interrupt_ad0++*/
			ERR_REGISTER =1;
		//if((cnt_interrupt_ad1 < (pwm_fc*9/10))||(cnt_interrupt_ad1 >= (pwm_fc*11/10)))  /*where cnt_interrupt_ad1++*/
			//ERR_REGISTER =1;
		//eepcheck();
		r_interrupttm = 0;
		cnt_interrupt_ad0 = 0;
		cnt_interrupt_ad1 = 0;
	}
}


unsigned short getromcrc(unsigned int romaddrstrt,unsigned short length,unsigned short u16CRC)
{
	unsigned char	*adr = 0;
	unsigned char i;

	adr = (unsigned char *)romaddrstrt;
	
	while(adr < (unsigned char *)(romaddrstrt+length)){
 
		u16CRC ^= ((unsigned short) *adr) << 8;
		++adr;
		for(i=0;i<8;i++){
			if( u16CRC & 0x8000 ){
				u16CRC = (u16CRC << 1) ^ 0x1021;
			}else{
				u16CRC = u16CRC << 1;
			}
		} 		
		
	}
	return u16CRC;
}
USHORT caleepromCRC_check(USHORT strtaddress,USHORT endaddress)
{
	unsigned char ramdata[16];
	unsigned short address,u16CRC;

	u16CRC= 0;
	for(address = strtaddress;address < endaddress;address+=16){
		ep_read_exec_eep(ramdata,address,16);
		u16CRC = u16CRC_Calc16_eep(ramdata,16,u16CRC);
	}
	return u16CRC;
}

unsigned short u16CRC_Calc16_eep( const unsigned char *pu8Data, unsigned short i16Len,unsigned short u16calCRC )
{
  unsigned char i;
  unsigned short u16CRC;
 
  u16CRC = u16calCRC;

  while( i16Len-- )
  {
    u16CRC = u16CRC ^ (((unsigned short) *pu8Data++) << 8);

    for(i=0;i<8;i++)
    {
      if( u16CRC & 0x8000 )
      {
        u16CRC = (u16CRC << 1) ^ 0x1021;
      }
      else
      {
        u16CRC = u16CRC << 1;
      }
    } 
  }
  return u16CRC;
}
unsigned char ep_read_exec_eep(UCHAR *ram_adr,USHORT ep_address,USHORT n)
{
	UCHAR work;
	union{
		USHORT WORD;
		struct{
			UCHAR lo;
			UCHAR hi;
		}BYTE;
	}spad_wk;
	
	UCHAR wk_data1;
	UCHAR com_err = 0;
	if(n==0){
		return(0);
	}
	
	del4u_boot();
	
	P1OUT &= (~0x40);
	
	work = 0x03;
	SC1CTR = (0x0a81 |0x8000);
	SC1TB = work;
	while((SC1STR&0x80)!=0){}
	del4u_boot();
	spad_wk.WORD = ep_address;
	
	work = spad_wk.BYTE.hi;	
	SC1CTR = (0x0a81 |0x8000);
	SC1TB = work;
	while((SC1STR&0x80)!=0){}
	del4u_boot();
	
	work = spad_wk.BYTE.lo;
	SC1CTR = (0x0a81 |0x8000);
	SC1TB = work;
	while((SC1STR&0x80)!=0){}
	del4u_boot();	
	
	SC1CTR = (0x0a81 |0x8000|0x4000);
	while(n > 0)
	{
		SC1TB = 0xff;
		while((SC1STR&0x80)!=0){}
		del4u_boot();
		if((SC1STR&0x80)!=0){
			com_err = 0x01;	
		}
		wk_data1 = SC1RB;
		del4u_boot();

		*ram_adr++ = wk_data1;
		n--;
	}
	SC1CTR = 0x0a81;
	
	P1OUT |= 0x40;
	return(com_err);
}
