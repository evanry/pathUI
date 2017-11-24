#include <string>
#include <vector>
#include <iostream>

#include <QFileDialog>
#include <QToolButton>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QGraphicsEffect>
#include <QDebug>
#include <QPushButton>
#include <QDockWidget>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPluginLoader>
#include <QComboBox>
#include <QToolBar>
#include <QStyle>
#include <QActionGroup>
#include <QSettings>
#include <QFileInfo>
#include <QStandardPaths>
#include <QtUiTools>
#include <QTreeWidget>

#include "pathologyworkstation.h"
#include "PathologyViewer.h"
#include "interfaces/interfaces.h"
#include "WSITileGraphicsItemCache.h"
#include "config/ASAPMacros.h"
#include "io/multiresolutionimageinterface/MultiResolutionImageReader.h"
#include "io/multiresolutionimageinterface/MultiResolutionImage.h"
#include "io/multiresolutionimageinterface/OpenSlideImage.h"

#ifdef WIN32
const char* PathologyWorkstation::sharedLibraryExtensions = ".dll";
#else
const char* PathologyWorkstation::sharedLibraryExtensions = ".so";
#endif

using namespace std;

PathologyWorkstation::PathologyWorkstation(QWidget *parent) :
    QMainWindow(parent),
    _cacheMaxByteSize(1000*512*512*3),
    _settings(NULL)
{
  setupUi();
  retranslateUi();
  connect(actionOpen, SIGNAL(triggered(bool)), this, SLOT(on_actionOpen_triggered()));
  connect(actionClose, SIGNAL(triggered(bool)), this, SLOT(on_actionClose_triggered()));
  connect(actionAbout, SIGNAL(triggered(bool)), this, SLOT(on_actionAbout_triggered()));
  connect(actionConnect, SIGNAL(triggered(bool)), this, SLOT(on_actionConnect_triggered()));
  connect(save, SIGNAL(triggered(bool)), this, SLOT(on_actionsave_triggered()));
  connect(rotate, SIGNAL(triggered(bool)), this, SLOT(on_actionRotate_triggered()));
  connect(info, SIGNAL(triggered(bool)), this, SLOT(on_actionInfo_triggered()));
  connect(flat, SIGNAL(triggered()), this, SLOT(on_actionflat_triggered()));
  connect(one, SIGNAL(triggered(bool)), this, SLOT(on_actionone_triggered()));
 // connect(actionSend, SIGNAL(triggered(bool)), this, SLOT(on_actionSend_triggered()));

  PathologyViewer* view = this->findChild<PathologyViewer*>("pathologyView");
  PathologyViewer* view_2 = this->findChild<PathologyViewer*>("view2");
  PathologyViewer* view_3 = this->findChild<PathologyViewer*>("view3");
  PathologyViewer* view_4 = this->findChild<PathologyViewer*>("view4");

  this->loadPlugins();
  view->setCacheSize(_cacheMaxByteSize);
  view_2->setCacheSize(_cacheMaxByteSize);
  view_3->setCacheSize(_cacheMaxByteSize);
  view_4->setCacheSize(_cacheMaxByteSize);
  if (view->hasTool("pan")) {
    view->setActiveTool("pan");
    QList<QAction*> toolButtons = mainToolBar->actions();
    for (QList<QAction*>::iterator it = toolButtons.begin(); it != toolButtons.end(); ++it) {
      if ((*it)->objectName() == "pan") {
        (*it)->setChecked(true);
      }
    }
  }  
  view->setEnabled(false);
  if (view_2->hasTool("pan")) {
    view_2->setActiveTool("pan");
    QList<QAction*> toolButtons = mainToolBar->actions();
    for (QList<QAction*>::iterator it = toolButtons.begin(); it != toolButtons.end(); ++it) {
      if ((*it)->objectName() == "pan") {
        (*it)->setChecked(true);
      }
    }
  }
  view_2->setEnabled(false);
  if (view_3->hasTool("pan")) {
    view_3->setActiveTool("pan");
    QList<QAction*> toolButtons = mainToolBar->actions();
    for (QList<QAction*>::iterator it = toolButtons.begin(); it != toolButtons.end(); ++it) {
      if ((*it)->objectName() == "pan") {
        (*it)->setChecked(true);
      }
    }
  }
  view_3->setEnabled(false);
  if (view_4->hasTool("pan")) {
    view_4->setActiveTool("pan");
    QList<QAction*> toolButtons = mainToolBar->actions();
    for (QList<QAction*>::iterator it = toolButtons.begin(); it != toolButtons.end(); ++it) {
      if ((*it)->objectName() == "pan") {
        (*it)->setChecked(true);
      }
    }
  }
  view_4->setEnabled(false);
  _settings = new QSettings(QSettings::IniFormat, QSettings::UserScope, "DIAG", "ASAP", this);
  readSettings();
  QStringList args = QApplication::arguments();
  if (args.size() > 1) {
    openFile(args[1]);
  }
}

void PathologyWorkstation::writeSettings()
{
  _settings->beginGroup("ASAP");
  _settings->setValue("size", size());
  _settings->setValue("maximized", isMaximized());
  _settings->endGroup();
}

void PathologyWorkstation::readSettings()
{
  _settings->beginGroup("ASAP");
  resize(_settings->value("size", QSize(1037, 786)).toSize());
  if (_settings->value("maximized", false).toBool()) {
    this->setWindowState(Qt::WindowMaximized);
  }
  _settings->endGroup();
}

