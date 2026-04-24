# Linux Input 子系统 GPIO 按键驱动实验

## 1. 项目简介

本项目实现了一个基于 Linux Input 子系统的 GPIO 按键驱动。

实验平台使用 RK3568 开发板，通过设备树描述 GPIO 按键资源，驱动中完成 GPIO 获取、中断申请、定时器消抖、input 设备注册以及按键事件上报。最终用户态可以通过 `/dev/input/eventX` 或 `evtest` 读取标准 input 事件。

这个项目的重点不是简单读取 GPIO 电平，而是学习 Linux 内核如何将一个普通 GPIO 按键接入标准输入框架。

---

## 2. 项目目标

通过本项目主要学习以下内容：

- Linux Input 子系统的基本模型
- `input_dev` 的申请、初始化和注册
- GPIO descriptor 接口的使用
- 设备树中 GPIO 和 pinctrl 的配置方式
- GPIO 中断申请与双边沿触发
- 定时器软件消抖
- `input_report_key()` 和 `input_sync()` 的使用
- `/dev/input/eventX` 与 `evtest` 测试方法

---

## 3. 实验环境

| 项目 | 说明 |
| --- | --- |
| 开发板 | LubanCat 2 / RK3568 平台 |
| 内核 | Linux |
| 驱动类型 | platform driver |
| 输入框架 | Linux Input Subsystem |
| 测试工具 | evtest |
| 按键类型 | GPIO 低电平有效按键 |

---

## 4. 项目目录

```text
.
├── input_key.c              # GPIO input 按键驱动源码
├── Makefile                 # 内核模块编译文件
├── dts/
│   └── my_input_key.dts     # 设备树节点示例
├── README.md
└── .gitignore
```

