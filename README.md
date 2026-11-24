# GS3108T PRO 控制中心（Linux / Qt 6 Widgets / C++26）

这是给 GANSS GS3108T-PRO 键盘写的 Linux 配置工具。界面用 **Qt 6 Widgets** 手写绘制，
不依赖 QML；编译器使用 **Clang 21**，语言标准 **C++26**。

它直接通过 Linux 的 `/dev/hidraw*` 和键盘通信，不需要安装 Windows 驱动，
也不需要 Wine。

---

## 当前能做什么

| 功能 | 状态 |
|---|---|
| USB 有线灯效（模式 / 颜色 / 亮度 / 速度 / 方向 / 多彩） | 已验证 |
| 2.4G 无线灯效 | 已验证 |
| 我的专属配置（本地预设，一键应用） | 已支持 |
| TFT 时钟同步 | USB / 2.4G 都已验证 |
| USB / 2.4G 接口自动识别 | 已支持 |
| 设备列表、连接方式切换、权限提示 | 已支持 |
| 用户灯光 / 自定义逐键 RGB | 待逆向 |
| TFT GIF | 待逆向 |

> 结论先说：**灯效和校时已经能用**。Windows 驱动里还有逐键灯光、
> TFT 动图等模块，它们需要继续从 `DeviceDriver.exe` 里把报文挖出来，
> 不是 UI 层能凭空变出来的。侧灯、音乐律动、宏和按键映射不在本项目的范围内。

---

## 编译环境

需要：

- Linux（Wayland 或 X11 都可以）
- CMake 3.30 或更新
- Clang 21（需要支持 `-std=c++26`）
- Qt 6.5 或更新：Core / Gui / Widgets / Svg
- make（预设使用 Unix Makefiles）

Debian / Ubuntu 可以这样装（按项目规范用 `pkexec`，不要用 `sudo`）：

```bash
pkexec apt-get install -y --no-install-recommends \
    clang cmake make qt6-base-dev qt6-svg-dev
```

确认版本：

```bash
clang++ --version     # 需要 21.x
cmake --version       # 需要 3.30+
qmake6 -query QT_VERSION
```

---

## 构建和运行

```bash
cd gs3108t-pro-linux-controller
cmake --preset clang-cxx26
cmake --build --preset clang-cxx26
./build-clang/gs3108t-pro
```

`CMakePresets.json` 已经把编译器固定为 `clang++`，构建目录是 `build-clang`。
如果想手动指定：

```bash
cmake -S . -B build-clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_BUILD_TYPE=Release
cmake --build build-clang -j
```

---

## 界面

左侧四个页面：

- **灯光控制**：模式网格、色轮、十六进制颜色、亮度、速度、方向、多彩。
  右下角固定一个「应用到键盘」按钮，滚动页面时也一直可见。
  页面底部是「我的专属配置」：把当前这整套参数存成一份预设，
  以后一键应用或删除。
- **TFT 时钟**：显示本机时间，点「同步当前时间」会把时间写进键盘屏幕的 RTC。
- **设备与连接**：显示检测到的配置接口、当前连接方式、udev 权限命令。
- **关于**：版本、协议摘要、功能进度。

灯效页的编号就是键盘固件的模式号，范围 1–20，已经**逐项实测校准**。
命名规律是「驱动语言文件（2052.lan）的字符串 ID − 520」，与键盘本体
`FN+8` / `FN+9` 循环时的显示顺序并不相同——那个顺序是固件菜单的排列，
两套编号不要互相换算。

| 编号 | 灯效 | 编号 | 灯效 |
|---|---|---|---|
| 1 | 静态恒亮 | 11 | 随波逐流 |
| 2 | 单点亮 | 12 | 峰回路转 |
| 3 | 单熄灭 | 13 | 一触即发 |
| 4 | 繁星点点 | 14 | 一石二鸟 |
| 5 | 漫天飞舞 | 15 | 涟漪扩散 |
| 6 | 百花争艳 | 16 | 川流不息 |
| 7 | 动态呼吸 | 17 | 重峦叠嶂 |
| 8 | 光谱循环 | 18 | 斜风细雨 |
| 9 | 彩泉涌动 | 19 | 来回穿梭 |
| 10 | 多彩纵横 | 20 | 关闭背光 |

