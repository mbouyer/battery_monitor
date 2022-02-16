/* $Id: main.c,v 1.36 2019/03/12 19:24:19 bouyer Exp $ */
/*
 * Copyright (c) 2022 Manuel Bouyer
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <xc.h>
#include <stdio.h> 
#include <string.h> 
#include <stdlib.h> 
#include <nmea2000.h>
#include <nmea2000_pgn.h>
#include <raddeg.h>
#include "serial.h"
#include "i2c.h"
#include "ntc_tab.h"
#include "pac195x.h"

unsigned int devid, revid; 

unsigned long nmea2000_user_id; 

static unsigned char sid;

static struct nmea2000_msg msg;
static unsigned char nmea2000_data[NMEA2000_DATA_FASTLENGTH];

unsigned int timer0_read(void);

#define TIMER0_5MS 48
#define TIMER0_1MS 10

#define LEDBATT_R LATCbits.LATC0
#define LEDBATT_G LATCbits.LATC1

#define NDOWN LATCbits.LATC7
#define NCANOK PORTCbits.RC2

static char counter_10hz;
static char counter_1hz;
static char seconds;
static volatile union softintrs {
	struct softintrs_bits {
		char int_10hz : 1;	/* 0.1s timer */
	} bits;
	char byte;
} softintrs;

static uint16_t a2d_acc;

#define PAC_I2C_ADDR 0x2e

int16_t batt_v[4];
int16_t batt_i[4];
uint16_t batt_temp[4];

static int32_t voltages_acc[4];
pac_ctrl_t pac_ctrl;

static void
adctotemp(unsigned char c)
{
	char i;
	for (i = 1; temps[i].val != 0; i++) {
		if (a2d_acc > temps[i].val) {
			batt_temp[c] = temps[i - 1].temp -
			((temps[i - 1].temp - temps[i].temp)  /
		         (temps[i - 1].val - temps[i].val)  * 
			 (temps[i - 1].val - a2d_acc));
			return;
		}
	} 
	batt_temp[c] = 0xffff;
}

static void
send_batt_status(char c)
{
	if (nmea2000_status != NMEA2000_S_OK)
		return;

	struct nmea2000_battery_status_data *data = (void *)&nmea2000_data[0];

	if ((pac_ctrl.ctrl_chan_dis & (8 >> c)) != 0)
		return;

	PGN2ID(NMEA2000_BATTERY_STATUS, msg.id);
	msg.id.priority = NMEA2000_PRIORITY_INFO;
	msg.dlc = sizeof(struct nmea2000_battery_status_data);
	msg.data = &nmea2000_data[0];
	data->voltage = batt_v[c];
	data->current = batt_i[c];
	data->temp = batt_temp[c];
	data->sid = sid;
	data->instance = c;
	if (! nmea2000_send_single_frame(&msg))
		printf("send NMEA2000_BATTERY_STATUS failed\n");
}

#if 0

static void
send_dc_status(void)
{
	struct nmea2000_dc_status_data *data = (void *)&nmea2000_data[0];
	static unsigned char fastid;

	if (input_volt == 0xffff)
		return;

	fastid = (fastid + 1) & 0x7;
	printf("power voltage %d.%03dV\n",
	    (int)(input_volt / 1000), (int)(input_volt % 1000));

	PGN2ID(NMEA2000_DC_STATUS, msg.id);
	msg.id.priority = NMEA2000_PRIORITY_INFO;
	msg.dlc = sizeof(struct nmea2000_dc_status_data);
	msg.data = &nmea2000_data[0];
	data->sid = sid;
	data->instance = 0;
	switch(power_status) {
	case UNKOWN:
		return;
	case OFF:
		data->type = DCSTAT_TYPE_BATT;
		if (batt_v > 1240) {
			data->soc = 100 - ((unsigned long)time_on_batt * 100UL / 7200UL);
		} else {
			data->soc = 50 - (1240 - batt_v) * 50 / (1240 - 1100);
		}
		data->soh = 0xff;
		data->timeremain = 0xffff; /* XXX compute */
		data->ripple = 0xffff;
		break;
	default:
		/* assume operating on mains power */
		data->type = DCSTAT_TYPE_CONV;
		data->soc = 0xff;
		data->soh =
		    ((unsigned long)input_volt * 100UL + 6000UL) / 12000;
		data->timeremain = 0xffff;
		data->ripple = 0xffff;
		break;
	}
	if (! nmea2000_send_fast_frame(&msg, fastid))
		printf("send NMEA2000_DC_STATUS failed\n");
}

