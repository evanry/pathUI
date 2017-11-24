#include "TextQtAnnotation.h"
#include "Annotation.h"
#include <QObject>
#include <QPainter>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QStyleOptionGraphicsItem>
#include "AnnotationWorkstationExtensionPlugin.h"
#include "io/multiresolutionimageinterface/MultiResolutionImage.h"
#include <iostream>
#include <QGraphicsTextItem>

TextQtAnnotation::TextQtAnnotation(PathologyViewer* viewer,const std::shared_ptr<Annotation>& annotation, QObject* parent, float scale) :
  QtAnnotation(annotation, parent, scale),
  _rectSize(3.),
  _rectColor(QColor("blue")),
  _rectSelectedColor(QColor("red")),
  _currentLOD(1.)
{
    _1viewer=viewer;
//  annotation->setColor("#000000");
//  AnnotationWorkstationExtensionPlugin* annot_plugin = dynamic_cast<AnnotationWorkstationExtensionPlugin*>(parent);
//  if (annot_plugin) {
//    if (std::shared_ptr<MultiResolutionImage> local_img = annot_plugin->getCurrentImage().lock()) {
//      _spacing = local_img->getSpacing();
//    }
//  }
}

QRectF TextQtAnnotation::boundingRect() const {
    QRectF bRect;
    if (_annotation) {
      QPointF tl(-4.5*_rectSize / _currentLOD, -4.5*_rectSize / _currentLOD);
      QPointF br(4.5*_rectSize / _currentLOD, 4.5*_rectSize / _currentLOD);
      bRect = QRectF(tl, br);
    }
    return bRect;
}

void TextQtAnnotation::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
  QWidget *widget) {

    if (_annotation) {
//      _currentLOD = option->levelOfDetailFromTransform(painter->worldTransform());
//      std::vector<Point> coords = _annotation->getCoordinates();
//          Point lastPoint = coords[0];
//          QPointF centerPoint = this->mapFromScene(lastPoint.getX()*_scale, lastPoint.getY()*_scale) ;
//    painter->setPen(Qt::NoPen);
//    painter->setBrush(QBrush(QColor(0, 0, 0, 135)));
    //painter->drawRect(boundingRect().adjusted(-5, -5, 5, 5));

//    QPainterPath* textPath=new QPainterPath();
//    QFont ft = QFont("Arial");
//    QString vtx=_1viewer->annotext;
//    QGraphicsTextItem* ttt=new QGraphicsTextItem(vtx);
//    ttt->setPos(centerPoint);
//    _1viewer->scene()->addItem(ttt);
//    textPath->addText(centerPoint,ft,vtx);
//    painter->drawRect(textPath->boundingRect().adjusted(-5,-5,5,5));
//    painter->setBrush(QBrush(Qt::white));
//    painter->drawPath(*textPath);
//    textPath=NULL;
    //QGraphicsTextItem::paint(painter, o, w);
 //     }
    }
}

