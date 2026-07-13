/**
 * uClibc 平台适配层 —— 交叉工具链(构建期)与设备运行时 uClibc 之间的 ABI 兼容。
 *
 * 适用平台:所有 arm uClibc 交叉平台(现 armv7_uclibc、gcc12_uclibc;设备均为
 * uClibc-1.0.46)。已实测坐实二者的 struct stat/stat64 布局逐字节相同、ctype 缺
 * *_loc 访问器同源 —— 属 uClibc ABI 共性,非单板特性,故一套实现共享。
 *
 * ============================ 平台隔离契约（务必遵守） ============================
 * 本文件含 stat/fstat/lstat **强符号** 与 ctype *_loc 符号,只对 uClibc 交叉工具链
 * 正确,在 glibc/musl/Windows/Android 等平台会污染甚至踩坏系统 libc。因此采用三重
 * 对齐的编译期隔离,任一层失守另两层兜底:
 *   1) CMake(tool/CMakeLists.txt)默认从 PRJ_SRCS 排除所有 src/port/**,仅把与
 *      当前 ${PLATFORM} 映射的 port 子目录加回(uClibc 系平台映射到本 uclibc/ 目录),
 *      同时为命中的分组定义 LCU_PORT_GROUP_<GROUP>=1 传给代码层。
 *   2) 本文件从第一行到最后一行整体由 LCU_PLATFORM_UCLIBC_COMPAT 门控(含所有
 *      #include)—— 即便被误扫进编译,也退化为空翻译单元,不产生任何符号。
 *   3) 调用侧 port_anchor.c 对锚点 lcu_uclibc_port_anchor() 的声明与调用同样由
 *      LCU_PORT_GROUP_UCLIBC 门控,与本文件对称,避免"文件被排除→锚点未定义→链接失败"。
 *
 * 门控宏:LCU_PLATFORM_UCLIBC_COMPAT 由 CMake 传递的 LCU_PORT_GROUP_UCLIBC 推导,
 * 而 LCU_PORT_GROUP_UCLIBC 又由 CMakeLists 的 PRJ_PORT_GROUPS(PLATFORM->group 映射)
 * 单一真相源自动定义。以后新增 uClibc 板子(如 hisi_uclibc)只需在 PRJ_PORT_GROUPS
 * 加一行 "hisi_uclibc uclibc", 本文件与 port_anchor.c 均无需改动。新增**非** uClibc
 * 平台适配则另建 src/port/<平台>/ 与独立 port group,复刻本三重模式。
 *
 * 链接可达性:静态库(.a)按需拉取成员,若最终 exe 未引用本 .o 任何符号,链接器会丢弃
 * 它 → stat 强符号覆盖与 ctype 构造函数失效。故提供 lcu_uclibc_port_anchor(),由
 * port_anchor.c 的 lcu_port_anchor() 显式调用,强制本 .o 进入链接闭包。
 */
#ifdef LCU_PORT_GROUP_UCLIBC
#define LCU_PLATFORM_UCLIBC_COMPAT 1
#pragma message("uclibc_port: LCU_PORT_GROUP_UCLIBC enabled -> LCU_PLATFORM_UCLIBC_COMPAT=1")
#endif

#ifdef LCU_PLATFORM_UCLIBC_COMPAT
#pragma message("uclibc_port: compiling stat/ctype ABI adaptation layer for uClibc platforms")

/* stat wrapper 需 struct stat64, 必须在任何系统头前定义 _LARGEFILE64_SOURCE */
#define _LARGEFILE64_SOURCE

#include "mem/mem_debug.h"
#include <stdint.h>

/* ============================ ctype 兼容层 ============================ *
 * 设备 uClibc-1.0.46 运行时缺 __ctype_*_loc 访问器(只有旧式 __ctype_b 全局);
 * 而构建工具链 <ctype.h> 把 isspace/isalpha/... 展开成对 *_loc 的调用。
 * 提供自带 C-locale 静态表的 weak 实现:链到有 *_loc 的 libc 时用 libc 强符号,
 * 本平台缺失时用本实现。仅 C locale / ASCII(0x00-0x7F)语义。
 * 位掩码/元素类型须与本工具链头一致:_ISbit(bit)=(1<<bit)(无 glibc 字节序交换),
 * mask 用 uint16_t,tolower/toupper 用 int16_t。
 * 表下标 [-128,255] 支持 EOF 与 signed char 提升后的负值。
 */

/* uClibc <ctype.h>: _ISbit(bit) = (1 << bit)，无字节序交换（与 glibc 关键区别） */
#define _LCU_ISbit(bit) (1 << (bit))

