/* file:          enc28j60.c
 * description:   Microchip ENC28J60 MAC/Ethernet network driver
 * date:          12/2016
 * author:        Sergio Johann Filho <sergio.filho@pucrs.br>
 * 
 * based on several sources:
 * 
 * http://www.oryx-embedded.com/doc/enc28j60_8c_source.html
 * https://chromium.googlesource.com/chromiumos/third_party/u-boot-next/+/master/drivers/net/enc28j60.c
 * https://github.com/grissiom/rtgui-stm32-tut/blob/master/enc28j60.c
 * https://chromium.googlesource.com/chromiumos/third_party/u-boot-next/+/master/drivers/net/enc28j60_lpc2292.c
 * 
 * TODO: link state diagnostics, error recovery and datasheet errata.
 */

#include <hellfire.h>
#include <eth_enc28j60.h>
#include <spi.h>
#include <enc28j60.h>

static uint8_t encbank;
static uint16_t encpktptr;
mutex_t enclock;

uint8_t enc28j60_readop(uint8_t op, uint8_t address)
{
	uint8_t data;

	spi_start();
	// issue read command
	spi_sendrecv(op | (address & ADDR_MASK));
	// do dummy read if needed (for mac and mii, see datasheet page 29)
	if (address & 0x80)
		spi_sendrecv(0x00);
	// read data
	data = spi_sendrecv(0x00);
	spi_stop();

	return data;
}

void enc28j60_writeop(uint8_t op, uint8_t address, uint8_t data)
{
	spi_start();
	// issue write command
	spi_sendrecv(op | (address & ADDR_MASK));
	// write data
	spi_sendrecv(data);
	spi_stop();
}

void enc28j60_readbuf(uint8_t *data, uint16_t len)
{
	spi_start();
	// issue read command
	spi_sendrecv(ENC28J60_READ_BUF_MEM);
	while(len--)
		*data++ = spi_sendrecv(0x00);
	spi_stop();
}

void enc28j60_writebuf(uint8_t *data, uint16_t len)
{
	spi_start();
	// issue write command
	spi_sendrecv(ENC28J60_WRITE_BUF_MEM);
	while(len--)
		spi_sendrecv(*data++);
	spi_stop();
}

void enc28j60_setbank(uint8_t address)
{
	if((address & BANK_MASK) != encbank){
		// set the bank
		enc28j60_writeop(ENC28J60_BIT_FIELD_CLR, ECON1, (ECON1_BSEL1|ECON1_BSEL0));
		enc28j60_writeop(ENC28J60_BIT_FIELD_SET, ECON1, (address & BANK_MASK) >> 5);
		encbank = (address & BANK_MASK);
	}
}

uint8_t enc28j60_read(uint8_t address)
{
	uint8_t tmp;
	
	// set the bank
	enc28j60_setbank(address);
	// do the read
	tmp = enc28j60_readop(ENC28J60_READ_CTRL_REG, address);
	
	return tmp;
}

void enc28j60_write(uint8_t address, uint8_t data)
{
	// set the bank
	enc28j60_setbank(address);
	// do the write
	enc28j60_writeop(ENC28J60_WRITE_CTRL_REG, address, data);
}

void enc28j60_phywrite(uint8_t address, uint16_t data)
{
	uint16_t i = 0;
	// set the PHY register address
	enc28j60_write(MIREGADR, address);
	// write the PHY data
	enc28j60_write(MIWRL, data);
	enc28j60_write(MIWRH, data >> 8);
	// wait until the PHY write completes
	while(enc28j60_read(MISTAT) & MISTAT_BUSY){
		delay_us(15);
		if (i++ > 3333) break;
	}
}

uint16_t enc28j60_phyread(uint8_t address)
{
	uint16_t data, i = 0;
	
	// set the PHY register address
	enc28j60_write(MIREGADR, address);
	// read the PHY data
	enc28j60_write(MICMD, MICMD_MIIRD);
	// wait until the PHY read completes
	while(enc28j60_read(MISTAT) & MISTAT_BUSY){
		delay_us(15);
		if (i++ > 3333) break;
	}
	// clear command register
	enc28j60_write(MICMD, 0x00);
	data = enc28j60_read(MIRDL);
	data |= enc28j60_read(MIRDH);
	
	return data;	
}

void enc28j60_clkout(uint8_t clk)
{
	//setup clkout: 2 is 12.5MHz:
	enc28j60_write(ECOCON, clk & 0x7);
}