选中一个编号后点「应用到键盘」即可预览。命令行的 `--modes` 能打印同一张表。

预设保存在 `~/.config/gs3108t-pro/presets.json`，纯人可读的 JSON，
可以直接备份、拷贝到别的机器，或手工编辑。文件不存在时界面显示空状态，
第一次点「保存当前设置」才会创建。

---

## 命令行参数

```bash
./build-clang/gs3108t-pro                 # 打开图形界面
./build-clang/gs3108t-pro --list          # 只打印检测到的 HID 接口
./build-clang/gs3108t-pro --apply-test    # 发一组测试灯效（模式 7 动态呼吸、红色）
./build-clang/gs3108t-pro --clock-test    # 同步当前时间
./build-clang/gs3108t-pro --wireless      # 强制使用 2.4G 接口
./build-clang/gs3108t-pro --usb           # 强制使用 USB 接口
./build-clang/gs3108t-pro --page 1        # 启动时打开第 2 个页面（0 开始）
./build-clang/gs3108t-pro --screenshot /tmp/ui.png   # 截图后退出
```

排错时可以打开底层报文日志：

```bash
GSKBD_DEBUG=1 ./build-clang/gs3108t-pro --apply-test
```

它会打印每一次 Feature 写入和读回的内容，方便对照协议。

---

## 2.4G 怎么测

1. 按键盘上的 `FN+4` 切到 2.4G 模式（说明书里的模式切换键）。
2. 按一下键盘按键，解除休眠。
3. 先确认接口存在：

   ```bash
   ./build-clang/gs3108t-pro --list
   ```

   应该能看到 `usage 0xFF60`、`output 32` 的 `GS3108T-PRO 2.4G` 接口。

4. 先确认 2.4G 链路是通的。校时协议有回执，适合用来确认：

   ```bash
   ./build-clang/gs3108t-pro --clock-test --wireless
   ```

   如果键盘 TFT 时间更新了，说明 2.4G 通信正常；没反应就先检查 `FN+4` 和接口。

5. 发一个测试灯效：

   ```bash
   ./build-clang/gs3108t-pro --apply-test --wireless
   ```

6. 看键盘有没有变成红色呼吸。**回执/写入成功不等于灯真的变了**，一定要肉眼看。
6. 如果没反应，回到图形界面「设备与连接」页，把连接方式切到「2.4G 无线」，
   再回灯光页点应用；同时确认键盘已经被 `FN+4` 切到 2.4G。

> 2.4G 灯效的报文是从驱动反汇编里推出来的，目前还没有在真实 2.4G 连接下
> 由用户目视确认。USB 灯效和两种模式的校时都已经验证过。

---

## 权限（udev）

普通用户需要能读写 `/dev/hidraw*`。仓库里带了一份规则文件：

以下命令在项目根目录执行：

```bash
pkexec install -m 644 assets/70-gs3108t-pro.rules \
    /etc/udev/rules.d/70-gs3108t-pro.rules
pkexec udevadm control --reload-rules
pkexec udevadm trigger --subsystem-match=hidraw --action=change
```

验证：

```bash
getfacl /dev/hidrawN
```

应该能看到当前用户有 `rw-`。没有权限时，图形界面会显示失败原因，
设备页也会给出同样的命令。

---

## 键盘自带灯效快捷键

不打开软件也能调，说明书最后一页的组合键：

| 组合键 | 作用 |
|---|---|
| `FN+8` / `FN+9` | 灯光模式正向 / 反向循环 |
| `FN+0` | 切换背光颜色 |
| `FN+↑` / `FN+↓` | 亮度增加 / 降低 |
| `FN+←` / `FN+→` | 速度减慢 / 加快 |
| `FN+(-)` / `FN+(+)` | 两组自定义灯光；长按 3 秒进入/退出录制 |
| `FN+三角键` | 三角灯模式循环 |
| `FN+N`（长按 3 秒） | 极省电模式，会关背光和屏幕 |
| `FN+空格`（长按） | 恢复出厂设置 |

---

## 协议摘要

