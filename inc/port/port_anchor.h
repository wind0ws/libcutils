#pragma once
#ifndef _LCU_PORT_ANCHOR_H
#define _LCU_PORT_ANCHOR_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 平台适配层统一锚点
 *
 * 由 lcu_global_init() 无条件调用, 确保所有平台特定的适配代码(如 uClibc 的
 * stat/ctype 强符号覆盖)能被链接器拉入最终可执行文件。
 *
 * 内部根据 CMake 传递的 LCU_PORT_GROUP_* 宏自动调度到对应平台的具体 anchor 实现。
 * 若当前平台无需任何适配, 则为空操作。调用侧(lcu.c)不需要知道任何平台宏。
 *
 * @note 本函数本身无副作用, 存在的目的是链接期依赖注入。
 */
void lcu_port_anchor(void);

#ifdef __cplusplus
}
#endif

#endif /* _LCU_PORT_ANCHOR_H */
