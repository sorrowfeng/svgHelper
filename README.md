# svgHelper
用Qt解析SVG图片中的所有路径信息

# 使用示例
``` cpp
SvgHelper svgHelper;
svgHelper.parseSvgImage("../temp.svg");
svgHelper.getSvgPathList(); // 获取QPainterPath的List
svgHelper.getSvgImage();    // 获取预览图
```