static int32_t en_configure()
{
	uint16_t phyid1, phyid2;
	
	// perform system reset (see Rev. B4 Silicon Errata point 2)
	enc28j60_writeop(ENC28J60_SOFT_RESET, 0, ENC28J60_SOFT_RESET);
	delay_ms(10);
	
	/* check chip id */
	phyid1 = enc28j60_phyread(PHHID1);
	phyid2 = enc28j60_phyread(PHHID2) << 8;
	
	if (phyid1 != ENC_PHHID1_VALUE || (phyid2 & ENC_PHHID2_MASK) != ENC_PHHID2_VALUE){
		kprintf("\nHAL: Ethernet PHY not detected phyid1 = %x, phyid2 = %x", phyid1, phyid2);
		return 0;
	}else{
		kprintf("\nHAL: Ethernet device: enc28j60, revision id: %x", enc28j60_read(EREVID));	
	}
	
	// do bank 0 stuff
	// initialize receive buffer
	// 16-bit transfers, must write low byte first
	// set receive buffer start address
	encpktptr = RXSTART_INIT;
	// Rx start
	enc28j60_write(ERXSTL, RXSTART_INIT & 0xFF);
	enc28j60_write(ERXSTH, RXSTART_INIT >> 8);
	// set receive pointer address
	enc28j60_write(ERXRDPTL, RXSTART_INIT & 0xFF);
	enc28j60_write(ERXRDPTH, RXSTART_INIT >> 8);
	// RX end
	enc28j60_write(ERXNDL, RXSTOP_INIT & 0xFF);
	enc28j60_write(ERXNDH, RXSTOP_INIT >> 8);
	// TX start
	enc28j60_write(ETXSTL, TXSTART_INIT & 0xFF);
	enc28j60_write(ETXSTH, TXSTART_INIT >> 8);
	// TX end
	enc28j60_write(ETXNDL, TXSTOP_INIT & 0xFF);
	enc28j60_write(ETXNDH, TXSTOP_INIT >> 8);
	
	// do bank 1 stuff, packet filter:
	// For broadcast packets we allow only ARP packets
	// All other packets should be unicast only for our mac (MAADR)
	//
	// The pattern to match on is therefore
	// Type     ETH.DST
	// ARP      BROADCAST
	// 06 08 -- ff ff ff ff ff ff -> ip checksum for theses bytes=f7f9
	// in binary these poitions are:11 0000 0011 1111
	// This is hex 303F->EPMM0=0x3f,EPMM1=0x30
	enc28j60_write(ERXFCON, ERXFCON_UCEN | ERXFCON_CRCEN | ERXFCON_PMEN);
	enc28j60_write(EPMM0, 0x3f);
	enc28j60_write(EPMM1, 0x30);
	enc28j60_write(EPMCSL, 0xf9);
	enc28j60_write(EPMCSH, 0xf7);

	// do bank 2 stuff
	// enable MAC receive
	enc28j60_write(MACON1, MACON1_MARXEN | MACON1_TXPAUS | MACON1_RXPAUS);
	// bring MAC out of reset
	enc28j60_write(MACON2, 0x00);
	// enable automatic padding to 60bytes and CRC operations
	enc28j60_writeop(ENC28J60_BIT_FIELD_SET, MACON3, MACON3_PADCFG0 | MACON3_TXCRCEN | MACON3_FRMLNEN);
	// set inter-frame gap (non-back-to-back)
	enc28j60_write(MAIPGL, 0x12);
	enc28j60_write(MAIPGH, 0x0C);
	// set inter-frame gap (back-to-back)
	enc28j60_write(MABBIPG, 0x12);
	// Set the maximum packet size which the controller will accept
	// Do not send packets longer than MAX_FRAMELEN:
	enc28j60_write(MAMXFLL, MAX_FRAMELEN & 0xFF);
	enc28j60_write(MAMXFLH, MAX_FRAMELEN >> 8);
	
	// do bank 3 stuff
	// write MAC address
	// NOTE: MAC address in ENC28J60 is byte-backward
	enc28j60_write(MAADR5, ENC28J60_MAC0);
	enc28j60_write(MAADR4, ENC28J60_MAC1);
	enc28j60_write(MAADR3, ENC28J60_MAC2);
	enc28j60_write(MAADR2, ENC28J60_MAC3);
	enc28j60_write(MAADR1, ENC28J60_MAC4);
	enc28j60_write(MAADR0, ENC28J60_MAC5);
	
	mymac[0] = ENC28J60_MAC0;
	mymac[1] = ENC28J60_MAC1;
	mymac[2] = ENC28J60_MAC2;
	mymac[3] = ENC28J60_MAC3;
	mymac[4] = ENC28J60_MAC4;
	mymac[5] = ENC28J60_MAC5;
	
	kprintf("\nHAL: Ethernet interface en0: MAC address %x:%x:%x:%x:%x:%x",
		mymac[0], mymac[1], mymac[2], mymac[3], mymac[4], mymac[5]);
	
	// no loopback of transmitted frames
	enc28j60_phywrite(PHCON2, PHCON2_HDLDIS);
	// switch to bank 0
	enc28j60_setbank(ECON1);
	// enable interrutps
	enc28j60_writeop(ENC28J60_BIT_FIELD_SET, EIE, EIE_INTIE|EIE_PKTIE);
	// enable packet reception
	enc28j60_writeop(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);
	
	return 1;
}

