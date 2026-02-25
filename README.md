# MicroQuickJS on RISC-V
---

在 RISC-V MCU 上运行轻量级 JavaScript 运行时 [MicroQuickJS](https://github.com/bellard/mquickjs).

MCU型号为 GD32VF103CBT6, 128K ROM, 32K RAM.

---

## 编译

`js_stdlib.h`和`mquickjs_atom.h`这两个头文件需要通过程序生成:

首先编译生成程序:

```bash
gcc -o mquickjs_build.exe mquickjs_build.c mqjs_stdlib.c
```

生成头文件:

```bash
mquickjs_build.exe -a -m32 > mquickjs_atom.h
mquickjs_build.exe -m32 > js_stdlib.h
```

`js_stdlib.h` 在`mquickjs.c`中include即可.


