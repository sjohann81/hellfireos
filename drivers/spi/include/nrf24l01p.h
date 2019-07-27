/* file:          nrf24l01p.h
 * description:   Nordic nRF24L01+ transceiver driver
 * date:          07/2019
 * author:        Sergio Johann Filho <sergio.filho@pucrs.br>
 */

/******************* low level definitions (radio control / frame driver) *******************/

/* register map */
#define CONFIG			0x00
#define EN_AA			0x01
#define EN_RXADDR		0x02
#define SETUP_AW		0x03
#define SETUP_RETR		0x04
#define RF_CH			0x05
#define RF_SETUP		0x06
#define STATUS			0x07
#define OBSERVE_TX		0x08
#define CD			0x09
#define RX_ADDR_P0		0x0A
#define RX_ADDR_P1		0x0B
#define RX_ADDR_P2		0x0C
#define RX_ADDR_P3		0x0D
#define RX_ADDR_P4		0x0E
#define RX_ADDR_P5		0x0F
#define TX_ADDR			0x10
#define RX_PW_P0		0x11
#define RX_PW_P1		0x12
#define RX_PW_P2		0x13
#define RX_PW_P3		0x14
#define RX_PW_P4		0x15
#define RX_PW_P5		0x16
#define FIFO_STATUS		0x17
#define DYNPD			0x1C

/* configuration register masks */
#define MASK_RX_DR		6
#define MASK_TX_DS		5
#define MASK_MAX_RT		4
#define EN_CRC			3
#define CRCO			2
#define PWR_UP			1
#define PRIM_RX			0

/* enable auto acknowledgment on pipe */
#define ENAA_P5			5
#define ENAA_P4			4
#define ENAA_P3			3
#define ENAA_P2			2
#define ENAA_P1			1
#define ENAA_P0			0

/* enable rx data pipe */
#define ERX_P5			5
#define ERX_P4			4
#define ERX_P3			3
#define ERX_P2			2
#define ERX_P1			1
#define ERX_P0			0

/* address width setup */
#define AW			0

/* auto re-transmission setup */
#define ARD			4
#define ARC			0

/* RF data rate */
#define RATE_2MBPS		0x01
#define RATE_1MBPS		0x00
#define RATE_250KBPS		0x04

/* RF setup register */
#define PLL_LOCK		4
#define RF_DR			3
#define RF_PWR			1

/* general status register */
#define RX_DR			6
#define TX_DS			5
#define MAX_RT			4
#define RX_P_NO			1
#define TX_FULL			0

/* transmit observe register */
#define PLOS_CNT		4
#define ARC_CNT			0

/* fifo status */
#define TX_REUSE		6
#define FIFO_FULL		5
#define TX_EMPTY		4
#define RX_FULL			1
#define RX_EMPTY		0

/* dynamic length */
#define DPL_P0			0
#define DPL_P1			1
#define DPL_P2			2
#define DPL_P3			3
#define DPL_P4			4
#define DPL_P5			5

/* SPI commands */
#define R_REGISTER		0x00
#define W_REGISTER		0x20
#define R_RX_PAYLOAD		0x61
#define W_TX_PAYLOAD		0xA0
#define FLUSH_TX		0xE1
#define FLUSH_RX		0xE2
#define REUSE_TX_PL		0xE3
#define ACTIVATE		0x50
#define R_RX_PL_WID		0x60
#define NOP			0xFF
#define REGISTER_MASK		0x1F

#define NRF24L01_DEF_CHANNNEL	2
#define NRF24L01_PAYLOAD	32
#define NRF24L01_ADDR_LEN	5
#define NRF24L01_CONFIG		((1 << EN_CRC) | (0 << CRCO))
#define NRF24L01_TX_OK		0
#define NRF24L01_TX_ERROR  	1

/******************* low level interface (radio control / frame driver) *******************/

/* initialization functions */
uint8_t nrf24l01_init(void);
void nrf24l01_rx_address(uint8_t *addr);
void nrf24l01_tx_address(uint8_t *addr);
void nrf24l01_config(uint8_t rate, uint8_t channel, uint8_t ack_en);

/* state check functions */
uint8_t nrf24l01_dataready(void);
uint8_t nrf24l01_issending(void);
uint8_t nrf24l01_fifoempty(void);
uint8_t nrf24l01_payload(void);

/* post transmission analysis */
uint8_t nrf24l01_laststatus(void);
uint8_t nrf24l01_retcount(void);

/* frame level TX / RX functions */
void nrf24l01_tx(uint8_t *data);
void nrf24l01_rx(uint8_t *data);

/* power management */
void nrf24l01_rx_powerup(void);
void nrf24l01_tx_powerup(void);
void nrf24l01_powerdown(void);

/* low level interface / SPI bus packets */
void nrf24l01_ce(uint8_t val);
uint8_t nrf24l01_status(void);
void nrf24l01_xmitsync(uint8_t *dataout, uint8_t len);
void nrf24l01_xfersync(uint8_t *dataout, uint8_t *datain, uint8_t len);
void nrf24l01_setreg(uint8_t reg, uint8_t value);
uint8_t nrf24l01_getreg(uint8_t reg);
void nrf24l01_readreg(uint8_t reg, uint8_t *val, uint8_t len);
void nrf24l01_writereg(uint8_t reg, uint8_t *val, uint8_t len);

/******************* high level interface (packet driver) *******************/

/* packet level TX / RX definitions and functions */
#define NRF24L01_START_WAIT_MS	100
#define NRF24L01_XFER_LIMIT_MS	500
#define NRF24L01_XFER_RETRIES	15
#define NRF24L01_XFER_WAIT_MS	5

#define NRF24L01_ETIMEOUT	-1
#define NRF24L01_EFRAMING	-2
#define NRF24L01_EPAYLOAD	-3
#define NRF24L01_ESYNC		-4
#define NRF24L01_ERETRIES	-5
#define NRF24L01_ETOOBIG	-6
#define NRF24L01_ECRC		-7

struct nrf24l01_frame_s {
	uint8_t src_addr[NRF24L01_ADDR_LEN];				// source address
	uint8_t payload;						// number of frames
	uint8_t seq;							// frame sequence number
	uint8_t data[NRF24L01_PAYLOAD - NRF24L01_ADDR_LEN - 2];		// frame data
};

struct nrf24l01_driver_s {
	uint8_t rx_addr[NRF24L01_ADDR_LEN];
	uint8_t tx_addr[NRF24L01_ADDR_LEN];
	uint8_t en_ack;
	uint8_t payload;
	uint8_t seq;
};

int16_t nrf24l01_setup(uint8_t rate, uint8_t channel, uint8_t en_ack);
int16_t nrf24l01_send(uint8_t *dst_addr, uint8_t *data, uint16_t len);
int16_t nrf24l01_recv(uint8_t *src_addr, uint8_t *data);
