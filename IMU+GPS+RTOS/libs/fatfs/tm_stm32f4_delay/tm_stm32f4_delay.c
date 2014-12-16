/**	
 * |----------------------------------------------------------------------
 * | Copyright (C) Tilen Majerle, 2014
 * | 
 * | This program is free software: you can redistribute it and/or modify
 * | it under the terms of the GNU General Public License as published by
 * | the Free Software Foundation, either version 3 of the License, or
 * | any later version.
 * |  
 * | This program is distributed in the hope that it will be useful,
 * | but WITHOUT ANY WARRANTY; without even the implied warranty of
 * | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * | GNU General Public License for more details.
 * | 
 * | You should have received a copy of the GNU General Public License
 * | along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * |----------------------------------------------------------------------
 */
#include "tm_stm32f4_delay.h"

static __IO uint32_t TM_Time1 = 0;
static __IO uint32_t TM_lastTime1 = 0;
static __IO uint32_t TM_Time2 = 0;
static __IO uint32_t TM_lastTime2 = 0;

void Delay(__IO uint32_t nTime) {
    uint32_t timerValue;
    uint32_t lastTime = TIM2->CNT;
    while(1)
    {
    	timerValue = TIM2->CNT;
    	if( timerValue - lastTime > nTime )
    	{
    		break;
    	}
    }
    return;
}

void Delayms(__IO uint32_t nTime) {
	uint32_t timerValue;
	uint32_t lastTime = TIM2->CNT;
	nTime = nTime*1000;
	while(1)
	{
		timerValue = TIM2->CNT;
	    if( timerValue - lastTime > nTime )
	    {
	    	break;
	    }
	}
	return;
}

uint32_t TM_DELAY_Time(void) {
	uint32_t timerValue = TIM2->CNT;
	if( timerValue - TM_lastTime1 > TM_Time1 )
		return 0;
	else
		return 1;
}

uint32_t TM_DELAY_Time2(void) {
	uint32_t timerValue = TIM2->CNT;
	if( timerValue - TM_lastTime2 > TM_Time2 )
		return 0;
	else
		return 1;
}

void TM_DELAY_SetTime(uint32_t time) {
	TM_Time1 = time;
	TM_lastTime1 = TIM2->CNT;
}

void TM_DELAY_SetTime2(uint32_t time) {
	TM_Time2 = time;
	TM_lastTime2 = TIM2->CNT;
}

