#include "VisualizationWorkstationExtensionPlugin.h"
#include "pathologyworkstation.h"
#include "annotation/DotAnnotationTool.h"
#include "annotation/PolyAnnotationTool.h"
#include "annotation/PointSetAnnotationTool.h"
#include "annotation/SplineAnnotationTool.h"
#include "annotation/AnnotationService.h"
#include "annotation/AnnotationList.h"
#include "annotation/AnnotationGroup.h"
#include "annotation/QtAnnotation.h"
#include "annotation/QtAnnotationGroup.h"
#include "annotation/Annotation.h"
#include "annotation/ImageScopeRepository.h"
#include "annotation/AnnotationToMask.h"
#include "annotation/DotQtAnnotation.h"
#include "annotation/PolyQtAnnotation.h"
#include "annotation/PointSetQtAnnotation.h"
#include "../PathologyViewer.h"
#include <QDockWidget>
#include <QtUiTools>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QGraphicsPolygonItem>
#include <QPolygonF>
#include <QPushButton>
#include <QFrame>
#include <QGroupBox>
#include <QComboBox>
#include <QPushButton>
#include <QFileDialog>
#include <QHBoxLayout>

#include "interfaces/interfaces.h"
#include "WSITileGraphicsItemCache.h"
#include "config/ASAPMacros.h"
#include "io/multiresolutionimageinterface/MultiResolutionImageReader.h"
#include "io/multiresolutionimageinterface/MultiResolutionImage.h"
#include "io/multiresolutionimageinterface/OpenSlideImage.h"
#include "core/filetools.h"
#include "core/PathologyEnums.h"
#include "annotation/XmlRepository.h"
#include "annotation/Annotation.h"
#include "annotation/AnnotationList.h"
#include <vector>
#include <QtNetwork/QHostAddress>
#include <QMessageBox>
#include <windows.h>
//#include "pathologyworkstation.h"

//#include <QLineEdit>
//#include <QProgressBar>

QHostAddress SERVERADDR=QHostAddress("10.18.129.173");
const quint16 PORT=1129;

//QHostAddress SERVERADDR=QHostAddress("127.0.0.1");
//const quint16 PORT=80100;


VisualizationWorkstationExtensionPlugin::VisualizationWorkstationExtensionPlugin() :
  WorkstationExtensionPluginInterface(),
  _dockWidget(NULL),
  _likelihoodCheckBox(NULL),
  _foreground(NULL),
  _foregroundScale(1.),
  _opacity(1.0),
  _window(1.0),
  _level(0.5),
  _foregroundChannel(0),
  _renderingEnabled(false),
  fName(NULL)
{
  _lst = std::make_shared<AnnotationList>();
  _xmlRepo = std::make_shared<XmlRepository>(_lst);
  _settings = new QSettings(QSettings::IniFormat, QSettings::UserScope, "DIAG", "ASAP", this);

}

VisualizationWorkstationExtensionPlugin::~VisualizationWorkstationExtensionPlugin() { 
  if (_foreground) {
    _foregroundScale = 1.;
    emit changeForegroundImage(std::weak_ptr<MultiResolutionImage>(), _foregroundScale);
    _foreground.reset();
  }
  _dockWidget = NULL;
}

bool VisualizationWorkstationExtensionPlugin::initialize(PathologyViewer* viewer) {
  _viewer = viewer;
  connect(this, SIGNAL(changeForegroundImage(std::weak_ptr<MultiResolutionImage>, float)), viewer, SLOT(onForegroundImageChanged(std::weak_ptr<MultiResolutionImage>, float)));
  return true;
}

bool VisualizationWorkstationExtensionPlugin::initialize2(PathologyViewer* viewer,PathologyViewer* viewer3,PathologyViewer* viewer4) {
  _viewer2 = viewer;
  _viewer3 = viewer3;
  _viewer4 = viewer4;
  connect(this, SIGNAL(changeForegroundImage(std::weak_ptr<MultiResolutionImage>, float)), viewer, SLOT(onForegroundImageChanged(std::weak_ptr<MultiResolutionImage>, float)));
  return true;
}

