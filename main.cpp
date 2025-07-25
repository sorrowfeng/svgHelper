#include <QApplication>
#include <QDebug>

#include "svghelper.hpp"

int main(int argc, char* argv[]) {
  QApplication a(argc, argv);

  SvgHelper helper;
  helper.parseSvgImage("../../example.svg");
  auto svgPaths = helper.getSvgPathList();

  qDebug() << svgPaths;

  return a.exec();
}
