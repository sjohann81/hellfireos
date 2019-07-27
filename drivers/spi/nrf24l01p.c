/* file:          nrf24l01p.c
 * description:   Nordic nRF24L01+ transceiver driver
 * date:          07/2019
 * author:        Sergio Johann Filho <sergio.filho@pucrs.br>
 */

#include <hellfire.h>
#include <spi.h>
#include <nrf24l01p.h>

/******************* low level interface (radio control / frame driver) *******************/

/* init the hardware pins
 * NRF24L01P is on SPI2 (SS: SPI_CS0, SCK: SPI_SCK, MISO: SPI_MISO, MOSI: SPI_MOSI, chip enable: MASK_P3)
 */
uint8_t nrf24l01_init(void)
{
	kprintf("\nHAL: nrf24l01_init()");

	nrf24l01_ce(0);

	/* initialize SPI */
	spi_setup(SPI_CS0, 0);

	/* detect NRF24L01 device */
	nrf24l01_setreg(RF_CH, 77);
	if (nrf24l01_getreg(RF_CH) == 77)
		return 1;

	return 0;
}

/* configure the module */
void nrf24l01_config(uint8_t rate, uint8_t channel, uint8_t ack_en)
{
	// set RF channel
	if (channel < 2 || channel > 123)
		nrf24l01_setreg(RF_CH, NRF24L01_DEF_CHANNNEL);
	else
		nrf24l01_setreg(RF_CH, channel);

	// set length of incoming payload on each pipe
	nrf24l01_setreg(RX_PW_P0, 0x00);			// Auto-ACK pipe ...
	nrf24l01_setreg(RX_PW_P1, NRF24L01_PAYLOAD);		// Data payload pipe
	nrf24l01_setreg(RX_PW_P2, 0x00);			// Pipe not used
	nrf24l01_setreg(RX_PW_P3, 0x00);			// Pipe not used
	nrf24l01_setreg(RX_PW_P4, 0x00);			// Pipe not used
	nrf24l01_setreg(RX_PW_P5, 0x00);			// Pipe not used

	// todo: allow other TX power levels...
	switch (rate) {
	case RATE_2MBPS:
		// 2 Mbps, TX gain: 0dbm
		nrf24l01_setreg(RF_SETUP, (RATE_2MBPS << RF_DR) | ((0x03) << RF_PWR));
		break;
	case RATE_250KBPS:
		// 250 kbps, TX gain: 0dbm
		nrf24l01_setreg(RF_SETUP, (RATE_250KBPS << RF_DR) | ((0x03) << RF_PWR));
		break;
	default:
		// 1 Mbps, TX gain: 0dbm
		nrf24l01_setreg(RF_SETUP, (RATE_1MBPS << RF_DR) | ((0x03) << RF_PWR));
	}

	// CRC enable, 1 byte CRC length
	nrf24l01_setreg(CONFIG, NRF24L01_CONFIG);

	if (ack_en) {
		// auto ack
		nrf24l01_setreg(EN_AA, (1 << ENAA_P0) | (1 << ENAA_P1) | (0 << ENAA_P2) | (0 << ENAA_P3) | (0 << ENAA_P4) | (0 << ENAA_P5));
		// enable RX addresses
		nrf24l01_setreg(EN_RXADDR, (1 << ERX_P0) | (1 << ERX_P1) | (0 << ERX_P2) | (0 << ERX_P3) | (0<<ERX_P4) | (0 << ERX_P5));
		// auto retransmit delay: 2000 us and up to 15 retransmit trials
		nrf24l01_setreg(SETUP_RETR, (0x08 << ARD) | (0x0f << ARC));
	} else {
		// no auto ack
		nrf24l01_setreg(EN_AA, (0 << ENAA_P0) | (0 << ENAA_P1) | (0 << ENAA_P2) | (0 << ENAA_P3) | (0 << ENAA_P4) | (0 << ENAA_P5));
		// enable RX addresses
		nrf24l01_setreg(EN_RXADDR, (0 << ERX_P0) | (1 << ERX_P1) | (0 << ERX_P2) | (0 << ERX_P3) | (0<<ERX_P4) | (0 << ERX_P5));
		// no retransmit
		nrf24l01_setreg(SETUP_RETR, 0x00);
	}

	// no dynamic length config
	nrf24l01_setreg(DYNPD, (0 << DPL_P0) | (0 << DPL_P1) | (0 << DPL_P2) | (0 << DPL_P3) | (0 << DPL_P4) | (0 << DPL_P5));

	// start listening
	nrf24l01_rx_powerup();
}

