#include "RulerTool.h"
#include <QAction>
#include "../PathologyViewer.h"

void RulerTool::mouseMoveEvent(QMouseEvent *event) {
  if (_viewer) {
    if (_viewer->_ruler) {
      _viewer->ruler(event->pos());
      event->accept();
    }
  }
//  if (_viewer2) {
//    if (_viewer2->_ruler) {
//        _viewer2->ruler(event->pos());
//      event->accept();
//    }
//  }
}

void RulerTool::mousePressEvent(QMouseEvent *event) {
  if (_viewer) {
    _viewer->toggleRuler(true,event->pos());
    event->accept();
  }
//  if (_viewer2) {
//    _viewer2->toggleRuler(true,event->pos());
//    event->accept();
//  }
}

void RulerTool::mouseReleaseEvent(QMouseEvent *event) {
  if (_viewer) {
      if (_viewer->_ruler) {
        _viewer->rulerDone(event->pos());
      }
    _viewer->toggleRuler(false);
    event->accept();
  }
//  if (_viewer2) {
//      if (_viewer2->_ruler) {
//        _viewer2->rulerDone(event->pos());
//      }
//    _viewer2->toggleRuler(false);
//    event->accept();
//  }
}

QAction* RulerTool::getToolButton() {
  if (!_button) {
    _button = new QAction(QString::fromLocal8Bit("¾ØÐÎÑ¡¿ò"), this);
    _button->setObjectName(QString::fromStdString(name()));
    _button->setIcon(QIcon(QPixmap(":/basictools_icons/measure.png")));
  }
  return _button;
}

std::string RulerTool::name() {
  return std::string("ruler");
}
