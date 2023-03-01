# GXMicro Display Controller
drm 框架显示驱动

# 内核版本

V5.4.18

# 驱动位置
需要文件放入以下目录
```shell
drivers/gpu/drm/<目录>
```

# 文件目录说明
| 文件目录 | 说明 |
| :---: | :---: |
| gxmicro_drv.c | 驱动 probe 入口, pcie 和 drm 相关初始化 |
| gxmicro_i2c.c | gpio 模拟 i2c |
| gxmicro_ttm.c | drm 中内存管理 vram 注册 |
| gxmicro_kms.c | drm 中各部分的初始化和使用, 设置 Display Controller, Sil9134 等 |
| gxmicro_dc.h |  读写函数与设备结构体 |

# TODO
1. 使用 vga 接口, sil9134 去除
2. stride 更改分辨率可能需要重新计算, 不能通过 fb->pitches[0] 直接获取, 计算: hdisplay * bpc(fb-format->cpp[0])
3. cursor location 可能需要改变, (x + hotx, y + hoty), 当前 ip location为 (x - hotx, y - hoty)
4. hsync, vsync polarity
5. vga 切换分辨率问题