/* set RX address */
void nrf24l01_rx_address(uint8_t *addr)
{
	nrf24l01_ce(0);
	nrf24l01_writereg(RX_ADDR_P1, addr, NRF24L01_ADDR_LEN);
	nrf24l01_ce(1);
}

/* set TX address */
void nrf24l01_tx_address(uint8_t *addr)
{
	nrf24l01_writereg(RX_ADDR_P0, addr, NRF24L01_ADDR_LEN);
	nrf24l01_writereg(TX_ADDR, addr, NRF24L01_ADDR_LEN);
}

/* checks if data is available for reading */
uint8_t nrf24l01_dataready(void)
{
	if (nrf24l01_status() & (1 << RX_DR))
		return 1;

	return (!nrf24l01_fifoempty());
}

uint8_t nrf24l01_issending(void)
{
	if ((nrf24l01_status() & ((1 << TX_DS) | (1 << MAX_RT))))
		return 0;
	else
		return 1;

}

/* checks if receive FIFO is empty */
uint8_t nrf24l01_fifoempty(void)
{
	uint8_t fifostatus;

	nrf24l01_readreg(FIFO_STATUS, &fifostatus, 1);

	return (fifostatus & (1 << RX_EMPTY));
}

/* returns the length of data waiting in the RX fifo */
uint8_t nrf24l01_payload(void)
{
	uint8_t status;

	spi_start();
	status = spi_sendrecv(R_RX_PL_WID);
	status = spi_sendrecv(0x00);
	spi_stop();

	return status;
}

/* returns the state of the last message */
uint8_t nrf24l01_laststatus(void)
{
	uint8_t val;

	val = nrf24l01_status();
	if (val & ((1 << TX_DS))) {
		return NRF24L01_TX_OK;
	} else if (val & ((1 << MAX_RT))) {
		return NRF24L01_TX_ERROR;
	} else {
		return -1;
	}
}

/* returns the number of retransmissions occured for the last message */
uint8_t nrf24l01_retcount(void)
{
	uint8_t val;

	nrf24l01_readreg(OBSERVE_TX, &val, 1);

	return (val & 0x0f);
}

/* receives data frame */
void nrf24l01_rx(uint8_t *data)
{
	spi_start();
	/* send cmd to read rx payload */
	spi_sendrecv(R_RX_PAYLOAD);
	/* read payload */
	nrf24l01_xfersync(data, data, NRF24L01_PAYLOAD);
	spi_stop();

	/* reset status register */
	nrf24l01_setreg(STATUS, (1 << RX_DR));
}

/* sends data frame to the configured address */
void nrf24l01_tx(uint8_t *data)
{
	nrf24l01_ce(0);
	/* set to TX mode */
	nrf24l01_tx_powerup();

	/* flush TX FIFO */
	spi_start();
	spi_sendrecv(FLUSH_TX);
	spi_stop();

	/* write payload */
	spi_start();
	spi_sendrecv(W_TX_PAYLOAD);
	nrf24l01_xmitsync(data, NRF24L01_PAYLOAD);
	spi_stop();

	/* start TX */
	nrf24l01_ce(1);
}

/* low level interface / SPI bus packets */

