#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "noc.h"

/*


	BUFFERS FUNCTIONS


*/

void create(Buffer* buffer, int max)
{
	buffer->buffer = (Flit*) malloc(sizeof(Flit)*max);
	buffer->start = 0;
	buffer->end = 0;
	buffer->size = 0;
	buffer->max = max;
}

int isFull(Buffer* buffer)
{
	return buffer->size == buffer->max;
}

int isEmpty(Buffer* buffer)
{
	return buffer->size == 0;
}

void put(Buffer* buffer, Flit value)
{
	buffer->buffer[ buffer->end ] = value;
	buffer->end = ( buffer->end + 1 ) % buffer->max;
	buffer->size++;
}

Flit read(Buffer* buffer)
{
	return buffer->buffer[ buffer->start ];
}

Flit take(Buffer* buffer)
{
	Flit i = buffer->buffer[ buffer->start ];
	buffer->size--;
	buffer->start = (buffer->start + 1) % buffer->max;
	return i;
}

void destroy(Buffer* buffer)
{
	free(buffer->buffer);
}

/*


	REMAINING FUNCTIONS
 
 
*/

void cleanPort(Port *port)
{
    port->in = 0;
    port->in_request = OFF;
    port->in_ack = OFF;
    port->out = 0;
    port->out_request = OFF;
    port->out_ack = OFF;
}

#ifndef BUS
void unload_architecture()
{
	int i, k;
	Router *router;
	NetworkInterface *network_interface;
	Core *core;
	for( i = 0 ; i < N_CORES ; i++ )
	{
		router = getRouter(i);
		for( k = 0 ; k < 5 ; k++ )
		{
			destroy(&(router->buffers[k]));
		}
	}
	for( i = 0 ; i < 2 ; i++ )
	{
		network_interface = getNetworkInterface(i);
		for( k = 0 ; k < 2 ; k++ )
		{
			destroy(&(network_interface->buffers[k]));
		}
	}
	free(routers);
	free(network_interfaces);
	free(cores);
}
#else
void unload_architecture()
{
	int i, k;
	Router *router;
	NetworkInterface *network_interface;
	Core *core;
	router = getRouter(0);
	for( k = 0 ; k < ROUTERSIZE ; k++ )
	{
		destroy(&(router->buffers[k]));
	}
	for( i = 0 ; i < 2 ; i++ )
	{
		network_interface = getNetworkInterface(i);
		for( k = 0 ; k < 2 ; k++ )
		{
			destroy(&(network_interface->buffers[k]));
		}
	}
	free(routers);
	free(network_interfaces);
	free(cores);
}
#endif