int32_t en_init()
{
	// initialize I/O
	spi_setup(SPI_CS0, 0);
	
	// configure the Ethernet chip
	if (!en_configure()) return 0;
	
	/* allocate raw frame buffers */
	frame_in = (uint8_t *)malloc(MAX_FRAMELEN);
	frame_out = (uint8_t *)malloc(MAX_FRAMELEN);
	if (!frame_in || !frame_out) panic(PANIC_OOM);
	
	hf_mtxinit(&enclock);
	
	en_irqconfig();
	
	return 1;
}

// read the revision of the chip:
uint8_t enc28j60_getrev(void)
{
	return (enc28j60_read(EREVID));
}


int32_t en_watchdog(void)
{
	return 0;
}

int32_t en_ll_input(uint8_t *frame)
{
	uint16_t rxstat;
	uint16_t size;
	
	hf_mtxlock(&enclock);
	
	// check if a frame has been received
	if(enc28j60_read(EPKTCNT) == 0){
		hf_mtxunlock(&enclock);
		return 0;
	}

	hf_schedlock(1);

	// Set the read pointer to the start of the received frame
	enc28j60_write(ERDPTL, (encpktptr));
	enc28j60_write(ERDPTH, (encpktptr) >> 8);
	
	// read the next frame pointer
	encpktptr = enc28j60_readop(ENC28J60_READ_BUF_MEM, 0);
	encpktptr |= enc28j60_readop(ENC28J60_READ_BUF_MEM, 0) << 8;
	
	// read frame length (see datasheet page 43)
	size = enc28j60_readop(ENC28J60_READ_BUF_MEM, 0);
	size |= enc28j60_readop(ENC28J60_READ_BUF_MEM, 0) << 8;
	//remove the CRC count
	size -= 4;
	if (size > MAX_FRAMELEN) size = 0;
	// read the receive status (see datasheet page 43)
	rxstat = enc28j60_readop(ENC28J60_READ_BUF_MEM, 0);
	rxstat |= enc28j60_readop(ENC28J60_READ_BUF_MEM, 0) << 8;

	// check CRC and symbol errors (see datasheet page 44, table 7-3):
	// The ERXFCON.CRCEN is set by default. Normally we should not
	// need to check this.
	if ((rxstat & 0x80) == 0)	// invalid	TODO: check this logic (0 or not) on the 3rd link
		size = 0;
	else				// copy frame from the receive buffer
		enc28j60_readbuf(frame, size);

	// Move the RX read pointer to the start of the next received frame, freeing memory
	enc28j60_write(ERXRDPTL, (encpktptr));
	enc28j60_write(ERXRDPTH, (encpktptr) >> 8);
	// decrement the packet counter indicates we are done with this frame
	enc28j60_writeop(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);

	hf_schedlock(0);
	hf_mtxunlock(&enclock);
	
	en_irqack();
	
	return size;
}

void en_ll_output(uint8_t *frame, uint16_t size)
{
	hf_mtxlock(&enclock);
	hf_schedlock(1);
	
	// Set the write pointer to start of transmit buffer area
	enc28j60_write(EWRPTL, TXSTART_INIT & 0xFF);
	enc28j60_write(EWRPTH, TXSTART_INIT >> 8);
	
	// Set the TXND pointer to correspond to the frame size given
	enc28j60_write(ETXNDL, (TXSTART_INIT + size) & 0xFF);
	enc28j60_write(ETXNDH, (TXSTART_INIT + size) >> 8);
	
	// write per-packet control byte (0x00 means use macon3 settings)
	enc28j60_writeop(ENC28J60_WRITE_BUF_MEM, 0, 0x00);
	
	// copy the frame into the transmit buffer
	enc28j60_writebuf(frame, size);
	
	// Reset the transmit logic problem. See Rev. B4 Silicon Errata point 12.
	if ((enc28j60_read(EIR) & EIR_TXERIF)){
		enc28j60_writeop(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRST);
		enc28j60_writeop(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_TXRST);
	}
	// send the contents of the transmit buffer onto the network
	enc28j60_writeop(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);

	hf_schedlock(0);
	hf_mtxunlock(&enclock);
}