void nrf24l01_rx_powerup(void)
{
	spi_start();
	spi_sendrecv(FLUSH_RX);
	spi_stop();

	nrf24l01_setreg(STATUS, (1 << RX_DR) | (1 << TX_DS) | (1 << MAX_RT));

	nrf24l01_ce(0);
	nrf24l01_setreg(CONFIG, NRF24L01_CONFIG | ((1 << PWR_UP) | (1 << PRIM_RX)));
	nrf24l01_ce(1);
}

void nrf24l01_tx_powerup(void)
{
	nrf24l01_setreg(STATUS, (1 << RX_DR) | (1 << TX_DS) | (1 << MAX_RT));
	nrf24l01_setreg(CONFIG, NRF24L01_CONFIG | ((1 << PWR_UP) | (0 << PRIM_RX)));
}

void nrf24l01_powerdown(void)
{
	nrf24l01_ce(0);
	nrf24l01_setreg(CONFIG, NRF24L01_CONFIG);
	delay_ms(2);
}

void nrf24l01_ce(uint8_t val)
{
	if (val)
		SPI_OUTPORT |= MASK_P3;
	else
		SPI_OUTPORT &= ~MASK_P3;

	delay_us(150);
}

uint8_t nrf24l01_status(void)
{
	uint8_t val;

	spi_start();
	val = spi_sendrecv(NOP);
	spi_stop();

	return val;
}

void nrf24l01_xfersync(uint8_t *dataout, uint8_t *datain, uint8_t len)
{
	uint8_t i;

	for (i = 0; i < len; i++)
		datain[i] = spi_sendrecv(dataout[i]);
}

void nrf24l01_xmitsync(uint8_t *dataout, uint8_t len)
{
	uint8_t i;

	for (i = 0; i < len; i++)
		spi_sendrecv(dataout[i]);
}

void nrf24l01_setreg(uint8_t reg, uint8_t val)
{
	spi_start();
	spi_sendrecv(W_REGISTER | (REGISTER_MASK & reg));
	spi_sendrecv(val);
	spi_stop();
}

uint8_t nrf24l01_getreg(uint8_t reg)
{
	uint8_t val;

	spi_start();
	spi_sendrecv(R_REGISTER | (REGISTER_MASK & reg));
	val = spi_sendrecv(0x00);
	spi_stop();

	return val;
}

void nrf24l01_readreg(uint8_t reg, uint8_t *val, uint8_t len)
{
	spi_start();
	spi_sendrecv(R_REGISTER | (REGISTER_MASK & reg));
	nrf24l01_xfersync(val, val, len);
	spi_stop();
}

void nrf24l01_writereg(uint8_t reg, uint8_t *val, uint8_t len)
{
	spi_start();
	spi_sendrecv(W_REGISTER | (REGISTER_MASK & reg));
	nrf24l01_xmitsync(val, len);
	spi_stop();
}

/******************* high level interface (packet driver) *******************/

static struct nrf24l01_driver_s rfcontext;

int16_t nrf24l01_setup(uint8_t rate, uint8_t channel, uint8_t en_ack)
{
	struct nrf24l01_driver_s *prfcontext;

	prfcontext = &rfcontext;
	if (!nrf24l01_init()) {
		printf("\nHAL: NRF24L01+ radio not detected");
		return -1;
	} else {
		printf("\nHAL: NRF24L01+ radio detected");
		nrf24l01_config(rate, channel, en_ack);
	}

	prfcontext->rx_addr[0] = 0x0e;
	prfcontext->rx_addr[1] = 0x01;
	prfcontext->rx_addr[2] = 0x02;
	prfcontext->rx_addr[3] = 0x03;
	prfcontext->rx_addr[4] = 0x04;

	nrf24l01_rx_address(prfcontext->rx_addr);
	printf("\nHAL: nrf24l01_setup(), MAC: %02x:%02x:%02x:%02x:%02x",
		prfcontext->rx_addr[0], prfcontext->rx_addr[1], prfcontext->rx_addr[2],
		prfcontext->rx_addr[3], prfcontext->rx_addr[4]);

	return 0;
}

