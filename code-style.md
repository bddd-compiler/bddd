# code style

## 前言

因为是代码协作，大家肯定会看互相写的代码，为了大家看彼此的代码都能看下去而不反胃，我们先约法三章，把该有的代码编写规范说在前头，有助于以后看代码互相都减少一点血压。

这个项目想用C++17来写，尽量使用现代C++的解决方案，因此有一些约束，包括但不限于：

- 不要传裸指针，而是使用`std::shared_ptr`或`std::unique_ptr`
- 尽量不要使用`printf`或`scanf`（除非在debug的时候需要），使用C++的流

## 命令规范

- 创建文件的时候，多个单词之间用短横杠分隔（类似`symbol-table.cpp`这种）
- enum类型里面的variant用全大写，单词之间用下划线分隔（类似`FUNC_CALL`这种）
- 方法名称要用驼峰式命名，用于多个类之间交互的重要方法第一个单词开头也大写
- 变量不要用驼峰式命名，用全小写，下划线分隔
- 成员变量开头加个m（`m_varname`这样）
- 全局变量开头加个g（`g_builtin_funcs`这样）

## 注释规范

- 写TODO注释的时候，把自己的名字也标上去，自己的TODO自己补锅（）

> ```cpp
> // TODO(kl@gmail.com): Use a "*" here for concatenation operator.
> // TODO(Zeke) change this to use relations.
> // TODO(bug 12345): remove the "Last visitors" feature
> ```

- 如果一个变量可能有特殊值，需要在注释加以说明（比如-1表示找不到就要顺手写进注释）
- 实现过程的注释暂时不做要求吧

## commit前要做的事情

- 过一遍clang-format，把代码先格式化了再提交（`.clang-format`已经有了）
- 好好看一遍属于自己的TODOs
- 认真写commit message（这很重要）