QDockWidget* VisualizationWorkstationExtensionPlugin::getDockWidget() {
  _dockWidget = new QDockWidget(QString::fromLocal8Bit("任务管理"));
  _dockWidget->setObjectName("file");
  QUiLoader loader;
  QFile file(":/VisualizationWorkstationExtensionPlugin_ui/VisualizationWorkstationExtensionPlugin.ui");
  file.open(QFile::ReadOnly);
  QWidget* content = loader.load(&file, _dockWidget);
  file.close();
  _likelihoodCheckBox = content->findChild<QCheckBox*>("LikelihoodCheckBox");
  QDoubleSpinBox* spinBox = content->findChild<QDoubleSpinBox*>("OpacitySpinBox");
  spinBox->setValue(_opacity);
  QDoubleSpinBox* windowSpinBox = content->findChild<QDoubleSpinBox*>("WindowSpinBox");
  windowSpinBox->setValue(_window);
  QDoubleSpinBox* levelSpinBox = content->findChild<QDoubleSpinBox*>("LevelSpinBox");
  levelSpinBox->setValue(_level);
  QSpinBox* channelSpinBox = content->findChild<QSpinBox*>("ChannelSpinBox");
  QGroupBox* visgrop=content->findChild<QGroupBox*>("VisualizationGroupBox");
  visgrop->setVisible(false);
  channelSpinBox->setValue(_foregroundChannel);
  _segmentationCheckBox = content->findChild<QCheckBox*>("SegmentationCheckBox");
  QPushButton* openResultButton = content->findChild<QPushButton*>("OpenResultPushButton");
  openResultButton->setVisible(false);
  QPushButton* beginButton = content->findChild<QPushButton*>("BeginPushButton");
  fName=content->findChild<QLabel*>("label");
  info=content->findChild<QLineEdit*>("lineEdit_2");
  info->setVisible(false);
  progress=content->findChild<QProgressBar*>("progressBar");
  QComboBox* LUTBox = content->findChild<QComboBox*>("LUTComboBox");
  LUTBox->setEditable(false);
  for (std::map<std::string, pathology::LUT>::const_iterator it = pathology::ColorLookupTables.begin(); it != pathology::ColorLookupTables.end(); ++it) {
    LUTBox->addItem(QString::fromStdString(it->first));
  }
  LUTBox->setCurrentText("Normal");
  connect(_likelihoodCheckBox, SIGNAL(toggled(bool)), this, SLOT(onEnableLikelihoodToggled(bool)));
  connect(_segmentationCheckBox, SIGNAL(toggled(bool)), this, SLOT(onEnableSegmentationToggled(bool)));
  connect(spinBox, SIGNAL(valueChanged(double)), this, SLOT(onOpacityChanged(double)));
  connect(windowSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onWindowValueChanged(double)));
  connect(levelSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onLevelValueChanged(double)));
  connect(channelSpinBox, SIGNAL(valueChanged(int)), this, SLOT(onChannelChanged(int)));
  connect(openResultButton, SIGNAL(clicked()), this, SLOT(onOpenResultImageClicked()));
  connect(beginButton, SIGNAL(clicked()), this, SLOT(onBeginClicked()));
  connect(LUTBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(onLUTChanged(const QString&)));
  _dockWidget->setEnabled(true);
  //showButton->setVisible(false);
  // Not used for know
  QGroupBox* segmentationGroupBox = content->findChild<QGroupBox*>("SegmentationGroupBox");
  segmentationGroupBox->setVisible(false);
  progress->setVisible(false);
  fName->setVisible(false);
  verpro = content->findChild<QGroupBox*>("groupBox")->findChild<QVBoxLayout*>("vert3");
  filelist = content->findChild<QGroupBox*>("groupBox_2");
  openedfile = content->findChild<QGroupBox*>("groupBox_2")->findChild<QVBoxLayout*>("vert3_2");
  return _dockWidget;
}


void VisualizationWorkstationExtensionPlugin::onBeginClicked() {
  //  qfilename=QFileDialog::getOpenFileName(_dockWidget, tr("Choose File"), _settings->value("lastOpenendPath", QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation)).toString(), tr("Choose files (*.tif);;All files (*.*)"));
    qfilename=QFileDialog::getOpenFileName(_dockWidget, QString::fromLocal8Bit("选择文件"), _settings->value("lastOpenendPath", QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation)).toString(), QString::fromLocal8Bit("选择文件 (*.tif);;所有文件 (*.*)"));
    _settings->setValue("lastOpenendPath", QFileInfo(qfilename).dir().path());
    if(!qfilename.isEmpty()) {
        QStringList secs= qfilename.split(QRegExp("[/]"));
        int len=secs.length();
        f_name=secs.at(len-1);
//        fName->setText(f_name);
//        fName->setVisible(true);
//        progress->setMaximum(100);
//        progress->setValue(0);
//        progress->setVisible(true);

        QHBoxLayout* hprogress=new QHBoxLayout();
        QLabel *label = new QLabel();
        label->setText(f_name);
        QProgressBar* tpro=new QProgressBar();
        tpro->setValue(0);
        hprogress->addWidget(label);
        hprogress->addWidget(tpro);
        verpro->addLayout(hprogress);

        processThread* transThr=new processThread(f_name,qfilename,tpro);
        connect(transThr,SIGNAL(xmldone(QString)),this,SLOT(onShowClicked(QString)));
        transThr->start();
      //  socketTrans();
    }
}