#ifndef BUS
void load_architecture()
{
	int k, i;
	Router *router;
	Core *core;
	NetworkInterface *network_interface;
	routers = (Router*) malloc(sizeof(Router)*N_CORES);
	network_interfaces = (NetworkInterface*) malloc(sizeof(NetworkInterface)*N_CORES);
	cores = (Core*) malloc(sizeof(Core)*N_CORES);
	for( i = 0 ; i < N_CORES ; i++ )
	{
		//router
		router = getRouter(i);
		router->arbiter = 0;
		//network interface
		network_interface = getNetworkInterface(i);
		create(getBuffer(network_interface, NOC), NI_BUFFER_LENGTH);
		create(getBuffer(network_interface, PLASMA), NI_BUFFER_LENGTH);
		//core
		core = getCore(i);
		cleanPort(&(core->port));
		for( k = 0 ; k < 5 ; k++ )
		{
			//router
			router->packets_remaining[k] = 0;
			router->status[k] = IDLE;
			router->redirect_to[k] = NONE;
			router->routing_delay[k] = NONE;
			create(getBuffer(router, k), NOC_BUFFER_SIZE);
			//ports
			cleanPort(&(router->ports[k]));
			if( k <= 1 )
			{
				cleanPort(&(network_interface->ports[k]));
			}
		}
	}
}
#else
void load_architecture()
{
	int k, i;
	Router *router;
	Core *core;
	NetworkInterface *network_interface;
	routers = (Router*) malloc(sizeof(Router));
	network_interfaces = (NetworkInterface*) malloc(sizeof(NetworkInterface)*N_CORES);
	cores = (Core*) malloc(sizeof(Core)*N_CORES);
	router = getRouter(i);
	router->arbiter = 0;
	for( k = 0 ; k < ROUTERSIZE ; k++ )
	{
		//router
		router->packets_remaining[k] = 0;
		router->status[k] = IDLE;
		router->redirect_to[k] = NONE;
		router->routing_delay[k] = NONE;
		create(getBuffer(router, k), NOC_BUFFER_SIZE);
		//ports
		cleanPort(&(router->ports[k]));
	}
	for( i = 0 ; i < N_CORES ; i++ )
	{
		//network interface
		network_interface = getNetworkInterface(i);
		create(getBuffer(network_interface, NOC), NI_BUFFER_LENGTH);
		create(getBuffer(network_interface, PLASMA), NI_BUFFER_LENGTH);
		//core
		core = getCore(i);
		cleanPort(&(core->port));
		for( k = 0 ; k < 2 ; k++ )
		{
			cleanPort(&(network_interface->ports[k]));
		}
	}
}
#endif

