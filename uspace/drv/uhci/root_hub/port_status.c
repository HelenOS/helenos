#include <assert.h>
#include <stdio.h>

#include "port_status.h"

void print_port_status( port_status_t *status )
{
	assert( status );
	printf( "\tsuspended: %s\n", status->suspended ? "YES" : "NO" );
	printf( "\tin reset: %s\n", status->reset ? "YES" : "NO" );
	printf( "\tlow speed: %s\n", status->low_speed ? "YES" : "NO" );
	printf( "\tresume detected: %s\n", status->resume ? "YES" : "NO" );
	printf( "\talways \"1\" reserved bit: %s\n",
	  status->always_one ? "YES" : "NO" );
	/* line status skipped */
	printf( "\tenable/disable change: %s\n", status->enabled_change ? "YES" : "NO" );
	printf( "\tport enabled: %s\n", status->enabled ? "YES" : "NO" );
	printf( "\tconnect change: %s\n", status->connect_change ? "YES" : "NO" );
	printf( "\tconnected: %s\n", status->connected ? "YES" : "NO" );
}
