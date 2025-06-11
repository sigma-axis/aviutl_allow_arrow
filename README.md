# Allow Arrow AviUtl プラグイン

拡張編集のタイムラインウィンドウや設定ダイアログで，方向キーを含むショートカットが上手く動かなかったのを修正するプラグインです．

[ダウンロードはこちら．](https://github.com/sigma-axis/aviutl_allow_arrow/releases) [紹介動画．](https://www.nicovideo.jp/watch/sm43284722)


##  動作要件

- AviUtl 1.10 + 拡張編集 0.92

  http://spring-fragrance.mints.ne.jp/aviutl

  （AviUtl 1.00 や拡張編集 0.93rc1 でも動作するが 1.10 / 0.92 を推奨．）

- Visual C++ 再頒布可能パッケージ（\[2015/2017/2019/2022\] の x86 対応版が必要）

  https://learn.microsoft.com/ja-jp/cpp/windows/latest-supported-vc-redist

##  このプラグインでできること

...の前に，このプラグインがないと出来なかったことをば．

### 何ができなかったの？

次のような挙動がありました．

1.  AviUtl の設定で `Ctrl + →` などの矢印キーを含むショートカットキーを何かのコマンドに登録する．この `Ctrl + →` を入力すると...
1.  メインウィンドウにフォーカスがある場合，登録したコマンドが実行される（正常）．
1.  拡張編集のタイムラインウィンドウや設定ダイアログにフォーカスがある場合は登録したコマンドが実行されず，現在フレーム位置が1フレームだけ右に動く．

つまり，`Ctrl + →` のショートカットキーがタイムラインや設定ダイアログでは使えませんでした．他にも，次の場合にはショートカットキーが動作しません．

- 拡張編集タイムラインウィンドウにフォーカスがあるとき，`←`, `↑`, `→`, `↓` が含まれるショートカットキーが動作しない．代わりに現在フレームの前後移動やレイヤー方向の縦スクロールが実行される．

- 設定ダイアログにフォーカスがあるとき，`←`, `→` が含まれるショートカットキーが動作しない．代わりに「選択中のオブジェクト内に入るような現在フレームの前後移動」が実行される．

- 設定ダイアログにフォーカスがあるとき，`Alt + ↑`, `Alt + ↓` が含まれるショートカットキーが動作しない．（こちらは特に代わりに起こる動作はない模様．）

これらの特殊挙動を取り除くのがこのプラグインの目的です．

### どうしてこうなった？

そもそも通常のショートカットキーがタイムラインや設定ダイアログで使えること自体が，拡張編集の意図した配慮です．次のような仕組みが元々働いています．

1.  タイムラインウィンドウや設定ダイアログでキー入力が行われる．
1.  そのキー入力メッセージがタイムラインや設定ダイアログに送られる．
1.  タイムラインや設定ダイアログのメッセージ処理部分で，そのメッセージをメインウィンドウに転送する．
1.  メインウィンドウ側の処理部分で，それをショートカットキーと解釈してコマンドを実行する．

ただし矢印キーなどの一部キー入力は，タイムラインや設定ダイアログが特別扱いして (3) のメインウィンドウへの転送がなされません．このためショートカットキーと認識されず，そのコマンドも実行されません．その代わりフレーム移動や縦スクロールなど，当該ウィンドウ独自の専用処理が実行されます．

### このプラグインがやっていること

拡張編集タイムラインウィンドウや設定ダイアログのメッセージ処理部分を乗っ取って，矢印などの一部キー入力を検知したら本来の処理に回さずにメインウィンドウに流すような処理をしています．

ただしそのままだと代償として，矢印キー入力で本来実行されるフレーム移動やレイヤー方向の縦スクロールなどが使えなくなるため，これらを実行するコマンドを新たなショートカットキー登録先として追加しています．

##  導入方法

以下のフォルダのいずれかに `allow_arrow.auf` と `allow_arrow.ini` をコピーしてください．

1.  `aviutl.exe` のあるフォルダ
1.  (1) のフォルダにある `plugins` フォルダ
1.  (2) のフォルダにある任意のフォルダ

また導入後は，導入前の挙動と合わせるために次のショートカットキー設定をしておくのをお勧めします．

1.  `↑` キーを「タイムライン上入力」に割り当てる．
    - デフォルトだと「削除フレームを飛ばして前のフレームに移動 (↑)」に割り当てられているはずですが，拡張編集がある今日日これは無用の長物でしかないので設定を解除してもまず問題ありません．

1.  `↓` キーを「タイムライン下入力」に割り当てる．
    - デフォルトだと「削除フレームを飛ばして次のフレームに移動 (↓)」に割り当てられているはずですが，同様に設定を解除してまず問題ありません．

1.  `←`, `→` を「前のフレームに移動 (←)」「次のフレームに移動 (→)」に割り当てる．
    - デフォルトで割り当てられてるはずですし，余程のことがない限り変更しないと思われますが念のため．

(1), (2) の設定によってメインウィンドウにフォーカスがあるときでも `↑` や `↓` 入力でレイヤー方向の縦スクロールができるようになります．

##  設定ファイル

テキストエディタで `allow_arrow.ini` を編集することで，このプラグインによる介入項目を個別に設定できます．

1.  `[Timeline]`

    拡張編集のタイムラインウィンドウにフォーカスがあるときの設定で，`hookLeft`, `hookRight`, `hookUp`, `hookDown`, `hookDelete` の5項目があります．

1.  `[SettingDlg]`

    設定ダイアログにフォーカスがあるときの設定で，`hookLeft`, `hookRight`, `hookUp`, `hookDown` の4項目があります．

これらの項目に `1` を設定すると対応したキー入力に対してこのプラグインが介入し，結果ショートカットキーが動くようになります．`0` を指定したり行を削除 / コメントアウトすると介入が無効化されます．初期値は全て `1`.

##  追加コマンド

以下のコマンドが追加されます．

![編集→Allow Arrowのメニュー](https://github.com/user-attachments/assets/0695cccf-71f6-4db0-8929-228c63cb2faf)

それぞれタイムラインウィンドウ，設定ダイアログで矢印キーを押したときの元々の動作を実行します．

- **タイムライン左入力 / タイムライン右入力**

  それぞれ AviUtl に元々備わっている「前のフレーム (←)」 / 「次のフレームに移動 (→)」と同等の動作です．これを使う場面はないと思いますが，念のため追加しています．

- **タイムライン上入力 / タイムライン下入力**

  タイムラインをレイヤー方向（縦方向）に1レイヤー分だけスクロールします．

- **タイムラインDelete入力**

  タイムラインにある現在選択中のオブジェクトを削除します．
  - 「拡張編集」 :arrow_right: 「オブジェクトの削除(メインウィンドウ)」とは異なり，メインウィンドウにフォーカスがなくても動作します．

- **設定ダイアログ左入力 / 設定ダイアログ右入力**

  前後のフレームに移動するコマンドですが，現在選択中オブジェクトが表示される範囲に留まるという性質があります．ユニークな機能ではあるものの，あまり使う機会はないと思われます．

##  その他

1.  プラグイン名について

    - 読み方は「アラウアロー」です．「アローアロー」ではありません．

    - この名前，結構気に入ってます．

    - ~~日本語名を「矢印ショトカ解放」とかいうクソダサネームにしようか迷ったけど踏み留まった．~~

1.  拙作の[TLショトカ移動](https://github.com/sigma-axis/aviutl_tl_walkaround)を使いやすくするために作成しました．

1.  他にも拡張編集による特別扱いを受けたキーはありますが，仕組みが違うなどの理由から手を付けていません．

##  改版履歴

- **v1.20** (2025-06-11)

  - タイムラインでの Delete キー入力への対応を追加 (初期状態では無効化されています).

- **v1.12** (2024-03-10)

  - アンロード順序によっては終了時にエラー落ちしていたのを修正 (v1.11 からのバグ)．

- **v1.11** (2024-02-28)

  - 初期化周りを少し改善．

  - 終了処理で一部の関数呼び出しタイミングが適切ではなかったのを修正．

- **v1.10** (2024-01-09)

  - 設定ダイアログ上の `Alt + ↑`, `Alt + ↓` 入力も同様に転送されていなかったのを見つけたので，それにも対処．

    更新の際は，`allow_arrow.ini` の `[SettingDlg]` セクションに以下の追記をしてください:

    ```ini
    hookUp=1
    hookDown=1
    ```

  - コード整理．

- **v1.00** (2024-01-05)

  - 初版．


##  ライセンス

このプログラムの利用・改変・再頒布等に関しては MIT ライセンスに従うものとします．

---

The MIT License (MIT)

Copyright (C) 2024-2025 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

https://mit-license.org/


# Credits

##  aviutl_exedit_sdk

https://github.com/ePi5131/aviutl_exedit_sdk

---

1条項BSD

Copyright (c) 2022
ePi All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
THIS SOFTWARE IS PROVIDED BY ePi “AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL ePi BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# 連絡・バグ報告

- GitHub: https://github.com/sigma-axis
- Twitter: https://twitter.com/sigma_axis
- nicovideo: https://www.nicovideo.jp/user/51492481
- Misskey.io: https://misskey.io/@sigma_axis
