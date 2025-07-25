#ifndef SVGHELPER_H
#define SVGHELPER_H

#include <QDomDocument>
#include <QPainter>
#include <QPainterPath>
#include <QSvgRenderer>
#include <QSvgWidget>
#include <QVector>
#include <QXmlStreamReader>
#include <QtXml>

class SvgHelper {
 public:
  SvgHelper() = default;
  ~SvgHelper() = default;

  void parseSvgImage(const QString& filepath);

  QList<QPainterPath> getSvgPathList() const;
  QImage getSvgImage();

  QList<QList<QPointF>> getSvgPointList() const;

 private:
  void parseSVGTag(QDomElement e, QString tagname);
  void parseSvgPath(const QString& path, QPainterPath& paintPath);
  void dealParsePainter(QPainterPath& path, QString line);
  QVector<float> segmentationCoordinates(QString value);
  float getValueWithoutUnit(QString input);
  double radian(double ux, double uy, double vx, double vy);
  int svgArcToCenterParam(double x1, double y1, double rx, double ry,
                          double phi, double fA, double fS, double x2,
                          double y2, double& cx_out, double& cy_out,
                          double& startAngle_out, double& deltaAngle_out);
  double getAngleWithPoints(double x1, double y1, double x2, double y2,
                            double x3, double y3);

  QString filepath;
  QPainterPath paintPath;
  QList<QPointF> testpathlist;
  QPointF nowPositon = QPointF(0, 0);
  QPointF pathStartPosition = QPointF(0, 0);
  QPointF lastControlPosition = QPointF(0, 0);
  QList<QPainterPath> svgPathList;
  QList<QList<QPointF>> svgPointList;
};

#endif  // SVGHELPER_H
