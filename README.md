# -
大作业要求的图形版贪吃蛇

## 修复说明

如果你遇到 exe 中中文显示乱码，请用下面方式编译（将源文件按 UTF-8 读取，编译到 CP936/GBK 执行字符集）：

```bat
:: Windows CMD
:: 使用你本机可用的 g++（示例：TDM-GCC-64）
g++ -I"C:\TDM-GCC-64\x86_64-w64-mingw32\include" -O2 -std=c++11 -finput-charset=UTF-8 -fexec-charset=CP936 *.cpp -o snake.exe -leasyx -lwinmm -lgdi32 -luser32 -lkernel32 -lws2_32
```

你也可以临时改用 `CP936` 的命令行执行：

```bat
chcp 936
```

如果你用了旧版 mingw（`easyx` 路径不同），把 `-I` 路径替换为你当前 MinGW 的 `easyx.h` 所在目录。

## 编译输出约定（不再生成新 exe）

为了避免每次都产生 `snake_xxx.exe`，统一使用固定输出名：

```bat
snake.exe
```

你可以直接用仓库里的脚本一键编译（默认覆盖 `snake.exe`）：

```bat
build_cn.bat
```

如果想手动覆盖同名文件：

```bat
g++ ... -o snake.exe ...
```
