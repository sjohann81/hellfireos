/**
 * @file ni_hermes.c
 * @author Sergio Johann Filho
 * @date July 2018
 *
 * @section LICENSE
 *
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file 'doc/license/gpl-2.0.txt' for more details.
 *
 * @section DESCRIPTION
 *
 * Network interface driver for a simple interface (queue based) attached to an Hermes NoC.
 * This driver works with 16-bit flits. Basic media access functions (_ni_status(), _ni_read()
 * and _ni_write()) defined in the processor architecture memory map are used in this driver.
 *
 * Packet format is as follows:
 *
 \verbatim
  2 bytes   2 bytes         ....
 --------------------------------------
 |tgt_cpu  |payload  |  ... data ...  |
 --------------------------------------
 \endverbatim
 *
 */

#include <hellfire.h>
#include <ni.h>

int32_t ni_ready(void)
{
	return (int32_t)_ni_status() & 0x1;
}

int32_t ni_flush(uint16_t pkt_size)
{
	uint32_t status;
	int32_t i;

	status = _di();
	_ni_read();
	for(i = 0; i < pkt_size; i++)
		_ni_read();
	_ei(status);

	return ni_ready();
}

int32_t ni_read_packet(uint16_t *buf, uint16_t pkt_size)
{
	uint32_t status;
	int32_t i;

	status = _di();
	_ni_read();
	for (i = 0; i < pkt_size; i++)
		buf[i] = _ni_read();
	_ei(status);

	return 0;
}

int32_t ni_write_packet(uint16_t *buf, uint16_t pkt_size)
{
	uint32_t status;
	int32_t i;

retry:
	while (!ni_ready());
	status = _di();
	if (!ni_ready()) {
		_ei(status);
		goto retry;
	}
	for (i = 0; i < pkt_size; i++)
		_ni_write(buf[i]);
	_ei(status);

	return 0;
}
