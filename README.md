# GXMicro Display Controller

显示驱动

# 文件目录说明

| 文件目录 | 说明 |
| :---: | :---: |
| gxmicro_dc_drv.c | 驱动 probe 入口, pcie 设备相关初始化和内存映射 |
| gxmicro_i2c.c | gpio 模拟 i2c |
| gxmicro_drm.c | drm 相关初始化, 注册 drm_device 设备等 |
| gxmicro_ttm.c | drm 中内存管理 vram 注册 |
| gxmicro_kms.c | drm 中各部分的初始化和使用, 设置 Display Controller, Sil9134 等 |
| gxmicro_dc.h |  读写函数与设备结构体 |

# TODO
1. 显示高分辨率出现换撕裂, 因此采用默认分辨率 640 * 480, 后续排查
2. 第一次烧完 bit 后, 通过 I2C 操作 Sil9134 异常, 后续接口使用 VGA 再排查
3. 猜测 xorg 检测到非标准的分辨率会选择较安全的分辨率, 使用高分辨率可能需要在配置文件中加入对应的分辨率(Modeline)
4. VGA 显示