int16_t nrf24l01_send(uint8_t *dst_addr, uint8_t *data, uint16_t len)
{
	struct nrf24l01_driver_s *prfcontext;
	struct nrf24l01_frame_s *pframe;
	uint8_t null_address[NRF24L01_ADDR_LEN];
	uint8_t frame[NRF24L01_PAYLOAD];
	int32_t packets, tries;

	prfcontext = &rfcontext;
	pframe = (struct nrf24l01_frame_s *)&frame;
	memset(null_address, 0x00, NRF24L01_ADDR_LEN);
	memcpy(prfcontext->tx_addr, dst_addr, NRF24L01_ADDR_LEN);
	prfcontext->payload = 0;
	packets = len / (NRF24L01_PAYLOAD - NRF24L01_ADDR_LEN - 2) + 1;

	if (packets > 255)
		return NRF24L01_ETOOBIG;

	// first frame
	for (tries = 0; tries < NRF24L01_XFER_RETRIES; tries++) {
		nrf24l01_tx_address(prfcontext->tx_addr);
		// fill source, payload, sequence and data
		memcpy(pframe->src_addr, prfcontext->rx_addr, NRF24L01_ADDR_LEN);
		pframe->payload = packets;
		pframe->seq = 0;
		memcpy(pframe->data, data, (NRF24L01_PAYLOAD - NRF24L01_ADDR_LEN - 2));
		nrf24l01_tx(frame);
		while (nrf24l01_issending());
		if (prfcontext->en_ack == 0) {
			nrf24l01_rx_powerup();
			// wait a bit, remote must process this frame
			delay_ms(NRF24L01_XFER_WAIT_MS);
			if (nrf24l01_dataready()) {
				nrf24l01_rx(frame);
				// check if this is an ack
				if ((!memcmp(prfcontext->tx_addr, pframe->src_addr, NRF24L01_ADDR_LEN)) && (pframe->seq == 0))
					break;
			}
		} else {
			if (nrf24l01_laststatus() == NRF24L01_TX_OK) {
				nrf24l01_rx_powerup();
				break;
			}
		}
		delay_ms(NRF24L01_START_WAIT_MS);
	}

	if (tries == NRF24L01_XFER_RETRIES)
		return NRF24L01_ESYNC;

	// in sync with remote radio, send other frames
	for (prfcontext->seq = 1; prfcontext->seq < packets; prfcontext->seq++) {
		for (tries = 0; tries < NRF24L01_XFER_RETRIES; tries++) {
			nrf24l01_tx_address(prfcontext->tx_addr);
			memcpy(pframe->src_addr, prfcontext->rx_addr, NRF24L01_ADDR_LEN);
			pframe->payload = packets;
			pframe->seq = prfcontext->seq;
			memcpy(pframe->data, data + (prfcontext->seq * (NRF24L01_PAYLOAD - NRF24L01_ADDR_LEN - 2)), (NRF24L01_PAYLOAD - NRF24L01_ADDR_LEN - 2));
			nrf24l01_tx(frame);
			while (nrf24l01_issending());
			if (prfcontext->en_ack == 0) {
				nrf24l01_rx_powerup();
				// wait a bit, remote must process this frame
				delay_ms(NRF24L01_XFER_WAIT_MS);
				if (nrf24l01_dataready()) {
					nrf24l01_rx(frame);
					// check if this is an ack
					if ((!memcmp(prfcontext->tx_addr, pframe->src_addr, NRF24L01_ADDR_LEN)) && (pframe->seq == 0))
						break;
				}
			} else {
				if (nrf24l01_laststatus() == NRF24L01_TX_OK) {
					nrf24l01_rx_powerup();
					break;
				}
			}
		}

		if (tries == NRF24L01_XFER_RETRIES)
			return NRF24L01_ERETRIES;
	}

	return 0;
}

