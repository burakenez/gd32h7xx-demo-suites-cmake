/*!
    \file    gd32h7xx_it.c
    \brief   interrupt service routines

    \version 2024-07-31, V2.0.0, demo for GD32H7xx
*/

/*
    Copyright (c) 2024, GigaDevice Semiconductor Inc.

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

#include "gd32h7xx.h"
#include "gd32h7xx_it.h"
#include "main.h"
#include "semphr.h"
#include "queue.h"
#include "lwip/sys.h"

extern xSemaphoreHandle g_rx_semaphore;

/*!
    \brief      this function handles NMI exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void NMI_Handler(void)
{
    /* if NMI exception occurs, go to infinite loop */
    while(1) {
    }
}

/*!
    \brief      this function handles HardFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void HardFault_Handler(void)
{
    /* if Hard Fault exception occurs, go to infinite loop */
    while(1) {
    }
}

/*!
    \brief      this function handles MemManage exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void MemManage_Handler(void)
{
    /* if Memory Manage exception occurs, go to infinite loop */
    while(1) {
    }
}

/*!
    \brief      this function handles BusFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void BusFault_Handler(void)
{
    /* if Bus Fault exception occurs, go to infinite loop */
    while(1) {
    }
}

/*!
    \brief      this function handles UsageFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void UsageFault_Handler(void)
{
    /* if Usage Fault exception occurs, go to infinite loop */
    while(1) {
    }
}

/*!
    \brief      this function handles DebugMon exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void DebugMon_Handler(void)
{
    /* if DebugMon exception occurs, go to infinite loop */
    while(1) {
    }
}

/*!
    \brief      this function handles EXTI10_15 exception
    \param[in]  none
    \param[out] none
    \retval     none
*/

/*!
    \brief      this function handles FPU exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void FPU_IRQHandler(void)
{
    while(1) { 
    }
}

#ifdef USE_ENET0
/*!
    \brief      this function handles ethernet interrupt request
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ENET0_IRQHandler(void)
{
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    /* frame received */
    if(SET == enet_interrupt_flag_get(ENET0, ENET_DMA_INT_FLAG_RS)){ 
        /* give the semaphore to wakeup LwIP task */
        xSemaphoreGiveFromISR(g_rx_semaphore, &xHigherPriorityTaskWoken);
    }

    /* clear the enet DMA Rx interrupt pending bits */
    enet_interrupt_flag_clear(ENET0, ENET_DMA_INT_FLAG_RS_CLR);
    enet_interrupt_flag_clear(ENET0, ENET_DMA_INT_FLAG_NI_CLR);

    /* switch tasks if necessary */
    if(pdFALSE != xHigherPriorityTaskWoken){
        portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
    }
}
#endif /* USE_ENET0 */
#ifdef USE_ENET1
/*!
    \brief      this function handles ethernet interrupt request
    \param[in]  none
    \param[out] none
    \retval     none
*/
void ENET1_IRQHandler(void)
{
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    /* frame received */
    if(SET == enet_interrupt_flag_get(ENET1, ENET_DMA_INT_FLAG_RS)){ 
        /* give the semaphore to wakeup LwIP task */
        xSemaphoreGiveFromISR(g_rx_semaphore, &xHigherPriorityTaskWoken);
    }

    /* clear the enet DMA Rx interrupt pending bits */
    enet_interrupt_flag_clear(ENET1, ENET_DMA_INT_FLAG_RS_CLR);
    enet_interrupt_flag_clear(ENET1, ENET_DMA_INT_FLAG_NI_CLR);

    /* switch tasks if necessary */
    if(pdFALSE != xHigherPriorityTaskWoken){
        portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
    }
}
#endif /* USE_ENET1 */