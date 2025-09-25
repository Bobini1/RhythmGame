<p align=center>
    <a href="https://github.com/Bobini1/RhythmGame/actions"><img src="https://github.com/Bobini1/RhythmGame/actions/workflows/ci.yml/badge.svg"/></a>
    <a href="https://github.com/Bobini1/RhythmGame/blob/master/LICENSE.md"><img src="https://img.shields.io/github/license/Bobini1/RhythmGame"/></a>
    <img href="https://github.com/Bobini1/RhythmGame/releases/latest"><img alt="GitHub Downloads (all assets, all releases)" src="https://img.shields.io/github/downloads/Bobini1/RhythmGame/total"></a>
    <a href="https://aur.archlinux.org/packages/rhythmgame-git"><img alt="AUR Popularity" src="https://img.shields.io/aur/popularity/rhythmgame-git?logo=arch-linux"></a>
    <br>
    <a href="https://discord.gg/bDxmuSzXBW"><img src="https://img.shields.io/discord/1410743088686829661.svg?color=7289DA&label=RhythmGame%20Community&logo=Discord"/></a>
</p>

# RhythmGame

A customizable BMS player for Windows and Linux.

New to BMS? Check out w's [Beatoraja English Guide](https://github.com/wcko87/beatoraja-english-guide/wiki/BMS-Overview)
to learn about BMS and how to find songs to play.

## Features

### Customizable themes

Customize the default theme by pressing F2 during gameplay and moving the elements around!

![Customize mode (song: wa. - Black Lotus)](docs/images/customize.webp)

You can also create your own custom theme with [QML](https://doc.qt.io/qt-6/qmlreference.html).
Contact me if you're interested, I can help you get started!
You can use the [default theme](https://github.com/Bobini1/RhythmGame/tree/master/data/themes/Default) as a reference.
See the [DEV_THEME.md](DEV_THEME.md) document for more information.

### Rules based on Lunatic Rave 2

The timing windows and gauges match Lunatic Rave 2/Lr2oraja
so you can compare your scores with those games easily.

### Local battle mode

Play with a friend! Press start twice in song select to enable battle mode.

![Local battle mode](docs/images/battle.png)

### Table support

RhythmGame supports BMS tables natively.
Simply paste a link in settings.

![Tables](docs/images/tables.png)

![Course](docs/images/course.png)

### Smooth scaling

All resolutions supported! Press F11 to toggle fullscreen.

![Scaling (song: isocosa - data lake)](docs/images/resize.webp)

### Translations

RhythmGame supports English and Polish by default.
Contact me if you would like to help translating it to your language!

![Language selection](docs/images/languages.webp)

### A beautiful default theme

Based on the works of [Shimi999](https://github.com/Shimi9999/GenericTheme) and 
[souki202](https://github.com/souki202/my_beatoraja_skin),
RhythmGame's default theme contains all the necessary features to play BMS.

![Song selection](docs/images/select.png)

![Result screen](docs/images/result.png)

### Asynchronous scanning of the song library

RhythmGame scans your song library in the background,
so you can start playing immediately!

# Building and installing

See the [DEV_ENGINE](DEV_ENGINE.md) document.

# Contributing

See the [CONTRIBUTING](CONTRIBUTING.md) document.

# Licensing

The project is distributed under the [MIT license](LICENSE.md).
