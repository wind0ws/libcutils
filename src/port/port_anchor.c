#include "mem/mem_debug.h"
#include "port/port_anchor.h"

/**
 * 平台适配层调度中心
 *
 * 职责: 根据 CMake 传递的 LCU_PORT_GROUP_* 宏, 判断当前需要哪些平台的 port anchor, 并调用之。
 * 业务层(lcu.c)只调用 lcu_port_anchor(), 不需要感知任何平台宏。
 *
 * ============================ 单一真相源 ============================
 * LCU_PORT_GROUP_<GROUP> 宏由 tool/CMakeLists.txt 依据 PRJ_PORT_GROUPS 映射
 * (PLATFORM -> port 子目录名)自动定义: 当前 PLATFORM 命中某个 group 时, 该 group
 * 的源码目录被加入编译, 同时定义 LCU_PORT_GROUP_<GROUP大写>=1。
 * 故平台映射只维护 CMakeLists 一处, 代码层据此自动生效。
 *
 * 新增平台适配:
 *   1) 若新平台与现有分组同源(如新增 hisi_uclibc 也走 uClibc 兼容层):
 *      仅在 CMakeLists 的 PRJ_PORT_GROUPS 添加 "hisi_uclibc uclibc" 一行,
 *      本文件与 uclibc_port.c 均无需改动(自动命中 LCU_PORT_GROUP_UCLIBC)。
 *   2) 若是全新平台类型(如 windows):
 *      - CMakeLists PRJ_PORT_GROUPS 添加 "windows windows";
 *      - 在 src/port/windows/ 创建实现文件并提供 lcu_windows_port_anchor();
 *      - 在本文件仿照下方 uClibc 块, 增加 #ifdef LCU_PORT_GROUP_WINDOWS 块。
 */


/* ==================== 统一调度入口 ==================== */
void lcu_port_anchor(void)
{
/* ==================== uClibc 平台适配 ==================== */
#ifdef LCU_PORT_GROUP_UCLIBC
  #pragma message("port_anchor: LCU_PORT_GROUP_UCLIBC enabled -> will call lcu_uclibc_port_anchor()")
  /* 前向声明; 实现在 src/port/uclibc/uclibc_port.c */
  void lcu_uclibc_port_anchor(void);
  
  /* port_anchor 调用, 确保 port .o 被依赖. */
  lcu_uclibc_port_anchor();
#endif

/* ==================== 未来其他平台 (示例) ==================== */
/*
#ifdef LCU_PORT_GROUP_WINDOWS
  #pragma message("port_anchor: LCU_PORT_GROUP_WINDOWS enabled -> will call lcu_windows_port_anchor()")
  void lcu_windows_port_anchor(void);
  lcu_windows_port_anchor();
#endif
*/

    /* 未来其他平台在此按 LCU_PORT_GROUP_* 追加调用, 参考上面示例 */
}
