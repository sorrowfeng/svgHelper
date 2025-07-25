#include <QApplication>
#include <QDebug>

#include "svghelper.h"

int main(int argc, char* argv[]) {
  QApplication a(argc, argv);

  SvgHelper helper;
  helper.parseSvgImage("../../example.svg");
  auto svgPaths = helper.getSvgPathList();

  return a.exec();
}