完整说明在项目根目录的 `GS3108T-PRO-时间同步-协议与实现.md` 和
`HID设备驱动逆向方法论-交接文档.md`。

### USB 有线

- 接口：`usage page 0xFF13`，64 字节 Feature。
- 每次发送 65 字节（1 字节 report ID 0 + 64 字节数据）。
- 灯效序列：

  ```
  00 04 18                                    进入会话
  00 04 13 ... 第 9 字节 = 01                 准备
  00 mode R G B 00 00 00 00                  数据
     colorful brightness speed direction
     00 00 AA 55
  00 04 02                                    结束
  00 04 F0                                    保存
  ```

- 校时序列：`04 18` → `04 28` → 时间包 → `04 02`。
  时间包从 `00 01 5A 年 月 日 时 分 秒 00 星期` 开始，末尾 `AA 55`。

### 2.4G 无线

- 接口：`usage page 0xFF60`，32 字节 Output。
- 发送 33 字节（report ID 0 + 32 字节数据），第 32 字节是前 32 字节之和的低 8 位。
- 灯效帧：

  ```
  00 05 10 00 mode R G B 00 00 00 00
  colorful brightness speed direction 00 00 AA 55
  ... 00 校验和
  ```

- 校时帧：`00 0C 10 00 00 01 5A 年 月 日 时 分 秒 00 星期 ... AA 55 校验和`。

### 已验证的字段

- `mode` 取值 1–20，与驱动语言文件里的灯效名称一一对应（`语言 ID − 520`），
  完整表格见 `src/lightmodes.h`。
- RGB 顺序是 R、G、B（发 `FF0000` 得到红色）。
- `data[8]` 是多**彩**开关：光谱循环这类模式不受颜色影响，但随波逐流等
  单色可用的模式在多彩关闭时才会用 `data[1:4]` 的颜色。
- `data[9]` 是亮度（5 = 高亮）。
- `data[10]` 是速度，`data[11]` 是方向；
  这几个字段的顺序是从驱动结构体偏移推出来的，如果发现某个参数没效果，
  可以在 `LightPage` 里调整；本地逆向目录里的 `gskbd-light.py` 支持
  `--params` 直接试（该脚本不随仓库分发）。

---

## 目录结构

```
gs3108t-pro-linux-controller/
├── CMakeLists.txt
├── CMakePresets.json
├── resources.qrc
├── assets/
│   ├── style.qss
│   ├── 70-gs3108t-pro.rules       # udev 权限规则
│   └── icons/*.svg
├── src/
│   ├── main.cpp
│   ├── hidmanager.h/.cpp          # /dev/hidraw 枚举、Feature/Output 读写
│   ├── keyboardcontroller.h/.cpp  # 灯光和校时协议、连接方式
│   ├── lightmodes.h               # 灯效模式 1–20 与名称对照
│   ├── presetstore.h/.cpp         # 本地预设的读写
│   ├── theme.h/.cpp               # 颜色、字体、图标染色、QSS
│   ├── widgets.h/.cpp             # 导航、色轮、滑块、模式卡片、对话框等
│   ├── pages.h/.cpp               # 四个页面
│   └── mainwindow.h/.cpp          # 无边框窗口、标题栏、侧边栏
└── build-clang/                   # cmake --preset 的构建目录
```

---

## 开发路线

1. ~~校准灯效模式~~（已完成）：1–20 全部与名称对上，见 `src/lightmodes.h`。
2. ~~我的专属配置~~（已完成）：本地预设的保存、应用、删除，见 `src/presetstore.*`。
3. **用户灯光 / 自定义灯光**：逆向逐键 RGB 的报文，把键位图和颜色写入接上。
4. **TFT GIF**：驱动里有分块上传协议（`0x1000` 字节分块），可以按同样方法还原。

每加一个模块，建议沿用现有验证方法：先 `--dry-run` 看报文，再发一个
显眼的值让用户目视确认，确认后恢复现场。

---

## 相关文件

仓库只包含桌面应用本身。逆向过程中用到的协议文档和 Python 脚本留在
作者本地的 `gs3108_analysis/` 目录，不随仓库分发。
> 命令行里的 `--wireless` / `--usb` 只影响本次运行；图形界面里的「连接方式」会保存到 QSettings。
