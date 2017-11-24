#ifndef TEXTANNOTATIONTOOL_H
#define TEXTANNOTATIONTOOL_H

#include "AnnotationTool.h"
#include "core/Point.h"
#include <QGraphicsTextItem>

class AnnotationWorkstationExtensionPlugin;
class PathologyViewer;
class QGraphicsLineItem;
class QLineEdit;

class ANNOTATIONPLUGIN_EXPORT TextAnnotationTool : public AnnotationTool {
  Q_OBJECT

public :
  TextAnnotationTool(AnnotationWorkstationExtensionPlugin* annotationPlugin, PathologyViewer* viewer);
  virtual std::string name();
  virtual QAction* getToolButton();
  void mousePressEvent(QMouseEvent *event);
 // void cancelAnnotation();

private:

  class QGraphicsTextItemWithBackground : public QGraphicsTextItem
  {
  public:
    QGraphicsTextItemWithBackground(const QString &text);
    QRectF boundingRect() const;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *o, QWidget *w);
  };

//  void addCoordinate(const QPointF& scenePos);
  QGraphicsTextItemWithBackground* zhushi;
};

#endif
