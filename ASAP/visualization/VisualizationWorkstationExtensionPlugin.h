#ifndef VISUALIZATIONWORKSTATIONEXTENSIONPLUGIN_H
#define VISUALIZATIONWORKSTATIONEXTENSIONPLUGIN_H

#include <memory>
#include "../interfaces/interfaces.h"
#include <QtNetwork/QTcpSocket>
#include <QTextEdit>

class QCheckBox;
class XmlRepository;
class QGraphicsPolygonItem;
class AnnotationList;

class VisualizationWorkstationExtensionPlugin : public WorkstationExtensionPluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "Diag.PathologyWorkstation.VisualizationWorkstationExtensionPlugin/1.0")
    Q_INTERFACES(WorkstationExtensionPluginInterface)

private :

  void addSegmentationsToViewer();
  void removeSegmentationsFromViewer();

  std::shared_ptr<MultiResolutionImage> _foreground;
  QDockWidget* _dockWidget;
  QCheckBox* _likelihoodCheckBox;
  QCheckBox* _segmentationCheckBox;
  float _opacity;
  float _window;
  float _level;
  float _foregroundChannel;
  bool  _renderingEnabled;
  float _foregroundScale;
  QString _currentLUT;
  std::shared_ptr<XmlRepository> _xmlRepo;
  std::shared_ptr<AnnotationList> _lst;
  QList<QGraphicsPolygonItem*> _polygons;
  std::vector<unsigned long long> _backgroundDimensions;
  void loadNewForegroundImage(const std::string& resultImagePth);
  void setDefaultVisualizationParameters(std::shared_ptr<MultiResolutionImage> img);
  QTcpSocket *cSocket;
  //QTextEdit *eInfo;
  void socketTrans(const char * filename);

public :
    bool initialize(PathologyViewer* viewer);
    VisualizationWorkstationExtensionPlugin();
    ~VisualizationWorkstationExtensionPlugin();
    QDockWidget* getDockWidget();


public slots:
    void onNewImageLoaded(std::weak_ptr<MultiResolutionImage> img, std::string fileName);
    void onImageClosed();
    void onEnableLikelihoodToggled(bool toggled);
    void onOpacityChanged(double opacity);
    void onEnableSegmentationToggled(bool toggled);
    void onOpenResultImageClicked();
    void onBeginClicked();
    void onLUTChanged(const QString& LUTname);
    void onWindowValueChanged(double window);
    void onLevelValueChanged(double level);
    void onChannelChanged(int channel);
    void readMessageFromTCPServer(QTcpSocket *cSocket);
    void displayError(QAbstractSocket::SocketError socketerror);

signals: 
    void changeForegroundImage(std::weak_ptr<MultiResolutionImage>, float scale);
};


#endif
