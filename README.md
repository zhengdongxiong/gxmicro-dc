# GXMicro Display Controller driver
GXMicro Display Controller driver

# 内核版本
V5.4.18

# 目录
```shell
drivers/gpu/drm/gxmicro
```

# 文件说明
| 文件目录 | 说明 |
| :---: | :---: |
| gxmicro_drv.c | pcie 和 drm 相关初始化 |
| gxmicro_i2c.c | gpio 模拟 i2c |
| gxmicro_ttm.c | drm 中内存管理 vram 注册 |
| gxmicro_kms.c | drm 中各部分的初始化和使用, 设置 Display Controller 等 |
| gxmicro_dc.h |  dc 寄存器 |
| 10-gxmicro.conf | xorg 配置文件 |

# TODO
1. stride 更改分辨率可能需要重新计算, 不能通过 fb->pitches[0] 直接获取, 计算: hdisplay * bpc(fb-format->cpp[0])