void VisualizationWorkstationExtensionPlugin::socketTrans(){

    cSocket=new QTcpSocket(this);
    cSocket->abort();
    connect(cSocket,SIGNAL(connected()),this,SLOT(sendFileName()));
    connect(cSocket,SIGNAL(readyRead()),this,SLOT(readMessageFromTCPServer()));
    cSocket->connectToHost(SERVERADDR,PORT);
    cSocket->waitForConnected();
    cSocket->waitForReadyRead();

   // connect(cSocket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(displayError(QAbstractSocket::SocketError)));
}

void VisualizationWorkstationExtensionPlugin::sendFileName(){
    cSocket->waitForBytesWritten();
    cSocket->write(f_name.toStdString().c_str(),strlen(f_name.toStdString().c_str()));
    cSocket->flush();
}

void VisualizationWorkstationExtensionPlugin::readMessageFromTCPServer()
{
info->setText("IMG1");
    QString r_str=cSocket->readAll();
    info->setText("IMG");
    int val=std::stoi(r_str.toStdString());
    progress->setValue(val);
    while (val<100) {
        bool read= cSocket->waitForReadyRead();
        if(read){
        r_str=cSocket->readAll();
        val=std::stoi(r_str.toStdString());
        progress->setValue(val);
        }
    }
    if(val>=100){

        //if (_img) {
            //info->setText("IMG");
          //onImageClosed();
        //}
//        std::string fn = qfilename.toStdString();
//        _settings->setValue("lastOpenendPath", QFileInfo(qfilename).dir().path());
//        _settings->setValue("currentFile", QFileInfo(qfilename).fileName());

//        MultiResolutionImageReader imgReader;
//        _img.reset(imgReader.open(fn));
//        if (_img) {
//          if (_img->valid()) {
//              if (std::shared_ptr<OpenSlideImage> openslide_img = std::dynamic_pointer_cast<OpenSlideImage>(_img)) {
//                openslide_img->setIgnoreAlpha(false);
//              }
//            _viewer2->initialize(_img);
//            onxmlLoaded(_img,fn);
//            }
//          }

         return;

     }

}



void VisualizationWorkstationExtensionPlugin::displayError(QAbstractSocket::SocketError socketerror){
    switch (socketerror) {
    case QAbstractSocket::RemoteHostClosedError:

        break;
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::information(_dockWidget,tr("client request"),tr("host not found."));
        break;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::information(_dockWidget,tr("client request"),tr("connection was refused."));
        break;
    default:
        QMessageBox::information(_dockWidget,tr("client request"),tr("the following error occured:%1.").arg(cSocket->errorString()));
        break;
    }

}

void VisualizationWorkstationExtensionPlugin::onxmlLoaded(std::weak_ptr<MultiResolutionImage> img, std::string fileName) {
  std::shared_ptr<MultiResolutionImage> local_img = img.lock();
  _backgroundDimensions = local_img->getDimensions();
  if (_dockWidget) {
    _dockWidget->setEnabled(true);
  }
  if (!fileName.empty()) {
    std::string base = core::extractBaseName(fileName);

    std::string likImgPth = core::completePath(base + ".tif", core::extractFilePath(fileName));
    std::string xmlPath = core::completePath(base + ".xml", core::extractFilePath(fileName));
    //this->loadNewForegroundImage(likImgPth);
    if (core::fileExists(xmlPath)) {
      _xmlRepo->setSource(xmlPath);
      _xmlRepo->load();
      addSegmentationsToViewer();
    }
  }
//    AnnotationWorkstationExtensionPlugin *anno=NULL;
//    anno=new AnnotationWorkstationExtensionPlugin();
//    anno->onNewImageLoaded(img,fileName);
}

