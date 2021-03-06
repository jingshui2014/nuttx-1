/****************************************************************************
 * arch/arm/src/stm32/stm32_exti_gpio.c
 *
 *   Copyright (C) 2009, 2011-2012 Gregory Nutt. All rights reserved.
 *   Copyright (C) 2011 Uros Platise. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *           Uros Platise <uros.platise@isotel.eu>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/
/*
 * Copyright (c) 2015 Google, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * * may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/irq.h>
#include <nuttx/arch.h>
#include <nuttx/gpio/debounce.h>

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <debug.h>

#include <arch/irq.h>

#include "up_arch.h"
#include "chip.h"
#include "stm32_gpio.h"
#include "stm32_exti.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Interrupt handlers attached to each EXTI */
struct stm32_exti_handlers_priv_t {
    xcpt_t stm32_exti_callback;
    uint32_t pin;
    void *priv;
    struct debounce_data debounce;
    bool risingedge;
    bool fallingedge;
    uint32_t pinset;
};

/* debounce handler prototype */
static int stm32_exti_debounce_isr(int irq, void *context, void *priv);

/* Handlers that pass private data ptr */
static struct stm32_exti_handlers_priv_t stm32_exti_handlers[16] = {
    { .debounce.gpio = 0, .debounce.ms = 0,
        .debounce.isr = stm32_exti_debounce_isr },
    { .debounce.gpio = 1, .debounce.ms = 0,
        .debounce.isr = stm32_exti_debounce_isr },
    { .debounce.gpio = 2, .debounce.ms = 0,
        .debounce.isr = stm32_exti_debounce_isr },
    { .debounce.gpio = 3, .debounce.ms = 0,
        .debounce.isr = stm32_exti_debounce_isr },
    { .debounce.gpio = 4, .debounce.ms = 0,
        .debounce.isr = stm32_exti_debounce_isr },
    { .debounce.gpio = 5, .debounce.ms = 0,
        .debounce.isr = stm32_exti_debounce_isr },
    { .debounce.gpio = 6, .debounce.ms = 0,
        .debounce.isr = stm32_exti_debounce_isr },
    { .debounce.gpio = 7, .debounce.ms = 0,
        .debounce.isr = stm32_exti_debounce_isr },
    { .debounce.gpio = 8, .debounce.ms = 0,
        .debounce.isr = stm32_exti_debounce_isr },
    { .debounce.gpio = 9, .debounce.ms = 0,
        .debounce.isr = stm32_exti_debounce_isr },
    { .debounce.gpio = 10, .debounce.ms = 0,
        .debounce.isr = stm32_exti_debounce_isr },
    { .debounce.gpio = 11, .debounce.ms = 0,
        .debounce.isr = stm32_exti_debounce_isr },
    { .debounce.gpio = 12, .debounce.ms = 0,
        .debounce.isr = stm32_exti_debounce_isr },
    { .debounce.gpio = 13, .debounce.ms = 0,
        .debounce.isr = stm32_exti_debounce_isr },
    { .debounce.gpio = 14, .debounce.ms = 0,
        .debounce.isr = stm32_exti_debounce_isr },
    { .debounce.gpio = 15, .debounce.ms = 0,
        .debounce.isr = stm32_exti_debounce_isr },
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

