/*
    This file is part of Repetier-Firmware.

    Repetier-Firmware is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Repetier-Firmware is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Repetier-Firmware.  If not, see <http://www.gnu.org/licenses/>.

    This firmware is a nearly complete rewrite of the sprinter firmware
    by kliment (https://github.com/kliment/Sprinter)
    which based on Tonokip RepRap firmware rewrite based off of Hydra-mmm firmware.
#
  Functions in this file are used to control the Real Time Operating System running Repetier
*/

#include "Repetier.h"
#include "RTOS.h"

#if RTOS_ENABLE

bool RTOS::tasksInitialized = false;

TaskHandle_t RTOS::mainTaskHandle = 0;
StaticTask_t RTOS::mainTaskStruct;
StackType_t RTOS::mainStack[RTOS::MAIN_STACK_SIZE];

TaskHandle_t RTOS::uiTaskHandle = 0;
StaticTask_t RTOS::uiTaskStruct;
StackType_t RTOS::uiStack[RTOS::UI_STACK_SIZE];

void RTOS::setupTasks() {
    mainTaskHandle = xTaskCreateStatic(mainTaskEntryPoint, "main", RTOS::MAIN_STACK_SIZE, NULL, 3, mainStack, &mainTaskStruct);
    uiTaskHandle = xTaskCreateStatic(uiTaskEntryPoint, "ui", RTOS::UI_STACK_SIZE, NULL, 1, uiStack, &uiTaskStruct);
    tasksInitialized = true;
}

void RTOS::mainTaskEntryPoint(void *pvParameters) {
    while (1) {
        Commands::commandLoop();
    }
}

void RTOS::uiTaskEntryPoint(void *pvParameters) {
    while (1) {
        uid.UILoop();
    }
}

extern "C" {
    static StaticTask_t idleTaskStruct;
    static StackType_t idleTaskStack[RTOS::IDLE_STACK_SIZE];

    void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize) {
        *ppxIdleTaskTCBBuffer = &idleTaskStruct;
        *ppxIdleTaskStackBuffer = idleTaskStack;
        *pulIdleTaskStackSize = RTOS::IDLE_STACK_SIZE;
    }

    void __attribute__ ((noinline, used)) vApplicationStackOverflowHook(TaskHandle_t xTask, portCHAR *pcTaskName) {
        InterruptProtectedBlock ipb;
        Printer::kill(0);

        uint8_t taskId = xTask != NULL ? RTOS::getTaskNumberForDebug(xTask) : 10;
        while (1) {
            for (uint8_t i = 0; i < taskId; i++) {
                WRITE(LED_PIN, 1);
                _delay_ms(200);
                WRITE(LED_PIN, 0);
                _delay_ms(200);
            }
            _delay_ms(2000);
        }
    }

    void __attribute__ ((noinline, used)) vApplicationMallocFailedHook( void ) {
        InterruptProtectedBlock ipb;
        Printer::kill(0);

        while (1) {
            WRITE(LED_PIN, 1);
            _delay_ms(2000);
            WRITE(LED_PIN, 0);
            _delay_ms(2000);
        }
    }
}

#endif
