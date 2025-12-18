#include "uart.h"
void main(){
    uart_init();
    uart_send('H');
    uart_send('e');
    uart_send('l');
    uart_send('l');
    uart_send('o');

    while(1){
    char c = uart_getc();
    uart_send(c);
    }







    ;}