#ifndef BUS
void cycleRouter(int n)
{
	unsigned char in_use = 0, active = 0;
	int i, j, dest, l, c;
	long long int header;
	Flit flit;
	Router *router = getRouter(n);
	Buffer *buffer;
	Port *port_source, *port_dest;

	for( i = 0 ; i < 5 ; i++ )
	{
		if( router->ports[i].in_request == ON )
		{
			buffer = getBuffer(router, i);
			if( ! isFull( buffer ) )
			{
				port_source = getPort(router, i);
				put(buffer, port_source->in);
				port_source->in_ack = ON;				
			}
		}
	}

	if( SIMULTANEOUS_SWITCHING == 0 )
	{
		for( j = 0 ; j <= 5 ; j++ )
		{
			if( router->status[ router->arbiter ] != IDLE )
			{
				active++;
			}
		}
	}

	if( router->status[ router->arbiter ] == IDLE && active == 0 )
	{        
		buffer = getBuffer(router, router->arbiter);
		if( ! isEmpty(buffer) )
		{
			i = router->arbiter;
			port_source = getPort(router, i);
			flit = read(buffer);
			header = (long long int) flit;
			header = headerToDecimal(header);
		    
			if( GET_LINE(n) == GET_LINE(header) && GET_COLUMN(n) == GET_COLUMN(header) )
			{
				dest = LOCAL;
			}
			else if( GET_COLUMN(n) < GET_COLUMN(header) )
			{
				dest = EAST;
			}
			else if( GET_COLUMN(n) > GET_COLUMN(header) )
			{
				dest = WEST;
			}
			else if( GET_LINE(n) > GET_LINE(header) )
			{
				dest = SOUTH;
			}
		    	else if( GET_LINE(n) < GET_LINE(header) )
			{
				dest = NORTH;
			}
			
			in_use = 0;
			for( j = 0 ; j < 5 ; j++ )
			{
				if( j != i )
				{
					if( router->status[j] != IDLE && router->redirect_to[j] == dest )
					{
			        		in_use = 1;
			        	}
				}
			}

			if( ! in_use )
			{
				router->redirect_to[i] = dest;
				router->status[i] = ROUTING_DELAY;
		        	router->routing_delay[i] = ROUTING_ALGORITHM_DELAY;
				active = 1;
				//printf("\nNEW CONNECTION ROUTER: %d (%s->%s)", n, directions[i], directions[dest]);
			}
		}
    	}

	for( i = 0 ; i <= 4 ; i++ )
	{
		if( router->status[i] != IDLE )
		{
			buffer = getBuffer(router, i);
			port_dest = getPort(router, router->redirect_to[i]);
			if( router->status[i] == ROUTING_DELAY )
			{
				if( router->routing_delay[i]-- <= 0 )
				{
					if( ! isEmpty( buffer ) )
					{
						flit = take(buffer);
						port_dest->out = flit;
						port_dest->out_request = ON;
						port_dest->out_ack = OFF;
						router->status[i] = ROUTING_HEADER;
						//printf("\n\tROUTER %d HEADER %d", n, flit);
					}
				}
			}
			else
			{
				if( port_dest->out_ack == ON )
				{
					if( router->status[i] == ROUTING_HEADER && ! isEmpty(buffer) )
					{
						flit = take(buffer);
						port_dest->out_request = ON;
						port_dest->out_ack = OFF;
						port_dest->out = flit;
						router->packets_remaining[i] = (long long int) flit;
						router->status[i] = ROUTING_PAYLOAD;
						//printf("\n\tROUTER %d PAYLOAD %d", n, flit);
					}
					else if( router->status[i] == ROUTING_PAYLOAD || router->status[i] == ROUTING_DATA )
					{
						if( router->status[i] == ROUTING_PAYLOAD )
						{
							router->status[i] = ROUTING_DATA;
						}				
					
						if( router->packets_remaining[i] == 0 )
						{
							//printf("\nCONNECTION CLOSED ROUTER %d (%s->%s)", n, directions[i], directions[router->redirect_to[i]]);
							router->status[i] = IDLE;
							router->packets_remaining[i] = 0;
							router->redirect_to[i] = NONE;
							port_dest->out_request = OFF;
							port_dest->out = 0;
							port_dest->out_ack = OFF;
						}
						else
						{
							if( ! isEmpty( buffer ) )
							{
								flit = take(buffer);
								port_dest->out_request = ON;
								port_dest->out_ack = OFF;
								port_dest->out = flit;
									router->packets_remaining[i]--;
							}
						}
					}        
				}
			}
		}
	}   
    
	router->arbiter = ++router->arbiter % 5;


	if( ARBITRATION_CONSIDERING_POS == 1 )
	{
		l = GET_LINE(n);
		c = GET_COLUMN(n);
		if( l == 0 && c == 0)
		{
			if( router->arbiter == WEST )
			{
				router->arbiter = NORTH;
			}
			else if( router->arbiter == SOUTH )
			{
			router->arbiter = LOCAL;
			}
		}
		else if( l == 0 && c == NOC_WIDTH-1 )
		{
			if( router->arbiter == EAST )
			{
				router->arbiter = WEST;
			}
			else if( router->arbiter == SOUTH )
			{
				router->arbiter = LOCAL;
			}
		}
		else if( l == NOC_HEIGHT-1 && c == 0 )
		{
			if( router->arbiter == WEST )
			{
				router->arbiter = SOUTH;
			}
			else if( router->arbiter == NORTH )
			{
				router->arbiter = LOCAL;
			}
		}
		else if( l == NOC_HEIGHT-1 && c == NOC_WIDTH-1 )
		{
			if( router->arbiter == EAST )
			{
				router->arbiter = WEST;
			}
			else if( router->arbiter == NORTH )
			{
				router->arbiter = SOUTH;
			}
		}
		else if( l == 0 )
		{
			if( router->arbiter == SOUTH )
			{
			router->arbiter = LOCAL;
			}
		}
		else if( c == 0 )
		{
			if( router->arbiter == WEST )
			{
				router->arbiter = NORTH;
			}
		}
		else if( l == NOC_HEIGHT-1 )
		{
			if( router->arbiter == NORTH )
			{
				router->arbiter = SOUTH;
			}
		}
		else if( c == NOC_WIDTH-1 )
		{
			if( router->arbiter == EAST )
			{
				router->arbiter = WEST;
			}
		}
	}
}
#else
void cycleRouter(int n)
{
	unsigned char in_use = 0, active = 0;
	int i, j, dest, l, c;
	long long int header;
	Flit flit;
	Router *router = getRouter(n);
	Buffer *buffer;
	Port *port_source, *port_dest;

	for( i = 0 ; i < ROUTERSIZE ; i++ )
	{
		if( router->ports[i].in_request == ON )
		{
			buffer = getBuffer(router, i);
			if( ! isFull( buffer ) )
			{
				port_source = getPort(router, i);
				put(buffer, port_source->in);
				port_source->in_ack = ON;				
			}
		}
	}

	for( j = 0 ; j <= ROUTERSIZE ; j++ )
	{
		if( router->status[ router->arbiter ] != IDLE )
		{
			active++;
		}
	}

	if( router->status[ router->arbiter ] == IDLE && active == 0 )
	{        
		buffer = getBuffer(router, router->arbiter);
		if( ! isEmpty(buffer) )
		{
			i = router->arbiter;
			port_source = getPort(router, i);
			flit = read(buffer);
			header = (long long int) flit;	
				    
			dest = header;		
						
			//i removed the is_use logic because it will never happen with a bus arch
			router->redirect_to[i] = dest;
			router->status[i] = ROUTING_DELAY;
		        router->routing_delay[i] = ROUTING_ALGORITHM_DELAY;			
//			printf("\nNEW CONNECTION ROUTER: %d (%d->%d)", n, i, dest);
		}
    	}

	for( i = 0 ; i <= ROUTERSIZE ; i++ )
	{
		if( router->status[i] != IDLE )
		{
			buffer = getBuffer(router, i);
			port_dest = getPort(router, router->redirect_to[i]);
			if( router->status[i] == ROUTING_DELAY )
			{
				if( router->routing_delay[i]-- <= 0 )
				{
					if( ! isEmpty( buffer ) )
					{
						flit = take(buffer);
						port_dest->out = flit;
						port_dest->out_request = ON;
						port_dest->out_ack = OFF;
						router->status[i] = ROUTING_HEADER;
//						printf("\n\tROUTER %d HEADER %d", n, flit);
					}
				}
			}
			else
			{
				if( port_dest->out_ack == ON )
				{
					if( router->status[i] == ROUTING_HEADER && ! isEmpty(buffer) )
					{
						flit = take(buffer);
						port_dest->out_request = ON;
						port_dest->out_ack = OFF;
						port_dest->out = flit;
						router->packets_remaining[i] = (long long int) flit;
						router->status[i] = ROUTING_PAYLOAD;
//						printf("\n\tROUTER %d PAYLOAD %d", n, flit);
					}
					else if( router->status[i] == ROUTING_PAYLOAD || router->status[i] == ROUTING_DATA )
					{
						if( router->status[i] == ROUTING_PAYLOAD )
						{
							router->status[i] = ROUTING_DATA;
						}				
					
						if( router->packets_remaining[i] == 0 )
						{
//							printf("\nCONNECTION CLOSED ROUTER %d (%d->%d)", n, i, router->redirect_to[i]);
							router->status[i] = IDLE;
							router->packets_remaining[i] = 0;
							router->redirect_to[i] = NONE;
							port_dest->out_request = OFF;
							port_dest->out = 0;
							port_dest->out_ack = OFF;
						}
						else
						{
							if( ! isEmpty( buffer ) )
							{
								flit = take(buffer);
								port_dest->out_request = ON;
								port_dest->out_ack = OFF;
								port_dest->out = flit;
								router->packets_remaining[i]--;
							}
						}
					}        
				}
			}
		}
	}
	
	active = 0;
	for( j = 0 ; j <= ROUTERSIZE ; j++ )
	{
		if( router->status[ j ] != IDLE )
		{
			active++;
		}
	}
	if( active == 0 )
	{
		router->arbiter = ++router->arbiter % ROUTERSIZE;
	}
}
#endif

