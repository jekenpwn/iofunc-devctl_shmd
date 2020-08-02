#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/resmgr.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <hw/inout.h>

#include <sys/slog.h>
#include <sys/slogcodes.h>
#include <errno.h>
#include <sys/procmgr.h>
#include <drvr/hwinfo.h>
// DEVCTL libs
#include <devctl.h>

/*******shm header*******************/
#include <limits.h>
#include <sys/mman.h>
#include <fcntl.h>

#define SAMPLE_DEV_FILE "/dev/sample"
#define SAMPLE_SHM_FILE "/shm_smp"

#define DCMD_SAMPLE  __DION  (_DCMD_MISC, 0x1)


static resmgr_connect_funcs_t _sample_connect_funcs;
static resmgr_io_funcs_t _sample_io_funcs;
static iofunc_attr_t _sample_iofunc_attr;


static int _sample_devctl (resmgr_context_t *ctp, io_devctl_t *msg, RESMGR_OCB_T *ocb)
{
	int     sts;

	/* 1) Create the reply buffer at the start of ctp->msg so we make
	 * sure we have enough space */
	struct _io_devctl_reply *reply = (struct _io_devctl_reply *)ctp->msg;

	/* Create a pointer to the rate variable for the _GET_ devctls. */
	unsigned nbytes = 0;
	uint32_t *rate = _DEVCTL_DATA(*reply);

	/* 2) Verify we have the entire devctl header in the buffer */
	if( ctp->size < sizeof(msg->i) ) {
		return EBADMSG;
	}
	/* 3) See if it's a standard devctl() */
	if ((sts = iofunc_devctl_default (ctp, msg, ocb)) !=
		_RESMGR_DEFAULT)
	{
		return (sts);
	}

	/* How many bytes did the client send in addition to the devctl header? */
	size_t payload_size = ctp->size - sizeof(msg->i);

	/* 4) See which command it was, and act on it */

    switch (msg->i.dcmd)
    {
        case DCMD_SAMPLE:
        {


            fprintf( stderr, "Sample cmd comming!!.\n");

        }
        break;

        default:
            return (ENOSYS);
    }
//
//    //* 6) Tell the client that it worked */
//    memset(reply, 0, sizeof(*reply));
//    reply->nbytes = nbytes;
//    SETIOV (ctp->iov, reply, sizeof(*reply) + nbytes);
//    return (_RESMGR_NPARTS (1));

    /* 6) tell the client it worked */
    memset(reply, 0, sizeof(*reply));
    reply->nbytes = nbytes;
    return (_RESMGR_PTR (ctp, reply, sizeof(*reply) + nbytes));

}


static int fd;
static unsigned* addr;
static int shm_init()
{

	/*
	 * In case the unlink code isn't executed at the end
	 */

	shm_unlink( SAMPLE_SHM_FILE );

	/* Create a new memory object */
	fd = shm_open( SAMPLE_SHM_FILE, O_RDWR | O_CREAT, 0777 );
	if( fd == -1 ) {
		fprintf( stderr, "Open failed:%s\n",
			strerror( errno ) );
		return EXIT_FAILURE;
	}

	/* Set the memory object's size */
	if( ftruncate( fd, 1024 ) == -1 ) {
		fprintf( stderr, "ftruncate: %s\n",
			strerror( errno ) );
		return EXIT_FAILURE;
	}

	/* Map the memory object */
	addr = mmap( 0, 1024,
			PROT_READ | PROT_WRITE,
			MAP_SHARED, fd, 0 );
	if( addr == MAP_FAILED ) {
		fprintf( stderr, "mmap failed: %s\n",
			strerror( errno ) );
		return EXIT_FAILURE;
	}



	/* Write to shared memory */
	char* buf = addr;
	memset(buf,0xAA,1024);


}

static void shm_deinit()
{
	/*
	 * The memory object remains in
	 * the system after the close
	 */
	close( fd );

	/*
	 * To remove a memory object
	 * you must unlink it like a file.
	 *
	 * This may be done by another process.
	 */
	shm_unlink( SAMPLE_SHM_FILE );
}
/******************************************************************************/
/*Name : main                                                            */
/*Role : resource manager entery                           */
/*Interface :     void                                                        */
/*Pre-condition : none                                                        */
/*Constraints :   none                                                        */
/*Behaviour :                                                                 */
/*  DO                                                                        */
/*  OD                                                                        */
/******************************************************************************/
int main(int argc, char **argv)
{
	dispatch_t *dpp;
	resmgr_attr_t resmgr_attr;
	resmgr_context_t *ctp;
	int id;


	shm_init();

    printf("init sample\n");
	 /* Enable IO capability.*/
	if (-1 == ThreadCtl( _NTO_TCTL_IO_PRIV, NULL )) {
		return EXIT_FAILURE;
	}

	/*run in the background as daemon thread*/
	if ( procmgr_daemon( EXIT_SUCCESS, PROCMGR_DAEMON_NOCLOSE | PROCMGR_DAEMON_NODEVNULL ) == -1 ) {
		return EXIT_FAILURE;
	}

	/* initialize dispatch interface */
	if ( (dpp = dispatch_create()) == NULL )
	{
		fprintf( stderr, "%s: Unable to allocate dispatch handle.\n", argv[0] );
		return EXIT_FAILURE;
	}

	/* initialize resource manager attributes */
	memset( &resmgr_attr, 0, sizeof(resmgr_attr));

	resmgr_attr.nparts_max = 1;
	resmgr_attr.msg_max_size = 2048;

	/* initialize functions for handling messages */
	iofunc_func_init( _RESMGR_CONNECT_NFUNCS, &_sample_connect_funcs, _RESMGR_IO_NFUNCS, &_sample_io_funcs );
	_sample_io_funcs.devctl = _sample_devctl;

	/* initialize attribute structure */
	iofunc_attr_init( &_sample_iofunc_attr, S_IFNAM | 0666, 0, 0 );

	/* register device name */
	/* attach our device name (passing in the POSIX defaults
	from the iofunc_func_init and iofunc_attr_init functions)
	*/
	if ( (id = resmgr_attach( dpp, &resmgr_attr, SAMPLE_DEV_FILE, _FTYPE_ANY, 0,
			&_sample_connect_funcs, &_sample_io_funcs, &_sample_iofunc_attr)) == -1 )
	{
		fprintf( stderr, "%s: Unable to attach name.\n", argv[0] );
		return EXIT_FAILURE;
	}
	/* allocate a context structure */
	ctp = resmgr_context_alloc( dpp );
	/* start the resource manager message loop */
	while (1)
	{
		if ( (ctp = resmgr_block( ctp )) == NULL )
		{
			fprintf(stderr, "block error\n");
			return EXIT_FAILURE;
		}
		resmgr_handler(ctp);
	}

	shm_deinit();

	return EXIT_FAILURE;
}



