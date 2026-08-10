[English](./README.md) | [简体中文](./README.zh-CN.md) | [日本語](./README_ja.md) | **Español**

<p align=center>
    <a href="https://github.com/Bobini1/RhythmGame/actions"><img src="https://github.com/Bobini1/RhythmGame/actions/workflows/ci.yml/badge.svg"/></a>
    <a href="https://github.com/Bobini1/RhythmGame/blob/master/LICENSE.md"><img src="https://img.shields.io/github/license/Bobini1/RhythmGame"/></a>
    <a href="https://github.com/Bobini1/RhythmGame/releases/latest"><img alt="Descargas de GitHub (todos los recursos, todas las versiones)" src="https://img.shields.io/github/downloads/Bobini1/RhythmGame/total"></a>
    <a href="https://aur.archlinux.org/packages/rhythmgame-git"><img alt="Popularidad en AUR" src="https://img.shields.io/aur/popularity/rhythmgame-git?logo=arch-linux"></a>
    <a href="https://github.com/Bobini1/RhythmGame/blob/master/flake.nix"><img alt="Nix" src="https://img.shields.io/badge/Nix-5277C3?logo=nixos&logoColor=fff"></a>
    <br>
    <a href="https://discord.gg/bDxmuSzXBW"><img src="https://img.shields.io/discord/1410743088686829661.svg?color=7289DA&label=RhythmGame%20Community&logo=Discord"/></a>
    <a href="https://rhythmgame.eu"><img src="https://img.shields.io/website?url=https%3A%2F%2Frhythmgame.eu&label=IR"/></a>
</p>

# RhythmGame

[Inglés](README.md) | [Chino simplificado](README.zh-CN.md)

Un reproductor BMS personalizable para Windows y Linux.

¿Eres nuevo en los BMS? Echa un vistazo a la [Guía en inglés de Beatoraja](https://github.com/wcko87/beatoraja-english-guide/wiki/BMS-Overview) de w para aprender sobre los BMS y cómo encontrar canciones para tocar.

## Características

### Temas personalizables

Personaliza el tema predeterminado presionando F2 durante el juego y moviendo los elementos a tu gusto.

![Modo de personalización (canción: wa. - Black Lotus)](docs/images/customize.webp)

También puedes crear tu propio tema personalizado con [QML](https://doc.qt.io/qt-6/qmlreference.html).  
¡Contáctame si estás interesado, que puedo ayudarte a empezar!  
Puedes usar el [tema predeterminado](https://github.com/Bobini1/RhythmGame/tree/master/share/RhythmGame/themes/Default) como referencia.  
Consulta el documento [DEV_THEME.md](DEV_THEME.md) para obtener más información.

El juego también admite skins basadas en CSV para Lunatic Rave 2 y Beatoraja.

![Skin de Lunatic Rave 2 - LR2 Default - Jugar](docs/images/lr2-play.png)

![Skin de Lunatic Rave 2 - LR2 Default - Seleccionar](docs/images/lr2.png)

### Clasificación en línea con soporte para Bokutachi y LR2IR

¡Compite con jugadores de todo el mundo! RhythmGame cuenta con su propio servidor IR nativo en https://rhythmgame.eu, pero también puedes enviar tus puntuaciones a [Bokutachi](https://boku.tachi.ac/) y consultar las puntuaciones del [Ranking Internacional de Lunatic Rave 2](http://www.dream-pro.info/~lavalse/LR2IR/search.cgi).

[![Ranking en línea](docs/images/ranking.png)](https://rhythmgame.eu)

|                      Clasificación                       |                      Estadísticas en línea                       |
|:--------------------------------------------------:|:-------------------------------------------------------:|
|![Clasificación dentro del juego](docs/images/ranking-ingame.png) |![Estadísticas de la clasificación dentro del juego](docs/images/ranking-stats.png) |

### Reglas basadas en Lunatic Rave 2

Las ventanas de tiempo y los indicadores son compatibles con Lunatic Rave 2/Lr2oraja, por lo que puedes comparar fácilmente tus puntuaciones con las de esos juegos.

### Modo de batalla local

¡Juega con un amigo! Presiona “Start” dos veces en la selección de canciones para activar el modo de batalla.

![Modo de batalla local](docs/images/battle.png)

### Soporte para tablas

RhythmGame admite tablas BMS de forma nativa. Simplemente pegue un enlace en la configuración.

![Tablas](docs/images/tables.png)

![Curso](docs/images/course.png)

### Escalado suave

¡Se admiten todas las resoluciones! Presione F11 para cambiar entre modo normal y pantalla completa.

![Escalado (canción: isocosa - data lake)](docs/images/resize.webp)

### Traducciones

Por defecto, RhythmGame admite inglés, polaco y chino simplificado. ¡Contáctenme si desean ayudar a traducirlo a su idioma!

![Selección de idioma](docs/images/languages.webp)

### Un hermoso tema predeterminado

Basado en el trabajo de [Shimi999](https://github.com/Shimi9999/GenericTheme) y [souki202](https://github.com/souki202/my_beatoraja_skin), el tema predeterminado de RhythmGame cuenta con todas las funciones necesarias para jugar BMS.

![Selección de canciones](docs/images/select.png)

![Pantalla de resultados](docs/images/result.png)

### Escaneo asíncrono de la biblioteca de canciones

RhythmGame escanea tu biblioteca de canciones en segundo plano,
¡así que puedes empezar a jugar de inmediato!

# Compilación e instalación

Consulte el documento [DEV_ENGINE](DEV_ENGINE.md).

# Contribuciones

Consulte el documento [CONTRIBUTING](CONTRIBUTING.md).

# Licenciamiento

El proyecto se distribuye bajo la [licencia MIT](LICENSE.md).
