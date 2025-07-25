// svghelper.hpp

#ifndef SVGHELPER_HPP
#define SVGHELPER_HPP

#include <QChar>
#include <QDebug>
#include <QDomDocument>
#include <QFile>
#include <QImage>
#include <QList>
#include <QPainter>
#include <QPainterPath>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QString>
#include <QSvgRenderer>
#include <QVector>
#include <QXmlStreamReader>
#include <QtMath>  // For qRadiansToDegrees
#include <QtXml>
#include <cmath>  // For M_PI, sqrt, abs, atan2, acos

class SvgHelper {
 public:
  SvgHelper() = default;
  ~SvgHelper() = default;

  void parseSvg(const QString& filepath);

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

// --- Implementation ---

namespace {  // Anonymous namespace for internal linkage

const QList<QChar> kCmdList = {'M', 'm', 'L', 'l', 'H', 'h', 'V',
                               'v', 'C', 'c', 'S', 's', 'Q', 'q',
                               'T', 't', 'A', 'a', 'Z', 'z'};
const QList<QChar> kCmdkTypeList = {'M', 'L', 'H', 'V', 'C',
                                    'S', 'Q', 'T', 'A', 'Z'};
const QList<QChar> kSplitList = {' ',  '-',  ',',
                                 '\r', '\n', '\t'};  // Added \n explicitly
const QList<QString> kUnitList = {"em", "px", "%",  "cm",
                                  "mm", "in", "pt", "pc"};
const QList<QString> kTypeList = {"path", "rect",    "circle",  "ellipse",
                                  "line", "polygon", "polyline"};

#define ABSOLUTE_COORDINATES 1
#define RELATIVE_COORDINATES 2

// Helper function to get SVG size
QSize matchSize(const QString& path) {
  QSvgRenderer renderer(path);
  if (renderer.isValid()) {
    QSize size = renderer.defaultSize();
    QRect viewBox = renderer.viewBox();
    // Often defaultSize already reflects viewBox, but check if viewBox is more explicit
    // if (viewBox.isValid() && !viewBox.isEmpty() && viewBox.size() != size) {
    //     size = viewBox.size(); // Use viewBox size if different and valid
    // }
    if (size.isValid() && !size.isEmpty()) {
      return size;
    }
  }
  qWarning() << "Could not determine valid size for SVG:" << path
             << ". Using default 100x100.";
  return QSize(100, 100);
}

}  // namespace

void SvgHelper::parseSvg(const QString& filepath) {
  this->filepath = filepath;
  // Clear previous data
  svgPathList.clear();
  svgPointList.clear();

  QFile svgFile(filepath);
  if (svgFile.open(QFile::ReadOnly)) {
    QDomDocument doc;
    if (doc.setContent(&svgFile)) {
      svgFile.close();
      QDomElement root = doc.documentElement();
      QDomNode node = root.firstChild();
      while (!node.isNull()) {
        if (node.isElement()) {
          QDomElement e = node.toElement();
          QString tagname = e.tagName();
          if (kTypeList.contains(tagname)) {
            parseSVGTag(e, tagname);
          } else {
            // Search for nested elements of interest
            foreach (const QString& type, kTypeList) {
              QDomNodeList list = e.elementsByTagName(type);
              for (int i = 0; i < list.count(); i++) {
                QDomNode n = list.at(i);
                if (n.isElement()) {  // Extra check for safety
                  parseSVGTag(n.toElement(), n.nodeName());
                }
              }
            }
          }
        }
        node = node.nextSibling();
      }
    } else {
      qWarning() << "Failed to parse SVG content from file:" << filepath;
    }
  } else {
    qWarning() << "Failed to open SVG file for reading:" << filepath;
  }
}

QImage SvgHelper::getSvgImage() {
  QSize imagesize = matchSize(filepath);
  if (imagesize.isEmpty() || imagesize.width() <= 0 ||
      imagesize.height() <= 0) {
    qWarning() << "Invalid SVG size detected for" << filepath
               << ", using default 100x100";
    imagesize = QSize(100, 100);
  }

  QImage image(imagesize, QImage::Format_ARGB32);
  image.fill(Qt::white);  // Fill with white background

  QPainter p(&image);
  if (!p.isActive()) {
    qCritical() << "Failed to activate QPainter on QImage";
    return QImage();  // Return null image on failure
  }
  p.setRenderHint(QPainter::Antialiasing, true);  // Often useful for SVGs
  p.setPen(Qt::black);

  foreach (const QPainterPath& ppath, svgPathList) {
    p.drawPath(ppath);
  }
  // p.end() is called automatically by QPainter destructor
  return image;
}

void SvgHelper::parseSVGTag(QDomElement e, QString tagname) {
  // Clear data for this specific tag
  paintPath.clear();
  testpathlist.clear();

  if (QString::compare(tagname, "path", Qt::CaseInsensitive) == 0) {
    QString pathvalue = e.attribute("d");
    parseSvgPath(pathvalue, paintPath);

  } else if (QString::compare(tagname, "rect", Qt::CaseInsensitive) == 0) {
    // Default values if attributes are missing
    float x = getValueWithoutUnit(e.attribute("x", "0"));
    float y = getValueWithoutUnit(e.attribute("y", "0"));
    float width = getValueWithoutUnit(e.attribute("width"));
    float height = getValueWithoutUnit(e.attribute("height"));
    // rx/ry default to -1 to indicate "not specified"
    float rx = getValueWithoutUnit(e.attribute("rx", "-1"));
    float ry =
        getValueWithoutUnit(e.attribute("ry", "-1"));  // Corrected from "rx"

    // Handle invalid dimensions
    if (width <= 0 || height <= 0) {
      qWarning() << "Invalid rect dimensions (w:" << width << ", h:" << height
                 << ")";
      return;  // Don't add an invalid path
    }

    // Normalize rx/ry according to SVG spec
    if (rx < 0)
      rx = (ry >= 0) ? ry : 0.0f;
    if (ry < 0)
      ry = (rx >= 0) ? rx : 0.0f;
    // Constrain radii to prevent overlap (SVG spec)
    rx = qMin(rx, width / 2.0f);
    ry = qMin(ry, height / 2.0f);

    QList<QPointF> pointsForList;

    if (rx <= 0 && ry <= 0) {
      // Sharp corners rectangle
      paintPath.addRect(x, y, width, height);
      // Add points for rect
      pointsForList.append(QPointF(x, y));
      pointsForList.append(QPointF(x + width, y));
      pointsForList.append(QPointF(x + width, y + height));
      pointsForList.append(QPointF(x, y + height));
      pointsForList.append(QPointF(x, y));  // Close

    } else {
      // Rounded rectangle using QPainterPath::addRoundedRect
      paintPath.addRoundedRect(QRectF(x, y, width, height), rx, ry);
      // Approximate points for rounded rect (more complex to get exact arc points)
      QPainterPath tempPath;
      tempPath.addRoundedRect(QRectF(x, y, width, height), rx, ry);
      // Sample points along the path
      const int numSamples =
          qMax(20, static_cast<int>(tempPath.length() / 2.0));  // Heuristic
      for (int i = 0; i <= numSamples; ++i) {
        qreal percent = static_cast<qreal>(i) / numSamples;
        pointsForList.append(tempPath.pointAtPercent(percent));
      }
      // Ensure the list closes properly if needed by your logic
      if (!pointsForList.isEmpty()) {
        pointsForList.append(pointsForList.first());
      }
    }
    svgPointList.append(pointsForList);

  } else if (QString::compare(tagname, "circle", Qt::CaseInsensitive) == 0) {
    float cx = getValueWithoutUnit(e.attribute("cx", "0"));
    float cy = getValueWithoutUnit(e.attribute("cy", "0"));
    float r = getValueWithoutUnit(e.attribute("r"));

    QList<QPointF> pointsForList;
    if (r > 0) {
      paintPath.addEllipse(QPointF(cx, cy), r, r);
      // Add points for circle
      QPainterPath tempPath;
      tempPath.addEllipse(QPointF(cx, cy), r, r);
      const int numSamples = 36;  // Or based on path length
      for (int i = 0; i <= numSamples; ++i) {
        qreal percent = static_cast<qreal>(i) / numSamples;
        pointsForList.append(tempPath.pointAtPercent(percent));
      }
      // Ensure the list closes properly if needed by your logic
      if (!pointsForList.isEmpty()) {
        pointsForList.append(pointsForList.first());
      }
      svgPointList.append(pointsForList);
    } else {
      qWarning() << "Circle with invalid radius r=" << r;
    }

  } else if (QString::compare(tagname, "ellipse", Qt::CaseInsensitive) == 0) {
    float cx = getValueWithoutUnit(e.attribute("cx", "0"));
    float cy = getValueWithoutUnit(e.attribute("cy", "0"));
    float rx = getValueWithoutUnit(e.attribute("rx"));
    float ry = getValueWithoutUnit(e.attribute("ry"));

    QList<QPointF> pointsForList;
    if (rx > 0 && ry > 0) {
      paintPath.addEllipse(QPointF(cx, cy), rx, ry);
      // Add points for ellipse
      QPainterPath tempPath;
      tempPath.addEllipse(QPointF(cx, cy), rx, ry);
      const int numSamples = 36;  // Or based on path length
      for (int i = 0; i <= numSamples; ++i) {
        qreal percent = static_cast<qreal>(i) / numSamples;
        pointsForList.append(tempPath.pointAtPercent(percent));
      }
      // Ensure the list closes properly if needed by your logic
      if (!pointsForList.isEmpty()) {
        pointsForList.append(pointsForList.first());
      }
      svgPointList.append(pointsForList);
    } else {
      qWarning() << "Ellipse with invalid radii rx=" << rx << ", ry=" << ry;
    }

  } else if (QString::compare(tagname, "line", Qt::CaseInsensitive) == 0) {
    float x1 = getValueWithoutUnit(e.attribute("x1", "0"));
    float y1 = getValueWithoutUnit(e.attribute("y1", "0"));
    float x2 = getValueWithoutUnit(e.attribute("x2", "0"));
    float y2 = getValueWithoutUnit(e.attribute("y2", "0"));

    paintPath.moveTo(x1, y1);
    paintPath.lineTo(x2, y2);
    // Add points for line
    QList<QPointF> pointsForList;
    pointsForList.append(QPointF(x1, y1));
    pointsForList.append(QPointF(x2, y2));
    svgPointList.append(pointsForList);

  } else if (QString::compare(tagname, "polygon", Qt::CaseInsensitive) == 0 ||
             QString::compare(tagname, "polyline", Qt::CaseInsensitive) == 0) {
    QString value = e.attribute("points");
    QVector<float> vPos = segmentationCoordinates(value);
    QList<QPointF> pointsForList;

    if (vPos.size() >= 2) {
      QPointF startPoint(vPos[0], vPos[1]);
      paintPath.moveTo(startPoint);
      pointsForList.append(startPoint);

      for (int i = 2; i < vPos.size() - 1; i += 2) {
        QPointF point(vPos[i], vPos[i + 1]);
        paintPath.lineTo(point);
        pointsForList.append(point);
      }

      if (QString::compare(tagname, "polygon", Qt::CaseInsensitive) == 0 &&
          !pointsForList.isEmpty()) {
        paintPath.closeSubpath();  // Connect last point to first for polygon
        pointsForList.append(startPoint);  // Add start point to close the list
      } else if (!pointsForList.isEmpty()) {
        // For polyline, just ensure the path ends at the last point
        // The path already does this implicitly with lineTo calls.
        // QPainterPath keeps track of the current point.
      }
      svgPointList.append(pointsForList);
    } else {
      qWarning() << "Insufficient points for" << tagname << ":" << value;
    }
  }

  // Add the constructed path to the main list if it's not empty
  if (!paintPath.isEmpty()) {
    svgPathList.append(paintPath);
  }
  // Note: paintPath and testpathlist are cleared at the beginning of the function
  // or will be cleared for the next tag. No need to clear here explicitly.
}

void SvgHelper::parseSvgPath(const QString& path, QPainterPath& paintPath) {
  QString cmdLine = "";
  for (QChar c : path) {  // Use range-based loop for clarity
    if (kCmdList.contains(c)) {
      if (!cmdLine.isEmpty()) {
        dealParsePainter(paintPath, cmdLine);
        cmdLine.clear();
      }
    }
    cmdLine += c;
  }
  if (!cmdLine.isEmpty()) {
    dealParsePainter(paintPath, cmdLine);
    cmdLine.clear();
  }
}

void SvgHelper::dealParsePainter(QPainterPath& path, QString line) {
  line = line.trimmed();
  if (line.isEmpty())
    return;  // Safety check

  QString cmdStr = line.mid(0, 1);
  QString valueStr = line.mid(1).trimmed();  // Trim value part as well

  if (cmdStr.isEmpty())
    return;  // Should not happen after checks, but safe

  QChar cmd = cmdStr.at(0);
  bool isAbsolute = cmd.isUpper();
  int coordinates = isAbsolute ? ABSOLUTE_COORDINATES : RELATIVE_COORDINATES;

  // Find command type index (case-insensitive)
  int cmdIndex = kCmdkTypeList.indexOf(cmd.toUpper());
  if (cmdIndex == -1) {
    qWarning() << "Unknown SVG path command:" << cmd;
    return;
  }

  QVector<float> vNum = segmentationCoordinates(valueStr);

  switch (cmdIndex) {
    case 0: {  // M/m - moveto
      bool hasLineFlag = vNum.length() > 2;
      bool lineto = false;
      while (vNum.length() >= 2) {
        QPointF point(vNum[0], vNum[1]);
        if (isAbsolute) {
          nowPositon = point;
        } else {
          nowPositon += point;
        }

        if (!lineto) {
          path.moveTo(nowPositon);
          pathStartPosition =
              nowPositon;  // Set start position for potential 'Z'
          testpathlist.append(nowPositon);
        } else {
          path.lineTo(nowPositon);
          testpathlist.append(nowPositon);
        }

        vNum.remove(0, 2);
        lineto = true;  // Subsequent pairs are implicit linetos
      }
      break;
    }
    case 1: {  // L/l - lineto
      while (vNum.length() >= 2) {
        QPointF point(vNum[0], vNum[1]);
        if (isAbsolute) {
          nowPositon = point;
        } else {
          nowPositon += point;
        }
        path.lineTo(nowPositon);
        testpathlist.append(nowPositon);
        // QPainterPath automatically sets current point, no need for explicit moveTo
        vNum.remove(0, 2);
      }
      break;
    }
    case 2: {  // H/h - horizontal lineto
      while (!vNum.isEmpty()) {
        float x = vNum[0];
        if (isAbsolute) {
          nowPositon.setX(x);
        } else {
          nowPositon.rx() += x;  // rx() returns a reference
        }
        path.lineTo(nowPositon);
        testpathlist.append(nowPositon);
        vNum.remove(0, 1);
      }
      break;
    }
    case 3: {  // V/v - vertical lineto
      while (!vNum.isEmpty()) {
        float y = vNum[0];
        if (isAbsolute) {
          nowPositon.setY(y);
        } else {
          nowPositon.ry() += y;
        }
        path.lineTo(nowPositon);
        testpathlist.append(nowPositon);
        vNum.remove(0, 1);
      }
      break;
    }
    case 4: {  // C/c - cubic bezier curveto
      while (vNum.length() >= 6) {
        QPointF c1(vNum[0], vNum[1]);
        QPointF c2(vNum[2], vNum[3]);
        QPointF endPoint(vNum[4], vNum[5]);

        if (isAbsolute) {
          // c1, c2, endPoint are already absolute
        } else {
          // Convert relative coordinates to absolute
          c1 += nowPositon;
          c2 += nowPositon;
          endPoint += nowPositon;
        }

        path.cubicTo(c1, c2, endPoint);
        lastControlPosition = c2;  // Store last control point for potential 'S'
        nowPositon = endPoint;     // Update current position

        // Sample points for testpathlist (approximation)
        QPainterPath tempSegment;
        tempSegment.moveTo(nowPositon);  // Start from current position
        tempSegment.cubicTo(c1, c2, endPoint);
        const int numSamples =
            qMax(5, static_cast<int>(tempSegment.length() / 2.0));
        for (int i = 0; i <= numSamples; ++i) {
          qreal percent = static_cast<qreal>(i) / numSamples;
          testpathlist.append(tempSegment.pointAtPercent(percent));
        }

        vNum.remove(0, 6);
      }
      break;
    }
    case 5: {  // S/s - smooth cubic bezier curveto
      while (vNum.length() >= 4) {
        QPointF c2(vNum[0], vNum[1]);
        QPointF endPoint(vNum[2], vNum[3]);

        // Reflect previous control point (if available, otherwise use current point)
        QPointF c1 = (lastControlPosition == QPointF(0, 0))
                         ? nowPositon
                         : (2 * nowPositon) - lastControlPosition;

        if (isAbsolute) {
          c2 = c2;  // Already absolute
        } else {
          c2 += nowPositon;
        }

        path.cubicTo(c1, c2, endPoint);
        lastControlPosition = c2;
        nowPositon = endPoint;

        // Sample points (approximation)
        QPainterPath tempSegment;
        tempSegment.moveTo(nowPositon);  // Start from current position
        tempSegment.cubicTo(c1, c2, endPoint);
        const int numSamples =
            qMax(5, static_cast<int>(tempSegment.length() / 2.0));
        for (int i = 0; i <= numSamples; ++i) {
          qreal percent = static_cast<qreal>(i) / numSamples;
          testpathlist.append(tempSegment.pointAtPercent(percent));
        }

        vNum.remove(0, 4);
      }
      break;
    }
    case 6: {  // Q/q - quadratic bezier curveto
      while (vNum.length() >= 4) {
        QPointF cPoint(vNum[0], vNum[1]);
        QPointF endPoint(vNum[2], vNum[3]);

        if (isAbsolute) {
          // cPoint, endPoint are already absolute
        } else {
          cPoint += nowPositon;
          endPoint += nowPositon;
        }

        path.quadTo(cPoint, endPoint);
        lastControlPosition = cPoint;  // Store for potential 'T'
        nowPositon = endPoint;

        // Sample points (approximation)
        QPainterPath tempSegment;
        tempSegment.moveTo(nowPositon);  // Start from current position
        tempSegment.quadTo(cPoint, endPoint);
        const int numSamples =
            qMax(5, static_cast<int>(tempSegment.length() / 2.0));
        for (int i = 0; i <= numSamples; ++i) {
          qreal percent = static_cast<qreal>(i) / numSamples;
          testpathlist.append(tempSegment.pointAtPercent(percent));
        }

        vNum.remove(0, 4);
      }
      break;
    }
    case 7: {  // T/t - smooth quadratic bezier curveto
      while (vNum.length() >= 2) {
        QPointF endPoint(vNum[0], vNum[1]);

        // Reflect previous control point (if available, otherwise use current point)
        QPointF cPoint = (lastControlPosition == QPointF(0, 0))
                             ? nowPositon
                             : (2 * nowPositon) - lastControlPosition;

        if (isAbsolute) {
          endPoint = endPoint;  // Already absolute
        } else {
          endPoint += nowPositon;
        }

        path.quadTo(cPoint, endPoint);
        lastControlPosition = cPoint;
        nowPositon = endPoint;

        // Sample points (approximation)
        QPainterPath tempSegment;
        tempSegment.moveTo(nowPositon);  // Start from current position
        tempSegment.quadTo(cPoint, endPoint);
        const int numSamples =
            qMax(5, static_cast<int>(tempSegment.length() / 2.0));
        for (int i = 0; i <= numSamples; ++i) {
          qreal percent = static_cast<qreal>(i) / numSamples;
          testpathlist.append(tempSegment.pointAtPercent(percent));
        }

        vNum.remove(0, 2);
      }
      break;
    }
    case 8: {  // A/a - elliptical arc
      while (vNum.length() >= 7) {
        float rx = vNum[0];
        float ry = vNum[1];
        float x_axis_rotation = vNum[2];
        float large_arc_flag = vNum[3];
        float sweep_flag = vNum[4];
        QPointF endPoint(vNum[5], vNum[6]);

        if (isAbsolute) {
          // endPoint is already absolute
        } else {
          endPoint += nowPositon;
        }

        if (rx <= 0 || ry <= 0) {
          // SVG spec: If rx or ry is 0, treat as line
          path.lineTo(endPoint);
          testpathlist.append(nowPositon);
          testpathlist.append(endPoint);
          nowPositon = endPoint;
          vNum.remove(0, 7);
          continue;
        }

        double cx, cy, start_angle, delta_angle;
        int result = svgArcToCenterParam(nowPositon.x(), nowPositon.y(), rx, ry,
                                         x_axis_rotation, large_arc_flag,
                                         sweep_flag, endPoint.x(), endPoint.y(),
                                         cx, cy, start_angle, delta_angle);

        if (result == 1) {  // Success
          start_angle = qRadiansToDegrees(start_angle);
          delta_angle = qRadiansToDegrees(delta_angle);
          QRectF rect(cx - rx, cy - ry, 2 * rx, 2 * ry);
          if (delta_angle != 0) {
            // QPainterPath::arcTo draws counter-clockwise from startAngle for angleSpan.
            // SVG sweep_flag determines direction.
            // We need to adjust angles based on the coordinate system difference.
            double qtStartAngle = -start_angle;
            double qtSpanAngle = -delta_angle;
            if (sweep_flag ==
                0) {  // If sweep flag is 0 (counter-clockwise), negate the span
              qtSpanAngle = -qtSpanAngle;
            }

            path.arcTo(rect, qtStartAngle, qtSpanAngle);

            // Sample points along the arc for testpathlist
            QPainterPath tempArcPath;
            tempArcPath.arcMoveTo(rect, qtStartAngle);  // Move to start
            tempArcPath.arcTo(rect, qtStartAngle, qtSpanAngle);
            const int numSamples =
                qMax(5, static_cast<int>(tempArcPath.length() / 2.0));
            for (int i = 0; i <= numSamples; ++i) {
              qreal percent = static_cast<qreal>(i) / numSamples;
              testpathlist.append(tempArcPath.pointAtPercent(percent));
            }
          }
        } else {
          // Fallback if arc calculation fails
          path.lineTo(endPoint);
          testpathlist.append(nowPositon);
          testpathlist.append(endPoint);
          qWarning() << "Arc calculation failed for A/a command";
        }

        nowPositon = endPoint;
        vNum.remove(0, 7);
      }
      break;
    }
    case 9: {  // Z/z - closepath
      path.closeSubpath();  // This automatically draws a line back to the start of the current subpath
      testpathlist.append(
          path.currentPosition());  // Append the point it closed to (start of subpath)
      // Note: QPainterPath handles the start position internally for Z command.
      // If you need the explicit start point for your logic, you might need to track it differently.
      // The provided `pathStartPosition` is an attempt, but might not be perfect for complex paths.
      // path.lineTo(pathStartPosition); // Alternative, but closeSubpath is preferred.
      // testpathlist.append(pathStartPosition);
      break;
    }
  }
}

QVector<float> SvgHelper::segmentationCoordinates(QString value) {
  // 将科学记数法暂时替换, 防止分割出错
  if (value.contains("e", Qt::CaseInsensitive)) {
    value.replace("e-", "[KXJSFF]");
    value.replace("E-", "[KXJSFF]");
  }
  if (value.contains(" -")) {
    value.replace(" -", " [KGFS]");
  }
  if (value.contains(",-")) {
    value.replace(",-", ",[DHFS]");
  }
  QVector<float> vPos;
  QString num = "";
  foreach (QChar c, value) {
    if (kSplitList.contains(c)) {
      num = num.trimmed();
      if (!num.isEmpty()) {
        if (num.contains(','))
          num.remove(',');
        vPos.append(getValueWithoutUnit(num));
        num.clear();
      }
    }
    num += c;
  }
  num = num.trimmed();
  if (num.contains(','))
    num.remove(',');
  if (!num.isEmpty())
    vPos.append(getValueWithoutUnit(num));
  return vPos;
}

float SvgHelper::getValueWithoutUnit(QString input) {
  // 将科学记数法替换回来
  if (input.contains("[KXJSFF]"))
    input.replace("[KXJSFF]", "e-");
  if (input.contains("[KGFS]"))
    input.replace("[KGFS]", "-");
  if (input.contains("[DHFS]"))
    input.replace("[DHFS]", "-");
  if (input.isEmpty())
    return -1;
  QString str = input;
  foreach (QString unit, kUnitList) {
    if (str.contains(
            unit, Qt::CaseInsensitive)) {  // Make unit removal case-insensitive
      str.remove(unit);
    }
  }
  bool ok;
  float result = str.toFloat(&ok);
  if (!ok) {
    // qDebug() << "Failed to convert string to float:" << str;
    return 0.0f;  // Or another default/error value if -1 is ambiguous
  }
  return result;
}

/**
svg : [A | a] (rx ry x-axis-rotation large-arc-flag sweep-flag x2 y2)+
(x1 y1)圆弧路径起点
(x2 y2)圆弧路径终点
rx 椭圆弧的X半轴长度
ry 椭圆弧的Y半轴长度
x-axis-rotation 椭圆弧X轴方向的旋转角度
large-arc-flag 标记是否大弧段
sweep-flag 标记是否顺时针绘制
sample :  svgArcToCenterParam(200,200,50,50,0,1,1,300,200, output...)
*/
double SvgHelper::radian(double ux, double uy, double vx, double vy) {
  double dot = ux * vx + uy * vy;
  double mod = sqrt((ux * ux + uy * uy) * (vx * vx + vy * vy));
  if (mod == 0.0)
    return 0.0;                                     // Prevent division by zero
  double rad = acos(qBound(-1.0, dot / mod, 1.0));  // Clamp to [-1, 1] for acos
  if (ux * vy - uy * vx < 0.0)
    rad = -rad;
  return rad;
}

int SvgHelper::svgArcToCenterParam(double x1, double y1, double rx, double ry,
                                   double phi, double fA, double fS, double x2,
                                   double y2, double& cx_out, double& cy_out,
                                   double& startAngle_out,
                                   double& deltaAngle_out) {
  double cx, cy, startAngle, deltaAngle;
  const double PIx2 = 2.0 * M_PI;

  if (rx < 0) {
    rx = -rx;
  }
  if (ry < 0) {
    ry = -ry;
  }
  if (rx == 0.0 || ry == 0.0) {  // invalid arguments
    return -1;                   // Indicate failure
  }

  double s_phi = sin(phi * M_PI / 180.0);  // Convert degrees to radians
  double c_phi = cos(phi * M_PI / 180.0);
  double hd_x = (x1 - x2) / 2.0;  // half diff of x
  double hd_y = (y1 - y2) / 2.0;  // half diff of y
  double hs_x = (x1 + x2) / 2.0;  // half sum of x
  double hs_y = (y1 + y2) / 2.0;  // half sum of y
  // F6.5.1
  double x1_ = c_phi * hd_x + s_phi * hd_y;
  double y1_ = c_phi * hd_y - s_phi * hd_x;
  // F.6.6 Correction of out-of-range radii
  //   Step 3: Ensure radii are large enough
  double lambda = (x1_ * x1_) / (rx * rx) + (y1_ * y1_) / (ry * ry);
  if (lambda > 1) {
    rx = rx * sqrt(lambda);
    ry = ry * sqrt(lambda);
  }
  double rxry = rx * ry;
  double rxy1_ = rx * y1_;
  double ryx1_ = ry * x1_;
  double sum_of_sq = rxy1_ * rxy1_ + ryx1_ * ryx1_;  // sum of square
  if (sum_of_sq == 0) {                              // Prevent division by zero
    return -1;                                       // Indicate failure
  }
  double coe = sqrt(abs((rxry * rxry - sum_of_sq) / sum_of_sq));
  if (fA == fS) {
    coe = -coe;
  }
  // F6.5.2
  double cx_ = coe * rxy1_ / ry;
  double cy_ = -coe * ryx1_ / rx;
  // F6.5.3
  cx = c_phi * cx_ - s_phi * cy_ + hs_x;
  cy = s_phi * cx_ + c_phi * cy_ + hs_y;
  double xcr1 = (x1_ - cx_) / rx;
  double xcr2 = (x1_ + cx_) / rx;
  double ycr1 = (y1_ - cy_) / ry;
  double ycr2 = (y1_ + cy_) / ry;
  // F6.5.5
  startAngle = radian(1.0, 0.0, xcr1, ycr1);
  // F6.5.6
  deltaAngle = radian(xcr1, ycr1, -xcr2, -ycr2);
  while (deltaAngle > PIx2) {
    deltaAngle -= PIx2;
  }
  while (deltaAngle < 0.0) {
    deltaAngle += PIx2;
  }
  if (fS == false || fS == 0) {  // If sweep flag is 0 (counter-clockwise)
    deltaAngle -= PIx2;
  }
  // Ensure deltaAngle is within [-2PI, 2PI]
  while (deltaAngle > PIx2) {
    deltaAngle -= PIx2;
  }
  while (deltaAngle < -PIx2) {
    deltaAngle += PIx2;
  }

  cx_out = cx;
  cy_out = cy;
  startAngle_out = startAngle;
  deltaAngle_out = deltaAngle;
  return 1;  // Indicate success
}

double SvgHelper::getAngleWithPoints(double x1, double y1, double x2, double y2,
                                     double x3, double y3) {
  // This function seems unused in the provided code, but kept for completeness.
  double theta = atan2(x1 - x3, y1 - y3) - atan2(x2 - x3, y2 - y3);
  if (theta > M_PI)
    theta -= 2 * M_PI;
  if (theta < -M_PI)
    theta += 2 * M_PI;
  theta = abs(theta * 180.0 / M_PI);
  if (y2 <= y3) {
    theta = 360.0 - theta;
  }
  return theta;
}

QList<QList<QPointF>> SvgHelper::getSvgPointList() const {
  return svgPointList;
}

QList<QPainterPath> SvgHelper::getSvgPathList() const {
  return svgPathList;
}

#endif  // SVGHELPER_HPP