void cycleNetworkInterface(int n)
{
	int i;
	NetworkInterface *ni = getNetworkInterface(n);
    	Port *plasma_port = getPort(ni, PLASMA);
    	Port *noc_port = getPort(ni, NOC);
	Buffer *buffer_noc, *buffer_plasma;

	if(NI_BUFFER_LENGTH==0)
	{
		return;
	}

	buffer_noc = getBuffer(ni, NOC);
	buffer_plasma = getBuffer(ni, PLASMA);
	
	//máquina de recebimento de flits do plasma
	if( plasma_port->in_request == ON )
	{
		if( ! isFull(buffer_plasma) )
		// if plasma_buffer not full
		{
			put(buffer_plasma, plasma_port->in);
			plasma_port->in_ack = ON;
		}
	}
    
	// máquina de recebimento de flits da noc
	if( noc_port->in_request == ON )
	{
		if( ! isFull(buffer_noc) )
		// if noc_buffer not full
		{
			put(buffer_noc, noc_port->in);
			noc_port->in_ack = ON;
		}
	}
    
	//máquina de envio de flits para a noc
    	if( ! isEmpty(buffer_plasma) )
    	{
		if( noc_port->out_ack == ON )
		{
			take(buffer_plasma);
			noc_port->out_ack = OFF;
			noc_port->out_request = OFF;
			noc_port->out = 0;
		}
		if( ! isEmpty(buffer_plasma) )
		{
		    	noc_port->out = read(buffer_plasma);
		    	noc_port->out_request = ON;
		}
    	}
    
	// máquina de envio de flits para o plasma
	if( ! isEmpty(buffer_noc) )
	{
		if( plasma_port->out_ack == ON )
		{
			i = take(buffer_noc);
			plasma_port->out_ack = OFF;
			plasma_port->out_request = OFF;
			plasma_port->out = 0;
		}
		if( ! isEmpty(buffer_noc) )
		{			
			plasma_port->out = read(buffer_noc);
			plasma_port->out_request = ON;
		}
	}       
}

