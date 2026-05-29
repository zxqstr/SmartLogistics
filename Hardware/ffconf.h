#ifndef _FFCONF_H
#define _FFCONF_H

/* FATFS R0.14 配置 */

#define FF_FS_READONLY  0    /* 可读写 */
#define FF_FS_MINIMIZE  0    /* 完整功能 */
#define FF_USE_STRFUNC  1    /* 字符串函数 */
#define FF_USE_FIND      0
#define FF_USE_MKFS      1    /* 允许格式化 */
#define FF_USE_FASTSEEK  0
#define FF_USE_EXPAND    0
#define FF_USE_CHMOD     0
#define FF_USE_LABEL     0
#define FF_USE_FORWARD   0
#define FF_CODE_PAGE     437  /* US-English */
#define FF_USE_LFN       0    /* 无长文件名(省RAM) */
#define FF_VOLUMES       1
#define FF_STR_VOLUME_ID 0
#define FF_MULTI_PARTITION 0
#define FF_MIN_SS        512
#define FF_MAX_SS        512
#define FF_LBA64         0
#define FF_MIN_GPT       0x100
#define FF_USE_TRIM      0
#define FF_FS_NORTC      1    /* 无RTC */
#define FF_FS_READONLY   0
#define FF_FS_NORTC      1
#define FF_NORTC_MON     1
#define FF_NORTC_MDAY    1
#define FF_NORTC_YEAR    2026

#endif