#define _LCU_ISupper  _LCU_ISbit(0)
#define _LCU_ISlower  _LCU_ISbit(1)
#define _LCU_ISalpha  _LCU_ISbit(2)
#define _LCU_ISdigit  _LCU_ISbit(3)
#define _LCU_ISxdigit _LCU_ISbit(4)
#define _LCU_ISspace  _LCU_ISbit(5)
#define _LCU_ISprint  _LCU_ISbit(6)
#define _LCU_ISgraph  _LCU_ISbit(7)
#define _LCU_ISblank  _LCU_ISbit(8)
#define _LCU_IScntrl  _LCU_ISbit(9)
#define _LCU_ISpunct  _LCU_ISbit(10)
#define _LCU_ISalnum  _LCU_ISbit(11)

#define _LCU_CTYPE_LEN    384  /* 下标范围 [-128, 255] */
#define _LCU_CTYPE_OFFSET 128  /* 表首到下标 0 的偏移 */

static uint16_t _LCU_ctype_b[_LCU_CTYPE_LEN];
static int16_t  _LCU_ctype_tolower[_LCU_CTYPE_LEN];
static int16_t  _LCU_ctype_toupper[_LCU_CTYPE_LEN];
static int      _LCU_ctype_inited = 0;

/* 构造三张表：分类位掩码 + 大小写转换（C locale / ASCII 语义） */
static void _LCU_ctype_init(void)
{
    int i;
    if (_LCU_ctype_inited) {
        return;
    }
    for (i = 0; i < _LCU_CTYPE_LEN; ++i) {
        int c = i - _LCU_CTYPE_OFFSET; /* 实际字符值 [-128, 255] */
        uint16_t flags = 0;

        _LCU_ctype_tolower[i] = c; /* 默认恒等 */
        _LCU_ctype_toupper[i] = c;

        if (c < 0 || c > 0xFF) {
            _LCU_ctype_b[i] = 0;
            continue;
        }

        if (c >= 'A' && c <= 'Z') {
            flags = _LCU_ISupper | _LCU_ISalpha | _LCU_ISalnum | _LCU_ISprint | _LCU_ISgraph;
            if (c <= 'F') {
                flags |= _LCU_ISxdigit;
            }
            _LCU_ctype_tolower[i] = c + ('a' - 'A');
        } else if (c >= 'a' && c <= 'z') {
            flags = _LCU_ISlower | _LCU_ISalpha | _LCU_ISalnum | _LCU_ISprint | _LCU_ISgraph;
            if (c <= 'f') {
                flags |= _LCU_ISxdigit;
            }
            _LCU_ctype_toupper[i] = c - ('a' - 'A');
        } else if (c >= '0' && c <= '9') {
            flags = _LCU_ISdigit | _LCU_ISxdigit | _LCU_ISalnum | _LCU_ISprint | _LCU_ISgraph;
        } else if (c == ' ') {
            flags = _LCU_ISspace | _LCU_ISblank | _LCU_ISprint;
        } else if (c == '\t') {
            flags = _LCU_ISspace | _LCU_ISblank | _LCU_IScntrl;
        } else if (c == '\n' || c == '\v' || c == '\f' || c == '\r') {
            flags = _LCU_ISspace | _LCU_IScntrl;
        } else if (c >= 0x21 && c <= 0x7E) {
            flags = _LCU_ISpunct | _LCU_ISprint | _LCU_ISgraph; /* 其余可打印字符=标点 */
        } else if (c <= 0x1F || c == 0x7F) {
            flags = _LCU_IScntrl; /* 控制字符 */
        }
        /* 0x80-0xFF: 无分类，flags 保持 0 */
        _LCU_ctype_b[i] = flags;
    }
    _LCU_ctype_inited = 1;
}

/* 加载时初始化；构造函数顺序不确定时，各 loc 函数内再做一次惰性兜底 */
__attribute__((constructor))
static void _LCU_ctype_ctor(void)
{
    _LCU_ctype_init();
}

__attribute__((weak))
const unsigned short **__ctype_b_loc(void)
{
    static const unsigned short *ptr;
    _LCU_ctype_init();
    ptr = _LCU_ctype_b + _LCU_CTYPE_OFFSET;
    return &ptr;
}

__attribute__((weak))
const int16_t **__ctype_tolower_loc(void)
{
    static const int16_t *ptr;
    _LCU_ctype_init();
    ptr = _LCU_ctype_tolower + _LCU_CTYPE_OFFSET;
    return &ptr;
}

__attribute__((weak))
const int16_t **__ctype_toupper_loc(void)
{
    static const int16_t *ptr;
    _LCU_ctype_init();
    ptr = _LCU_ctype_toupper + _LCU_CTYPE_OFFSET;
    return &ptr;
}

