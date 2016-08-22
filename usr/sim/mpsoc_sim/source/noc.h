
typedef unsigned short int Flit;

//FLOW DEFINITIONS
#define NI_BUFFER_LENGTH		OS_PACKET_SIZE   	// in flits
#define ROUTING_ALGORITHM_DELAY		7
#define PACKET_LENGTH_NOHEADER		(PACKET_LENGTH-2)

#ifdef BUS
	#define ROUTERSIZE 			N_CORES
	#define ARBITRATION_CONSIDERING_POS	0
	#define SIMULTANEOUS_SWITCHING		0
#else	
	#define ROUTERSIZE 			5
	#define ARBITRATION_CONSIDERING_POS	1
	#define SIMULTANEOUS_SWITCHING		1
#endif

//USEFUL MACROS
#define headerToDecimal(X)		( ( ((unsigned int) X) & 0x0f )*NOC_WIDTH + ( (unsigned int) ((unsigned int) X & 0xf0)>>4 )  )
#define decimalToHeader(X)		( GET_COLUMN(X)<<4 | GET_LINE(X) ) 
#define GET_LINE(n)			((int) n / NOC_WIDTH)
#define GET_COLUMN(n)			((int) n % NOC_WIDTH)
#define getRouter(n)			(&routers[ n ])
#define getBuffer(x, p)			(&(x->buffers[ p ]))
#define getNetworkInterface(n)		(&network_interfaces[ n ])
#define getCore(n)			(&cores[ n ])
#define getPort(x,j)			(&(x->ports[ j ]))

//PORT IDENTIFIERS
#define PLASMA				0
#define NOC				1
#define EAST				0
#define WEST				1
#define NORTH				2
#define SOUTH				3
#define LOCAL				4
#define NONE				5

//SIGNAL IDENTIFIERS 
#define ON				1
#define OFF				0

//STATUS IDENTIFIERS
#define IDLE				0
#define ROUTING_DELAY			1
#define ROUTING_HEADER			2
#define ROUTING_PAYLOAD			3
#define ROUTING_DATA			4

typedef struct {
	Flit 				in;
	unsigned char			in_request;
	unsigned char			in_ack;
	Flit				out;
	unsigned char 			out_request;
	unsigned char 			out_ack;
} Port;

typedef struct {
	Flit*				buffer;
	int 				start;
	int 				end;
	int 				size;
	int 				max;
} Buffer;

typedef struct {
	Port 				port;
} Core;

typedef struct {
	Buffer				buffers[2];	
	Port				ports[2];
} NetworkInterface;

typedef struct {
	unsigned char			arbiter;
	unsigned char			status[ROUTERSIZE];
	long long int			packets_remaining[ROUTERSIZE];
	unsigned char			redirect_to[ROUTERSIZE];
	int				routing_delay[ROUTERSIZE];
	Buffer				buffers[ROUTERSIZE];
	Port				ports[ROUTERSIZE];
} Router;

int teste(int i);

// IMPORTANT FUNCTIONS FOR BUFFERS
void create(Buffer* buffer, int max);
int isFull(Buffer* buffer);
int isEmpty(Buffer* buffer);
void put(Buffer* buffer, Flit value);
Flit read(Buffer* buffer);
Flit take(Buffer* buffer);
void destroy(Buffer* buffer);

// IMPORTANT FUNCTIONS FOR SIMULATION
void cleanPort(Port *port);
void unload_architecture();
void load_architecture();
void cycleRouter(int n);
void cycleNetworkInterface(int n);
void synchronizePorts(Port *p1, Port *p2);
void synchronizeRouter(int n);
void synchronizeNetworkInterface(int n);
void synchronizeCore(int n);

// GLOBAL VARS
Router *routers;
NetworkInterface *network_interfaces;
Core *cores;
