#include "PanTool.h"
#include <QAction>
#include "../PathologyViewer.h"
#include <QApplication>

void PanTool::mouseMoveEvent(QMouseEvent *event) {
  if (_viewer) {
    if (_viewer->isPanning()) {
      _viewer->pan(event->pos());
      event->accept();
    }
  }
  if (_viewer2) {
    if (_viewer2->isPanning()) {
      _viewer2->pan(event->pos());
      event->accept();
    }
  }
  if (_viewer3) {
    if (_viewer3->isPanning()) {
      _viewer3->pan(event->pos());
      event->accept();
    }
  }
  if (_viewer4) {
    if (_viewer4->isPanning()) {
      _viewer4->pan(event->pos());
      event->accept();
    }
  }
}

void PanTool::mousePressEvent(QMouseEvent *event) {
  if (_viewer) {
    _viewer->togglePan(true, event->pos());
    event->accept();
  }
  if (_viewer2) {
    _viewer2->togglePan(true, event->pos());
    event->accept();
  }
  if (_viewer3) {
    _viewer3->togglePan(true, event->pos());
    event->accept();
  }
  if (_viewer4) {
    _viewer4->togglePan(true, event->pos());
    event->accept();
  }
}

void PanTool::mouseReleaseEvent(QMouseEvent *event) {
  if (_viewer) {
    _viewer->togglePan(false);
    event->accept();
  }
  if (_viewer2) {
    _viewer2->togglePan(false);
    event->accept();
  }
  if (_viewer3) {
    _viewer3->togglePan(false);
    event->accept();
  }
  if (_viewer4) {
    _viewer4->togglePan(false);
    event->accept();
  }
}

QAction* PanTool::getToolButton() {
  if (!_button) {
    _button = new QAction(QString::fromLocal8Bit("平移"), this);
    _button->setObjectName(QString::fromStdString(name()));
    _button->setIcon(QIcon(QPixmap(":/basictools_icons/pan.png")));
    _button->setShortcut(QApplication::translate("PathologyWorkstation", "Ctrl+P", 0));
  }
  return _button;
}

std::string PanTool::name() {
  return std::string("pan");
}

//void RectTool::mousePressEvent(QMouseEvent *event){
//    if (_viewer) {
//      _viewer->toggleRuler(true, event->pos());
//      event->accept();
//    }
//}

//void RectTool::mouseMoveEvent(QMouseEvent *event){
//    if (_viewer) {
//      if (_viewer->_ruler) {
//        _viewer->ruler(event->pos());
//        event->accept();
//      }
//    }
//}

//void RectTool::mouseReleaseEvent(QMouseEvent *event){
//    if (_viewer) {
//        _viewer->rulerDone(event->pos());
//      _viewer->toggleRuler(false);
//      event->accept();
//    }
//}

//QAction* RectTool::getToolButton() {
//  if (!_button) {
//    _button = new QAction(QString::fromLocal8Bit("矩形选区"), this);
//    _button->setObjectName(QString::fromStdString(name()));
//    _button->setIcon(QIcon(QPixmap(":/basictools_icons/pan.png")));
//    _button->setShortcut(QApplication::translate("PathologyWorkstation", "Ctrl+P", 0));
//  }
//  return _button;
//}

//std::string RectTool::name() {
//  return std::string("rect");
//}
