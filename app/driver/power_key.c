/*
 * flash_config.c
 *
 *  Created on: 2018年9月22日
 *      Author: G
 */
#include "driver/power_key.h"
#include "osapi.h"
LOCAL void ICACHE_FLASH_ATTR setTurnOnOffFlag(bool isReset) {

    if (!isReset) {
        //先读出来再加一
        u8 saveNumber[4];
        spi_flash_read(550 * 4096 + 20, (uint32 *) &saveNumber, 4);
        if (saveNumber[0] > 8 || saveNumber[0] < 0) {
            saveNumber[0] = 1;
        } else {
            saveNumber[0]++;
        }
        //先擦除再保存
        spi_flash_erase_sector(550);
        spi_flash_write(550 * 4096 + 20, (uint32 *) &saveNumber, 4);
    } else {
        u8 saveNumber[4];
        saveNumber[0] = 0;
        //先擦除再保存
        spi_flash_erase_sector(550);
        spi_flash_write(550 * 4096 + 20, (uint32 *) &saveNumber, 4);
    }
}


LOCAL u8 ICACHE_FLASH_ATTR getTurnOnOffFlag() {
    u8 tempSaveData[4];
    spi_flash_read(550 * 4096 + 20, (uint32 *) &tempSaveData, 4);
    //如果读取失败
    if (tempSaveData[0] == -1) {
        tempSaveData[0] = 1;
        spi_flash_erase_sector(550);
        spi_flash_write(550 * 4096 + 20, (uint32 *) &tempSaveData, 4);
    }
    return tempSaveData[0];
}

void hw_test_timer_cb(cinfiflback cb) {

    static u8 statusFlag = 0;
    statusFlag++;
    if (statusFlag == 1) {
        u8 flag = getTurnOnOffFlag();
        os_printf("current save flag : %d \n", flag);
        if (flag >= 3) {
            //保存为0
            setTurnOnOffFlag(true);
            //进去一键配网模式
            cb();
            //关闭定时器
            os_timer_disarm(&os_timer);
        }
    } else if (statusFlag == 3) {
        setTurnOnOffFlag(true);
        //关闭定时器
        os_timer_disarm(&os_timer);
    }

}
void power_key_init(cinfiflback cb1){

    setTurnOnOffFlag(false);

    /** 关闭该定时器 */
    os_timer_disarm(&os_timer);
    /** 配置该定时器回调函数 */
    os_timer_setfn(&os_timer, (ETSTimerFunc *) (hw_test_timer_cb), cb1);
    /** 启动该定时器 */
    os_timer_arm(&os_timer, 1000, true);
}
