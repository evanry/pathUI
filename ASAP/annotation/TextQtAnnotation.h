#ifndef TEXTQTANNOTATION_H
#define TEXTQTANNOTATION_H
#include "QtAnnotation.h"
#include <QColor>
#include <memory>
#include "annotationplugin_export.h"
#include "../PathologyViewer.h"

class ANNOTATIONPLUGIN_EXPORT TextQtAnnotation : public QtAnnotation
{
  Q_OBJECT
public:
  TextQtAnnotation(PathologyViewer* viewer,const std::shared_ptr<Annotation>& annotation, QObject *parent, float scale = 1.0);
  QRectF boundingRect() const;

  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
    QWidget *widget);


private:
  PathologyViewer* _1viewer;
  float _rectSize;
  float _currentLOD;
  QColor _rectColor;
  QColor _rectSelectedColor;
};
#endif
