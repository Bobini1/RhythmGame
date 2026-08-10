<p align=center>
    <a href="https://github.com/Bobini1/RhythmGame/actions"><img src="https://github.com/Bobini1/RhythmGame/actions/workflows/ci.yml/badge.svg"/></a>
    <a href="https://github.com/Bobini1/RhythmGame/blob/master/LICENSE.md"><img src="https://img.shields.io/github/license/Bobini1/RhythmGame"/></a>
    <a href="https://github.com/Bobini1/RhythmGame/releases/latest"><img alt="GitHub Downloads (all assets, all releases)" src="https://img.shields.io/github/downloads/Bobini1/RhythmGame/total"></a>
    <a href="https://aur.archlinux.org/packages/rhythmgame-git"><img alt="AUR Popularity" src="https://img.shields.io/aur/popularity/rhythmgame-git?logo=arch-linux"></a>
    <a href="https://github.com/Bobini1/RhythmGame/blob/master/flake.nix"><img alt="Nix" src="https://img.shields.io/badge/Nix-5277C3?logo=nixos&logoColor=fff"></a>
    <br>
    <a href="https://discord.gg/bDxmuSzXBW"><img src="https://img.shields.io/discord/1410743088686829661.svg?color=7289DA&label=RhythmGame%20Community&logo=Discord"/></a>
    <a href="https://rhythmgame.eu"><img src="https://img.shields.io/website?url=https%3A%2F%2Frhythmgame.eu&label=IR"/></a>
</p>

# RhythmGame

[English](README.md) | [简体中文](README.zh-CN.md) | [日本語](README_ja.md) | [Español](README_es.md)

一款可自定义的BMS播放器，在Windows 和 Linux 运行。

刚接触BMS？ 查看 [Beatoraja English Guide](https://github.com/wcko87/beatoraja-english-guide/wiki/BMS-Overview)
以了解BMS及如何寻找谱面并游玩。

## 功能

### 自定义主题

在游戏过程中按 F2 键可以自定义默认主题，同时还可以自由调整各个元素的位置！

![Customize mode (song: wa. - Black Lotus)](docs/images/customize.webp)

你也可以使用 [QML](https://doc.qt.io/qt-6/qmlreference.html) 来创建自己的主题。
如果你感兴趣可以联系我，我会帮你开始主题的创建。
你可以用 [default theme](https://github.com/Bobini1/RhythmGame/tree/master/share/RhythmGame/themes/Default) 作为参考。
见 [DEV_THEME.md](DEV_THEME.md) 文档以了解更多信息。

游戏还支持基于CSV的 Lunatic Rave 2 和 Beatoraja 皮肤。

![Lunatic Rave 2 skin - LR2 Default - Play](docs/images/lr2-play.png)]

![Lunatic Rave 2 skin - LR2 Default - Select](docs/images/lr2.png)

### 网络排行榜（支持 Bokutachi 和 LR2IR）

与世界各地的玩家竞争吧！RhythmGame 平台拥有自己的本地 IR 服务器，地址为https://rhythmgame.eu,
你也可以上传成绩至 [Bokutachi](https://boku.tachi.ac/) 并在 [Lunatic Rave 2 Internet Ranking](http://www.dream-pro.info/~lavalse/LR2IR/search.cgi) 查看。

[![Internet ranking](docs/images/ranking.png)](https://rhythmgame.eu)

|                        排名                        |                        在线统计                         |
|:--------------------------------------------------:|:-------------------------------------------------------:|
| ![In-game ranking](docs/images/ranking-ingame.png) | ![In-game ranking stats](docs/images/ranking-stats.png) |

### 基于 Lunatic Rave 2 的规则

时间窗口与 Lunatic Rave 2/Lr2oraja 一致，
你可以与其比较分数。

### 本地对战

和朋友一起玩吧！在选择歌曲时点击两次开始键，即可进入对战模式。

![Local battle mode](docs/images/battle.png)

### 支持难度表

RhythmGame 原生支持BMS难度表。
在设置里粘贴链接即可。

![Tables](docs/images/tables.png)

![Course](docs/images/course.png)

### 平滑缩放

支持所有分辨率，按F11进入全屏。

![Scaling (song: isocosa - data lake)](docs/images/resize.webp)

### 多语言支持

RhythmGame 默认支持英语、波兰语和简体中文。
如果你想帮助翻译成其他语言，欢迎联系我！

![Language selection](docs/images/languages.webp)

### 精美的默认主题

基于 [Shimi999](https://github.com/Shimi9999/GenericTheme) 和 [souki202](https://github.com/souki202/my_beatoraja_skin) 的作品，
RhythmGame 的默认主题包含了游玩 BMS 所需的所有必要功能。

![Song selection](docs/images/select.png)

![Result screen](docs/images/result.png)

### 异步扫描歌曲库

RhythmGame 会在后台扫描你的歌曲库，
因此你可以立即开始游玩！

# Building and installing

See the [DEV_ENGINE](DEV_ENGINE.md) document.

# Contributing

See the [CONTRIBUTING](CONTRIBUTING.md) document.

# Licensing

The project is distributed under the [MIT license](LICENSE.md).
