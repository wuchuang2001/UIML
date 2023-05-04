# 裁判系统模块

---

## 简介

这是整个代码库的裁判系统模块，根据串口接收的数据解析出相应的数据将其广播出去，同时根据其他模块请求绘制的ui信息发送给裁判系统完成ui绘制


## 项目文件及依赖项

- 本项目文件
	- `softbus.c/h`、`config.c/h`、`sys_conf.h`、`judge_drive.c/h`、`my_queue.c/h`
- 标准库文件
	- `stdint.h`、`stdbool.h`
- hal库文件 
    - `cmsis_os.h`

---

> 注：下面远程函数所写的数据类型为指针的项仅强调该项传递的应该是数组，实际传递的参数只需数组名即可，不需要传递数组名的地址。广播也是如此，所写的数据类型若为指针的仅强调该项传递的应该是数组，获取该项的值是仅需要强制类型转换成相应的指针即可，无需额外解引用

---

## 说明

本裁判系统模块会根据从裁判系统获取的部分数据，选择性的广播该类型所需要的数据

## 在`sys_conf.h`中的配置

```c
{"judge",CF_DICT{
    {"max-tx-queue-length",IM_PTR(uint16_t,20)},
    {"task-interval",IM_PTR(uint16_t,150)},
    {"uart-x",IM_PTR(uint8_t,6)},
    CF_DICT_END
}},
```
## 模块接口

- 广播
    
    英雄机器人

    

    1. `/judge/recv/robot-state`

        说明：解析裁判系统数据得到的机器人状态数据
        
        | 数据字段名 | 数据类型 | 说明 |
        | :---: | :---: | :---: |
        | `color` | `uint8_t` |机器人颜色（0：红 1：蓝） |
        | `42mm-speed-limit` | `uint16_t` |42mm弹速限制 |
        | `chassis-power-limit` | `uint16_t`|  底盘功率限制 |
        | `is-shooter-power-output` | `bool` | 电源管理发射机构端是否输出  |
        | `is-chassis-power-output` | `bool` | 电源管理底盘端是否输出|

    2. `/judge/recv/power-Heat`

        说明：解析裁判系统数据得到的功率/热量数据

        | 数据字段名 | 数据类型 | 说明 |
        | :---: | :---: |  :---: |
        | `chassis-power` | `float` |底盘功率|
        | `chassis-power_buffer` | `uint6_t` |底盘功率缓冲|
        | `42mm-cooling-heat` | `uint16_t` | 42mm热量 |

    3. `/judge/recv/shoot`

        说明：解析裁判系统数据得到的发射数据

        | 数据字段名 | 数据类型 | 说明 |
        | :---: | :---: |  :---: |
        | `bullet-speed` | `float` | 发射弹丸弹速|

    4. `/judge/recv/bullet`

        说明：解析裁判系统数据得到的弹丸数据

        | 数据字段名 | 数据类型  | 说明 |
        | :---: | :---: | :---: |
        | `42mm-bullet-remain` | `uint16_t` |42mm剩余发弹量 |

    非英雄机器人

    5. `/judge/recv/robot-state`

         说明：解析裁判系统数据得到的机器人状态数据
    
        | 数据字段名 | 数据类型 | 说明 |
        | :---: | :---: | :---: |
        | `color` | `uint8_t` |机器人颜色（0：红 1：蓝） |
        | `17mm-speed-limit` | `uint16_t` |17mm弹速限制 |
        | `chassis-power-limit` | `uint16_t`|  底盘功率限制 |
        | `is-shooter-power-output` | `bool` | 电源管理发射机构端是否输出  |
        | `is-chassis-power-output` | `bool` | 电源管理底盘端是否输出|
        | `is-gimbal-power-output` | `bool` | 电源管理云台端是否输出  |

    6. `/judge/recv/power-Heat`

        说明：解析裁判系统数据得到的功率/热量数据

        | 数据字段名 | 数据类型 | 说明 |
        | :---: | :---: |  :---: |
        | `chassis-power` | `float` |底盘功率|
        | `chassis-power_buffer` | `uint6_t` |底盘功率缓冲|
        | `17mm-cooling-heat` | `uint16_t` | 17mm热量 |

    7. `/judge/recv/shoot`

        说明：解析裁判系统数据得到的发射数据

        | 数据字段名 | 数据类型 | 说明 |
        | :---: | :---: |  :---: |
        | `bullet-speed` | `float` | 发射弹丸弹速|

    8. `/judge/recv/bullet`

        说明：解析裁判系统数据得到的弹丸数据

        | 数据字段名 | 数据类型  | 说明 |
        | :---: | :---: | :---: |
        | `17mm-bullet-remain` | `uint16_t` |17mm剩余发弹量 |