void VisualizationWorkstationExtensionPlugin::onShowClicked(QString fn_){
    if (!_qtAnnotations.empty()) {
      removeSegmentationsFromViewer();
    }
    _viewer2->close();
    _viewer2->setVisible(true);
    _img.reset();
            std::string fn = fn_.toStdString();
            //_settings->setValue("lastOpenendPath", QFileInfo(qfilename).dir().path());
            //_settings->setValue("currentFile", QFileInfo(fn_).fileName());
            MultiResolutionImageReader imgReader;
            _img.reset(imgReader.open(fn));
            if (_img) {
              if (_img->valid()) {
                  if (std::shared_ptr<OpenSlideImage> openslide_img = std::dynamic_pointer_cast<OpenSlideImage>(_img)) {
                    openslide_img->setIgnoreAlpha(false);
                  }
                _viewer2->initialize(_img);
                onxmlLoaded(_img,fn);
                }
              }
}

void VisualizationWorkstationExtensionPlugin::oncloseclicked(QString ofn){
    QCheckBox *cf= filelist->findChild<QCheckBox*>(ofn);
    cf->deleteLater();
}

void VisualizationWorkstationExtensionPlugin::oncloseclicked2(QString ofn){
    QCheckBox *cf= filelist->findChild<QCheckBox*>(ofn);
    cf->setChecked(false);
}

void VisualizationWorkstationExtensionPlugin::onSendClicked(QString ofn){
//    if(!fileName.isEmpty()) {
//        QStringList secs= fileName.split(QRegExp("[/]"));
//        int len=secs.length();
//        f_name=secs.at(len-1);
//        fName->setText(f_name);
//        fName->setVisible(true);
//        progress->setMaximum(100);
//        progress->setValue(0);
//        progress->setVisible(true);
//        //processThread* transThr=new processThread(f_name,progress);
//        //transThr->start();
//        socketTrans();
//    }
  //  QCheckBox *of1=new QCheckBox();
    of1=new QCheckBox();
    of1->setCheckable(true);
    of1->setText(ofn);
    of1->setChecked(true);
//    QStringList secs= ofn.split(QRegExp("[/]"));
//    int len=secs.length();
    of1->setObjectName(ofn);
    connect(of1,SIGNAL(clicked(bool)),this,SLOT(changeimg()));
    openedfile->addWidget(of1);
}

void VisualizationWorkstationExtensionPlugin::changeimg(){
    QString sn=sender()->objectName();
    emit switchimg(sn);
}

processThread::processThread(QString fnm,QString ffnm, QProgressBar *progrs):
    flnm(fnm),
    fflnm(ffnm),
    stopped(false),
    prog(progrs)
{
    prog->setMaximum(100);
    moveToThread(this);
    connect(this,SIGNAL(valChanged(int)),prog,SLOT(setValue(int)));
}

void processThread::run()
{
    //if(!stopped){
        socketTran();
    //}
//    int v=10;
//    while(v<=100)
//    {

//    emit valChanged(v);
//    sleep(3);
//    v+=10;
//    }
}

void processThread::socketTran()
{
    cSocket=new QTcpSocket(this);
    cSocket->abort();
    connect(cSocket,SIGNAL(connected()),this,SLOT(sendFileName()));
    connect(cSocket,SIGNAL(readyRead()),this,SLOT(readMessageFromTCPServer()));
    cSocket->connectToHost(SERVERADDR,PORT,QTcpSocket::ReadWrite);
    cSocket->waitForConnected();
    cSocket->waitForReadyRead();
}

void processThread::sendFileName(){
    cSocket->waitForBytesWritten();
    cSocket->write(flnm.toStdString().c_str(),strlen(flnm.toStdString().c_str()));
    cSocket->flush();
}

void processThread::readMessageFromTCPServer()
{

    QString r_str=cSocket->readAll();
    int val=std::stoi(r_str.toStdString());
    emit valChanged(val);
    while (val<100) {
        bool read= cSocket->waitForReadyRead();
        if(read){
        r_str=cSocket->readAll();
        val=std::stoi(r_str.toStdString());
        if(val) emit valChanged(val);
        }
    }
    if(val>=100){
        stopped=true;
        emit xmldone(fflnm);
        return;
    }
}


