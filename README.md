# SVGHelper - Qt SVG 路径解析工具

![Qt](https://img.shields.io/badge/Qt-5.12%2B-brightgreen) 
![License](https://img.shields.io/badge/license-Apache2.0-blue)

一个轻量级的 Qt 工具类，用于解析 SVG 文件中的所有路径信息并转换为 QPainterPath 对象。

## ✨ 功能特性

- 🖼️ 完整解析 SVG 文件中的路径数据
- 📐 将 SVG 路径转换为 QPainterPath 对象
- 🔍 支持获取路径上的任意点坐标
- 🎨 可生成 SVG 预览图像

## 🚀 快速开始

### 基本用法

```cpp
#include "svghelper.h"

// 初始化解析器
SvgHelper svgHelper;

// 解析SVG文件
svgHelper.parseSvgImage("example.svg");

// 获取所有路径
QList<QPainterPath> paths = svgHelper.getSvgPathList();

// 获取预览图像
QPixmap preview = svgHelper.getSvgImage();
```

## 📝 示例代码
### 1. 在窗口中绘制SVG路径

```cpp
// MainWindow.h
#include <QMainWindow>
#include "svghelper.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    void paintEvent(QPaintEvent *event) override;
    
private:
    QList<QPainterPath> svgPaths;
};

// MainWindow.cpp
MainWindow::MainWindow(QWidget *parent) 
    : QMainWindow(parent) {
    SvgHelper helper;
    helper.parseSvgImage(":/assets/icon.svg");
    svgPaths = helper.getSvgPathList();
}

void MainWindow::paintEvent(QPaintEvent *) {
    if(!svgPaths.isEmpty()) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        // 绘制所有路径
        for(const auto &path : svgPaths) {
            painter.drawPath(path);
        }
    }
}
```

### 2. 获取路径上的点坐标
```cpp
// 获取第一个路径上的等间距点
if(!svgPaths.isEmpty()) {
    for(double t = 0; t <= 1.0; t += 0.01) {
        QPointF point = svgPaths[0].pointAtPercent(t);
        qDebug() << "Point at" << t << ":" << point;
    }
}
```
## 🖼️ 效果演示


| 原始SVG图像 | 解析渲染效果 |
|-------------|-------------|
| ![原图](https://img-blog.csdnimg.cn/ddb7442ad1064a5186f0a48c468131d2.png) | ![解析后](https://img-blog.csdnimg.cn/886708a50a3b40489b83571f60f63d28.png?x-oss-process=image/watermark,type_ZHJvaWRzYW5zZmFsbGJhY2s,shadow_50,text_Q1NETiBA5pyo6aOO5Y-v5Y-v,size_20,color_FFFFFF,t_70,g_se,x_16) |

## ✨ Star历史

[![Star History Chart](https://api.star-history.com/svg?repos=sorrowfeng/svgHelper&type=Date)](https://www.star-history.com/#sorrowfeng/svgHelper&Date)

## 📌 实现原理
SVGHelper 通过 Qt 的 QSvgRenderer 类解析 SVG 文件，然后：

1. 遍历 SVG 中的所有 <path> 元素
2. 将每个路径转换为 QPainterPath 对象
3. 保留原始路径的所有几何信息
4. 支持对路径进行精确的数学操作

## 🛠️ 应用场景
- SVG 图形编辑工具开发
- 矢量图形分析处理
- CAD/CAM 系统集成
- 数据可视化
- 机器人路径规划

## 📦 集成方式
1. 将 svghelper.h 和 svghelper.cpp 添加到您的项目中
2. 在需要使用的文件中包含头文件：
```cpp
#include "svghelper.h"
```
3. 确保项目配置中启用了 SVG SvgWidgets Xml 模块：
```qmake
QT += svg
```
或 CMake:
```cmake
find_package(Qt5 REQUIRED COMPONENTS Svg SvgWidgets Xml)
```

## 📜 开源协议
本项目采用 Apache2.0 许可证 开源，您可以自由使用于商业和非商业项目。
