#include "mailbox.h"
#include "uart.h"
volatile unsigned int __attribute__((aligned(16))) mbox[36];
int mbox_call(unsigned char ch) {
  unsigned int r = (((unsigned int)((unsigned long)&mbox) & ~0xF) | (ch & 0xF));
  /* wait until we can write to the mailbox */
  do {
    asm volatile("nop");
  } while (*MBOX_STATUS & MBOX_FULL);
  /* write the address of our message to the mailbox with channel identifier */
  *MBOX_WRITE = r;
  /* now wait for the response */
  while (1) {
    /* is there a response? */
    do {
      asm volatile("nop");
    } while (*MBOX_STATUS & MBOX_EMPTY);
    /* is it a response to our message? */
    if (r == *MBOX_READ)
      /* is it a valid successful response? */
      return mbox[1] == MBOX_RESPONSE;
  }
  return 0;
}

void get_board_revision() {
  mbox[0] = 8 * 4;        // length of the message
  mbox[1] = MBOX_REQUEST; // this is a request message

  mbox[2] = MBOX_TAG_GETSERIAL; // get serial number command
  mbox[3] = 8;                  // buffer size
  mbox[4] = 0;
  mbox[5] = 0; // clear output buffer
  mbox[6] = 0;

  mbox[7] = MBOX_TAG_LAST;

  // send the message to the GPU and receive answer
  if (mbox_call(MBOX_CH_PROP)) {
    uart_puts("My serial number is: ");
    uart_hex(mbox[6]);
    uart_hex(mbox[5]);
    uart_puts("\n");
  } else {
    uart_puts("Unable to query serial!\n");
  }

  // echo everything back
  while (1) {
    uart_send(uart_getc());
  }
}
void get_arm_memory() {
  mbox[0] = 8 * 4; // buffer size in bytes
  mbox[1] = MBOX_REQUEST;
  // tags begin
  mbox[2] = MBOX_TAG_GETMEM; // tag identifier
  mbox[3] = 8; // maximum of request and response value buffer's length.
  mbox[4] = 0;
  mbox[5] = 0; // value buffer
  mbox[6] = 0; // value buffer
  mbox[7] = MBOX_TAG_LAST;
  if (mbox_call(MBOX_CH_PROP)) {
    uart_puts("ARM Base Address: ");
    uart_hex(mbox[5]);
    uart_puts("\n");
    uart_puts("ARM Memory Size: ");
    uart_hex(mbox[6]);
    uart_puts("\n");
  } else {
    uart_puts("Unable to query ARM memory!\n");
  };
}