- 远程函数
    >颜色说明：	0：己方主色 1：黄色 2：绿色 3：橙色 4：紫色 5：粉色 6：灰色 7：黑色 8：白色 

    >图形操作说明：	0：空操作 1：添加 2：更改 3：删除    
    绘制UI时必须先进行添加操作，才能更改和删除

    1. `/judge/send/ui/text`

        说明：发送文本UI数据

        传入参数数据：

        | 数据字段名 | 数据类型 | 是否为返回值 | 是否必须传输 | 说明 |
        | :---: | :---: | :---: | :---: | :---: |
        | `name` | `uint8_t` | × | 可选 |在操作中，作为客户端的唯一索引，有且只有3个字符 |
        | `text` | `uint8_t` | × | 可选 | 要发送的文本数据 |
        | `color` | `uint8_t` | × | 可选 | 颜色（详见颜色说明）  |
        | `width` | `uint8_t` | × | 可选 | 笔画宽度  |
        | `layer` | `uint8_t` | × | 可选 | 图层 |
        | `start-x` | `int16_t` | × | 可选 | 开始x坐标 |
        | `start-y` | `int16_t` | × | 可选 | 开始y坐标 |
        | `size` | `uint16_t` | × | 可选 | 文本大小  |
        | `len` | `uint16_t` | × | 可选 | 发送的文本长度  |
        | `lopera` | `uint8_t` | × | 可选 | 操作（详见操作说明）  |
    
    2. `/judge/send/ui/line`

        说明：发送直线UI数据

        传入参数数据：

        | 数据字段名 | 数据类型 | 是否为返回值 | 是否必须传输 | 说明 |
        | :---: | :---: | :---: | :---: | :---: |
        | `name` | `uint8_t` | × | 可选 |在操作中，作为客户端的唯一索引，有且只有3个字符 |
        | `color` | `uint8_t` | × | 可选 | 颜色（详见颜色说明）  |
        | `width` | `uint8_t` | × | 可选 | 宽度  |
        | `layer` | `uint8_t` | × | 可选 | 图层 |
        | `start-x` | `int16_t` | × | 可选 | 开始x坐标 |
        | `start-y` | `int16_t` | × | 可选 | 开始y坐标 |
        | `end-x` | `int16_t` | × | 可选 | 结束x坐标 |
        | `end-y` | `int16_t` | × | 可选 | 结束y坐标 |
        | `lopera` | `uint8_t` | × | 可选 | 操作（详见操作说明）  |
    
    3. `/judge/send/ui/rect`

        说明：发送矩形UI数据

        传入参数数据：

        | 数据字段名 | 数据类型 | 是否为返回值 | 是否必须传输 | 说明 |
        | :---: | :---: | :---: | :---: | :---: |
        | `name` | `uint8_t` | × | 可选 |在操作中，作为客户端的唯一索引，有且只有3个字符 |
        | `color` | `uint8_t` | × | 可选 | 颜色（详见颜色说明）  |
        | `width` | `uint8_t` | × | 可选 | 宽度  |
        | `layer` | `uint8_t` | × | 可选 | 图层 |
        | `start-x` | `int16_t` | × | 可选 | 开始x坐标 |
        | `start-y` | `int16_t` | × | 可选 | 开始y坐标 |
        | `end-x` | `int16_t` | × | 可选 | 对角x坐标 |
        | `end-y` | `int16_t` | × | 可选 | 对角y坐标 |
        | `lopera` | `uint8_t` | × | 可选 | 操作（详见操作说明）  |

    4. `/judge/send/ui/circle`

        说明：发送圆形UI数据

        传入参数数据：


        | 数据字段名 | 数据类型 | 是否为返回值 | 是否必须传输 | 说明 |
        | :---: | :---: | :---: | :---: | :---: |
        | `name` | `uint8_t` | × | 可选 |在操作中，作为客户端的唯一索引，有且只有3个字符 |
        | `color` | `uint8_t` | × | 可选 | 颜色（详见颜色说明）  |
        | `width` | `uint8_t` | × | 可选 | 宽度  |
        | `layer` | `uint8_t` | × | 可选 | 图层 |
        | `cent-x` | `int16_t` | × | 可选 | 中心x坐标 |
        | `cent-y` | `int16_t` | × | 可选 | 中心y坐标 |
        | `radius` | `uint16_t` | × | 可选 | 半径 |
        | `lopera` | `uint8_t` | × | 可选 | 操作（详见操作说明）  |

    5. `/judge/send/ui/oval`

        说明：发送椭圆UI数据

        传入参数数据：


        | 数据字段名 | 数据类型 | 是否为返回值 | 是否必须传输 | 说明 |
        | :---: | :---: | :---: | :---: | :---: |
        | `name` | `uint8_t` | × | 可选 |在操作中，作为客户端的唯一索引，有且只有3个字符 |
        | `color` | `uint8_t` | × | 可选 | 颜色（详见颜色说明）  |
        | `width` | `uint8_t` | × | 可选 | 宽度  |
        | `layer` | `uint8_t` | × | 可选 | 图层 |
        | `cent-x` | `int16_t` | × | 可选 | 中心x坐标 |
        | `cent-y` | `int16_t` | × | 可选 | 中心y坐标 |
        | `semiaxis-x` | `int16_t` | × | 可选 | x半轴长 |
        | `semiaxis-y` | `int16_t` | × | 可选 | y半轴长|
        | `lopera` | `uint8_t` | × | 可选 | 操作（详见操作说明）  |  

    6. `/judge/send/ui/arc`

        说明：发送圆弧UI数据

        传入参数数据：


        | 数据字段名 | 数据类型 | 是否为返回值 | 是否必须传输 | 说明 |
        | :---: | :---: | :---: | :---: | :---: |
        | `name` | `uint8_t` | × | 可选 |在操作中，作为客户端的唯一索引，有且只有3个字符 |
        | `color` | `uint8_t` | × | 可选 | 颜色（详见颜色说明）  |
        | `width` | `uint8_t` | × | 可选 | 宽度  |
        | `layer` | `uint8_t` | × | 可选 | 图层 |
        | `cent-x` | `int16_t` | × | 可选 | 中心x坐标 |
        | `cent-y` | `int16_t` | × | 可选 | 中心y坐标 |
        | `semiaxis-x` | `int16_t` | × | 可选 | x半轴长 |
        | `semiaxis-y` | `int16_t` | × | 可选 | y半轴长|
        | `start-angle` | `int16_t` | × | 可选 |起始角度|
        | `end-angle` | `int16_t` | × | 可选 | 终止角度|
        | `lopera` | `uint8_t` | × | 可选 | 操作（详见操作说明）  |

    7. `/judge/send/ui/float`

        说明：发送浮点数UI数据

        传入参数数据：


        | 数据字段名 | 数据类型 | 是否为返回值 | 是否必须传输 | 说明 |
        | :---: | :---: | :---: | :---: | :---: |
        | `name` | `uint8_t` | × | 可选 |在操作中，作为客户端的唯一索引，有且只有3个字符 |
        | `color` | `uint8_t` | × | 可选 | 颜色（详见颜色说明）  |
        | `width` | `uint8_t` | × | 可选 | 宽度  |
        | `layer` | `uint8_t` | × | 可选 | 图层 |
        | `value` | `float` | × | 可选 |值 |
        | `start-x` | `int16_t` | × | 可选 | 开始x坐标 |
        | `start-y` | `int16_t` | × | 可选 | 开始y坐标 |
        | `size` | `uint16_t` | × | 可选 | 大小 |
        | `digit` | `uint8_t` | × | 可选 | 有效位数 |
        | `lopera` | `uint8_t` | × | 可选 | 操作（详见操作说明）  |

    8. `/judge/send/ui/int`

        说明：发送整型UI数据

        传入参数数据：


        | 数据字段名 | 数据类型 | 是否为返回值 | 是否必须传输 | 说明 |
        | :---: | :---: | :---: | :---: | :---: |
        | `name` | `uint8_t` | × | 可选 |在操作中，作为客户端的唯一索引，有且只有3个字符 |
        | `color` | `uint8_t` | × | 可选 | 颜色（详见颜色说明）  |
        | `width` | `uint8_t` | × | 可选 | 宽度  |
        | `layer` | `uint8_t` | × | 可选 | 图层 |
        | `value` | `float` | × | 可选 |值 |
        | `start-x` | `int16_t` | × | 可选 | 开始x坐标 |
        | `start-y` | `int16_t` | × | 可选 | 开始y坐标 |
        | `size` | `uint16_t` | × | 可选 | 大小 |
        | `lopera` | `uint8_t` | × | 可选 | 操作（详见操作说明）  |