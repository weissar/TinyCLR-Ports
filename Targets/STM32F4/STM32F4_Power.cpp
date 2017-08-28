// Copyright Microsoft Corporation
// Copyright Oberon microsystems, Inc
// Copyright GHI Electronics, LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "STM32F4.h"

static void(*g_STM32F4_stopHandler)();
static void(*g_STM32F4_restartHandler)();

static TinyCLR_Power_Provider powerProvider;
static TinyCLR_Api_Info powerApi;

const TinyCLR_Api_Info* STM32F4_Power_GetApi() {
    powerProvider.Parent = &powerApi;
    powerProvider.Index = 0;
    powerProvider.Acquire = &STM32F4_Power_Acquire;
    powerProvider.Release = &STM32F4_Power_Release;
    powerProvider.Reset = &STM32F4_Power_Reset;
    powerProvider.Sleep = &STM32F4_Power_Sleep;

    powerApi.Author = "GHI Electronics, LLC";
    powerApi.Name = "GHIElectronics.TinyCLR.NativeApis.STM32F4.PowerProvider";
    powerApi.Type = TinyCLR_Api_Type::PowerProvider;
    powerApi.Version = 0;
    powerApi.Count = 1;
    powerApi.Implementation = &powerProvider;

    return &powerApi;
}

void STM32F4_Power_SetHandlers(void(*stop)(), void(*restart)()) {
    g_STM32F4_stopHandler = stop;
    g_STM32F4_restartHandler = restart;
}

void STM32F4_Power_Sleep(const TinyCLR_Power_Provider* self, TinyCLR_Power_Sleep_Level level) {
    switch (level) {

        case TinyCLR_Power_Sleep_Level::Hibernate: // stop
        // stop peripherals if needed
            if (g_STM32F4_stopHandler != 0)
                g_STM32F4_stopHandler();

            SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
            PWR->CR |= PWR_CR_CWUF | PWR_CR_FPDS | PWR_CR_LPDS; // low power deepsleep

            __WFI(); // stop clocks and wait for external interrupt
#if SYSTEM_CRYSTAL_CLOCK_HZ != 0
            RCC->CR |= RCC_CR_HSEON;             // HSE on
#endif
            SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;  // reset deepsleep

            while (!(RCC->CR & RCC_CR_HSERDY));

            RCC->CR |= RCC_CR_PLLON;             // pll on

            while (!(RCC->CR & RCC_CR_PLLRDY));

            RCC->CFGR |= RCC_CFGR_SW_PLL;        // sysclk = pll out
#if SYSTEM_CRYSTAL_CLOCK_HZ != 0
            RCC->CR &= ~RCC_CR_HSION;            // HSI off
#endif

// restart peripherals if needed
            if (g_STM32F4_restartHandler != 0)
                g_STM32F4_restartHandler();
            return;

        case TinyCLR_Power_Sleep_Level::Off: // standby
            // stop peripherals if needed
            if (g_STM32F4_stopHandler != 0)
                g_STM32F4_stopHandler();

            SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
            PWR->CR |= PWR_CR_CWUF | PWR_CR_PDDS; // power down deepsleep

            __WFI(); // soft power off, never returns
            return;

        default: // sleep
            PWR->CR |= PWR_CR_CWUF;

            __WFI(); // sleep and wait for interrupt
            return;
    }
}

void STM32F4_Power_Reset(const TinyCLR_Power_Provider* self, bool runCoreAfter) {
#if defined RAM_BOOTLOADER_HOLD_VALUE && defined RAM_BOOTLOADER_HOLD_ADDRESS && RAM_BOOTLOADER_HOLD_ADDRESS > 0
    if (!runCoreAfter)
        *((uint32_t*)RAM_BOOTLOADER_HOLD_ADDRESS) = RAM_BOOTLOADER_HOLD_VALUE;
#endif

    SCB->AIRCR = (0x5FA << SCB_AIRCR_VECTKEY_Pos)  // unlock key
        | (1 << SCB_AIRCR_SYSRESETREQ_Pos); // reset request

    while (1); // wait for reset
}

TinyCLR_Result STM32F4_Power_Acquire(const TinyCLR_Power_Provider* self) {
    return TinyCLR_Result::Success;
}

TinyCLR_Result STM32F4_Power_Release(const TinyCLR_Power_Provider* self) {
    return TinyCLR_Result::Success;
}
