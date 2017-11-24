#include "TextAnnotationTool.h"
#include "QtAnnotation.h"
#include "AnnotationWorkstationExtensionPlugin.h"
#include <QAction>
#include <QPen>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QGraphicsItem>
#include "PolyQtAnnotation.h"
#include "Annotation.h"
#include "../PathologyViewer.h"
#include <math.h>
#include <numeric>
#include <iostream>
#include <QTimeLine>
#include "io/multiresolutionimageinterface/MultiResolutionImage.h"

TextAnnotationTool::QGraphicsTextItemWithBackground::QGraphicsTextItemWithBackground(const QString &text) :
QGraphicsTextItem(text) { }

QRectF TextAnnotationTool::QGraphicsTextItemWithBackground::boundingRect() const {
  return QGraphicsTextItem::boundingRect().adjusted(-5, -5, 5, 5);
}

void TextAnnotationTool::QGraphicsTextItemWithBackground::paint(QPainter *painter, const QStyleOptionGraphicsItem *o, QWidget *w) {
  painter->setPen(Qt::NoPen);
  painter->setBrush(QBrush(QColor(0, 0, 0, 135)));
  painter->drawRect(boundingRect().adjusted(-5, -5, 5, 5));
  painter->setBrush(QBrush(Qt::white));
  QGraphicsTextItem::paint(painter, o, w);
}

TextAnnotationTool::TextAnnotationTool(AnnotationWorkstationExtensionPlugin* annotationPlugin, PathologyViewer* viewer) :
AnnotationTool(annotationPlugin, viewer)
{
}


void TextAnnotationTool::mousePressEvent(QMouseEvent *event) {
   AnnotationTool::mousePressEvent(event);
   QPointF scenePos = _viewer->mapToScene(event->pos());
   //addCoordinate(scenePos);
   zhushi=new QGraphicsTextItemWithBackground("");
   zhushi->font().setPixelSize(20);
   zhushi->setPlainText(_viewer->annotext);
   zhushi->setZValue(std::numeric_limits<float>::max());
   zhushi->setFlag(QGraphicsItem::ItemIgnoresTransformations);
   zhushi->setDefaultTextColor(Qt::white);
   zhushi->setX(scenePos.x());
   zhushi->setY(scenePos.y());
   _viewer->scene()->addItem(zhushi);
   if (_generating) {
     _annotationPlugin->finishAnnotation();
     _start = Point(-1, -1);
     _last = _start;
     _generating = false;
   }
   event->accept();
}

//void TextAnnotationTool::cancelAnnotation() {
//  if (_generating) {
//    AnnotationTool::cancelAnnotation();
//    if (zhushi) {
//      zhushi->hide();
//      _viewer->scene()->removeItem(zhushi);
//      delete zhushi;
//      zhushi = NULL;
//    }
//  }
//}

//void TextAnnotationTool::addCoordinate(const QPointF& scenePos) {

//    _annotationPlugin->getGeneratedAnnotation()->addCoordinate(scenePos.x() / _viewer->getSceneScale(), scenePos.y() / _viewer->getSceneScale());
//    _annotationPlugin->finishAnnotation();
//    if (zhushi) {
//      _viewer->scene()->removeItem(zhushi);
//      delete zhushi;
//      zhushi = NULL;
//    }
//    _start = Point(-1, -1);
//    _last = _start;
//    _generating = false;

//}

QAction* TextAnnotationTool::getToolButton() {
  if (!_button) {
    _button = new QAction(QString::fromLocal8Bit("ÎÄ×Ö×¢ÊÍ"), this);
    _button->setObjectName(QString::fromStdString(name()));
    _button->setIcon(QIcon(QPixmap(":/AnnotationWorkstationExtensionPlugin_icons/measure.png")));
  }
  return _button;
}

std::string TextAnnotationTool::name() {
  return std::string("textannotation");
}
