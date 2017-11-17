#ifndef VISUALIZATIONWORKSTATIONEXTENSIONPLUGIN_H
#define VISUALIZATIONWORKSTATIONEXTENSIONPLUGIN_H

#include <memory>
#include "pathologyworkstation.h"
#include "annotation/AnnotationWorkstationExtensionPlugin.h"
#include "../interfaces/interfaces.h"
#include <QtNetwork/QTcpSocket>
#include <QTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QProgressBar>
#include <QThread>
#include <QVBoxLayout>

class QCheckBox;
class QGroupBox;
class XmlRepository;
class QGraphicsPolygonItem;
class AnnotationList;
class AnnotationWorkstationExtensionPlugin;
class MultiResolutionImage;
class PathologyViewer;
class WorkstationExtensionPluginInterface;

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
  QLineEdit* info;
  QProgressBar* progress;
  QLabel* fName;
  QString f_name;
  QString qfilename;
  QVBoxLayout* verpro;
  QVBoxLayout* openedfile;
  QGroupBox* filelist;
  QCheckBox *of1;
  void socketTrans();
  std::shared_ptr<MultiResolutionImage> _img;
  QList<QtAnnotation*> _qtAnnotations;

public :
    bool initialize(PathologyViewer* viewer);
    bool initialize2(PathologyViewer* viewer,PathologyViewer* viewer3,PathologyViewer* viewer4);
    VisualizationWorkstationExtensionPlugin();
    ~VisualizationWorkstationExtensionPlugin();
    QDockWidget* getDockWidget();


public slots:
    void onNewImageLoaded(std::weak_ptr<MultiResolutionImage> img, std::string fileName);
    void onxmlLoaded(std::weak_ptr<MultiResolutionImage> img, std::string fileName);
    void onImageClosed();
    void onEnableLikelihoodToggled(bool toggled);
    void onOpacityChanged(double opacity);
    void onEnableSegmentationToggled(bool toggled);
    void onOpenResultImageClicked();
    void onBeginClicked();
    void onShowClicked(QString fn_);
    void onSendClicked(QString fileName);
    void oncloseclicked(QString fileName);
    void oncloseclicked2(QString fileName);
    void changeimg();
    void onLUTChanged(const QString& LUTname);
    void onWindowValueChanged(double window);
    void onLevelValueChanged(double level);
    void onChannelChanged(int channel);
    void sendFileName();
    void readMessageFromTCPServer();
    void displayError(QAbstractSocket::SocketError socketerror);
    void getfns();

signals: 
    void changeForegroundImage(std::weak_ptr<MultiResolutionImage>, float scale);
    void switchimg(QString);
    void retfns2(QString,QString);
    void retfns4(QString,QString,QString,QString);
};

class processThread : public QThread
{
    Q_OBJECT
public:
    processThread(QString fnm,QString ffnm,QProgressBar *progrs);
protected:
    void run();
private:
    QString flnm;
    QString fflnm;
    QProgressBar* prog;
    QTcpSocket *cSocket;
    volatile bool stopped;
    void socketTran();
    
public slots:
    void sendFileName();
    void readMessageFromTCPServer();
   // void setProVal(int);

signals:
    void valChanged(int);
    void xmldone(QString);
};


#endif

