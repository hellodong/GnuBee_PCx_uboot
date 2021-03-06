/*
 *	Copyright 1994, 1995, 2000 Neil Russell.
 *	(See License)
 *	Copyright 2000, 2001 DENX Software Engineering, Wolfgang Denk, wd@denx.de
 */

#include <common.h>
#include <command.h>
#include <net.h>
#include <asm/byteorder.h>
#include "httpd.h"

#include "../httpd/uipopt.h"
#include "../httpd/uip.h"
#include "../httpd/uip_arp.h"
#include <configs/rt2880.h>

#ifdef OLED_1_3
#include <oled.h>
#endif


#if !defined(WEBFAILSAFE_UPLOAD_ART_ADDRESS)
extern flash_info_t flash_info[];
#endif

static int arptimer = 0;

void HttpdHandler(void){
	int i;

	for(i = 0; i < UIP_CONNS; i++){
		uip_periodic(i);

		if(uip_len > 0){
			uip_arp_out();
			NetSendHttpd();
		}
	}

	if(++arptimer == 20){
		uip_arp_timer();
		arptimer = 0;
	}
}

// start http daemon
void HttpdStart(void){
	uip_init();
	httpd_init();
}

int do_http_upgrade(const ulong size, const int upgrade_type){
	char buf[96];	// erase 0xXXXXXXXX +0xXXXXXXXX; cp.b 0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX (68 signs)
    int ret;
#if !defined(WEBFAILSAFE_UPLOAD_ART_ADDRESS)
	flash_info_t *info = &flash_info[0];
#endif
	if(upgrade_type == WEBFAILSAFE_UPGRADE_TYPE_UBOOT){

		printf("\n\n****************************\n*     U-BOOT UPGRADING     *\n* DO NOT POWER OFF DEVICE! *\n****************************\n\n");
		ret = raspi_erase_write((unsigned char*)(WEBFAILSAFE_UPLOAD_RAM_ADDRESS),WEBFAILSAFE_UPLOAD_UBOOT_ADDRESS,size);

	} else if(upgrade_type == WEBFAILSAFE_UPGRADE_TYPE_FIRMWARE){

		printf("\n\n****************************\n*    FIRMWARE UPGRADING    *\n* DO NOT POWER OFF DEVICE! *\n****************************\n\n");
#ifdef OLED_1_3
	        OLED_Clear_page(2);
        	OLED_Clear_page(4);
	        OLED_Clear_page(6);
        	OLED_ShowString(0,2,"DownLoad:100%");
#endif
		printf("magic number: 0x%08X should be 0x%08X %s\n", ntohl(*(uint32_t *)WEBFAILSAFE_UPLOAD_RAM_ADDRESS),  IH_MAGIC,\
		       	(ntohl(*(uint32_t *)WEBFAILSAFE_UPLOAD_RAM_ADDRESS)==IH_MAGIC)?"Good":"BAD!");
		if (ntohl(*(uint32_t *)WEBFAILSAFE_UPLOAD_RAM_ADDRESS)==IH_MAGIC){
			ret = raspi_erase_write((unsigned char*)(WEBFAILSAFE_UPLOAD_RAM_ADDRESS),WEBFAILSAFE_UPLOAD_KERNEL_ADDRESS,size);
	}
#ifdef OLED_1_3
        if(ret == 0)
            OLED_ShowString(0,6,"Loading Firmware");
#endif
        return ret; 
	} else if(upgrade_type == WEBFAILSAFE_UPGRADE_TYPE_FACTORY){

		printf("\n\n****************************\n*      FACTORY  UPGRADING      *\n* DO NOT POWER OFF DEVICE! *\n****************************\n\n");
		ret = raspi_erase_write((unsigned char*)(WEBFAILSAFE_UPLOAD_RAM_ADDRESS),WEBFAILSAFE_UPLOAD_ART_ADDRESS,size);

	} else {
		return(-1);
	}
}

// info about current progress of failsafe mode
int do_http_progress(const int state){
	unsigned char i = 0;

	/* toggle LED's here */
	switch(state){
		case WEBFAILSAFE_PROGRESS_START:

			// blink LED fast 10 times
			for(i = 0; i < 10; ++i){
				//all_led_on();
				milisecdelay(25);
			    //all_led_off();
				milisecdelay(25);
			}

			printf("HTTPD READY!\n\n");
			break;

		case WEBFAILSAFE_PROGRESS_TIMEOUT:
			//printf("Waiting for request...\n");
			break;

		case WEBFAILSAFE_PROGRESS_UPLOAD_READY:
			printf("HTTP upload is done! Upgrading...\n");
			break;

		case WEBFAILSAFE_PROGRESS_UPGRADE_READY:
			printf("HTTP ugrade is done! Rebooting...\n\n");
			break;

		case WEBFAILSAFE_PROGRESS_UPGRADE_FAILED:
			printf("## Error: HTTP ugrade failed!\n\n");

			// blink LED fast for 4 sec
			for(i = 0; i < 80; ++i){
				//all_led_on();
				milisecdelay(25);
				//all_led_off();
				milisecdelay(25);
			}

			// wait 1 sec
			milisecdelay(1000);

			break;
	}

	return(0);
}