void PathologyWorkstation::loadPlugins() {
  PathologyViewer* viewer = this->findChild<PathologyViewer*>("pathologyView");
  PathologyViewer* viewer2 = this->findChild<PathologyViewer*>("view2");
  PathologyViewer* viewer3 = this->findChild<PathologyViewer*>("view3");
  PathologyViewer* viewer4 = this->findChild<PathologyViewer*>("view4");
  _pluginsDir = QDir(qApp->applicationDirPath());
  if (_pluginsDir.cd("plugins")) {
    if (_pluginsDir.cd("tools")) {
      foreach(QString fileName, _pluginsDir.entryList(QDir::Files)) {
        if (fileName.toLower().endsWith(sharedLibraryExtensions)) {
          QPluginLoader loader(_pluginsDir.absoluteFilePath(fileName));
          QObject *plugin = loader.instance();
          if (plugin) {
            std::shared_ptr<ToolPluginInterface> tool(qobject_cast<ToolPluginInterface*>(plugin));
            if (tool) {
              tool->setViewer(viewer);
              tool->setViewer2(viewer2,viewer3,viewer4);
              QAction* toolAction = tool->getToolButton();
              if(tool->name()=="zoom"||tool->name()=="ruler"){
                  connect(toolAction, SIGNAL(triggered(bool)), viewer, SLOT(changeActiveTool()));
              }
              else{
              connect(toolAction, SIGNAL(triggered(bool)), viewer, SLOT(changeActiveTool()));
              connect(toolAction, SIGNAL(triggered(bool)), viewer2, SLOT(changeActiveTool()));
              connect(toolAction, SIGNAL(triggered(bool)), viewer3, SLOT(changeActiveTool()));
              connect(toolAction, SIGNAL(triggered(bool)), viewer4, SLOT(changeActiveTool()));
              }
              _toolPluginFileNames.push_back(fileName.toStdString());
              viewer->addTool(tool);
              viewer2->addTool(tool);
              viewer3->addTool(tool);
              viewer4->addTool(tool);
              QToolBar* mainToolBar = this->findChild<QToolBar *>("mainToolBar");
              menuTool->addAction(toolAction);
              toolAction->setCheckable(true);
              _toolActions->addAction(toolAction);
              mainToolBar->addAction(toolAction);
            }
          }
        }
      }
      _pluginsDir.cdUp();
    }
    if (_pluginsDir.cd("workstationextension")) {
      QDockWidget* lastDockWidget = NULL;
      QDockWidget* firstDockWidget = NULL;
      QDockWidget* dock1 = NULL;
      QDockWidget* dock2 = NULL;
      QDockWidget* dock3 = NULL;
      foreach(QString fileName, _pluginsDir.entryList(QDir::Files)) {
        if (fileName.toLower().endsWith(sharedLibraryExtensions)) {
          QPluginLoader loader(_pluginsDir.absoluteFilePath(fileName));
          QObject *plugin = loader.instance();
          if (plugin) {
            std::unique_ptr<WorkstationExtensionPluginInterface> extension(qobject_cast<WorkstationExtensionPluginInterface*>(plugin));
            if (extension) {
              _extensionPluginFileNames.push_back(fileName.toStdString());
              connect(this, SIGNAL(newImageLoaded(std::weak_ptr<MultiResolutionImage>, std::string)), &*extension, SLOT(onNewImageLoaded(std::weak_ptr<MultiResolutionImage>, std::string)));
              connect(this, SIGNAL(imageClosed()), &*extension, SLOT(onImageClosed()));
              if(fileName.toStdString()=="VisualizationWorkstationExtensionPlugin.dll"||fileName.toStdString()=="VisualizationWorkstationExtensionPlugin_d.dll")
              {
                  connect(this,SIGNAL(fileSend(QString)), &*extension, SLOT(onSendClicked(QString)));
                  connect(this,SIGNAL(closeimg(QString)), &*extension, SLOT(oncloseclicked(QString)));
                  connect(this,SIGNAL(openf(QString)), &*extension, SLOT(oncloseclicked2(QString)));
                  connect(&*extension,SIGNAL(switchimg(QString)),this,SLOT(changeimg(QString)));
                  connect(this,SIGNAL(getfn()), &*extension, SLOT(getfns()));
                  connect(&*extension,SIGNAL(retfns2(QString,QString)),this,SLOT(twoview(QString,QString)));
                  connect(&*extension,SIGNAL(retfns4(QString,QString,QString,QString)),this,SLOT(fourview(QString,QString,QString,QString)));
              }
              if(fileName.toStdString()=="AnnotationPlugin_d.dll")
              {
                  connect(this,SIGNAL(newImageLoaded2(std::weak_ptr<MultiResolutionImage>, std::string)), &*extension, SLOT(onNewImageLoaded2(std::weak_ptr<MultiResolutionImage>, std::string)));
              }
              extension->initialize(viewer);
              extension->initialize2(viewer2,viewer3,viewer4);
              if (extension->getToolBar()) {
                this->addToolBar(extension->getToolBar());
              }
              if (extension->getDockWidget()) {
                QDockWidget* extensionDW = extension->getDockWidget();
                //extensionDW->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
                if (lastDockWidget) {
                  //this->tabifyDockWidget(lastDockWidget, extensionDW);
                }
                else {
                  //this->addDockWidget(Qt::RightDockWidgetArea, extensionDW);
                  firstDockWidget = extensionDW;
                }
                //extensionDW->setTitleBarWidget(new QWidget());
                lastDockWidget = extensionDW;
                if(extensionDW->objectName()=="annot") dock1=extensionDW;
                else if(extensionDW->objectName()=="filter") dock2=extensionDW;
                else if(extensionDW->objectName()=="file") dock3=extensionDW;
                QMenu* viewMenu = this->findChild<QMenu*>("menuView");
                QMenu* viewDocksMenu = viewMenu->findChild<QMenu*>("menuViewDocks");
                if (!viewDocksMenu) {
                  //viewDocksMenu = viewMenu->addMenu("Docks");
                  //viewDocksMenu = viewMenu->addMenu(QString::fromLocal8Bit("浮窗停靠栏"));
                  //viewDocksMenu->setObjectName("menuViewDocks");
                }
                //viewDocksMenu->addAction(extensionDW->toggleViewAction());
                viewMenu->addAction(extensionDW->toggleViewAction());
              }
              if (extension->getMenu()) {
                this->menuBar->addMenu(extension->getMenu());
              }
              std::vector<std::shared_ptr<ToolPluginInterface> > tools = extension->getTools();
              if (!tools.empty()) {
                mainToolBar->addSeparator();
                for (unsigned int i = 0; i < tools.size(); ++i) {
                  QAction* toolAction = tools[i]->getToolButton();
                  connect(toolAction, SIGNAL(triggered(bool)), viewer, SLOT(changeActiveTool()));
                  connect(toolAction, SIGNAL(triggered(bool)), viewer2, SLOT(changeActiveTool()));
                  connect(toolAction, SIGNAL(triggered(bool)), viewer3, SLOT(changeActiveTool()));
                  connect(toolAction, SIGNAL(triggered(bool)), viewer4, SLOT(changeActiveTool()));
                  viewer->addTool(tools[i]);
                  //viewer2->addTool(tools[i]);
                  mainToolBar->addAction(toolAction);
                  menuTool->addAction(toolAction);
                  toolAction->setCheckable(true);
                  _toolActions->addAction(toolAction);
                }
              }
              _extensions.push_back(std::move(extension));
            }
          }
        }
      }
      this->addDockWidget(Qt::BottomDockWidgetArea, dock1);
      //this->addDockWidget(Qt::LeftDockWidgetArea, dock2);
      this->addDockWidget(Qt::RightDockWidgetArea, dock3);
      this->splitDockWidget(dock1,dock2,Qt::Horizontal);
      _pluginsDir.cdUp();
      if (firstDockWidget) {
        firstDockWidget->raise();
      }
    }
  }
}

