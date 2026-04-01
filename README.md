# XIAO nRF52840 NuttX 开发模板

本仓库是 Seeed Studio XIAO nRF52840 开发板的 NuttX RTOS 开发模板，使用 Git Submodules 管理依赖，包含自定义 LED 控制应用（xled）。

## 功能特性

- 基于 NuttX 官方 jumbo 配置
- USB CDC/ACM 串口控制台（NSH Shell）
- 自定义 `xled` 命令行工具，支持：
  - 单独控制红/绿/蓝 LED 的开关
  - LED 闪烁功能
  - 查询当前 LED 状态
  - 通过位掩码直接设置 LED

## 目录结构

```
xiao-nrf52840-template/
├── nuttx/                   # Git Submodule: NuttX 内核
├── apps/                    # Git Submodule: NuttX 应用
├── uf2/                     # Git Submodule: UF2 工具
├── custom_apps/             # 自定义应用目录
│   └── xled/               # LED 控制应用
│       ├── xled_main.c     # 主程序源码
│       ├── Kconfig         # Kconfig 配置
│       ├── Make.defs       # Make 构建定义
│       └── CMakeLists.txt  # CMake 构建配置
├── configs/
│   └── defconfig           # NuttX 配置文件
├── scripts/
│   ├── setup.sh            # 初始化 submodules
│   └── build.sh            # 构建脚本
├── .gitmodules             # Submodules 配置
├── .gitignore
└── README.md
```

## 环境要求

- Linux / macOS
- Python 3
- Git
- ARM 交叉编译工具链（arm-none-eabi-gcc）

macOS 安装工具链：
```bash
brew install --cask gcc-aarch64-embedded
```

Ubuntu/Debian 安装工具链：
```bash
sudo apt install gcc-arm-none-eabi
```

## 快速开始

### 1. 克隆本仓库（包含 submodules）

克隆时自动初始化 submodules：
```bash
git clone --recurse-submodules <你的仓库地址> xiao-nrf52840-template
cd xiao-nrf52840-template
```

如果已经克隆但没有 submodules，手动初始化：
```bash
git submodule update --init --recursive
```

这会下载三个 submodules：
- `nuttx/` - NuttX 内核
- `apps/` - NuttX 应用程序
- `uf2/` - UF2 格式转换工具

### 2. 手动构建步骤

#### 步骤 2.1：链接自定义应用到 apps 目录

将 `custom_apps/xled` 目录创建符号链接到 `apps/xled`：

```bash
ln -s $(pwd)/custom_apps/xled apps/xled
```

#### 步骤 2.2：修改 apps/Kconfig

在 `apps/Kconfig` 文件中，找到 `endmenu` 行，在其前面添加：
```
source "$APPSDIR/xled/Kconfig"
```

#### 步骤 2.3：修改 apps/Make.defs

在 `apps/Make.defs` 文件末尾添加：
```makefile
include $(wildcard $(APPDIR)/xled/Make.defs)
```

#### 步骤 2.4：修改 apps/CMakeLists.txt

在 `apps/CMakeLists.txt` 文件末尾添加：
```cmake
add_subdirectory_ifdef(CONFIG_APPS_XLED xled)
```

#### 步骤 2.5：配置 NuttX

进入 nuttx 目录，清理并配置：
```bash
cd nuttx
make distclean
./tools/configure.sh xiao-nrf52840:jumbo
```

应用自定义配置：
```bash
cat ../configs/defconfig >> .config
make olddefconfig
```

#### 步骤 2.6：编译固件

在 nuttx 目录下执行编译：
```bash
make -j$(nproc)  # Linux
# 或
make -j$(sysctl -n hw.ncpu)  # macOS
```

编译成功后生成 `nuttx.hex` 文件。

#### 步骤 2.7：转换为 UF2 格式

使用 uf2 工具转换固件格式：
```bash
python3 ../uf2/utils/uf2conv.py -c -f 0xADA52840 -i nuttx.hex -o nuttx.uf2
```

### 3. 刷写固件

1. 将 XIAO nRF52840 通过 USB 连接到电脑
2. **快速双击** RESET 按钮进入 Bootloader 模式
3. 开发板会以 USB 大容量存储设备形式出现
4. 将 `nuttx.uf2` 复制到该设备：

```bash
# macOS
cp nuttx/nuttx.uf2 /Volumes/XIAO-SENSE/

# Linux
cp nuttx/nuttx.uf2 /media/$USER/XIAO-SENSE/
```

### 4. 连接串口

刷写完成后，开发板会以 USB CDC/ACM 串口设备出现。使用串口终端连接：

