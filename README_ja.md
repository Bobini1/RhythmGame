[English](./README.md) | [简体中文](./README.zh-CN.md) | **日本語** | [Español](./README_es.md)

<p align=center>
    <a href="https://github.com/Bobini1/RhythmGame/actions"><img src="https://github.com/Bobini1/RhythmGame/actions/workflows/ci.yml/badge.svg"/></a>
    <a href="https://github.com/Bobini1/RhythmGame/blob/master/LICENSE.md"><img src="https://img.shields.io/github/license/Bobini1/RhythmGame"/></a>
    <a href="https://github.com/Bobini1/RhythmGame/releases/latest"><img alt="GitHubのダウンロード数（全アセット、全リリース）" src="https://img.shields.io/github/downloads/Bobini1/RhythmGame/total"></a>
    <a href="https://aur.archlinux.org/packages/rhythmgame-git"><img alt="AURでの人気度" src="https://img.shields.io/aur/popularity/rhythmgame-git?logo=arch-linux"></a>
    <a href="https://github.com/Bobini1/RhythmGame/blob/master/flake.nix"><img alt="Nix" src="https://img.shields.io/badge/Nix-5277C3?logo=nixos&logoColor=fff"></a>
    <br>
    <a href="https://discord.gg/bDxmuSzXBW"><img src="https://img.shields.io/discord/1410743088686829661.svg?color=7289DA&label=RhythmGame%20Community&logo=Discord"/></a>
    <a href="https://rhythmgame.eu"><img src="https://img.shields.io/website?url=https%3A%2F%2Frhythmgame.eu&label=IR"/></a>
</p>

# RhythmGame

[English](README.md) | [简体中文](README.zh-CN.md)

WindowsおよびLinux向けの、カスタマイズ可能なBMSプレーヤーです。

BMSは初めてですか？BMSの概要やどのようにしてプレイ用の曲を見つけるかを学ぶために、w氏が作成した[Beatoraja English Guide](https://github.com/wcko87/beatoraja-english-guide/wiki/BMS-Overview)をご覧ください。

## 機能

### カスタマイズ可能なテーマ

ゲームプレイ中にF2キーを押して要素を移動させることで、デフォルトのテーマをカスタマイズできます！

![カスタマイズモード（曲: wa. - Black Lotus）](docs/images/customize.webp)

[QML](https://doc.qt.io/qt-6/qmlreference.html)を使って、独自のカスタムテーマを作成することも可能です。  
興味がある方はぜひご連絡ください。導入のサポートを致します！  
参考までに、[デフォルトテーマ](https://github.com/Bobini1/RhythmGame/tree/master/share/RhythmGame/themes/Default)を利用できます。  
詳細については、[DEV_THEME.md](DEV_THEME.md)ドキュメントをご覧ください。

このゲームでは、CSV形式のLunatic Rave 2およびBeatorajaスキンもサポートしています。

![Lunatic Rave 2 skin - LR2 Default - プレイ](docs/images/lr2-play.png)

![Lunatic Rave 2のスキン - LR2 Default - 選択](docs/images/lr2.png)

### BokutachiおよびLR2IRによるインターネットランキング対応

世界中のプレイヤーと競い合いましょう！RhythmGameにはhttps://rhythmgame.euに独自のIRサーバーがありますが、[Bokutachi](https://boku.tachi.ac/)に自分のスコアを提出したり、[Lunatic Rave 2インターネットランキング](http://www.dream-pro.info/~lavalse/LR2IR/search.cgi)のスコアを確認したりすることもできます。

[![インターネットランキング](docs/images/ranking.png)](https://rhythmgame.eu)

|                      ランキング                       |                      オンライン統計データ                       |
|:--------------------------------------------------:|:-------------------------------------------------------:|
|![ゲーム内ランキング](docs/images/ranking-ingame.png) |![ゲーム内ランキング統計](docs/images/ranking-stats.png) |

### Lunatic Rave 2に基づくルール

タイミングウィンドウやゲージはLunatic Rave 2/Lr2orajaと同じ仕様なので、これらのゲームのスコアと簡単に比較できます。

### ローカル対戦モード

友達と一緒にプレイしよう！曲選択画面でスタートを2回押すとバトルモードが有効になります。

![ローカルバトルモード](docs/images/battle.png)

### テーブルのサポート

RhythmGameはBMSテーブルをネイティブにサポートしています。
設定画面でリンクを貼り付けるだけです。

![Tables](docs/images/tables.png)

![コース](docs/images/course.png)

### スムーズなスケーリング

すべての解像度に対応！F11キーを押してフルスクリーンモードに切り替えられます。

![スケーリング（曲：isocosa - data lake）](docs/images/resize.webp)

### 翻訳言語

RhythmGameはデフォルトで英語、ポーランド語、簡体中国語に対応しています。
ご自身の言語への翻訳を手伝ってほしい場合は、ぜひご連絡ください！

![言語選択](docs/images/languages.webp)

### 美しいデフォルトテーマ

[Shimi999](https://github.com/Shimi9999/GenericTheme) および [souki202](https://github.com/souki202/my_beatoraja_skin) の作品をベースに、
RhythmGameのデフォルトテーマにはBMSをプレイするための必要な機能がすべて備わっています。

![曲の選択](docs/images/select.png)

![結果画面](docs/images/result.png)

### 曲ライブラリの非同期スキャン

RhythmGameはバックグラウンドで楽曲ライブラリをスキャンするため、すぐにプレイを開始できます！

# ビルドとインストール

[DEV_ENGINE](DEV_ENGINE.md)ドキュメントをご覧ください。

# 貢献方法

[CONTRIBUTING](CONTRIBUTING.md)ドキュメントをご覧ください。

# ライセンス

このプロジェクトは [MIT ライセンス](LICENSE.md) のもとで配布されています。