/* ============================ struct stat 兼容层 ============================ *
 * 设备实测(uClibc-1.0.46, kernel 5.10.117):
 * 构建工具链(uClibc-1.0.28) sizeof(struct stat)=88 / time_t=4B / st_mtime@64;
 * 设备 libc stat() 写 112B, 是 64位 time_t 布局(st_atim/mtim/ctim 各 16B 从 @56 起)。
 * 原实现"memcpy 前88字节"→ st_size(@44,时间字段之前)正确, 但工具链在 @64 读到的
 * 是设备 atim.nsec = 垃圾(实测 mtime 308885983 vs 真实 1783652793), 且设备多写 24B
 * 覆盖调用方栈 → 段错误。
 *
 * 修复(设备实测坐实): 三个 wrapper 统一走 *64 syscall(stat64=195/lstat64=196/
 * fstat64=197) 填入工具链 struct stat64(sizeof=104, 标准 kernel ABI, 32位 time,
 * 两版 uClibc 逐字节吻合、不受设备 time64 影响), 再逐字段搬到调用方的 struct stat。
 * 相比原"裸 memcpy": (1)size/mode/mtime 全部匹配 kernel truth; 
 * (2)fstat 与 stat/lstat 同源同布局; (3)不再依赖"设备写≤128B"的经验假设。
 * 勿用旧号 106/108: 实测只写 64B 老布局(16位 dev/uid), 与 libc 家族不一致。
 */
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#ifndef __NR_stat64
#define __NR_stat64  195  /* ARM32 */
#endif
#ifndef __NR_lstat64
#define __NR_lstat64 196
#endif
#ifndef __NR_fstat64
#define __NR_fstat64 197
#endif

/* 将标准 kernel struct stat64 逐字段搬到工具链 struct stat。
 * 用 struct stat64 而非裸偏移: 偏移由工具链头保证, 已与设备 syscall(195) 实测吻合。
 */
static void _lcu_fill_stat_from64(struct stat *dst, const struct stat64 *s)
{
    memset(dst, 0, sizeof(*dst));
    dst->st_dev     = (dev_t)s->st_dev;
    dst->st_ino     = (ino_t)s->st_ino;        /* 非LFS st_ino 32位, 大 inode 会截断 */
    dst->st_mode    = s->st_mode;
    dst->st_nlink   = s->st_nlink;
    dst->st_uid     = s->st_uid;
    dst->st_gid     = s->st_gid;
    dst->st_rdev    = (dev_t)s->st_rdev;
    dst->st_size    = (off_t)s->st_size;       /* 非LFS off_t 32位, >2GB 会截断 */
    dst->st_blksize = s->st_blksize;
    dst->st_blocks  = (blkcnt_t)s->st_blocks;
    dst->st_atime   = s->st_atim.tv_sec;
    dst->st_mtime   = s->st_mtim.tv_sec;
    dst->st_ctime   = s->st_ctim.tv_sec;
}

int stat(const char *__restrict path, struct stat *__restrict buf)
{
    if (!path || !buf) {
        errno = EFAULT;
        return -1;
    }
    struct stat64 s;
    memset(&s, 0, sizeof(s));
    int ret = (int)syscall(__NR_stat64, path, &s);
    if (ret == 0) {
        _lcu_fill_stat_from64(buf, &s);
    }
    return ret;
}

int lstat(const char *__restrict path, struct stat *__restrict buf)
{
    if (!path || !buf) {
        errno = EFAULT;
        return -1;
    }
    struct stat64 s;
    memset(&s, 0, sizeof(s));
    int ret = (int)syscall(__NR_lstat64, path, &s);
    if (ret == 0) {
        _lcu_fill_stat_from64(buf, &s);
    }
    return ret;
}

int fstat(int fd, struct stat *buf)
{
    if (fd < 0 || !buf) {
        errno = EBADF;
        return -1;
    }
    struct stat64 s;
    memset(&s, 0, sizeof(s));
    int ret = (int)syscall(__NR_fstat64, fd, &s);
    if (ret == 0) {
        _lcu_fill_stat_from64(buf, &s);
    }
    return ret;
}

/* 链接锚点: 由 port_anchor.c 的 lcu_port_anchor() 显式调用, 强制本 .o 进入最终
 * 链接闭包, 确保上述强符号覆盖与 ctype 构造函数生效(静态库按需拉取成员, 无引用则
 * 可能整个 .o 被丢弃)。
 *
 * 注意: 调用侧 port_anchor.c 对本函数的声明/调用同样由 LCU_PORT_GROUP_UCLIBC 门控,
 * 故本平台外不存在对它的引用, 无需在 #else 里提供空实现。
 */
void lcu_uclibc_port_anchor(void)
{
    /* 空实现即可, 存在被引用这一事实本身就是目的 */
}

#endif /* LCU_PLATFORM_UCLIBC_COMPAT */
