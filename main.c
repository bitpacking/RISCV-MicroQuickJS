/**
 * @file        main.c
 * @brief       running led
 * @details     template for GD32VF103
 * @version     V1.0.0
 * @author      ykaidong
 * @date        2022-02-17
 * @copyright   Copyright (c) 2022 DevLabs
 *****************************************************************
 *@attention
 * MCU:         GD32VF103CBT6
 * toolchain:   riscv32-unknown-elf-*
 * lib:         V1.6.0
 *@par modify:
 * <table>
 * <tr><th>Date         <th>Version <th>Author      <th>Description
 * <tr><td>2021-02-17   <td>1.0.0   <td>ykaidong    <td>first version
 * <tr><td>2026-01-05   <td>1.0.1   <td>ykaidong    <td>update lib to V1.6.0
 * </table>
 *
 ****************************************************************
 */

/**
    Copyright (c) 2022, DevLabs(http://www.DevLabs.cn)

    Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software without
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/

#include <stdio.h>

#include "gd32vf103.h"
#include "systick.h"

extern int js_runtime(void);
extern void js_run_repl(void);

void __attribute__((constructor))init(void)
{
    extern void _init(void);
    _init();
}

/**
 * \bref
 * \param
 * \return
 */
int main(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOC);

    gpio_init(GPIOA, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_1 | GPIO_PIN_2);
    gpio_init(GPIOC, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_13);

    gpio_bit_set(GPIOA, GPIO_PIN_1);      // green led off
    gpio_bit_set(GPIOA, GPIO_PIN_2);      // blue led off
    gpio_bit_set(GPIOC, GPIO_PIN_13);     // red led off

    /* USART0 */
    rcu_periph_clock_enable(RCU_USART0);
    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);         // TX
    gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_10);  // RX

    /* USART configure */
    usart_deinit(USART0);
    usart_baudrate_set(USART0, 115200U);
    usart_word_length_set(USART0, USART_WL_8BIT);
    usart_stop_bit_set(USART0, USART_STB_1BIT);
    usart_parity_config(USART0, USART_PM_NONE);
    usart_hardware_flow_rts_config(USART0, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(USART0, USART_CTS_DISABLE);
    usart_receive_config(USART0, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
    usart_enable(USART0);

    printf("Hello, RISC-V!\n\n");

    printf("SystemCoreClock: %d\n", SystemCoreClock);
    printf("CK_SYS: %d\n", rcu_clock_freq_get(CK_SYS));
    printf("CK_AHB: %d\n", rcu_clock_freq_get(CK_AHB));
    printf("CK_APB1: %d\n", rcu_clock_freq_get(CK_APB1));
    printf("CK_APB2: %d\n", rcu_clock_freq_get(CK_APB2));

    js_run_repl();

    // after exit repl, run script.js
    int js_rt_retval = js_runtime();
    printf("MicroQuickJS Runtime return %d\n", js_rt_retval);

    while (1) {
        gpio_bit_reset(GPIOA, GPIO_PIN_1);
        delay_1ms(500);
        gpio_bit_set(GPIOA, GPIO_PIN_1);
        delay_1ms(500);
    }
}

/* retarget the C library printf function to the USART */
int _put_char(int ch)
{
    usart_data_transmit(USART0, (uint8_t)ch);
    while (usart_flag_get(USART0, USART_FLAG_TBE) == RESET);

    return ch;
}

char uart_read(void)
{
    uint8_t ch;

    while (usart_flag_get(USART0, USART_FLAG_RBNE) == RESET);
    ch = usart_data_receive(USART0);

    return ch;
}