```bash
# macOS
screen /dev/cu.usbmodem* 115200

# Linux
screen /dev/ttyACM0 115200

# 或使用 minicom
minicom -D /dev/ttyACM0
```

## xled 命令使用

连接到 NSH 后，可以使用 `xled` 命令控制 LED：

### 查看帮助

```
nsh> xled
```

### 开启 LED

```
nsh> xled on red        # 开启红色 LED
nsh> xled on green      # 开启绿色 LED
nsh> xled on blue       # 开启蓝色 LED
nsh> xled on red+green  # 开启红色和绿色 LED
nsh> xled on all        # 开启所有 LED
```

### 关闭 LED

```
nsh> xled off red       # 关闭红色 LED
nsh> xled off all       # 关闭所有 LED
```

### 设置位掩码

```
nsh> xled set 0x01      # 只开启蓝色 (bit 0)
nsh> xled set 0x02      # 只开启红色 (bit 1)
nsh> xled set 0x04      # 只开启绿色 (bit 2)
nsh> xled set 0x07      # 开启所有 (蓝+红+绿)
nsh> xled set 0x00      # 关闭所有
```

### 查询状态

```
nsh> xled get
LED state: 0x01 [BLUE=ON RED=off GREEN=off ]
```

### LED 闪烁

```
nsh> xled blink red              # 红色闪烁 3 次，间隔 500ms
nsh> xled blink blue 200 5       # 蓝色闪烁 5 次，间隔 200ms
```

### RGB 设置

```
nsh> xled rgb 1 0 1    # 蓝=开 红=关 绿=开
```

## LED 位掩码对应表

| 位掩码 | LED   | 引脚  | 颜色 |
| ------ | ----- | ----- | ---- |
| 0x01   | LED 1 | P0.6  | 蓝色 |
| 0x02   | LED 2 | P0.26 | 红色 |
| 0x04   | LED 3 | P0.30 | 绿色 |

> **注意**：LED 编号遵循 board.h 定义，实际物理连接为 LED1=蓝、LED2=红、LED3=绿。

## 其他操作

### 仅配置（不编译）

执行上面的步骤 2.1-2.5，配置完成后不执行编译。

### 清理构建产物

进入 nuttx 目录执行清理：
```bash
cd nuttx
make distclean
```

### 恢复 apps 子模块到原始状态

如果需要移除对 apps 目录的修改：

1. 删除 xled 符号链接：
   ```bash
   rm apps/xled
   ```

2. 从 `apps/Kconfig` 中删除包含 `xled/Kconfig` 的行

3. 从 `apps/Make.defs` 中删除包含 `xled/Make.defs` 的行

4. 从 `apps/CMakeLists.txt` 中删除包含 `CONFIG_APPS_XLED` 的行

## Submodules 管理

### 更新 submodules 到最新版本

```bash
git submodule update --remote
```

### 切换到特定版本

```bash
cd nuttx
git checkout <tag或commit>
cd ..
git add nuttx
git commit -m "Update nuttx to <version>"
```

## 自定义开发

### 修改配置

```bash
cd nuttx
make menuconfig
```

修改后保存配置：
```bash
make savedefconfig
cp defconfig ../configs/defconfig
```

### 添加新应用

在 `custom_apps/` 目录下创建新目录，参照 `xled` 的结构：
- `<应用名>_main.c` - 主程序
- `Kconfig` - 配置选项
- `Make.defs` - Make 构建
- `CMakeLists.txt` - CMake 构建

然后修改 `scripts/build.sh` 添加新的符号链接和 patch 逻辑。

## 工作原理

构建脚本采用以下策略集成自定义应用：

1. **符号链接**：将 `custom_apps/xled` 链接到 `apps/xled`
2. **临时 patch**：修改 `apps/` 下的 Kconfig、Make.defs、CMakeLists.txt
3. **恢复原始状态**：`./scripts/build.sh restore` 可恢复 apps submodule

这种方式不污染 submodule 仓库，同时保持构建兼容性。

## 参考资源

- [NuttX 官方文档](https://nuttx.apache.org/docs/latest/)
- [NuttX XIAO nRF52840 文档](https://nuttx.apache.org/docs/latest/platforms/arm/nrf52/boards/xiao-nrf52840/index.html)
- [Seeed Studio XIAO nRF52840 Wiki](https://wiki.seeedstudio.com/XIAO_BLE/)
- [UF2 工具](https://github.com/microsoft/uf2)

## 许可证

本模板代码采用 Apache-2.0 许可证。NuttX 和相关组件遵循其各自许可证。