void VisualizationWorkstationExtensionPlugin::onNewImageLoaded(std::weak_ptr<MultiResolutionImage> img, std::string fileName) {
  std::shared_ptr<MultiResolutionImage> local_img = img.lock();
  _backgroundDimensions = local_img->getDimensions();
  if (_dockWidget) {
    _dockWidget->setEnabled(true);
  }
  if (!fileName.empty()) {
    std::string base = core::extractBaseName(fileName);
    std::string likImgPth = core::completePath(base + "_likelihood_map.tif", core::extractFilePath(fileName));
    std::string segmXMLPth = core::completePath(base + "_detections.xml", core::extractFilePath(fileName));
    this->loadNewForegroundImage(likImgPth);
    if (core::fileExists(segmXMLPth)) {
      _xmlRepo->setSource(segmXMLPth);
      _xmlRepo->load();
      if (_segmentationCheckBox && _segmentationCheckBox->isChecked()) {
        addSegmentationsToViewer();
      }
    }
  }
}

void VisualizationWorkstationExtensionPlugin::onOpenResultImageClicked() {
  QString fileName = QFileDialog::getOpenFileName(_dockWidget, tr("Open File"),_settings->value("lastChoosedPath", QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation)).toString(),tr("Result files (*.tif);;All files (*.*)"));
  if (!fileName.isEmpty()) {
    loadNewForegroundImage(fileName.toStdString());
  }
}


void VisualizationWorkstationExtensionPlugin::loadNewForegroundImage(const std::string& resultImagePth) {
  if (_foreground) {
    _foregroundScale = 1;
    emit changeForegroundImage(std::weak_ptr<MultiResolutionImage>(), _foregroundScale);
    _foreground.reset();
  }
  QGroupBox* segmentationGroupBox = _dockWidget->findChild<QGroupBox*>("SegmentationGroupBox");
  segmentationGroupBox->setEnabled(false);
  QGroupBox* visualizationGroupBox = _dockWidget->findChild<QGroupBox*>("VisualizationGroupBox");
  visualizationGroupBox->setEnabled(false);
  if (core::fileExists(resultImagePth)) {
    MultiResolutionImageReader reader;
    _foreground.reset(reader.open(resultImagePth));
    if (_foreground) {
      setDefaultVisualizationParameters(_foreground);
      std::vector<unsigned long long> dimsFG = _foreground->getDimensions();
      if (_backgroundDimensions[0] / dimsFG[0] == _backgroundDimensions[1] / dimsFG[1]) {
        _foregroundScale = _backgroundDimensions[0] / dimsFG[0];
        if (_likelihoodCheckBox) {
          if (_renderingEnabled) {
            if (_likelihoodCheckBox->isChecked()) {
              emit changeForegroundImage(_foreground, _foregroundScale);
            }
            else {
              _likelihoodCheckBox->setChecked(true);
            }
          }
          else {
            if (_likelihoodCheckBox->isChecked()) {
              _likelihoodCheckBox->setChecked(false);
            }
            else {
              emit changeForegroundImage(std::weak_ptr<MultiResolutionImage>(), _foregroundScale);
            }
          }
        }
      }
      QGroupBox* segmentationGroupBox = _dockWidget->findChild<QGroupBox*>("SegmentationGroupBox");
      segmentationGroupBox->setEnabled(true);
      QGroupBox* visualizationGroupBox = _dockWidget->findChild<QGroupBox*>("VisualizationGroupBox");
      visualizationGroupBox->setEnabled(true);
    }
  }
}