void PathologyWorkstation::closeEvent(QCloseEvent *event) {
  event->accept();
}

PathologyWorkstation::~PathologyWorkstation()
{
  on_actionClose_triggered();
  writeSettings();
}

void PathologyWorkstation::on_actionAbout_triggered() {
  QUiLoader loader;
  QFile file(":/ASAP_ui/aboutdialog.ui");
  file.open(QFile::ReadOnly);
  QDialog* content = qobject_cast<QDialog*>(loader.load(&file, this));
  content->setWindowTitle(QString::fromLocal8Bit("关于"));
  if (content) {
//    QLabel* generalInfoLabel = content->findChild<QLabel*>("generalInfoLabel");
//    QString generalInfoText = generalInfoLabel->text();
//    generalInfoText.replace("@VERSION_STRING@", ASAP_VERSION_STRING);
//    generalInfoLabel->setText(generalInfoText);
    QTreeWidget* pluginList = content->findChild<QTreeWidget*>("loadedPluginsOverviewTreeWidget");
    QList<QTreeWidgetItem*> root_items = pluginList->findItems(QString::fromLocal8Bit("工具"), Qt::MatchExactly);
    if (!root_items.empty()) {
      QTreeWidgetItem* root_item = root_items[0];
      for (std::vector<std::string>::const_iterator it = _toolPluginFileNames.begin(); it != _toolPluginFileNames.end(); ++it) {
        root_item->addChild(new QTreeWidgetItem(QStringList(QString::fromStdString(*it))));
      }
    }
    root_items = pluginList->findItems(QString::fromLocal8Bit("工作区域扩展"), Qt::MatchExactly);
    if (!root_items.empty()) {
      QTreeWidgetItem* root_item = root_items[0];
      for (std::vector<std::string>::const_iterator it = _extensionPluginFileNames.begin(); it != _extensionPluginFileNames.end(); ++it) {
        root_item->addChild(new QTreeWidgetItem(QStringList(QString::fromStdString(*it))));
      }
    }    
    content->exec();
  }
  file.close();
}

void PathologyWorkstation::on_actionClose_triggered()
{
    for (std::vector<std::unique_ptr<WorkstationExtensionPluginInterface> >::iterator it = _extensions.begin(); it != _extensions.end(); ++it) {
      if (!(*it)->canClose()) {
        return;
      }
    }
    emit imageClosed();
    QString fpn=_settings->value("lastOpenendPath").toString();
    if(fpn.endsWith('/'))
      fpn+=_settings->value("currentFile").toString();
    else
      fpn=fpn+"/"+ _settings->value("currentFile").toString();
    emit closeimg(fpn);
    //emit closeimg(_settings->value("lastOpenendPath").toString()+"/"+ _settings->value("currentFile").toString());
    _settings->setValue("currentFile", QString());
    this->setWindowTitle(QString::fromLocal8Bit("病理影像分析系统"));
    if (_img) {
		  PathologyViewer* view = this->findChild<PathologyViewer*>("pathologyView");
          view->close();
          PathologyViewer* view_2 = this->findChild<PathologyViewer*>("view2");
          view_2->close();
          view_2->setVisible(false);
          PathologyViewer* view_3 = this->findChild<PathologyViewer*>("view3");
          view_3->close();
          view_3->setVisible(false);
          PathologyViewer* view_4 = this->findChild<PathologyViewer*>("view4");
          view_4->close();
          view_4->setVisible(false);
          _img = NULL;
		  _img.reset();
          statusBar->showMessage(QString::fromLocal8Bit("关闭文件!"), 5);
    }
}

