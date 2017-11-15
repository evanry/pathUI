#ifndef RULERTOOL_H
#define RULERTOOL_H

#include "../interfaces/interfaces.h"

class RulerTool : public  ToolPluginInterface {
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "Diag.PathologyWorkstation.RulerTool/1.0")
  Q_INTERFACES(ToolPluginInterface)

public :
  std::string name();
  void mouseMoveEvent(QMouseEvent *event);
  void mousePressEvent(QMouseEvent *event);
  void mouseReleaseEvent(QMouseEvent *event);
  QAction* getToolButton();
};

#endif // RULERTOOL_H