void VisualizationWorkstationExtensionPlugin::setDefaultVisualizationParameters(std::shared_ptr<MultiResolutionImage> img) {
  if (_dockWidget) {
    double maxValue = img->getMaxValue(_foregroundChannel);
    double minValue = img->getMinValue(_foregroundChannel);
    if (_settings) {
      _settings->beginGroup("VisualizationWorkstationExtensionPlugin");
      pathology::DataType dtype = img->getDataType();
      if (dtype == pathology::Float) {
        _settings->beginGroup("VisualizationSettingsForFloatType");
      }
      else if (dtype == pathology::UChar) {
        _settings->beginGroup("VisualizationSettingsForUCharType");
      }
      else if (dtype == pathology::UInt16) {
        _settings->beginGroup("VisualizationSettingsForUInt16Type");
      }
      else if (dtype == pathology::UInt32) {
        _settings->beginGroup("VisualizationSettingsForUInt32Type");
      }
      _opacity = _settings->value("opacity", 0.5).toFloat();
      _foregroundChannel = _settings->value("foregroundchannel", 0).toUInt();
      if (_foregroundChannel >= img->getSamplesPerPixel()) {
        _foregroundChannel = 0;
      }
      _window = _settings->value("window", maxValue - minValue).toDouble();
      _level = _settings->value("level", (maxValue + minValue) / 2).toDouble();
      if (dtype == pathology::Float) {
        _currentLUT = _settings->value("lut", "Traffic Light").toString();
      }
      else {
        _currentLUT = _settings->value("lut", "Label").toString();
      }
      _renderingEnabled = _settings->value("visible", false).toBool();
      _settings->endGroup();
      _settings->endGroup();
    }
    else {
      _opacity = 0.5;
      _foregroundChannel = 0;
      _window = maxValue - minValue;
      _level = (maxValue + minValue) / 2;
      if (img->getDataType() == pathology::UChar || img->getDataType() == pathology::UInt32 || img->getDataType() == pathology::UInt16) {
        _currentLUT = "Label";
      }
      else {
        _currentLUT = "Traffic Light";
      }
    }
    QDoubleSpinBox* spinBox = _dockWidget->findChild<QDoubleSpinBox*>("OpacitySpinBox");
    spinBox->setValue(_opacity);
    _viewer->setForegroundOpacity(_opacity);
    QSpinBox* channelSpinBox = _dockWidget->findChild<QSpinBox*>("ChannelSpinBox");
    channelSpinBox->setMaximum(_foreground->getSamplesPerPixel() - 1);
    channelSpinBox->setValue(_foregroundChannel);
    _viewer->setForegroundChannel(_foregroundChannel);

    QDoubleSpinBox* windowSpinBox = _dockWidget->findChild<QDoubleSpinBox*>("WindowSpinBox");
    windowSpinBox->setValue(_window);
    QDoubleSpinBox* levelSpinBox = _dockWidget->findChild<QDoubleSpinBox*>("LevelSpinBox");
    levelSpinBox->setValue(_level);
    _viewer->setForegroundWindowAndLevel(_window, _level);
    QComboBox* LUTBox = _dockWidget->findChild<QComboBox*>("LUTComboBox");
    LUTBox->setCurrentText(_currentLUT);
    _viewer->setForegroundLUT(_currentLUT.toStdString());
  }
}

void VisualizationWorkstationExtensionPlugin::onImageClosed() {
  // Store current visualization settings based on ImageType (later replace this with Result specific settings)
  fName->setVisible(false);
  progress->setVisible(false);
    if (_settings && _foreground) {
    _settings->beginGroup("VisualizationWorkstationExtensionPlugin");
    pathology::DataType dtype = _foreground->getDataType();
    if (dtype == pathology::Float) {
      _settings->beginGroup("VisualizationSettingsForFloatType");
    }
    else if (dtype == pathology::UChar) {
      _settings->beginGroup("VisualizationSettingsForUCharType");
    }
    else if (dtype == pathology::UInt16) {
      _settings->beginGroup("VisualizationSettingsForUInt16Type");
    }
    else if (dtype == pathology::UInt32) {
      _settings->beginGroup("VisualizationSettingsForUInt32Type");
    }
    _settings->setValue("opacity", _opacity);
    _settings->setValue("foregroundchannel", _foregroundChannel);
    _settings->setValue("window", _window);
    _settings->setValue("level", _level);
    _settings->setValue("lut", _currentLUT);
    _settings->setValue("visible", _renderingEnabled);
    _settings->endGroup();
    _settings->endGroup();
  }
  if (!_polygons.empty()) {
    removeSegmentationsFromViewer();
  }

  if (_foreground) {
    _foregroundScale = 1;
    emit changeForegroundImage(std::weak_ptr<MultiResolutionImage>(), _foregroundScale);
    _foreground.reset();
  }
  if (_dockWidget) {
    //_dockWidget->setEnabled(false);
    QGroupBox* segmentationGroupBox = _dockWidget->findChild<QGroupBox*>("SegmentationGroupBox");
    segmentationGroupBox->setEnabled(false);
    QGroupBox* visualizationGroupBox = _dockWidget->findChild<QGroupBox*>("VisualizationGroupBox");
    visualizationGroupBox->setEnabled(false);
  }
  //onShowClicked();
}