void synchronizePorts(Port *p1, Port *p2)
{
	if( p1->out_request == ON && p2->in_ack == OFF && p1->out_ack == OFF )
	{
		p2->in_request = ON;
		p2->in = p1->out;
	}
	else if( p1->out_request == ON && p2->in_ack == ON )
	{
		p2->in_request = OFF;
		p1->out_ack = ON;
		p2->in_ack = OFF;
		p2->in = OFF;
	}
}

#ifndef BUS
void synchronizeRouter(int n)
{
	int l, c;
	Router *router = getRouter(n);
	Router *aux;
	NetworkInterface *ni;
	Core *core;
	Port *p1, *p2;
	char flags[4];
	
    	if( NI_BUFFER_LENGTH != 0 )
    	{
		ni = getNetworkInterface(n);
    	}
    	else
	{
	    	core = getCore(n);
    	}
	l = GET_LINE(n);
	c = GET_COLUMN(n);
    	flags[0] = flags[1] = flags[2] = flags[3] = OFF;

    	if( c == 0 )
    	{
        	flags[EAST] = ON;
	}
	else if( c == NOC_WIDTH-1 )
	{
	    	flags[WEST] = ON;
	}
	else
	{
	    	flags[EAST] = ON;
	    	flags[WEST] = ON;
	}

	if( l == 0 ) 
	{
	    	flags[NORTH] = ON;
	}
    	else if( l == NOC_HEIGHT-1 ) 
    	{
        	flags[SOUTH] = ON;
	}
    	else 
	{ 
	    	flags[SOUTH] = ON;
	    	flags[NORTH] = ON;
	}

	p1 = &(router->ports[LOCAL]);
	if( NI_BUFFER_LENGTH != 0 )
	{
        	p2 = getPort(ni, NOC);
    	}
	else
	{
        	p2 = &(core->port);
    	}
    	synchronizePorts(p1, p2);
	
	if( flags[SOUTH] )
	{
		aux = getRouter(n-NOC_WIDTH);
		p1 = getPort(router, SOUTH);
		p2 = getPort(aux, NORTH);
		synchronizePorts(p1, p2);
	}
	if( flags[NORTH] )
	{
	    	aux = getRouter(n+NOC_WIDTH);
		p1 = getPort(router, NORTH);
		p2 = getPort(aux, SOUTH);
		synchronizePorts(p1, p2);
	}
	if( flags[EAST] )
	{
	    	aux = getRouter(n+1);
		p1 = getPort(router, EAST);
		p2 = getPort(aux, WEST);
		synchronizePorts(p1, p2);
	}
	if( flags[WEST] )
	{
	    	aux = getRouter(n-1);
		p1 = getPort(router, WEST);
		p2 = getPort(aux, EAST);
		synchronizePorts(p1, p2);
	}
}
#else
void synchronizeRouter(int n)
{
	int l, c;
	Router *router = getRouter(n);
	Router *aux;
	NetworkInterface *ni;
	Core *core;
	Port *p1, *p2;
	for( l = 0 ; l < ROUTERSIZE ; l++ )
	{
		p1 = &(router->ports[l]);	
	    	if( NI_BUFFER_LENGTH != 0 )
	    	{
			ni = getNetworkInterface(l);
			p2 = getPort(ni, NOC);
	    	}
	    	else
		{
		    	core = getCore(l);
			p2 = &(core->port);
	    	}
	    	synchronizePorts(p1, p2);
    	}
}
#endif