void PathologyWorkstation::on_actionClose_triggered2()
{
    for (std::vector<std::unique_ptr<WorkstationExtensionPluginInterface> >::iterator it = _extensions.begin(); it != _extensions.end(); ++it) {
      if (!(*it)->canClose()) {
        return;
      }
    }
    emit imageClosed();
    _settings->setValue("currentFile", QString());
    this->setWindowTitle(QString::fromLocal8Bit("病理影像分析系统"));
    if (_img) {
          PathologyViewer* view = this->findChild<PathologyViewer*>("pathologyView");
          view->close();
//          PathologyViewer* view_2 = this->findChild<PathologyViewer*>("view2");
//          view_2->close();
//          view_2->setVisible(false);
//          PathologyViewer* view_3 = this->findChild<PathologyViewer*>("view3");
//          view_3->close();
//          view_3->setVisible(false);
//          PathologyViewer* view_4 = this->findChild<PathologyViewer*>("view4");
//          view_4->close();
//          view_4->setVisible(false);
          _img = NULL;
          _img.reset();
          statusBar->showMessage(QString::fromLocal8Bit("关闭文件!"), 5);
    }
}

void PathologyWorkstation::openFile(const QString& fileName) {
  statusBar->clearMessage();

  if (!fileName.isEmpty()) {
    if (_img) {
      QString fpn=_settings->value("lastOpenendPath").toString();
      if(fpn.endsWith('/'))
        fpn+=_settings->value("currentFile").toString();
      else
        fpn=fpn+"/"+ _settings->value("currentFile").toString();
      emit openf(fpn);
      on_actionClose_triggered2();
    }
    std::string fn = fileName.toStdString();
    _settings->setValue("lastOpenendPath", QFileInfo(fileName).dir().path());
    _settings->setValue("currentFile", QFileInfo(fileName).fileName());
    //this->setWindowTitle(QString("ASAP - ") + QFileInfo(fileName).fileName());
    this->setWindowTitle(QString::fromLocal8Bit("病理影像分析系统 - ")+ QFileInfo(fileName).fileName());
    MultiResolutionImageReader imgReader;
    _img.reset(imgReader.open(fn));
    if (_img) {
      if (_img->valid()) {
        if (std::shared_ptr<OpenSlideImage> openslide_img = dynamic_pointer_cast<OpenSlideImage>(_img)) {
          openslide_img->setIgnoreAlpha(false);
        }
        vector<unsigned long long> dimensions = _img->getLevelDimensions(_img->getNumberOfLevels() - 1);
        PathologyViewer* view = this->findChild<PathologyViewer*>("pathologyView");
        view->initialize(_img);
//        PathologyViewer* view_2 = this->findChild<PathologyViewer*>("view2");
//        view_2->initialize(_img);
        emit newImageLoaded(_img, fn);
      }
      else {
        statusBar->showMessage(QString::fromLocal8Bit("不支持当前文件类型版本"));
      }
    }
    else {
      statusBar->showMessage(QString::fromLocal8Bit("文件类型无效"));
    }
  }
}

void PathologyWorkstation::openFile2(const QString& fileName) {
  statusBar->clearMessage();
  if (!fileName.isEmpty()) {
    if (_img) {
      on_actionClose_triggered2();
    }
    std::string fn = fileName.toStdString();
    _settings->setValue("lastOpenendPath", QFileInfo(fileName).dir().path());
    _settings->setValue("currentFile", QFileInfo(fileName).fileName());
    this->setWindowTitle(QString("ASAP - ") + QFileInfo(fileName).fileName());
    MultiResolutionImageReader imgReader;
    _img.reset(imgReader.open(fn));
    if (_img) {
      if (_img->valid()) {
        if (std::shared_ptr<OpenSlideImage> openslide_img = dynamic_pointer_cast<OpenSlideImage>(_img)) {
          openslide_img->setIgnoreAlpha(false);
        }
        vector<unsigned long long> dimensions = _img->getLevelDimensions(_img->getNumberOfLevels() - 1);
    //    PathologyViewer* view = this->findChild<PathologyViewer*>("pathologyView");
     //   view->initialize(_img);
        PathologyViewer* view_2 = this->findChild<PathologyViewer*>("view2");
        view_2->setVisible(true);
        view_2->initialize(_img);
        emit newImageLoaded2(_img, fn);
      }
      else {
        statusBar->showMessage(QString::fromLocal8Bit("不支持当前文件类型版本"));
      }
    }
    else {
      statusBar->showMessage(QString::fromLocal8Bit("文件类型无效"));
    }
  }
}

void PathologyWorkstation::on_actionOpen_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, QString::fromLocal8Bit("打开文件"), _settings->value("lastOpenendPath", QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation)).toString(), QString::fromLocal8Bit("Slide 文件 (*.lif *.svs *.mrxs *.ndpi *.tif *.tiff);;所有文件 (*.*)"));
    //emit fileSend(fileName);
    if (!fileName.isEmpty()) {
        emit fileSend(fileName);
    }
    openFile(fileName);

}

void PathologyWorkstation::changeimg(QString ofn){
    openFile(ofn);
}

void PathologyWorkstation::on_actionSend_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, QString::fromLocal8Bit("打开文件"), _settings->value("lastOpenendPath", QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation)).toString(), QString::fromLocal8Bit("tif 文件 (*.tif);;所有文件 (*.*)"));
    emit fileSend(fileName);
    openFile2(fileName);
}