void VisualizationWorkstationExtensionPlugin::onEnableLikelihoodToggled(bool toggled) {
  if (!toggled) {
    emit changeForegroundImage(std::weak_ptr<MultiResolutionImage>(), _foregroundScale);
    _renderingEnabled = false;
  }
  else {
    emit changeForegroundImage(_foreground, _foregroundScale);
    _renderingEnabled = true;
  }
}

void VisualizationWorkstationExtensionPlugin::onOpacityChanged(double opacity) {
  if (_viewer) {
    _viewer->setForegroundOpacity(opacity);
    _opacity = opacity;
  }
}

void VisualizationWorkstationExtensionPlugin::onLUTChanged(const QString& LUTname) {
  if (_viewer && LUTname != _currentLUT) {
    _currentLUT = LUTname;
    _viewer->setForegroundLUT(LUTname.toStdString());
  }
}
void VisualizationWorkstationExtensionPlugin::onWindowValueChanged(double window) {
  if (_viewer && window != _window) {
    _window = window;
    _viewer->setForegroundWindowAndLevel(_window, _level);
  }
}
void VisualizationWorkstationExtensionPlugin::onLevelValueChanged(double level) {
  if (_viewer && level != _level) {
    _level = level;
    _viewer->setForegroundWindowAndLevel(_window, _level);
  }
}
void VisualizationWorkstationExtensionPlugin::onChannelChanged(int channel) {
  if (_viewer && channel != _foregroundChannel) {
    _foregroundChannel = channel;
    _viewer->setForegroundChannel(_foregroundChannel);
  }
}

void VisualizationWorkstationExtensionPlugin::onEnableSegmentationToggled(bool toggled) {
  if (!toggled) {
    removeSegmentationsFromViewer();
  }
  else {
    addSegmentationsToViewer();
  }
}

void VisualizationWorkstationExtensionPlugin::addSegmentationsToViewer() {
  if (_lst) {
    std::vector<std::shared_ptr<Annotation> > tmp = _lst->getAnnotations();
    float scl = _viewer->getSceneScale();
    for (std::vector<std::shared_ptr<Annotation> >::iterator it = tmp.begin(); it != tmp.end(); ++it) {
//      QPolygonF poly;
//      std::vector<Point> coords = (*it)->getCoordinates();
//      for (std::vector<Point>::iterator pt = coords.begin(); pt != coords.end(); ++pt) {
//        poly.append(QPointF(pt->getX()*scl, pt->getY()*scl));
//      }
//      QGraphicsPolygonItem* cur = new QGraphicsPolygonItem(poly);
//      cur->setBrush(QBrush());
//      cur->setPen(QPen(QBrush(QColor("blue")), 2.));
//      _viewer->scene()->addItem(cur);
//      cur->setZValue(std::numeric_limits<float>::max());
//      _polygons.append(cur);

        QtAnnotation* annot = NULL;
        if ((*it)->getType() == Annotation::Type::DOT) {
          annot = new DotQtAnnotation((*it), this, _viewer2->getSceneScale());
        }
        else if ((*it)->getType() == Annotation::Type::POLYGON) {
          annot = new PolyQtAnnotation((*it), this, _viewer2->getSceneScale());
          dynamic_cast<PolyQtAnnotation*>(annot)->setInterpolationType("linear");
        }
        else if ((*it)->getType() == Annotation::Type::SPLINE) {
          annot = new PolyQtAnnotation((*it), this, _viewer2->getSceneScale());
          dynamic_cast<PolyQtAnnotation*>(annot)->setInterpolationType("spline");
        }
        else if ((*it)->getType() == Annotation::Type::POINTSET) {
          annot = new PointSetQtAnnotation((*it), this, _viewer2->getSceneScale());
        }
        if (annot) {
          annot->finish();
          _qtAnnotations.append(annot);
          _viewer2->scene()->addItem(annot);
          annot->setZValue(20.);
        }


    }
  }
}

void VisualizationWorkstationExtensionPlugin::removeSegmentationsFromViewer() {
//  if (!_polygons.empty()) {
//    for (QList<QGraphicsPolygonItem*>::iterator it = _polygons.begin(); it != _polygons.end(); ++it) {
//      _viewer->scene()->removeItem(*it);
//      delete (*it);
//    }
//    _polygons.clear();
//  }

  if(!_qtAnnotations.empty()){
    for(QList<QtAnnotation*>::iterator i=_qtAnnotations.begin();i!=_qtAnnotations.end();i++){
        _viewer->scene()->removeItem(*i);
        delete (*i);
    }
    _qtAnnotations.clear();
  }
}
