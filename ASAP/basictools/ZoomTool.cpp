#include "ZoomTool.h"
#include <QAction>
#include "../PathologyViewer.h"
#include <iostream>
#include <QApplication>


ZoomTool::ZoomTool() :
_zooming(false),
_accumZoom(0),
_prevZoomPoint(QPoint(0, 0))
{
}

void ZoomTool::mouseMoveEvent(QMouseEvent *event) {
  if (_viewer && _zooming) {
    float delta = event->pos().y() - _prevZoomPoint.y();
    _prevZoomPoint = event->pos();
    _accumZoom += delta/20.;
    if (_accumZoom * delta < 0) {
      _accumZoom = 0;
    }
    if (_accumZoom > 1.0 || _accumZoom < -1.0) {
      _viewer->zoom(_accumZoom);
      _accumZoom = 0;
    }
    event->accept();
  }
  if ( _zooming2 &&_viewer2) {
    float delta = event->pos().y() - _prevZoomPoint2.y();
    _prevZoomPoint2 = event->pos();
    _accumZoom2 += delta/20.;
    if (_accumZoom2 * delta < 0) {
      _accumZoom2 = 0;
    }
    if (_accumZoom2 > 1.0 || _accumZoom2 < -1.0) {
      _viewer2->zoom(_accumZoom2);
      _accumZoom2 = 0;
    }
    event->accept();
  }
}

void ZoomTool::mousePressEvent(QMouseEvent *event) {
  if (_viewer) {

    _zooming = true;
    _accumZoom = 0;
    _prevZoomPoint = event->pos();
    _viewer->setCursor(QCursor(Qt::CursorShape::SizeVerCursor));
    _viewer->_zoomToViewPos = event->pos();
    _viewer->_zoomToScenePos = _viewer->mapToScene(event->pos());
    event->accept();
  }
  if (_viewer2) {
    _zooming2 = true;
    _accumZoom2 = 0;
    _prevZoomPoint2 = event->pos();
    _viewer2->setCursor(QCursor(Qt::CursorShape::SizeVerCursor));
    _viewer2->_zoomToViewPos = event->pos();
    _viewer2->_zoomToScenePos = _viewer2->mapToScene(event->pos());
    event->accept();
  }
}

void ZoomTool::mouseReleaseEvent(QMouseEvent *event) {
  if (_viewer) {
    _zooming = false;
    _prevZoomPoint = QPoint(0, 0);
    _viewer->setCursor(Qt::ArrowCursor);
    event->accept();
  }
  if (_viewer2) {
    _zooming2 = false;
    _prevZoomPoint2 = QPoint(0, 0);
    _viewer2->setCursor(Qt::ArrowCursor);
    event->accept();
  }
}

QAction* ZoomTool::getToolButton() {
  if (!_button) {
    _button = new QAction(QString::fromLocal8Bit("Ëõ·Å"), this);
    _button->setObjectName(QString::fromStdString(name()));
    _button->setIcon(QIcon(QPixmap(":/basictools_icons/zoom.png")));
    _button->setShortcut(QApplication::translate("PathologyWorkstation", "Ctrl+Z", 0));
  }
  return _button;
}

std::string ZoomTool::name() {
  return std::string("zoom");
}