static void
send_charger_status()
{
	struct nmea2000_charger_status_data *data = (void *)&nmea2000_data[0];

	PGN2ID(NMEA2000_CHARGER_STATUS, msg.id);
	msg.id.priority = NMEA2000_PRIORITY_INFO;
	msg.dlc = sizeof(struct nmea2000_charger_status_data);
	msg.data = &nmea2000_data[0];
	data->instance = 0;
	data->batt_instance = 0;
	data->op_state = charger_status;
	data->mode = CHARGER_MODE_STANDALONE;
	data->enable = 1;
	data->eq_pending = 0;
	data->eq_time_remain = 0;
	if (! nmea2000_send_single_frame(&msg))
		printf("send NMEA2000_CHARGER_STATUS failed\n");
}
#endif

void
user_handle_iso_request(unsigned long pgn)
{
	printf("ISO_REQUEST for %ld from %d\n", pgn, rid.saddr);
	switch(pgn) {
	case NMEA2000_BATTERY_STATUS:
		for (char c = 0; c < 4; c++) {
			if ((pac_ctrl.ctrl_chan_dis & (8 >> c)) == 0)
				send_batt_status(c);
		}
		break;
#if 0
	case NMEA2000_DC_STATUS:
		send_dc_status();
		break;
	case NMEA2000_CHARGER_STATUS:
		send_charger_status();
		break;
#endif
	}
}

void
user_receive()
{
	unsigned long pgn;

	pgn = ((unsigned long)rid.page << 16) | ((unsigned long)rid.iso_pg << 8);
	if (rid.iso_pg > 239)
		pgn |= rid.daddr;

#if 0
	switch(pgn) {
	case PRIVATE_COMMAND_STATUS:
	{
		struct private_command_status *command_status = (void *)rdata;
		last_command_data = timer0_read();
		nmea2000_command_address = rid.saddr;
		command_received_heading = command_status->heading;
		received_auto_mode = command_status->auto_mode;
		received_param_slot = command_status->params_slot;
		break;
	}
	}
#endif
}

void
putch(char c)
{
        if (PORTBbits.RB7) {
		usart_putchar(c);
	}
}

static void
read_pac_channel(void)
{
	char c;
	static pac_acccnt_t pac_acccnt;
	int64_t acc_value;
	double v;

	if (i2c_readreg_be(PAC_I2C_ADDR, PAC_ACCCNT,
	    &pac_acccnt, sizeof(pac_acccnt)) != sizeof(pac_acccnt)) {
		printf("read pac_acccnt fail\n");
		pac_acccnt.acccnt_count = 0;
		return;
	} 
	printf("\n %d count %lu", NCANOK, pac_acccnt.acccnt_count);

	for (c = 0; c < 4; c++) {
		if ((pac_ctrl.ctrl_chan_dis & (8 >> c)) != 0)
			continue;

		acc_value = 0;
		if (i2c_readreg_be(PAC_I2C_ADDR, PAC_ACCV1 + c,
		    &acc_value, 7) != 7) {
			printf("read acc_value[%d] fail\n", c);
			continue;
		}
		if (acc_value & 0x0080000000000000) {
			/* adjust negative value */
			acc_value |= 0xff00000000000000;
		}
		/* batt_i = acc_value * 0.00075 * 100 */
		v = (double)acc_value * 0.075 / pac_acccnt.acccnt_count;
		batt_i[c] = v + 0.5;
		printf(" %d %4.4fA", c, v / 100);
		/* volt = vbus * 0.000488 */
		/* batt_v = voltages_acc * 0.000488 * 100 / 10; */
		v = (double)voltages_acc[c] * 0.00488;
		batt_v[c] = v + 0.5;
		printf(" %4.3fV", v / 100);
	}
}

