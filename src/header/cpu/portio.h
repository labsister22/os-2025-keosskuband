#ifndef _PORTIO_H
#define _PORTIO_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/** out:
 *  Sends the given data to the given I/O port
 *
 *  @param port The I/O port to send the data to
 *  @param data The data to send to the I/O port
 */
void out(uint16_t port, uint8_t data);

/** in:
 *  Read data from the given I/O port
 *
 *  @param port The I/O port to request the data
 *  @return Recieved data from the corresponding I/O port
 */
uint8_t in(uint16_t port);

/** out16:
 *  Sends the given data to the given I/O port with the size of 16 bits
 *
 *  @param port The I/O port to send the data to
 *  @param data The data to send to the I/O port
 */
void out16(uint16_t port, uint16_t data);

/** in16:
 *  Read data from the given I/O port with the size of 16 bits
 *
 *  @param port The I/O port to request the data
 *  @return Recieved data from the corresponding I/O port
 */
uint16_t in16(uint16_t port);

#endif