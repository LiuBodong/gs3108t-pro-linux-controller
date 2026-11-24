#pragma once

#include <QString>

namespace LightModes {

// 固件模式号 = Windows 驱动语言文件（2052.lan）ID - 520，实测范围 1..20。
// 这套编号与键盘本体的菜单顺序不同，已逐项实测确认：
//   1 静态恒亮   2 单点亮   3 单熄灭    4 繁星点点   5 漫天飞舞
//   6 百花争艳   7 动态呼吸 8 光谱循环  9 彩泉涌动  10 多彩纵横
//  11 随波逐流  12 峰回路转 13 一触即发 14 一石二鸟 15 涟漪扩散
//  16 川流不息  17 重峦叠嶂 18 斜风细雨 19 来回穿梭 20 关闭背光
inline constexpr int kFirst = 1;
inline constexpr int kLast = 20;

inline QString name(int mode)
{
    switch (mode) {
    case 1: return QStringLiteral("静态恒亮");
    case 2: return QStringLiteral("单点亮");
    case 3: return QStringLiteral("单熄灭");
    case 4: return QStringLiteral("繁星点点");
    case 5: return QStringLiteral("漫天飞舞");
    case 6: return QStringLiteral("百花争艳");
    case 7: return QStringLiteral("动态呼吸");
    case 8: return QStringLiteral("光谱循环");
    case 9: return QStringLiteral("彩泉涌动");
    case 10: return QStringLiteral("多彩纵横");
    case 11: return QStringLiteral("随波逐流");
    case 12: return QStringLiteral("峰回路转");
    case 13: return QStringLiteral("一触即发");
    case 14: return QStringLiteral("一石二鸟");
    case 15: return QStringLiteral("涟漪扩散");
    case 16: return QStringLiteral("川流不息");
    case 17: return QStringLiteral("重峦叠嶂");
    case 18: return QStringLiteral("斜风细雨");
    case 19: return QStringLiteral("来回穿梭");
    case 20: return QStringLiteral("关闭背光");
    default: return QStringLiteral("模式 %1").arg(mode);
    }
}

// 关闭背光模式会熄灭所有灯珠，与颜色无关。
inline constexpr int kOff = 20;

} // namespace LightModes