void PathologyWorkstation::on_actionsave_triggered(){
    QString fileName = QFileDialog::getSaveFileName(this, QString::fromLocal8Bit("保存文件"), _settings->value("lastsavedPath", QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation)).toString(), QString::fromLocal8Bit("Slide 文件 (*.lif *.svs *.mrxs *.ndpi *.tif *.tiff);;所有文件 (*.*)"));
}

void PathologyWorkstation::on_actionConnect_triggered()
{
    QDialog *connServ=new QDialog(this);
    Qt::WindowFlags flags=Qt::Dialog;
     flags |=Qt::WindowCloseButtonHint;
     connServ->setWindowFlags(flags);
    connServ->setWindowTitle(QString::fromLocal8Bit("访问远程服务器"));
    QVBoxLayout* vLay=new QVBoxLayout();
    QHBoxLayout* hLay1=new QHBoxLayout();
    QHBoxLayout* hLay2=new QHBoxLayout();
    QLabel *serv=new QLabel(QString::fromLocal8Bit("服务器地址: "));
    QLabel *port=new QLabel(QString::fromLocal8Bit("服务器端口: "));
    QLineEdit *addr=new QLineEdit();
    QLineEdit *port_in=new QLineEdit();
    hLay1->addWidget(serv);
    hLay1->addWidget(addr);
    hLay2->addWidget(port);
    hLay2->addWidget(port_in);
    vLay->addLayout(hLay1);
    vLay->addLayout(hLay2);
    QPushButton* cancel = new QPushButton(QString::fromLocal8Bit("取消"));
    QPushButton* con = new QPushButton(QString::fromLocal8Bit("连接"));
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(con);
    buttonLayout->addWidget(cancel);
    vLay->addLayout(buttonLayout);
    connServ->setLayout(vLay);
    connServ->show();
}

void PathologyWorkstation::on_actionRotate_triggered(){

    QDialog *rotat=new QDialog(this);
    Qt::WindowFlags flags=Qt::Dialog;
     flags |=Qt::WindowCloseButtonHint;
     rotat->setWindowFlags(flags);
    rotat->setWindowTitle(QString::fromLocal8Bit("图像旋转"));
    QVBoxLayout* vLay=new QVBoxLayout();
    QPushButton* b1=new QPushButton(QString::fromLocal8Bit("顺时针旋转90度"));
    QPushButton* b2=new QPushButton(QString::fromLocal8Bit("逆时针旋转90度"));
    QPushButton* b3=new QPushButton(QString::fromLocal8Bit("旋转180度"));
    QPushButton* b4=new QPushButton(QString::fromLocal8Bit("水平翻转"));
    QPushButton* b5=new QPushButton(QString::fromLocal8Bit("垂直翻转"));
    connect(b1,SIGNAL(clicked(bool)),this,SLOT(onrat()));
    connect(b2,SIGNAL(clicked(bool)),this,SLOT(onrat2()));
    connect(b3,SIGNAL(clicked(bool)),this,SLOT(onrat3()));
    vLay->addWidget(b1);
    vLay->addWidget(b2);
    vLay->addWidget(b3);
    vLay->addWidget(b4);
    vLay->addWidget(b5);
    rotat->setLayout(vLay);
    rotat->show();

}

void PathologyWorkstation::onrat(){
    PathologyViewer* view = this->findChild<PathologyViewer*>("pathologyView");
    view->rotate(90);
}

void PathologyWorkstation::onrat2(){
    PathologyViewer* view = this->findChild<PathologyViewer*>("pathologyView");
    view->rotate(-90);
}

void PathologyWorkstation::onrat3(){
    PathologyViewer* view = this->findChild<PathologyViewer*>("pathologyView");
    view->rotate(180);
}

void PathologyWorkstation::on_actionInfo_triggered(){
    QDialog *infod=new QDialog(this);
    Qt::WindowFlags flags=Qt::Dialog;
    flags |=Qt::WindowCloseButtonHint;
    infod->setWindowFlags(flags);
    infod->setWindowTitle(QString::fromLocal8Bit("图像信息"));
    QVBoxLayout* vLay=new QVBoxLayout();
    QPushButton* b1=new QPushButton(QString::fromLocal8Bit("确定"));
    connect(b1,SIGNAL(clicked(bool)),infod,SLOT(deleteLater()));
    QString path=_settings->value("lastOpenendPath").toString();
    QString fil=_settings->value("currentFile").toString();
    QFile file(path+"/"+fil);
    qint64 fs=file.size();
    QString sfs=QString::number(fs/(1024*1024),10)+"MB";
    QDateTime current_date_time =QDateTime::currentDateTime();
    QString current_date =current_date_time.toString("yyyy.MM.dd hh:mm:ss");
    QLabel* cont1=new QLabel();
    QLabel* cont2=new QLabel();
    QLabel* cont3=new QLabel();
    QLabel* cont4=new QLabel();
    QLabel* cont5=new QLabel();
    cont1->setText(QString::fromLocal8Bit("文件名：")+fil);
    cont2->setText(QString::fromLocal8Bit("文件位置：")+path);
    cont3->setText(QString::fromLocal8Bit("文件大小：")+sfs);
    cont4->setText(QString::fromLocal8Bit("文件类型：tif图像文件(.tif)"));
    cont5->setText(QString::fromLocal8Bit("修改时间：")+current_date);
    vLay->addWidget(cont1);
    vLay->addWidget(cont2);
    vLay->addWidget(cont3);
    vLay->addWidget(cont4);
    vLay->addWidget(cont5);
    vLay->addWidget(b1);
    infod->setLayout(vLay);
    infod->show();
}

