# 模块导入

在 .tx 文件的顶层使用 import 导入 .tx 源码模块或 .txh 接口：

```tx
import "string.txh"

def main() -> int {
    print(contains("hello", "ell"))
    return 0
}
```

导入时可用 as 命名模块，并以 别名.函数(...) 调用：

```tx
import "../modules/math.txh" as math
import "../modules/offset.txh" as offset

def main() -> int {
    print(math.add(2, 3))
    print(offset.add(2, 3))
    return 0
}
```

导入路径相对写出 import 的文件所在目录解析，推荐使用正斜杠。导入相对 `.txh` 路径时，例如 `import "io.txh"` 或 `import "xx/xx.txh"`，先查找导入文件所在目录；找不到时，再查找编译器可执行文件所在目录下的 `stdlib/`，并保留路径中的子目录。若本地文件存在，就使用本地文件，即使它有语法错误也不会回退到标准库。绝对路径和向上跳出标准库目录的相对路径只按原路径解析。导入可以继续导入其他文件；同一个实际文件在一次编译中只加载一次。任何循环导入都报错，包括自导入、A→B→A 和 A→B→C→A；诊断会显示完整依赖链。txc check 和正常编译都会加载并检查导入的文件。通常导入 .txh；没有接口文件的简单源码模块仍可直接导入 .tx。

未使用 as 的导入将名称直接引入当前模块；本模块自身声明优先于未命名导入。若多个未命名导入模块提供同名声明，使用该名称时报歧义错误，即使函数参数类型不同也不会跨模块合并重载。使用 as 的导入只通过别名访问，可让不同模块拥有同名函数、结构体或类；同一模块内的别名不能重复，也不能与本模块声明重名。同名 .txh 与 .tx 共享模块别名，别名不向其他模块传递。结构体和类的类型与构造也可写为 别名.类型名。被导入的文件不能声明 main，入口函数由主文件提供。路径必须是双引号字符串，目前不支持路径转义和模块级私有声明。

## `.txh` 与配对 `.tx` 的分工

导入 `name.txh` 时，编译器会查找同目录、同名的 `name.tx`。两者存在时按下表组织；`.txh` 是对外可见的接口，`.tx` 提供实现。

| 声明 | `name.txh` | 配对的 `name.tx` |
| --- | --- | --- |
| `struct` | 写完整字段定义及运算符签名 | 有运算符时重写相同字段和运算符签名，并提供运算符方法体；没有运算符时**不要重写**该结构体 |
| 普通函数 | 只写签名，不写 `{ ... }` | 按相同参数名、参数类型及返回类型写函数体；所有接口重载都要实现 |
| `class` | 写完整的继承列表、访问修饰符、字段和方法签名；方法不写 `{ ... }` | **重新写出同一个类声明**，保持继承列表、字段顺序、访问修饰符、方法顺序和签名一致，并给具体方法写函数体 |
| `interface` / `abstract class` | 接口方法和抽象方法只写签名 | 若有配对文件，同样重新声明；抽象方法仍不写函数体，具体方法写函数体 |

配对 `.tx` 不能增加 `.txh` 未声明的函数重载、类或接口，也不能遗漏 `.txh` 中的类或接口。含运算符的 `struct` 必须在配对文件中逐字段、逐运算符对应；不含运算符的 `struct` 只保留在 `.txh`。函数和方法的无返回值类型可写 `-> void` 或省略；返回类型不能单独用于区分重载。

### `struct` 文件示例

[point.txh](../examples/struct_module/point.txh) 定义字段并声明函数：

```tx
struct point {
    x: int
    y: int
}

def move_x(value: point, delta: int) -> point
```

同名 [point.tx](../examples/struct_module/point.tx) **不重复**没有运算符成员的 `struct point`，直接使用接口中的类型：

```tx
def move_x(value: point, delta: int) -> point {
    return point(value.x + delta, value.y)
}
```

入口文件导入接口，例如 [struct 模块示例](../examples/struct_module/main.tx) 中的 `import "point.txh" as geometry`，然后使用 `geometry.point(...)` 与 `geometry.move_x(...)`。如果 `.txh` 只有没有运算符的 `struct`、没有需要实现的函数，可以不创建配对 `.tx`。含运算符成员的结构体模块见[运算符模块示例](../examples/operator_module/main.tx)。

### `class` 文件示例

[counter.txh](../examples/class_module/counter.txh) 写完整布局和方法签名，**不写方法体**：

```tx
class counter {
    public:
        value: int
        def init(start: int)
        def increment() -> int
}
```

同名 [counter.tx](../examples/class_module/counter.tx) 重写相同声明，并在具体方法上添加方法体：

```tx
class counter {
    public:
        value: int
        def init(start: int) {
            self.value = start
        }
        def increment() -> int {
            self.value++
            return self.value
        }
}
```

入口文件导入 `counter.txh`，见 [class 模块示例](../examples/class_module/main.tx)。多继承、`virtual`、`override`、方法重载、`deinit` 也遵循同样规则：在两边写一致的声明，具体方法体只写在 `.tx`。若 `.txh` 中的类含有任何具体方法（包括 `init`、`deinit`），就必须提供同名 `.tx`。

### 纯接口、抽象类和单文件模块

只有抽象方法的 `interface` 或 `abstract class` 可以只写在 `.txh`，不用创建同名 `.tx`；见 [tagged.txh](../examples/advanced_class_module/tagged.txh)。没有具体方法的普通 `class`（例如只有字段的类）也可以只放在 `.txh`。只要类或抽象类包含具体方法，就需要配对 `.tx`。配对文件中的类和接口必须逐一对应，即使某个接口本身只有抽象方法，也不能在已有配对 `.tx` 中省去它。

实现文件可直接使用 `.txh` 已导入的类型。例如 [holder.txh](../examples/advanced_class_module/holder.txh) 导入 `tagged.txh`，配对 [holder.tx](../examples/advanced_class_module/holder.tx) 可以直接写 `class holder : tagged`。实现专用的其他依赖可在 `.tx` 自己导入；同一个模块别名不要在 `.txh` 和 `.tx` 各声明一次。

不创建 `.txh` 时，也可以把 `struct`、`class`、函数及其方法体全部写在一个 `.tx` 中，并直接导入这个 `.tx`。如果已有同名 `.txh`，其他模块应导入 `.txh`，由编译器自动加载配对 `.tx`。

可用 txc check 路径.txh 单独检查接口及其配对实现；.txh 不能单独编译为可执行文件。

标准库的公开目录为 `tx/stdlib/`，只提供 .txh；实际实现编译为同一 `tx/` 目录下的静态库，生成可执行文件时由随 TX 分发的链接器链接。没有配对 `.tx` 的 `.txh` 仍可提供纯 `struct`、无具体方法的 `class`、`interface` 和纯抽象类声明；其中的普通函数签名则被视为二进制接口。当前后端只接受已提供的标准库二进制函数，其他二进制函数会在编译时报错。

标准库二进制函数按接口相对路径和函数名识别。不同标准库模块可以分别声明同名函数；同时使用时仍需遵守前述导入歧义规则。

可运行示例见 [导入与输入示例](../examples/import_io.tx)和[跨模块类与接口示例](../examples/advanced_class_module/main.tx)。
同名函数的别名调用见 [别名导入示例](../examples/import_alias.tx)；本地同名接口优先的示例见 [本地接口示例](../examples/local_priority/main.tx)。
