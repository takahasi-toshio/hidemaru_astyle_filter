#秀丸エディタ用の変換モジュール（C/C++/C#/Javaソースコード整形）
ソースコード整形ツールArtistic Style 3.1（DLL版）を利用した変換モジュールです。

Artistic Styleについては以下を参照してください。

http://astyle.sourceforge.net/

以下の点でEXE版を使用した場合と動作が異なります。

1. 先頭行のインデントを維持するようにしているので、ソースコードの部分的な整形にもある程度使用できます。
2. 整形のオプションは変換モジュールのパラメータで指定します。付属のマクロとArtistic Styleのドキュメントを参考にしてください。ただし、indent=spacesとindent=tabはファイルタイプ別の設定を参照するので、パラメータでは指定しないでください。
3. UTF-8に変換してから整形処理を行っているため、マルチバイト文字列（特にsjis）に起因する問題は生じないはずです。

astyle.dllはastyle_filter.hmf（hmf64）と同じフォルダに置いてください。

この変換モジュールのソースコードは以下のリポジトリに配置してあります。

https://github.com/takahasi-toshio/hidemaru_astyle_filter