void PathologyWorkstation::on_actionflat_triggered(){
    emit getfn();
//    QString f3("Z:/16/TrainingData/Train_Tumor/Tumor_009.tif");
//    QString f4("Y:/Tumor_055.tif");
//    QString f5("F:/Tumor_076.tif");
//    std::string fn3=f3.toStdString();
//    MultiResolutionImageReader imgReader;
//    _img.reset(imgReader.open(fn3));
//    std::shared_ptr<MultiResolutionImage> v3;
//    v3.reset(imgReader.open(f4.toStdString()));
//    std::shared_ptr<MultiResolutionImage> v4;
//    v4.reset(imgReader.open(f5.toStdString()));
//    if (_img) {
//      if (_img->valid()) {
//        if (std::shared_ptr<OpenSlideImage> openslide_img = dynamic_pointer_cast<OpenSlideImage>(_img)) {
//          openslide_img->setIgnoreAlpha(false);
//        }
//        if (std::shared_ptr<OpenSlideImage> openslide_img3 = dynamic_pointer_cast<OpenSlideImage>(v3)) {
//          openslide_img3->setIgnoreAlpha(false);
//        }
//        if (std::shared_ptr<OpenSlideImage> openslide_img4 = dynamic_pointer_cast<OpenSlideImage>(v4)) {
//          openslide_img4->setIgnoreAlpha(false);
//        }
//        PathologyViewer* view2 = this->findChild<PathologyViewer*>("view2");
//        view2->initialize(_img);
//        PathologyViewer* view3 = this->findChild<PathologyViewer*>("view3");
//        view3->initialize(v3);
//        PathologyViewer* view4 = this->findChild<PathologyViewer*>("view4");
//        view4->initialize(v4);
//        view2->setVisible(true);
//        view3->setVisible(true);
//        view4->setVisible(true);
//        }
//      }
}

void PathologyWorkstation::fourview(QString f1,QString f2,QString f3,QString f4){
    std::string fn1=f1.toStdString();
    MultiResolutionImageReader imgReader;
    _img = NULL;
    _img.reset();
    _img.reset(imgReader.open(fn1));
    std::shared_ptr<MultiResolutionImage> v2;
    v2.reset(imgReader.open(f2.toStdString()));
    std::shared_ptr<MultiResolutionImage> v3;
    v3.reset(imgReader.open(f3.toStdString()));
    std::shared_ptr<MultiResolutionImage> v4;
    v4.reset(imgReader.open(f4.toStdString()));
    if (_img) {
      if (_img->valid()) {
        if (std::shared_ptr<OpenSlideImage> openslide_img = dynamic_pointer_cast<OpenSlideImage>(_img)) {
          openslide_img->setIgnoreAlpha(false);
        }
        if (std::shared_ptr<OpenSlideImage> openslide_img2 = dynamic_pointer_cast<OpenSlideImage>(v2)) {
          openslide_img2->setIgnoreAlpha(false);
        }
        if (std::shared_ptr<OpenSlideImage> openslide_img3 = dynamic_pointer_cast<OpenSlideImage>(v3)) {
          openslide_img3->setIgnoreAlpha(false);
        }
        if (std::shared_ptr<OpenSlideImage> openslide_img4 = dynamic_pointer_cast<OpenSlideImage>(v4)) {
          openslide_img4->setIgnoreAlpha(false);
        }
        PathologyViewer* view1 = this->findChild<PathologyViewer*>("pathologyView");
        view1->initialize(_img);
        PathologyViewer* view2 = this->findChild<PathologyViewer*>("view2");
        view2->initialize(v2);
        PathologyViewer* view3 = this->findChild<PathologyViewer*>("view3");
        view3->initialize(v3);
        PathologyViewer* view4 = this->findChild<PathologyViewer*>("view4");
        view4->initialize(v4);
        view2->setVisible(true);
        view3->setVisible(true);
        view4->setVisible(true);
      }
    }
}

void PathologyWorkstation::twoview(QString f1,QString f2){
    std::string fn1=f1.toStdString();
    MultiResolutionImageReader imgReader;
    _img = NULL;
    _img.reset();
    _img.reset(imgReader.open(fn1));
    std::shared_ptr<MultiResolutionImage> v2;
    v2.reset(imgReader.open(f2.toStdString()));
    if (_img) {
      if (_img->valid()) {
        if (std::shared_ptr<OpenSlideImage> openslide_img = dynamic_pointer_cast<OpenSlideImage>(_img)) {
          openslide_img->setIgnoreAlpha(false);
        }
        if (std::shared_ptr<OpenSlideImage> openslide_img2 = dynamic_pointer_cast<OpenSlideImage>(v2)) {
          openslide_img2->setIgnoreAlpha(false);
        }
        PathologyViewer* view1 = this->findChild<PathologyViewer*>("pathologyView");
        view1->initialize(_img);
        PathologyViewer* view2 = this->findChild<PathologyViewer*>("view2");
        view2->initialize(v2);

        view2->setVisible(true);
      }
    }
}

void PathologyWorkstation::on_actionone_triggered(){
    PathologyViewer* view2 = this->findChild<PathologyViewer*>("view2");
    PathologyViewer* view3 = this->findChild<PathologyViewer*>("view3");
    PathologyViewer* view4 = this->findChild<PathologyViewer*>("view4");
    view2->close();
    view3->close();
    view4->close();
    view2->setVisible(false);
    view3->setVisible(false);
    view4->setVisible(false);
}

