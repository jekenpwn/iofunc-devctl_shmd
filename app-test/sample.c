/*
 * $QNXLicenseC:
 * Copyright 2015, QNX Software Systems.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You
 * may not reproduce, modify or distribute this software except in
 * compliance with the License. You may obtain a copy of the License
 * at: http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTIES OF ANY KIND, either express or implied.
 *
 * This file may contain contributions from others, either as
 * contributors under the License or as licensors under other terms.
 * Please review this entire file for other proprietary rights or license
 * notices, as well as the QNX Development Suite License Guide at
 * http://licensing.qnx.com/license-guide/ for other information.
 * $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <devctl.h>
#include <sys/mman.h>
#include <fcntl.h>


#define SAMPLE_NAME_NODE "/dev/sample"
#define DCMD_SAMPLE_CMD	__DION  (_DCMD_MISC, 0x1)

static void sample_devctl(){
	int fd = open(SAMPLE_NAME_NODE,O_RDWR);
	if (fd < 0) return ;

	while(1){
		if ( EOK == devctl(fd,DCMD_SAMPLE_CMD,NULL,NULL,NULL) ){
			printf("devctl send successful\n");
		}else{
			printf("devctl send failed\n");
			return ;
		}

		usleep(1000*500);

	}
	close(fd);
}


#define SAMPLE_SHM_FILE "/shm_smp"
static void sample_readshm()
{
	int fd = shm_open(SAMPLE_SHM_FILE, O_RDWR , 0777);
	unsigned* addr;

	if( fd == -1 ) {
		printf("Open failed\n");
		return;
	}

	/* Set the memory object's size */
	if( ftruncate( fd, 1024 ) == -1 ) {
		printf("ftruncate failed\n");
		return;
	}

	/* Map the memory object */
	addr = mmap( 0, 1024,
			PROT_READ | PROT_WRITE,
			MAP_SHARED, fd, 0 );
	if( addr == MAP_FAILED ) {
		printf("mmap failed\n");
		return ;
	}

	int i;
	char* buf = addr;
	printf("dump shm: \n");
	for(i=0;i < 1024;i++){
		if(i==0||i%4==0)
			printf("\n");
		printf("0x%8x ",*(buf+i));
	}
	printf("\n");

	close(fd);

}

int main(int argc, char *argv[])
{


	sample_readshm();
//	sample_devctl();


    return 1;

}






