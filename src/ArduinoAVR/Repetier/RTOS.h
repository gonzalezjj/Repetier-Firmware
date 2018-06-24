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

#ifndef RTOS_H_INCLUDED
#define RTOS_H_INCLUDED

#include "Repetier.h"

#if RTOS_ENABLE

#if RTOS_ENABLE == 1
#include "src/FreeRTOS/FreeRTOS.h"
#include "src/FreeRTOS/task.h"
#include "src/FreeRTOS/semphr.h"

#ifndef RTOS_MAIN_THREAD_SLEEP_MS
#define RTOS_MAIN_THREAD_SLEEP_MS 50
#endif

#ifndef RTOS_UI_THREAD_SLEEP_MS
#define RTOS_UI_THREAD_SLEEP_MS 500
#endif

struct RTOS_Mutex_t {
    SemaphoreHandle_t handle;
    StaticSemaphore_t data;
};
#define RTOS_Mutex_Init(_mutex_) (_mutex_).handle = xSemaphoreCreateRecursiveMutexStatic(&((_mutex_).data))
#define RTOS_Mutex_Take(_mutex_) xSemaphoreTakeRecursive((_mutex_).handle, portMAX_DELAY)
#define RTOS_Mutex_Release(_mutex_) xSemaphoreGiveRecursive((_mutex_).handle)

class RTOS_MutexBlock {
public:
    RTOS_MutexBlock(RTOS_Mutex_t& mutex) : m_mutex(mutex) {
        RTOS_Mutex_Take(m_mutex);
    }

    ~RTOS_MutexBlock() {
        RTOS_Mutex_Release(m_mutex);
    }

private:
    RTOS_Mutex_t& m_mutex;
};

class RTOS {
    
public:
    enum Notification {
        NOTIFY_PRINTLINES_FREED = 1ul<<0,
        NOTIFY_PERIODIC_ACTION_REQUIRED = 1ul<<3,
        NOTIFY_GCODE_BUFFER_FREED = 1ul<<5,
        NOTIFY_SCREEN_UPDATE_NEEDED = 1ul<<6,
    };

	/**
	 * The idle stack is also used by the interrupts, including the motion controller, so it might need some space.
	 */
    static const int IDLE_STACK_SIZE = configIDLE_STACK_SIZE;
    static const int MAIN_STACK_SIZE = 700;
    static const int UI_STACK_SIZE = 800;
    
    static void setupTasks();
    
    static inline const TaskHandle_t getMainTaskHandle() {
        return mainTaskHandle;
    }
    static inline const TaskHandle_t getUiTaskHandle() {
        return uiTaskHandle;
    }
    
    static inline void wait(millis_t millis = portMAX_DELAY, uint32_t *notificationValue = NULL) {
        if (!tasksInitialized) return;
    	if (millis == portMAX_DELAY) {
    		xTaskNotifyWait(0, 0xfffffffful, notificationValue, portMAX_DELAY);
    	} else {
			xTaskNotifyWait(0, 0xfffffffful, notificationValue, millis / portTICK_PERIOD_MS);
    	}
    }
    
    static inline void notify(Notification notification) {
        if (!tasksInitialized) return;
        switch (notification) {
        case NOTIFY_PERIODIC_ACTION_REQUIRED:
        case NOTIFY_PRINTLINES_FREED:
        case NOTIFY_GCODE_BUFFER_FREED:
            notifyWithContext(mainTaskHandle, notification);
            break;
        case NOTIFY_SCREEN_UPDATE_NEEDED:
            notifyWithContext(uiTaskHandle, notification);
            break;
        }
    }

    static inline uint8_t getTaskNumberForDebug(TaskHandle_t task) {
        if (!tasksInitialized) return 0;

        if (task == mainTaskHandle) return 1;
        else if (task == uiTaskHandle) return 2;
        return 0;
    }

private:
    static bool tasksInitialized;

    static TaskHandle_t mainTaskHandle;
    static StaticTask_t mainTaskStruct;
    static StackType_t mainStack[MAIN_STACK_SIZE];
    
    static TaskHandle_t uiTaskHandle;
    static StaticTask_t uiTaskStruct;
    static StackType_t uiStack[UI_STACK_SIZE];
    
    static void mainTaskEntryPoint(void *pvParameters);
    static void uiTaskEntryPoint(void *pvParameters);

    static inline void notifyWithContext(TaskHandle_t task, Notification notificationValue) {
        if (portIS_IN_ISR) {
            BaseType_t xHigherPriorityTaskWoken;
            xTaskNotifyFromISR(task, notificationValue, eSetBits, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        } else {
            xTaskNotify(task, notificationValue, eSetBits);
        }
    }
   
};

#endif // RTOS_ENABLE == 1

#else // RTOS_ENABLE
#define RTOS_TASK_MAIN NULL
#define RTOS_TASK_UI NULL

#define RTOS_NOTIFY(_task_, _notification_) {}
#define RTOS_NOTIFY_FROM_ISR(_task_, _notification_) {}
#endif // RTOS_ENABLE

#endif // RTOS_H_INCLUDED