void PathologyWorkstation::setCacheSize(const unsigned long long& cacheMaxByteSize) {
  PathologyViewer* view = this->findChild<PathologyViewer*>("pathologyView");
  if (view) {
    view->setCacheSize(_cacheMaxByteSize);
  }
  PathologyViewer* view_2 = this->findChild<PathologyViewer*>("view2");
  if (view_2) {
    view_2->setCacheSize(_cacheMaxByteSize);
  }
  PathologyViewer* view_3 = this->findChild<PathologyViewer*>("view2");
  if (view_3) {
    view_3->setCacheSize(_cacheMaxByteSize);
  }
  PathologyViewer* view_4 = this->findChild<PathologyViewer*>("view2");
  if (view_4) {
    view_4->setCacheSize(_cacheMaxByteSize);
  }
}
    
unsigned long long PathologyWorkstation::getCacheSize() const {
  PathologyViewer* view = this->findChild<PathologyViewer*>("pathologyView");
  if (view) {
    return view->getCacheSize();
  }
  PathologyViewer* view_2 = this->findChild<PathologyViewer*>("view2");
  if (view_2) {
      return view_2->getCacheSize();
  }
  PathologyViewer* view_3 = this->findChild<PathologyViewer*>("view2");
  if (view_3) {
      return view_3->getCacheSize();
  }
  PathologyViewer* view_4 = this->findChild<PathologyViewer*>("view2");
  if (view_4) {
      return view_4->getCacheSize();
  }
}

void PathologyWorkstation::setupUi()
{
  if (this->objectName().isEmpty()) {
      this->setObjectName(QStringLiteral("ASAP"));
  }
  this->resize(1037, 786);
  this->setTabPosition(Qt::DockWidgetArea::LeftDockWidgetArea, QTabWidget::East);
  this->setTabPosition(Qt::DockWidgetArea::RightDockWidgetArea, QTabWidget::West);
  QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  sizePolicy.setHorizontalStretch(0);
  sizePolicy.setVerticalStretch(0);
  sizePolicy.setHeightForWidth(this->sizePolicy().hasHeightForWidth());
  this->setSizePolicy(sizePolicy);
  actionOpen = new QAction(this);
  actionOpen->setObjectName(QStringLiteral("actionOpen"));
  actionClose = new QAction(this);
  actionClose->setObjectName(QStringLiteral("actionClose"));
  actionCloseall = new QAction(this);
  actionCloseall->setObjectName(QStringLiteral("actionCloseall"));
  actionOpen->setIcon(QIcon(QPixmap(":/ASAP_icons/open.png")));
  actionClose->setIcon(QIcon(QPixmap(":/ASAP_icons/close.png")));
  actionConnect=new QAction(this);
  actionConnect->setObjectName(QStringLiteral("actionConnect"));
  actionConnect->setIcon(QIcon(QPixmap(":/ASAP_icons/connect.png")));
  actionConnect->setIconText(QString::fromLocal8Bit("访问远程服务器"));
  save=new QAction(this);
  save->setText(QString::fromLocal8Bit("保存"));
  save->setIcon(QIcon(QPixmap(":/ASAP_icons/save.png")));
  save->setIconText(QString::fromLocal8Bit("保存"));
  rotate=new QAction(this);
  rotate->setObjectName(QStringLiteral("rotate"));
  rotate->setIcon(QIcon(QPixmap(":/ASAP_icons/rotate.png")));
  rotate->setIconText(QString::fromLocal8Bit("旋转"));
  info=new QAction(this);
  info->setText(QString::fromLocal8Bit("文件信息"));
  info->setIcon(QIcon(QPixmap(":/ASAP_icons/info.png")));
  info->setIconText(QString::fromLocal8Bit("文件信息"));
  flat=new QAction(this);
  flat->setText(QString::fromLocal8Bit("多窗口显示"));
  flat->setIcon(QIcon(QPixmap(":/ASAP_icons/flat.png")));
  flat->setIconText(QString::fromLocal8Bit("多窗口显示"));
  one=new QAction(this);
  one->setText(QString::fromLocal8Bit("单窗口显示"));
  one->setIcon(QIcon(QPixmap(":/ASAP_icons/one.png")));
  one->setIconText(QString::fromLocal8Bit("单窗口显示"));
//  actionSend=new QAction(this);
//  actionSend->setObjectName(QStringLiteral("actionSend"));
//  actionSend->setIcon(QIcon(QPixmap(":/ASAP_icons/open.png")));

  actionAbout = new QAction(this);
  actionAbout->setObjectName(QStringLiteral("actionAbout"));
  menuBar = new QMenuBar(this);
  menuBar->setObjectName(QStringLiteral("menuBar"));
  menuBar->setGeometry(QRect(0, 0, 1037, 21));
  menuFile = new QMenu(menuBar);
  menuFile->setObjectName(QStringLiteral("menuFile"));
  menuView = new QMenu(menuBar);
  menuView->setObjectName(QStringLiteral("menuView"));
  menuWindow = new QMenu(menuBar);
  menuWindow->setObjectName(QStringLiteral("menuWindow"));
  menuTool = new QMenu(menuBar);
  menuTool->setObjectName(QStringLiteral("menuTool"));
  menuImage = new QMenu(menuBar);
  menuImage->setObjectName(QStringLiteral("menuImage"));
  menuHelp = new QMenu(menuBar);
  menuHelp->setObjectName(QStringLiteral("menuHelp"));
  this->setMenuBar(menuBar);
  mainToolBar = new QToolBar(this);
  mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
  mainToolBar->insertSeparator(actionOpen);
  mainToolBar->addAction(actionOpen);
  mainToolBar->addAction(actionClose);
  mainToolBar->addAction(save);
  mainToolBar->addAction(actionConnect);
  mainToolBar->addSeparator();
  mainToolBar->addAction(one);
  mainToolBar->addAction(flat);
  mainToolBar->addAction(rotate);
  mainToolBar->addAction(info);

//  mainToolBar->addAction(actionSend);
//  mainToolBar->addSeparator();
//  this->addToolBar(Qt::TopToolBarArea, mainToolBar);
  this->addToolBar(Qt::LeftToolBarArea , mainToolBar);
  _toolActions = new QActionGroup(this);
  statusBar = new QStatusBar(this);
  statusBar->setObjectName(QStringLiteral("statusBar"));
  this->setStatusBar(statusBar);

  menuBar->addAction(menuFile->menuAction());
  menuBar->addAction(menuView->menuAction());
  menuBar->addAction(menuWindow->menuAction());
  menuBar->addAction(menuImage->menuAction());
  menuBar->addAction(menuTool->menuAction());
  menuBar->addAction(menuHelp->menuAction());
  menuFile->addAction(actionOpen);
  menuFile->addAction(actionClose);
  menuFile->addAction(save);
  menuFile->addSeparator();
  menuFile->addAction(actionConnect);
  menuFile->addAction(actionCloseall);
  menuWindow->addAction(one);
  menuWindow->addAction(flat);
  menuImage->addAction(rotate);
  menuImage->addAction(info);
  menuHelp->addAction(actionAbout);
  flat->setCheckable(true);
  //flat->setChecked(_settings->value("imgflat", true).toBool());
  one->setCheckable(true);
  //one->setChecked(_settings->value("imgone", false).toBool());
  centralWidget = new QWidget(this);
  centralWidget->setObjectName(QStringLiteral("centralWidget"));
  sizePolicy.setHeightForWidth(centralWidget->sizePolicy().hasHeightForWidth());
  centralWidget->setSizePolicy(sizePolicy);
  centralWidget->setLocale(QLocale(QLocale::English, QLocale::UnitedStates));
  QVBoxLayout* verticalLayout_3 = new QVBoxLayout(centralWidget);
  QHBoxLayout* horizontalLayout_3 = new QHBoxLayout(centralWidget);
  horizontalLayout_3->setSpacing(6);
  horizontalLayout_3->setContentsMargins(0, 0, 0, 0);
  horizontalLayout_3->setObjectName(QStringLiteral("horizontalLayout_3"));
  horizontalLayout_2 = new QHBoxLayout(centralWidget);
  horizontalLayout_2->setSpacing(6);
  horizontalLayout_2->setContentsMargins(0, 0, 0, 0);
  horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
  pathologyView = new PathologyViewer(centralWidget);
  view2=new PathologyViewer(centralWidget);
  view3=new PathologyViewer(centralWidget);
  view4=new PathologyViewer(centralWidget);
  pathologyView->setObjectName(QStringLiteral("pathologyView"));
  view2->setObjectName(QStringLiteral("view2"));
  view3->setObjectName(QStringLiteral("view3"));
  view4->setObjectName(QStringLiteral("view4"));
  horizontalLayout_2->addWidget(pathologyView);
  horizontalLayout_2->addWidget(view2);
  horizontalLayout_3->addWidget(view3);
  horizontalLayout_3->addWidget(view4);
  view2->setVisible(false);
  view3->setVisible(false);
  view4->setVisible(false);
  verticalLayout_3->addLayout(horizontalLayout_2);
  verticalLayout_3->addLayout(horizontalLayout_3);
  this->setCentralWidget(centralWidget);
}