int16_t nrf24l01_recv(uint8_t *src_addr, uint8_t *data)
{
	struct nrf24l01_driver_s *prfcontext;
	struct nrf24l01_frame_s *pframe;
	uint8_t null_addr[NRF24L01_ADDR_LEN];
	uint8_t frame[NRF24L01_PAYLOAD];
	uint64_t time;

	prfcontext = &rfcontext;
	pframe = (struct nrf24l01_frame_s *)&frame;
	memset(null_addr, 0x00, NRF24L01_ADDR_LEN);
	memset(prfcontext->tx_addr, 0x00, NRF24L01_ADDR_LEN);
	prfcontext->payload = 0;
	prfcontext->seq = 0;
	time = 0;

	while (1) {
		if (nrf24l01_dataready()) {
			nrf24l01_rx(frame);

			// already seen this source before...
			if (!memcmp(prfcontext->tx_addr, pframe->src_addr, NRF24L01_ADDR_LEN)) {
				// same frame! sender didn't get last ack, so ack it now!
				if (prfcontext->seq - 1 == pframe->seq) {
					if (prfcontext->en_ack == 0) {
						// send ack
						memcpy(pframe->src_addr, prfcontext->rx_addr, NRF24L01_ADDR_LEN);
						// frame seq remains the same, payload is 0, data is null
						pframe->payload = 0;
						memset(pframe->data, 0x00, (NRF24L01_PAYLOAD - NRF24L01_ADDR_LEN - 2));
						nrf24l01_tx(frame);
						while (nrf24l01_issending());
						nrf24l01_rx_powerup();
					}
					continue;
				}
			}

			if (pframe->seq == 0) {
				// start transfer timer
				time = _read_us() / 1000;
				// first frame from another remote radio, drop it
				if (memcmp(prfcontext->tx_addr, null_addr, NRF24L01_ADDR_LEN)) continue;
				// first frame, setup tx for acks for the remote (source) radio
				nrf24l01_tx_address(pframe->src_addr);
				memcpy(prfcontext->tx_addr, pframe->src_addr, NRF24L01_ADDR_LEN);
				// first frame, get payload size (number of frames of this packet)
				prfcontext->payload = pframe->payload;
				memcpy(data, pframe->data, (NRF24L01_PAYLOAD - NRF24L01_ADDR_LEN - 2));
			} else {
				// frame from another remote radio, drop it
				if (memcmp(prfcontext->tx_addr, pframe->src_addr, NRF24L01_ADDR_LEN)) continue;
				// detect framing error
				if (prfcontext->seq != pframe->seq)
					return NRF24L01_EFRAMING;
				// detect payload error
				if (prfcontext->payload != pframe->payload)
					return NRF24L01_EPAYLOAD;
				// other frames
				memcpy(data + (prfcontext->seq * (NRF24L01_PAYLOAD - NRF24L01_ADDR_LEN - 2)), pframe->data, (NRF24L01_PAYLOAD - NRF24L01_ADDR_LEN - 2));
			}

			if (prfcontext->en_ack == 0) {
				// ack received frame from this address
				memcpy(pframe->src_addr, prfcontext->rx_addr, NRF24L01_ADDR_LEN);
				// frame seq remains the same, payload is 0, data is null
				pframe->payload = 0;
				memset(pframe->data, 0x00, (NRF24L01_PAYLOAD - NRF24L01_ADDR_LEN - 2));
				nrf24l01_tx(frame);
				while (nrf24l01_issending());
				nrf24l01_rx_powerup();
			}

			prfcontext->seq++;
			// last frame
			if (prfcontext->seq == prfcontext->payload) {
				memcpy(src_addr, prfcontext->tx_addr, NRF24L01_ADDR_LEN);

				return prfcontext->payload * (NRF24L01_PAYLOAD - NRF24L01_ADDR_LEN - 2);
			}

		} else {
			delay_ms(1);
		}
		// transfer took too long...
		if (time && ((_read_us() / 1000 - time) > NRF24L01_XFER_LIMIT_MS))
			return NRF24L01_ETIMEOUT;
	}
}