 /****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Interrupt Service Routines - Dispatchers
 ****************************************************************************/

static int stm32_exti_debounce_isr(int irq, void *context, void *priv)
{
    int ret = OK;

    /* preserve ISR context */
    stm32_exti_handlers[irq].debounce.context = context;

    /* store private data */
    stm32_exti_handlers[irq].debounce.priv = priv;

    bool value = stm32_gpioread(stm32_exti_handlers[irq].pinset);

    /* check if GPIO value is stable */
    if (debounce_gpio(&stm32_exti_handlers[irq].debounce, value)) {

        enum debounce_state state = stm32_exti_handlers[irq].debounce.db_state;
        bool risingedge = stm32_exti_handlers[irq].risingedge;
        bool fallingedge = stm32_exti_handlers[irq].fallingedge;

        if (risingedge && state == DB_ST_ACTIVE_STABLE)
            goto call_irq;

        if (fallingedge && state == DB_ST_INACTIVE_STABLE)
            goto call_irq;
    }

    return ret;

call_irq:
    /* reset debounce state */
    stm32_exti_handlers[irq].debounce.db_state = DB_ST_INVALID;

    /* send off callback function */
    if (stm32_exti_handlers[irq].stm32_exti_callback) {
        ret = stm32_exti_handlers[irq].stm32_exti_callback(
                                                stm32_exti_handlers[irq].pin,
                                                context,
                                                priv);
    }

    return ret;
}

static int stm32_exti0_isr(int irq, void *context, void *priv)
{
  int ret = OK;

  /* Clear the pending interrupt */

  putreg32(0x0001, STM32_EXTI_PR);

  if (stm32_exti_handlers[0].debounce.ms == 0) {
    /* And dispatch the interrupt to the handler */
    if (stm32_exti_handlers[0].stm32_exti_callback) {
        ret = stm32_exti_handlers[0].stm32_exti_callback(
                                                stm32_exti_handlers[0].pin,
                                                context,
                                                stm32_exti_handlers[0].priv);
    }
  } else {
      /* call debouncing handler */
      ret = stm32_exti_debounce_isr(stm32_exti_handlers[0].pin, context,
                                    stm32_exti_handlers[0].priv);
  }

  return ret;
}

static int stm32_exti1_isr(int irq, void *context, void *priv)
{
  int ret = OK;

  /* Clear the pending interrupt */

  putreg32(0x0002, STM32_EXTI_PR);

  if (stm32_exti_handlers[1].debounce.ms == 0) {
    /* And dispatch the interrupt to the handler */
    if (stm32_exti_handlers[1].stm32_exti_callback) {
        ret = stm32_exti_handlers[1].stm32_exti_callback(
                                                stm32_exti_handlers[1].pin,
                                                context,
                                                stm32_exti_handlers[1].priv);
    }
  } else {
      /* call debouncing handler */
      ret = stm32_exti_debounce_isr(stm32_exti_handlers[1].pin, context,
                                    stm32_exti_handlers[1].priv);
  }

  return ret;
}

static int stm32_exti2_isr(int irq, void *context, void *priv)
{
  int ret = OK;

  /* Clear the pending interrupt */

  putreg32(0x0004, STM32_EXTI_PR);

  if (stm32_exti_handlers[2].debounce.ms == 0) {
    /* And dispatch the interrupt to the handler */
    if (stm32_exti_handlers[2].stm32_exti_callback) {
        ret = stm32_exti_handlers[2].stm32_exti_callback(
                                                stm32_exti_handlers[2].pin,
                                                context,
                                                stm32_exti_handlers[2].priv);
    }
  } else {
      /* call debouncing handler */
      ret = stm32_exti_debounce_isr(stm32_exti_handlers[2].pin, context,
                                    stm32_exti_handlers[2].priv);
  }

  return ret;
}

static int stm32_exti3_isr(int irq, void *context, void *priv)
{
  int ret = OK;

  /* Clear the pending interrupt */

  putreg32(0x0008, STM32_EXTI_PR);

  if (stm32_exti_handlers[3].debounce.ms == 0) {
    /* And dispatch the interrupt to the handler */
    if (stm32_exti_handlers[3].stm32_exti_callback) {
        ret = stm32_exti_handlers[3].stm32_exti_callback(
                                                stm32_exti_handlers[3].pin,
                                                context,
                                                stm32_exti_handlers[3].priv);
    }
  } else {
      /* call debouncing handler */
      ret = stm32_exti_debounce_isr(stm32_exti_handlers[3].pin, context,
                                    stm32_exti_handlers[3].priv);
  }

  return ret;
}

static int stm32_exti4_isr(int irq, void *context, void *priv)
{
  int ret = OK;

  /* Clear the pending interrupt */

  putreg32(0x0010, STM32_EXTI_PR);

  if (stm32_exti_handlers[4].debounce.ms == 0) {
    /* And dispatch the interrupt to the handler */
    if (stm32_exti_handlers[4].stm32_exti_callback) {
        ret = stm32_exti_handlers[4].stm32_exti_callback(
                                                stm32_exti_handlers[4].pin,
                                                context,
                                                stm32_exti_handlers[4].priv);
    }
  } else {
      /* call debouncing handler */
      ret = stm32_exti_debounce_isr(stm32_exti_handlers[4].pin, context,
                                    stm32_exti_handlers[4].priv);
  }

  return ret;
}

static int stm32_exti_multiisr(int irq, void *context, int first, int last)
{
  uint32_t pr;
  int pin;
  int ret = OK;

  /* Examine the state of each pin in the group */

  pr = getreg32(STM32_EXTI_PR);

  /* And dispatch the interrupt to the handler */

  for (pin = first; pin <= last; pin++)
    {
      /* Is an interrupt pending on this pin? */

      uint32_t mask = (1 << pin);
      if ((pr & mask) != 0)
        {
          /* Clear the pending interrupt */

          putreg32(mask, STM32_EXTI_PR);

          /* And dispatch the interrupt to the handler */

          int tmp = -1;
          if (stm32_exti_handlers[pin].debounce.ms == 0) {
            if (stm32_exti_handlers[pin].stm32_exti_callback) {
                tmp = stm32_exti_handlers[pin].stm32_exti_callback(
                                                stm32_exti_handlers[pin].pin,
                                                context,
                                                stm32_exti_handlers[pin].priv);
            }
          } else {
            /* call debouncing handler */
            tmp = stm32_exti_debounce_isr(stm32_exti_handlers[pin].pin,
                                          context,
                                          stm32_exti_handlers[pin].priv);
          }
          if (tmp != OK)
          {
            ret = tmp;
          }

        }
    }

  return ret;
}

static int stm32_exti95_isr(int irq, void *context, void *priv)
{
  return stm32_exti_multiisr(irq, context, 5, 9);
}

static int stm32_exti1510_isr(int irq, void *context, void *priv)
{
  return stm32_exti_multiisr(irq, context, 10, 15);
}

/****************************************************************************
 * Name: stm32_gpiosetevent
 *
 * Description:
 *   Sets/clears GPIO based event and interrupt triggers.
 *
 * Parameters:
 *  - pinset: gpio pin configuration
 *  - rising/falling edge: enables
 *  - event:  generate event when set
 *  - func:   when non-NULL, generate interrupt
 *
 * Returns: pin associated with the IRQ vector
 *
 ****************************************************************************/

static uint32_t stm32_gpio_setevent(uint32_t pinset, bool risingedge,
                                    bool fallingedge,
                                    bool event, bool func)
{
  uint32_t pin = pinset & GPIO_PIN_MASK;
  uint32_t exti = STM32_EXTI_BIT(pin);
  int      irq;
  xcpt_t   handler;

  /* Select the interrupt handler for this EXTI pin */

  if (pin < 5)
    {
      irq = pin + STM32_IRQ_EXTI0;
      switch (pin)
        {
          case 0:
            handler = stm32_exti0_isr;
            break;

          case 1:
            handler = stm32_exti1_isr;
            break;

          case 2:
            handler = stm32_exti2_isr;
            break;

          case 3:
            handler = stm32_exti3_isr;
            break;

          default:
            handler = stm32_exti4_isr;
            break;
        }
    }
  else if (pin < 10)
    {
      irq     = STM32_IRQ_EXTI95;
      handler = stm32_exti95_isr;
    }
  else
    {
      irq     = STM32_IRQ_EXTI1510;
      handler = stm32_exti1510_isr;
    }

  /* Install external interrupt handlers */

  if (func)
    {
      irq_attach(irq, handler, NULL);
      up_enable_irq(irq);
    }
  else
    {
      up_disable_irq(irq);
    }

  /* Configure GPIO, enable EXTI line enabled if event or interrupt is
   * enabled.
   */

  if (event || func)
    {
      pinset |= GPIO_EXTI;
    }

  stm32_configgpio(pinset);

  /* Configure rising/falling edges */

  modifyreg32(STM32_EXTI_RTSR,
              risingedge ? 0 : exti,
              risingedge ? exti : 0);
  modifyreg32(STM32_EXTI_FTSR,
              fallingedge ? 0 : exti,
              fallingedge ? exti : 0);

  /* Enable Events and Interrupts */

  modifyreg32(STM32_EXTI_EMR,
              event ? 0 : exti,
              event ? exti : 0);
  modifyreg32(STM32_EXTI_IMR,
              func ? 0 : exti,
              func ? exti : 0);

  return pin;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/*
 * Returns:
 *  The previous value of the interrupt handler function pointer.  This value may,
 *  for example, be used to restore the previous handler when multiple handlers are
 *  used.
 */
xcpt_t stm32_gpiosetevent_priv(uint32_t pinset, bool risingedge,
                                    bool fallingedge, bool event,
                                    xcpt_t func, void *priv) {
  xcpt_t   oldhandler = NULL;
  uint32_t pin;
  bool     bfunc = func ? true : false;
  irqstate_t flags;

  /*
   * Disable IRQs so that the handler gets installed correctly before the IRQ
   * is re-enabled.
   */
  flags = irqsave();

  /* Set GPIO based event and interrupt triggers */
  pin = stm32_gpio_setevent(pinset, risingedge, fallingedge, event, bfunc);

  /* Get the previous GPIO IRQ handler; Save the new IRQ handler. */
  oldhandler = stm32_exti_handlers[pin].stm32_exti_callback;
  stm32_exti_handlers[pin].stm32_exti_callback = func;
  stm32_exti_handlers[pin].pin = pin;
  stm32_exti_handlers[pin].priv = priv;
  stm32_exti_handlers[pin].risingedge = risingedge;
  stm32_exti_handlers[pin].fallingedge = fallingedge;
  stm32_exti_handlers[pin].pinset = pinset;
  stm32_exti_handlers[pin].debounce.db_state = DB_ST_INVALID;

  /* Re-enable IRQs */
  irqrestore(flags);

  /* Return the old IRQ handler */
  return oldhandler;
}

/*
 * Returns:
 *  OK if debounce time is set
 */
int stm32_gpiosetdebounce(uint32_t pinset, uint16_t delay_ms)
{
    uint32_t pin = pinset & GPIO_PIN_MASK;

    stm32_exti_handlers[pin].debounce.ms = delay_ms;
    stm32_exti_handlers[pin].debounce.db_state = DB_ST_INVALID;

    return OK;
}