void PathologyWorkstation::retranslateUi()
{
 // this->setWindowTitle(QApplication::translate("PathologyWorkstation", "ASAP", 0));
//  actionOpen->setText(QApplication::translate("PathologyWorkstation", "打开", 0));
//  actionOpen->setIconText(QApplication::translate("PathologyWorkstation", "打开", 0));
//  actionAbout->setText(QApplication::translate("PathologyWorkstation", "关于", 0));
  actionOpen->setShortcut(QApplication::translate("PathologyWorkstation", "Ctrl+O", 0));
//  actionClose->setText(QApplication::translate("PathologyWorkstation", "关闭", 0));
  actionClose->setShortcut(QApplication::translate("PathologyWorkstation", "Ctrl+C", 0));
  save->setShortcut(QApplication::translate("PathologyWorkstation", "Ctrl+S", 0));
//  actionClose->setIconText(QApplication::translate("PathologyWorkstation", "关闭", 0));
 // menuFile->setTitle(QApplication::translate("PathologyWorkstation", "文件", 0,QApplication::));
  //menuView->setTitle(QApplication::translate("PathologyWorkstation", "视图", 0));
  //menuHelp->setTitle(QApplication::translate("PathologyWorkstation", "帮助", 0));
  //menuFile->setTitle(QObject::trUtf8("文件"));
  this->setWindowTitle(QString::fromLocal8Bit("病理影像分析系统"));
  actionOpen->setText(QString::fromLocal8Bit("打开"));
  actionOpen->setIconText(QString::fromLocal8Bit("打开"));
  actionAbout->setText(QString::fromLocal8Bit("关于"));
  actionClose->setText(QString::fromLocal8Bit("关闭"));
  actionClose->setIconText(QString::fromLocal8Bit("关闭"));
  actionConnect->setText(QString::fromLocal8Bit("访问远程服务器"));
  actionCloseall->setText(QString::fromLocal8Bit("关闭所有图像"));
  rotate->setText(QString::fromLocal8Bit("旋转"));
  menuFile->setTitle(QString::fromLocal8Bit("文件"));
  menuView->setTitle(QString::fromLocal8Bit("视图"));
  menuWindow->setTitle(QString::fromLocal8Bit("窗口"));
  menuTool->setTitle(QString::fromLocal8Bit("工具"));
  menuImage->setTitle(QString::fromLocal8Bit("图像"));
  menuHelp->setTitle(QString::fromLocal8Bit("帮助"));
} 