#ifndef BUS
void synchronizeNetworkInterface(int n)
{
	Router *router = getRouter(n);
	Core *core = getCore(n);
	NetworkInterface *ni = getNetworkInterface(n);
	Port *p1, *p2;
	
	p1 = getPort(ni, NOC);
	p2 = getPort(router, LOCAL);
	synchronizePorts(p1, p2);
	
	p1 = getPort(ni, PLASMA);
	p2 = &(core->port);
	synchronizePorts(p1, p2);
}
#else
void synchronizeNetworkInterface(int n)
{
	Router *router = getRouter(0); //HERE CHANGES
	Core *core = getCore(n);
	NetworkInterface *ni = getNetworkInterface(n);
	Port *p1, *p2;
	
	p1 = getPort(ni, NOC);
	p2 = getPort(router, n); // HERE CHANGES
	synchronizePorts(p1, p2);
	
	p1 = getPort(ni, PLASMA);
	p2 = &(core->port);
	synchronizePorts(p1, p2);
}
#endif

#ifndef BUS
void synchronizeCore(int n)
{
	Core *core = getCore(n);
	NetworkInterface *ni;
	Router *router;
	if( NI_BUFFER_LENGTH != 0 )
	{
		ni = getNetworkInterface(n);
	}
	else
	{
		router = getRouter(n);
	}
	
	Port *p1, *p2;	
	p1 = &(core->port);
	if( NI_BUFFER_LENGTH != 0 )
	{
		p2 = getPort(ni, PLASMA);
	}
	else
	{
		p2 = getPort(router, LOCAL);
	}
	synchronizePorts(p1, p2);
}
#else
void synchronizeCore(int n)
{
	Core *core = getCore(n);
	NetworkInterface *ni;
	Router *router;
	Port *p1, *p2;	
	
	p1 = &(core->port);
	if( NI_BUFFER_LENGTH != 0 )
	{
		ni = getNetworkInterface(n);
		p2 = getPort(ni, PLASMA);
	}
	else
	{
		router = getRouter(0); // HERE CHANGES
		p2 = getPort(router, n); // HERE CHANGES
	}
	synchronizePorts(p1, p2);
}
#endif