int
main(void)
{
	char c;
	uint8_t i2cr;
	pac_accumcfg_t pac_accumcfg;
	pac_neg_pwr_fsr_t pac_neg_pwr_fsr;
	static unsigned int poll_count;
	uint16_t t0;
	static int32_t voltages_acc_cur[4];


	devid = 0;
	revid = 0;
	nmea2000_user_id = 0;
	/* read devid and user IDs */
	TBLPTR = 0x200000;
	asm("tblrd*+;");
	asm("movff TABLAT, _nmea2000_user_id;");
	asm("tblrd*+;");
	asm("movff TABLAT, _nmea2000_user_id + 1;");
	TBLPTR = 0x3ffffc;
	asm("tblrd*+;");
	asm("movff TABLAT, _revid;");
	asm("tblrd*+;");
	asm("movff TABLAT, _revid + 1;");
	asm("tblrd*+;");
	asm("movff TABLAT, _devid;");
	asm("tblrd*+;");
	asm("movff TABLAT, _devid + 1;");

	/* disable unused modules */
	PMD0 = 0x7a; /* keep clock and IOC */
#ifdef USE_TIMER2
	PMD1 = 0xfa; /* keep timer0/timer2 */
	PMD2 = 0x03; /* keep can module */
#else
	PMD1 = 0xfe; /* keep timer0 */
	PMD2 = 0x02; /* keep can module, TU16A */
#endif
	PMD3 = 0xdf; /* keep ADC */
	PMD4 = 0xff;
	PMD5 = 0xff;
	PMD6 = 0xf6; /* keep UART1 and I2C */
	PMD7 = 0xff;
	PMD8 = 0xff;

	ANSELC = 0;
	LEDBATT_G = LEDBATT_R = NDOWN = 0;
	TRISCbits.TRISC0 = TRISCbits.TRISC1 = TRISCbits.TRISC7 = 0;
	LEDBATT_R = 1;

	ANSELB = 0;
	/* CANRX on RB3 */
	CANRXPPS = 0x0B;
	/* CANTX on RB2 */
	LATBbits.LATB2 = 1; /* output value when idle */
	TRISBbits.TRISB2 = 0;
	RB2PPS = 0x46;

	LATCbits.LATC5 = TRISCbits.TRISC5 = 0; /* IO1 */
	LATBbits.LATB4 = TRISBbits.TRISB4 = 0; /* IO2 */
	LATBbits.LATB5 = TRISBbits.TRISB5 = 0; /* IO3 */

	ANSELA = 0x13; /* RA0, RA1 and RA4 analog */
	LATA = 0;
	TRISA = 0xD3; /* RA2, RA3, RA5 as outpout */

	/* configure watchdog timer for 2s */
	WDTCON0 = 0x16;
	WDTCON1 = 0x07;

	/* configure sleep mode: PRI_IDLE */
	CPUDOZE = 0x80;

	softintrs.byte = 0;
	counter_10hz = 25;
	counter_1hz = 10;
	seconds = 0;

	for (c = 0; c < 4; c++) {
		voltages_acc_cur[c] = 0;
	}

	IPR1 = 0;
	IPR2 = 0;
	IPR3 = 0;
	IPR4 = 0;
	IPR5 = 0;
	IPR6 = 0;
	IPR7 = 0;
	IPR8 = 0;
	IPR9 = 0;
	IPR10 = 0;
	IPR11 = 0;
	IPR12 = 0;
	IPR13 = 0;
	IPR14 = 0;
	IPR15 = 0;
	INTCON0 = 0;
	INTCON1 = 0;
	INTCON0bits.IPEN=1; /* enable interrupt priority */

	USART_INIT(0);

	/* configure timer0 as free-running counter at 9.765625Khz * 4 */
	T0CON0 = 0x0;
	T0CON0bits.MD16 = 1; /* 16 bits */
	T0CON1 = 0x48; /* 01001000 Fosc/4, sync, 1/256 prescale */
	PIR3bits.TMR0IF = 0;
	PIE3bits.TMR0IE = 0; /* no interrupt */
	T0CON0bits.T0EN = 1;

#ifdef USE_TIMER2
	/* configure timer2 for 250Hz interrupt */
	T2CON = 0x44; /* b01000100: postscaller 1/5, prescaler 1/16 */
	T2PR = 125; /* 250hz output */
	T2CLKCON = 0x01; /* Fosc / 4 */
	T2CONbits.TMR2ON = 1;
	PIR3bits.TMR2IF = 0;
	IPR3bits.TMR2IP = 1; /* high priority interrupt */
	PIE3bits.TMR2IE = 1;
#else
	/* configure UTMR for 10Hz interrupt */
	TU16ACON0 = 0x04; /* period match IE */
	TU16ACON1 = 0x00;
	TU16AHLT = 0x00; /* can't use hardware reset because of HW bug */
	TU16APS = 249; /* prescaler = 250 -> 40000 Hz */
	TU16APRH = (4000 >> 8) & 0xff;
	TU16APRL = 4000 & 0xff;
	TU16ACLK = 0x2; /* Fosc */
	TUCHAIN = 0;
	TU16ACON0bits.ON = 1;
	TU16ACON1bits.CLR = 1;
	IPR0bits.TU16AIP = 1; /* high priority interrupt */
	PIE0bits.TU16AIE = 1;
#endif

	I2C_INIT;

	INTCON0bits.GIEH=1;  /* enable high-priority interrupts */   
	INTCON0bits.GIEL=1; /* enable low-priority interrrupts */   

	LEDBATT_G = 1;

	printf("hello user_id 0x%lx devid 0x%x revid 0x%x\n", nmea2000_user_id, devid, revid);
	LEDBATT_R = 0;

	printf("n2k_init");
	nmea2000_init();
	poll_count = timer0_read();

	/* enable watchdog */
	WDTCON0bits.SEN = 1;

	/* set up ADC */
	PIR1bits.ADIF = 0;
	ADCON0 = 0x4; /* right-justified */
	ADCON1 = 0; /* no cap */
	ADCON2 = 0; /* basic mode */
	ADCON3 = 0; /* basic mode */
	ADCLK = 7; /* Fosc/16 */
	ADREF = 0;  /* vref = VDD */
	ADPCH = 0; /* channel 0 */
	ADACQH = 0;
	ADACQL = 20; /* 20Tad Aq */
	ADCON0bits.ADON = 1;

	c = 0;
	NDOWN = 1;

	/* wait 50ms for pac1953 to be up */
	for (c = 0; c < 50; c++) {
		t0 = timer0_read();
		while (timer0_read() - t0 < TIMER0_1MS) {
			; /* wait */
		}
	}
	
	LEDBATT_R = LEDBATT_G = 0;

again:
	c = 0;
	if (i2c_readreg(PAC_I2C_ADDR, PAC_PRODUCT, &i2cr, 1) != 1)
		c++;
	else 
		printf("product id 0x%x ", i2cr);

	if (i2c_readreg(PAC_I2C_ADDR, PAC_MANUF, &i2cr, 1) != 1)
		c++;
	else
		printf("manuf id 0x%x ", i2cr);

	if (i2c_readreg(PAC_I2C_ADDR, PAC_REV, &i2cr, 1) != 1)
		c++;
	else
		printf("rev id 0x%x\n", i2cr);

	if (i2c_readreg(PAC_I2C_ADDR,
	    PAC_CTRL_ACT, (uint8_t *)&pac_ctrl, sizeof(pac_ctrl)) !=
	    sizeof(pac_ctrl))
		c++;
	else {
		printf("CTRL_ACT al1 %d al2 %d mode %d dis %d\n",
		    pac_ctrl.ctrl_alert1,
		    pac_ctrl.ctrl_alert2,
		    pac_ctrl.ctrl_mode,
		    pac_ctrl.ctrl_chan_dis);
	}

	pac_ctrl.ctrl_alert1 = pac_ctrl.ctrl_alert2 = CTRL_ALERT_ALERT;
	pac_ctrl.ctrl_mode = CTRL_MODE_1024;

	if (i2c_writereg(PAC_I2C_ADDR,
	    PAC_CTRL, (uint8_t *)&pac_ctrl, sizeof(pac_ctrl)) == 0) {
		printf("wr PAC_CTRL fail\n");
		c++;
	}

	pac_accumcfg.accumcfg_acc1 = ACCUMCFG_VSENSE;
	pac_accumcfg.accumcfg_acc2 = ACCUMCFG_VSENSE;
	pac_accumcfg.accumcfg_acc3 = ACCUMCFG_VSENSE;
	pac_accumcfg.accumcfg_acc4 = ACCUMCFG_VSENSE;
	if (i2c_writereg(PAC_I2C_ADDR,
	    PAC_ACCUMCFG, (uint8_t *)&pac_accumcfg, sizeof(pac_accumcfg)) == 0) {
		printf("wr PAC_ACCUMCFG fail\n");
		c++;
	}

	pac_neg_pwr_fsr.pwrfsr_vb1 = pac_neg_pwr_fsr.pwrfsr_vs1 = PAC_PWRSFR_BHALF;
	pac_neg_pwr_fsr.pwrfsr_vb2 = pac_neg_pwr_fsr.pwrfsr_vs2 = PAC_PWRSFR_BHALF;
	pac_neg_pwr_fsr.pwrfsr_vb3 = pac_neg_pwr_fsr.pwrfsr_vs3 = PAC_PWRSFR_BHALF;
	pac_neg_pwr_fsr.pwrfsr_vb4 = pac_neg_pwr_fsr.pwrfsr_vs4 = PAC_PWRSFR_BHALF;
	if (i2c_writereg(PAC_I2C_ADDR,
	    PAC_NEG_PWR_FSR, (uint8_t *)&pac_neg_pwr_fsr,
	    sizeof(pac_neg_pwr_fsr)) == 0) {
		printf("wr PAC_NEG_PWR_FSR fail\n");
		c++;
	}

	if (i2c_writecmd(PAC_I2C_ADDR, PAC_REFRESH) == 0) {
		printf("cmd PAC_REFRESH fail\n");
		c++;
	}
	if (c != 0)
		goto again;

	while (1) {
		CLRWDT();
		if (C1INTLbits.RXIF) {
			nmea2000_receive();
		}
		if (NCANOK) {
			nmea2000_status = NMEA2000_S_ABORT;
			poll_count = timer0_read();;
		} else if (nmea2000_status != NMEA2000_S_OK) {
			uint16_t ticks, tmrv;

			tmrv = timer0_read();
			ticks = tmrv - poll_count;
			if (ticks > TIMER0_5MS) {
				poll_count = tmrv;
				nmea2000_poll(ticks / TIMER0_1MS);
			}
			if (nmea2000_status == NMEA2000_S_OK) {
				printf("new addr %d\n", nmea2000_addr);
			}
		}
		if (PIR1bits.ADIF) {
			PIR1bits.ADIF = 0;
			a2d_acc = ((unsigned int)ADRESH << 8) | ADRESL;
			switch (ADPCH) {
			case 0:
				/* channel 0: NTC */
				adctotemp(0);
				ADCON0bits.ADON = 0;
				ADPCH = 1;
				ADCON0bits.ADON = 1;
				break;
			case 1:
				/* channel 1: NTC */
				adctotemp(1);
				ADCON0bits.ADON = 0;
				ADPCH = 4;
				ADCON0bits.ADON = 1;
				break;
			case 4:
				/* channel 4: NTC */
				adctotemp(2);
				ADCON0bits.ADON = 0;
				ADPCH = 0;
				/* don't set ADON, will do on next second */
				break;
			default:
				printf("unknown channel 0x%x\n", ADCON0);
			}
		}

		if (softintrs.bits.int_10hz) {
			softintrs.bits.int_10hz = 0;
			/* read voltage values */
			for (c = 0; c < 4; c++) {
				pac_vbus_t pac_vbus;
				if ((pac_ctrl.ctrl_chan_dis & (8 >> c)) != 0)
					continue;
				if (i2c_readreg_be(PAC_I2C_ADDR,
				    PAC_VBUS1_AVG + c,
				    &pac_vbus, sizeof(pac_vbus)) ==
				    sizeof(pac_vbus))
					voltages_acc_cur[c] += pac_vbus.vbus_s;
				else
					printf("read v[%d] fail\n", c);
			}
			counter_1hz--;
			switch(counter_1hz) {
			case 6:
				/*
				 * get new values in registers and reset
				  * accumulator. Also freeze software voltage
				  * accumulator.
				  */
				if (i2c_writecmd(PAC_I2C_ADDR,
				    PAC_REFRESH) == 0)
					printf("PAC_REFRESH fail\n");
				for (c = 0; c < 4; c++) {
					voltages_acc[c] = voltages_acc_cur[c];
					voltages_acc_cur[c] = 0;
				}
				SIDINC(sid);
				break;
			case 5:
				read_pac_channel();
				/* FALLTHROUGH */
			case 4:
			case 3:
			case 2:
				send_batt_status(counter_1hz - 2);
				/* FALLTHROUGH */
			default:
				/* get new values for next voltage read */
				if (i2c_writecmd(PAC_I2C_ADDR,
				    PAC_REFRESH_V) == 0)
					printf("PAC_REFRESH_V fail\n");
			}

			if (counter_1hz == 0) {
				counter_1hz = 10;
				LEDBATT_G = 1;
				seconds++;
				if (seconds == 10) {
					seconds = 0;
				}
				ADCON0bits.ADON = 1; /* start a new cycle */
			} else {
				LEDBATT_G = 0;
			}
			if (ADCON0bits.ADON)
				ADCON0bits.GO = 1;
			if (!NCANOK && nmea2000_status == NMEA2000_S_OK) {
				uint16_t ticks, tmrv;

				tmrv = timer0_read();
				ticks = tmrv - poll_count;
				if (ticks > TIMER0_5MS) {
					poll_count = tmrv;
					nmea2000_poll(ticks / TIMER0_1MS);
				}
				if (nmea2000_status != NMEA2000_S_OK) {
					printf("lost CAN bus %d\n",
					    ticks);
				}
			}
		}
		if (PIR4bits.U1RXIF && (U1RXB == 'r'))
			break;
		SLEEP();
	}
	while ((c = getchar()) != 'r') {
		printf("resumed %u\n", timer0_read());
		goto again;
	}
	WDTCON0bits.SEN = 0;
	printf("returning\n");
	LEDBATT_R = LEDBATT_G = 0;
	while (!PIR4bits.U1TXIF) {
		; /* wait */ 
	}
	RESET();
	return 0;
}

unsigned int
timer0_read(void)
{
	unsigned int value;

	/* timer0_value = TMR0L | (TMR0H << 8), reading TMR0L first */
	di();
	asm("movff TMR0L, timer0_read@value");
	asm("movff TMR0H, timer0_read@value+1");
	ei();
	return value;
}

#ifdef USE_TIMER2
void __interrupt(__irq(TMR2), __high_priority, base(IVECT_BASE))
irqh_timer2(void)
{
	PIR3bits.TMR2IF = 0;
	if (--counter_10hz == 0) {
		counter_10hz = 25;
		softintrs.bits.int_10hz = 1;
	}			
}
#else
void __interrupt(__irq(TU16A), __high_priority, base(IVECT_BASE))
irqh_tu16a(void)
{
	TU16ACON1bits.CLR = 1;
	TU16ACON1bits.PRIF = 0;
	softintrs.bits.int_10hz = 1;
}
#endif
