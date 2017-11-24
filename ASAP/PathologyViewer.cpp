#include "PathologyViewer.h"

#include <iostream>

#include <QResizeEvent>
#include <QApplication>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QGLWidget>
#include <QTimeLine>
#include <QScrollBar>
#include <QHBoxLayout>
#include <QSettings>
#include <QGuiApplication>
#include <QMainWindow>
#include <QStatusBar>
#include <QSlider>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialog>
#include <QTextEdit>

#include "MiniMap.h"
#include "ScaleBar.h"
#include "RenderThread.h"
#include "PrefetchThread.h"
#include "io/multiresolutionimageinterface/MultiResolutionImage.h"
#include "interfaces/interfaces.h"
#include "core/PathologyEnums.h"
#include "WSITileGraphicsItem.h"
#include "WSITileGraphicsItemCache.h"
#include "TileManager.h"
#include "RenderWorker.h"

using std::vector;

PathologyViewer::PathologyViewer(QWidget *parent):
  QGraphicsView(parent),
  _renderthread(NULL),
  _prefetchthread(NULL),
  _zoomSensitivity(0.5),
  _panSensitivity(0.5),
  _numScheduledScalings(0),
  _pan(false),
  _ruler(false),
  _prevPan(0, 0),
  _prevRuler(0, 0),
  _map(NULL),
  _cache(NULL),
  _cacheSize(1000 * 512 * 512 * 3),
  _activeTool(NULL),
  _sceneScale(1.),
  _manager(NULL),
  _scaleBar(NULL),
  actLine(NULL)
{
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setResizeAnchor(QGraphicsView::ViewportAnchor::AnchorViewCenter);
  setDragMode(QGraphicsView::DragMode::NoDrag);
  setContentsMargins(0,0,0,0);
  setAutoFillBackground(true);
  //setViewport(new QGLWidget());
  setViewportUpdateMode(ViewportUpdateMode::FullViewportUpdate);
  setInteractive(false);
  this->setScene(new QGraphicsScene); //Memleak!
  this->setBackgroundBrush(QBrush(QColor(252, 252, 252)));
  this->scene()->setBackgroundBrush(QBrush(QColor(252, 252, 252)));
  this->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, SIGNAL(customContextMenuRequested(const QPoint&)),
          this, SLOT(showContextMenu(const QPoint&)));
  _settings = new QSettings(QSettings::IniFormat, QSettings::UserScope, "DIAG", "ASAP", this);
  _settings->beginGroup("ASAP");
  if (this->window()) {
    QMenu* viewMenu = this->window()->findChild<QMenu*>("menuView");
    QAction* action;
    if (viewMenu->actions().size()<5) {
      action = viewMenu->addAction(QString::fromLocal8Bit("È«ÆÁ"));
      action->setCheckable(true);
      action->setChecked(_settings->value("fullToggled", true).toBool());
      viewMenu->addSeparator();
      action = viewMenu->addAction(QString::fromLocal8Bit("±ÈÀý³ß"));
      action->setCheckable(true);
      action->setChecked(_settings->value("scaleBarToggled", true).toBool());
      action = viewMenu->addAction(QString::fromLocal8Bit("¸²¸ÇÊÓÍ¼"));
      action->setCheckable(true);
      action->setChecked(_settings->value("coverageViewToggled", true).toBool());
      action = viewMenu->addAction(QString::fromLocal8Bit("ËõÂÔÊÓÍ¼"));
      action->setCheckable(true);
      action->setChecked(_settings->value("miniMapToggled", true).toBool());
      viewMenu->addSeparator();
      action = viewMenu->addAction(QString::fromLocal8Bit("×´Ì¬À¸"));
      action->setCheckable(true);     
      action->setChecked(_settings->value("statusbarToggled", true).toBool());
      action = viewMenu->addAction(QString::fromLocal8Bit("Î»ÖÃ×·×Ù"));
      action->setCheckable(true);
      action->setChecked(_settings->value("trackerToggled", false).toBool());
      viewMenu->addSeparator();
    }
  }
  _settings->endGroup();
}

PathologyViewer::~PathologyViewer()
{
  close();
}

unsigned long long PathologyViewer::getCacheSize() {
  if (_cache) {
    return _cache->maxCacheSize();
  }
  else {
    return 0;
  }
}

void PathologyViewer::setCacheSize(unsigned long long& maxCacheSize) {
  if (_cache) {
    _cache->setMaxCacheSize(maxCacheSize);
  }
}

void PathologyViewer::resizeEvent(QResizeEvent *event) {
  QRect rect = QRect(QPoint(0, 0), event->size());
  QRectF FOV = this->mapToScene(rect).boundingRect();
  QRectF FOVImage = QRectF(FOV.left() / this->_sceneScale, FOV.top() / this->_sceneScale, FOV.width() / this->_sceneScale, FOV.height() / this->_sceneScale);
  float maxDownsample = 1. / this->_sceneScale;
  QGraphicsView::resizeEvent(event);
  if (_img) {    
    emit fieldOfViewChanged(FOVImage, _img->getBestLevelForDownSample(maxDownsample / this->transform().m11()));
    emit updateBBox(FOV);
  }
}

void PathologyViewer::zm5(){
    zoom(15);
}

void PathologyViewer::zm(int tm){
    zoom(tm-prezm);
    prezm=tm;
}

void PathologyViewer::wheelEvent(QWheelEvent *event) {
  int numDegrees = event->delta() / 8;
  int numSteps = numDegrees / 15;  // see QWheelEvent documentation
  _zoomToScenePos = this->mapToScene(event->pos());
  _zoomToViewPos = event->pos();
  zoom(numSteps);
}

void PathologyViewer::zoom(float numSteps) {
  if (!_img) {
    return;
  }
  _numScheduledScalings += numSteps;
  if (_numScheduledScalings * numSteps < 0)  {
    _numScheduledScalings = numSteps;
  }

  QTimeLine *anim = new QTimeLine(300, this);
  anim->setUpdateInterval(5);

  connect(anim, SIGNAL(valueChanged(qreal)), SLOT(scalingTime(qreal)));
  connect(anim, SIGNAL(finished()), SLOT(zoomFinished()));
  anim->start();
}

void PathologyViewer::scalingTime(qreal x)
{
  qreal factor = 1.0 + qreal(_numScheduledScalings) * x / 300.;
  float maxDownsample = 1. / this->_sceneScale;
  QRectF FOV = this->mapToScene(this->rect()).boundingRect();
  QRectF FOVImage = QRectF(FOV.left() / this->_sceneScale, FOV.top() / this->_sceneScale, FOV.width() / this->_sceneScale, FOV.height() / this->_sceneScale);
  float scaleX = static_cast<float>(_img->getDimensions()[0]) / FOVImage.width();
  float scaleY = static_cast<float>(_img->getDimensions()[1]) / FOVImage.height();
  float minScale = scaleX > scaleY ? scaleY : scaleX;
  float maxScale = scaleX > scaleY ? scaleX : scaleY;
  if ((factor < 1.0 && maxScale < 0.5) || (factor > 1.0 && minScale > 2*maxDownsample)) {
    return;
  }
  scale(factor, factor);
  centerOn(_zoomToScenePos);
  QPointF delta_viewport_pos = _zoomToViewPos - QPointF(width() / 2.0, height() / 2.0);
  QPointF viewport_center = mapFromScene(_zoomToScenePos) - delta_viewport_pos;
  centerOn(mapToScene(viewport_center.toPoint()));
  float tm11 = this->transform().m11();
  emit fieldOfViewChanged(FOVImage, _img->getBestLevelForDownSample((1. / this->_sceneScale) / this->transform().m11()));
  emit updateBBox(FOV);
}

void PathologyViewer::zoomFinished()
{
  if (_numScheduledScalings > 0)
    _numScheduledScalings--;
  else
    _numScheduledScalings++;
  sender()->~QObject();
}

void PathologyViewer::moveTo(const QPointF& pos) {
  this->centerOn(pos);
  float maxDownsample = 1. / this->_sceneScale;
  QRectF FOV = this->mapToScene(this->rect()).boundingRect();
  QRectF FOVImage = QRectF(FOV.left() / this->_sceneScale, FOV.top() / this->_sceneScale, FOV.width() / this->_sceneScale, FOV.height() / this->_sceneScale);
  emit fieldOfViewChanged(FOVImage, _img->getBestLevelForDownSample(maxDownsample / this->transform().m11()));
  emit updateBBox(FOV);
}

void PathologyViewer::addTool(std::shared_ptr<ToolPluginInterface> tool) {
  if (tool) {
    _tools[tool->name()] = tool;
  }
}

bool PathologyViewer::hasTool(const std::string& toolName) const {
  if (_tools.find(toolName) != _tools.end()) {
    return true;
  }
  else {
    return false;
  }
}

void PathologyViewer::setActiveTool(const std::string& toolName) {
  if (_tools.find(toolName) != _tools.end()) {
    if (_activeTool) {
      _activeTool->setActive(false);
    }
    _activeTool = _tools[toolName];
    _activeTool->setActive(true);
  }
}

std::shared_ptr<ToolPluginInterface> PathologyViewer::getActiveTool() {
  return _activeTool;
}

void PathologyViewer::changeActiveTool() {
  if (sender()) {
    QAction* button = qobject_cast< QAction*>(sender());
    std::shared_ptr<ToolPluginInterface> newActiveTool = _tools[button->objectName().toStdString()];
    if (_activeTool && newActiveTool && _activeTool != newActiveTool) {
      _activeTool->setActive(false);
    }
    if (newActiveTool) {
      _activeTool = newActiveTool;
      _activeTool->setActive(true);
      if(_activeTool->name()=="zoom"){

          QSlider *sld=new QSlider();
          sld->setMinimum(0);
          sld->setMaximum(50);
          sld->setSingleStep(10);
          QDialog *zm=new QDialog();
          Qt::WindowFlags flags=Qt::Dialog;
           flags |=Qt::WindowCloseButtonHint;
           zm->setWindowFlags(flags);
          zm->setWindowTitle(QString::fromLocal8Bit("Ëõ·Å"));

          QVBoxLayout* vLay=new QVBoxLayout();
          QPushButton* b1=new QPushButton(QString::fromLocal8Bit("Ô­Í¼"));
          QPushButton* b2=new QPushButton(QString::fromLocal8Bit("5±¶"));
          QPushButton* b3=new QPushButton(QString::fromLocal8Bit("10±¶"));
          QPushButton* b4=new QPushButton(QString::fromLocal8Bit("20±¶"));
          QPushButton* b5=new QPushButton(QString::fromLocal8Bit("2±¶"));
          QPushButton* b6=new QPushButton(QString::fromLocal8Bit("0.5±¶"));
          QPushButton* b7=new QPushButton(QString::fromLocal8Bit("40±¶"));
          b1->setFixedWidth(95);

          connect(b2, SIGNAL(clicked(bool)),this, SLOT(zm5()));
          connect(sld,SIGNAL(valueChanged(int)),this,SLOT(zm(int)));

          vLay->addWidget(b7);
          vLay->addWidget(b4);
          vLay->addWidget(b3);
          vLay->addWidget(b2);
          vLay->addWidget(b5);
          vLay->addWidget(b1);
          vLay->addWidget(b6);

          QHBoxLayout* hLay=new QHBoxLayout();
          hLay->addLayout(vLay);
          hLay->addWidget(sld);
          zm->setLayout(hLay);
          zm->show();

      }
      else if(_activeTool->name()=="ruler"){
          QDialog *clr=new QDialog();
          Qt::WindowFlags flags=Qt::Dialog;
           flags |=Qt::WindowCloseButtonHint;
          clr->setWindowFlags(flags);
          clr->setWindowTitle(QString::fromLocal8Bit("»­±ÊÑÕÉ«"));
          pen.setColor(Qt::black);
          QVBoxLayout* vLay=new QVBoxLayout();
          QPushButton* b1=new QPushButton();
          QPushButton* b2=new QPushButton();
          QPushButton* b3=new QPushButton();
          QPushButton* b4=new QPushButton();
          QPushButton* b5=new QPushButton();
          QPushButton* b6=new QPushButton();
          b1->setStyleSheet("border:0px;background-color: rgb(0, 0, 0);");
          b2->setStyleSheet("border:0px;background-color: rgb(255,255,255);");
          b3->setStyleSheet("border:0px;background-color: rgb(255, 0, 0);");
          b4->setStyleSheet("border:0px;background-color: rgb(255, 255, 0);");
          b5->setStyleSheet("border:0px;background-color: rgb(0, 255, 0);");
          b6->setStyleSheet("border:0px;background-color: rgb(0, 0, 255);");
          b1->setFixedSize(70,30);
          b2->setFixedSize(70,30);
          b3->setFixedSize(70,30);
          b4->setFixedSize(70,30);
          b5->setFixedSize(70,30);
          b6->setFixedSize(70,30);
          connect(b1, SIGNAL(clicked(bool)), this,SLOT(blkclr()));
          connect(b2,SIGNAL(clicked(bool)),this,SLOT(whiteclr()));
          connect(b3, SIGNAL(clicked(bool)), this,SLOT(redclr()));
          connect(b4,SIGNAL(clicked(bool)),this,SLOT(yellowclr()));
          connect(b5, SIGNAL(clicked(bool)),this, SLOT(greenclr()));
          connect(b6,SIGNAL(clicked(bool)),this,SLOT(blueclr()));
          vLay->addWidget(b4);
          vLay->addWidget(b3);
          vLay->addWidget(b2);
          vLay->addWidget(b5);
          vLay->addWidget(b1);
          vLay->addWidget(b6);
          clr->setLayout(vLay);
          clr->show();
      }
      else if(_activeTool->name()=="textannotation"){
          QDialog *zhushi=new QDialog();
          Qt::WindowFlags flags=Qt::Dialog;
           flags |=Qt::WindowCloseButtonHint;
          zhushi->setWindowFlags(flags);
          zhushi->setWindowTitle(QString::fromLocal8Bit("ÎÄ×Ö×¢ÊÍ"));
          input=new QTextEdit();
          QVBoxLayout* vLay=new QVBoxLayout();
          QPushButton* b1=new QPushButton(QString::fromLocal8Bit("È·¶¨"));
          b1->setFixedSize(80,35);
          connect(b1, SIGNAL(clicked(bool)), this,SLOT(gettext()));
          vLay->addWidget(input);
          vLay->addWidget(b1);
          zhushi->setLayout(vLay);
          zhushi->show();
      }
    }

    else {
      _activeTool = NULL;
    }
  }
}

void PathologyViewer::gettext(){
    annotext=input->toPlainText();
}

void PathologyViewer::blkclr(){
    pen.setColor(Qt::black);
}

void PathologyViewer::whiteclr(){
    pen.setColor(Qt::white);
}

void PathologyViewer::redclr(){
    pen.setColor(Qt::red);
}

void PathologyViewer::blueclr(){
    pen.setColor(Qt::blue);
}

void PathologyViewer::greenclr(){
    pen.setColor(Qt::green);
}

void PathologyViewer::yellowclr(){
    pen.setColor(Qt::yellow);
}

void PathologyViewer::onFieldOfViewChanged(const QRectF& FOV, const unsigned int level) {
  if (_manager) {
    _manager->loadTilesForFieldOfView(FOV, level);
  }
}

void PathologyViewer::initialize(std::shared_ptr<MultiResolutionImage> img) {
  close();
  setEnabled(true);
  _img = img;
  unsigned int tileSize = 512;
  unsigned int lastLevel = _img->getNumberOfLevels() - 1;
  for (int i = lastLevel; i >= 0; --i) {
    std::vector<unsigned long long> lastLevelDimensions = _img->getLevelDimensions(i);
    if (lastLevelDimensions[0] > tileSize && lastLevelDimensions[1] > tileSize) {
      lastLevel = i;
      break;
    }
  }
  _cache = new WSITileGraphicsItemCache();
  _cache->setMaxCacheSize(_cacheSize);
  _renderthread = new RenderThread(this);
  _renderthread->setBackgroundImage(img);
  _manager = new TileManager(_img, tileSize, lastLevel, _renderthread, _cache, scene());
  setMouseTracking(true);
  std::vector<RenderWorker*> workers = _renderthread->getWorkers();
  for (int i = 0; i < workers.size(); ++i) {
    QObject::connect(workers[i], SIGNAL(tileLoaded(QPixmap*, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int)), _manager, SLOT(onTileLoaded(QPixmap*, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int)));
  }
  initializeImage(scene(), tileSize, lastLevel);
  initializeGUIComponents(lastLevel);
  QObject::connect(this, SIGNAL(backgroundChannelChanged(int)), _renderthread, SLOT(onBackgroundChannelChanged(int)));
  QObject::connect(_cache, SIGNAL(itemEvicted(WSITileGraphicsItem*)), _manager, SLOT(onTileRemoved(WSITileGraphicsItem*)));
  QObject::connect(this, SIGNAL(fieldOfViewChanged(const QRectF, const unsigned int)), this, SLOT(onFieldOfViewChanged(const QRectF, const unsigned int)));
  QRectF FOV = this->mapToScene(this->rect()).boundingRect();
  QRectF FOVImage = QRectF(FOV.left() / this->_sceneScale, FOV.top() / this->_sceneScale, FOV.width() / this->_sceneScale, FOV.height() / this->_sceneScale);
  emit fieldOfViewChanged(FOVImage, _img->getBestLevelForDownSample((1. / this->_sceneScale) / this->transform().m11()));
}

void PathologyViewer::onForegroundImageChanged(std::weak_ptr<MultiResolutionImage> for_img, float scale) {
  _for_img = for_img;
  if (_renderthread) {
    _renderthread->setForegroundImage(_for_img, scale);
    _manager->refresh();
  }
}

void PathologyViewer::setForegroundLUT(const std::string& LUTname) {
  if (_renderthread) {
    _renderthread->onLUTChanged(LUTname);
    if (_for_img.lock()) {
      _manager->refresh();
    }
  }
}

void PathologyViewer::setForegroundWindowAndLevel(const float& window, const float& level) {
  if (_renderthread) {
    _renderthread->onWindowAndLevelChanged(window, level);
    if (_for_img.lock()) {
      _manager->refresh();
    }
  }
}

void PathologyViewer::setForegroundChannel(unsigned int channel) {
  if (_renderthread) {
    _renderthread->onForegroundChannelChanged(channel);
    if (_for_img.lock()) {
      _manager->refresh();
    }
  }
}


void PathologyViewer::setForegroundOpacity(const float& opacity) {
  if (_renderthread) {
    _renderthread->setForegroundOpacity(opacity);
    if (_for_img.lock()) {
      _manager->refresh();
    }
  }
}


float PathologyViewer::getForegroundOpacity() const {
  return _opacity;
}

void PathologyViewer::initializeImage(QGraphicsScene* scn, unsigned int tileSize, unsigned int lastLevel) {  
  unsigned int nrLevels = _img->getNumberOfLevels();
  std::vector<unsigned long long> lastLevelDimensions = _img->getLevelDimensions(lastLevel);
  float lastLevelWidth = ((lastLevelDimensions[0] / tileSize) + 1) * tileSize;
  float lastLevelHeight = ((lastLevelDimensions[1] / tileSize) + 1) * tileSize;
  float longest = lastLevelWidth > lastLevelHeight ? lastLevelWidth : lastLevelHeight;
  _sceneScale = 1. / _img->getLevelDownsample(lastLevel);
  QRectF n((lastLevelDimensions[0] / 2) - 1.5*longest, (lastLevelDimensions[1] / 2) - 1.5*longest, 3 * longest, 3 * longest);
  this->setSceneRect(n);
  this->fitInView(QRectF(0, 0, lastLevelDimensions[0], lastLevelDimensions[1]), Qt::AspectRatioMode::KeepAspectRatio);

  _manager->loadAllTilesForLevel(lastLevel);
  float maxDownsample = 1. / this->_sceneScale;
  QRectF FOV = this->mapToScene(this->rect()).boundingRect();
  QRectF FOVImage = QRectF(FOV.left() / this->_sceneScale, FOV.top() / this->_sceneScale, FOV.width() / this->_sceneScale, FOV.height() / this->_sceneScale);
  emit fieldOfViewChanged(FOVImage, _img->getBestLevelForDownSample(maxDownsample / this->transform().m11()));
  while (_renderthread->numberOfJobs() > 0) {
  }
}

void PathologyViewer::initializeGUIComponents(unsigned int level) {
  // Initialize the minimap
  std::vector<unsigned long long> overviewDimensions = _img->getLevelDimensions(level);
  unsigned int size = overviewDimensions[0] * overviewDimensions[1] * _img->getSamplesPerPixel();
  unsigned char* overview = new unsigned char[size];
  _img->getRawRegion<unsigned char>(0, 0, overviewDimensions[0], overviewDimensions[1], level, overview);
  QImage ovImg;
  if (_img->getColorType() == pathology::ARGB) {
    ovImg = QImage(overview, overviewDimensions[0], overviewDimensions[1], overviewDimensions[0] * 4, QImage::Format_ARGB32).convertToFormat(QImage::Format_RGB888);
  }
  else if (_img->getColorType() == pathology::RGB) {
    ovImg = QImage(overview, overviewDimensions[0], overviewDimensions[1], overviewDimensions[0] * 3, QImage::Format_RGB888);
  }
  QPixmap ovPixMap = QPixmap(QPixmap::fromImage(ovImg));
  delete[] overview;
  if (_map) {
    _map->deleteLater();
    _map = NULL;
  }
  _map = new MiniMap(ovPixMap, this);
  if (_scaleBar) {
    _scaleBar->deleteLater();
    _scaleBar = NULL;
  }
  std::vector<double> spacing = _img->getSpacing();
  if (!spacing.empty()) {
    _scaleBar = new ScaleBar(spacing[0], this);
  }
  else {
    _scaleBar = new ScaleBar(-1, this);
  }
  if (this->layout()) {
    delete this->layout();
  }
  QVBoxLayout * Hlayout = new QVBoxLayout(this);
  QHBoxLayout * Vlayout = new QHBoxLayout();
  QHBoxLayout * Vlayout2 = new QHBoxLayout();
  Vlayout2->addStretch(4);
  //Hlayout->addStretch(4);
  Hlayout->setContentsMargins(20, 20, 20, 20);
  Hlayout->addLayout(Vlayout);
  Hlayout->addStretch(5);
  Vlayout->addStretch(5);
  Hlayout->addLayout(Vlayout2);
  if (_map) {
    Vlayout->addWidget(_map);
  }
  if (_scaleBar) {
    Vlayout2->addWidget(_scaleBar);
  }
  _map->setTileManager(_manager);
  QObject::connect(this, SIGNAL(updateBBox(const QRectF&)), _map, SLOT(updateFieldOfView(const QRectF&)));
  QObject::connect(_manager, SIGNAL(coverageUpdated()), _map, SLOT(onCoverageUpdated()));
  QObject::connect(_map, SIGNAL(positionClicked(QPointF)), this, SLOT(moveTo(const QPointF&)));
  QObject::connect(this, SIGNAL(fieldOfViewChanged(const QRectF&, const unsigned int)), _scaleBar, SLOT(updateForFieldOfView(const QRectF&)));
  if (this->window()) {
    _settings->beginGroup("ASAP");
    QMenu* viewMenu = this->window()->findChild<QMenu*>("menuView");
    if (viewMenu)  {
      QList<QAction*> actions = viewMenu->actions();
      for (QList<QAction*>::iterator it = actions.begin(); it != actions.end(); ++it) {
        if ((*it)->text() == QString::fromLocal8Bit("±ÈÀý³ß") && _scaleBar) {
          QObject::connect((*it), SIGNAL(toggled(bool)), _scaleBar, SLOT(setVisible(bool)));
          bool showComponent = _settings->value("scaleBarToggled", true).toBool();
          (*it)->setChecked(showComponent);
          _scaleBar->setVisible(showComponent);
        }
        else if ((*it)->text() == QString::fromLocal8Bit("ËõÂÔÊÓÍ¼") && _map) {
          QObject::connect((*it), SIGNAL(toggled(bool)), _map, SLOT(setVisible(bool)));
          bool showComponent = _settings->value("miniMapToggled", true).toBool();
          (*it)->setChecked(showComponent);
          _map->setVisible(showComponent);
        }
        else if ((*it)->text() == QString::fromLocal8Bit("¸²¸ÇÊÓÍ¼") && _map) {
          QObject::connect((*it), SIGNAL(toggled(bool)), _map, SLOT(toggleCoverageMap(bool)));
          bool showComponent = _settings->value("coverageViewToggled", true).toBool();
          (*it)->setChecked(showComponent);
          _map->toggleCoverageMap(showComponent);
        }
      }
    }
    _settings->endGroup();
  }
}

void PathologyViewer::showContextMenu(const QPoint& pos)
{
  QPoint globalPos = this->mapToGlobal(pos);

  if (_img) {
    QMenu rightClickMenu;
    if (_img->getColorType() == pathology::ColorType::Indexed) {
      for (int i = 0; i < _img->getSamplesPerPixel(); ++i) {
        rightClickMenu.addAction(QString("Channel ") + QString::number(i+1));
      }
      QAction* selectedItem = rightClickMenu.exec(globalPos);
      if (selectedItem)
      {
        for (int i = 0; i < _img->getSamplesPerPixel(); ++i) {
          if (selectedItem->text() == QString("Channel ") + QString::number(i + 1)) {
            emit backgroundChannelChanged(i);
            _manager->refresh();
          }
        }
      }
    }
  }
}

void PathologyViewer::close() {
  if (this->window()) {
    QMenu* viewMenu = this->window()->findChild<QMenu*>("menuView");
    _settings->beginGroup("ASAP");
    if (viewMenu) {
      QList<QAction*> actions = viewMenu->actions();
      for (QList<QAction*>::iterator it = actions.begin(); it != actions.end(); ++it) {
        if ((*it)->text() == "±ÈÀý³ß" && _scaleBar) {
          _settings->setValue("scaleBarToggled", (*it)->isChecked());
        }
        else if ((*it)->text() == "ËõÂÔÊÓÍ¼" && _map) {
          _settings->setValue("miniMapToggled", (*it)->isChecked());
        }
        else if ((*it)->text() == "¸²¸ÇÊÓÍ¼" && _map) {
          _settings->setValue("coverageViewToggled", (*it)->isChecked());
        }
      }
    }
    _settings->endGroup();
  }
  if (_prefetchthread) {
    _prefetchthread->deleteLater();
    _prefetchthread = NULL;
  }
  scene()->clear();
  if (_manager) {
    _manager->clear();
    delete _manager;
    _manager = NULL;
  }
  if (_cache) {
    _cache->clear();
    delete _cache;
    _cache = NULL;
  }  
  //_img = NULL;
  if (_renderthread) {
    _renderthread->shutdown();
    _renderthread->deleteLater();
    _renderthread = NULL;
  }
  if (_map) {
    _map->setHidden(true);
    _map->deleteLater();
    _map = NULL;
  }
  if (_scaleBar) {
    _scaleBar->setHidden(true);
    _scaleBar->deleteLater();
    _scaleBar = NULL;
  }
  setEnabled(false);
}

void PathologyViewer::togglePan(bool pan, const QPoint& startPos) {
  if (pan) {
    if (_pan) {
      return;
    }
    _pan = true;
    _prevPan = startPos;
    setCursor(Qt::ClosedHandCursor);
  }
  else {
    if (!_pan) {
      return;
    }
    _pan = false;
    _prevPan = QPoint(0, 0);
    setCursor(Qt::ArrowCursor);
  }
}

void PathologyViewer::pan(const QPoint& panTo) {
  QScrollBar *hBar = horizontalScrollBar();
  QScrollBar *vBar = verticalScrollBar();
  QPoint delta = panTo - _prevPan;
  _prevPan = panTo;
  hBar->setValue(hBar->value() + (isRightToLeft() ? delta.x() : -delta.x()));
  vBar->setValue(vBar->value() - delta.y());
  float maxDownsample = 1. / this->_sceneScale;
  QRectF FOV = this->mapToScene(this->rect()).boundingRect();
  QRectF FOVImage = QRectF(FOV.left() / this->_sceneScale, FOV.top() / this->_sceneScale, FOV.width() / this->_sceneScale, FOV.height() / this->_sceneScale);
  emit fieldOfViewChanged(FOVImage, _img->getBestLevelForDownSample(maxDownsample / this->transform().m11()));
  emit updateBBox(FOV);
}

void PathologyViewer::toggleRuler(bool ruler, const QPoint &startPos){
    if(ruler){
        if(_ruler) return;
        _ruler=true;
        _prevRuler=startPos;
    }
    else{
        if(!_ruler) return;
        _ruler=false;
        //_prevRuler=QPoint(0,0);
    }
}

void PathologyViewer::ruler(QPoint &rulerTo)
{
    QPointF imgLoc1 = this->mapToScene(_prevRuler);
    QPointF imgLoc2 = this->mapToScene(rulerTo);
    if(!actLine){
        actLine=new QGraphicsRectItem();
        actLine->setZValue(std::numeric_limits<float>::max());
        this->scene()->addItem(actLine);
    }
    //QPen pen;
    pen.setWidthF(0);
    //pen.setColor(Qt::black);
    actLine->setPen(pen);
    actLine->setRect(imgLoc1.x(),imgLoc1.y(),imgLoc2.x()-imgLoc1.x(),imgLoc2.y()-imgLoc1.y());
}

void PathologyViewer::rulerDone(QPoint &rulerTo)
{
//    QPointF imgLoc1 = this->mapToScene(_prevRuler);
//    QPointF imgLoc2 = this->mapToScene(rulerTo);
    actLine=NULL;
//    double dist=sqrt(pow(imgLoc1.x()/_sceneScale-imgLoc2.x()/_sceneScale,2)+pow(imgLoc1.y()/_sceneScale-imgLoc2.y()/_sceneScale,2));
//    QGraphicsTextItem* l=new QGraphicsTextItem();
//    QRectF bound=l->boundingRect();
//   QGraphicsRectItem *rec=new QGraphicsRectItem(bound,l);
//    rec->setZValue(std::numeric_limits<float>::max());
//    l->setZValue(std::numeric_limits<float>::max());
//    this->scene()->addItem(rec);
//    this->scene()->addItem(l);
//    l->setPos((imgLoc1.x()+imgLoc2.x())/2,(imgLoc1.y()+imgLoc2.y())/2);
//    std::ostringstream s_dist;
//    s_dist<<dist;
//    rec->setBrush(Qt::green);
//    l->setDefaultTextColor(Qt::green);
//    l->setPlainText(QString::fromStdString(s_dist.str()+"mm"));

}

void PathologyViewer::updateCurrentFieldOfView() {
  float maxDownsample = 1. / this->_sceneScale;
  QRectF FOV = this->mapToScene(this->rect()).boundingRect();
  QRectF FOVImage = QRectF(FOV.left() / this->_sceneScale, FOV.top() / this->_sceneScale, FOV.width() / this->_sceneScale, FOV.height() / this->_sceneScale);
  emit fieldOfViewChanged(FOVImage, _img->getBestLevelForDownSample(maxDownsample / this->transform().m11()));
  emit updateBBox(FOV);
}

void PathologyViewer::mousePressEvent(QMouseEvent *event)
{
  if (event->button() == Qt::MiddleButton)
  {
    togglePan(true, event->pos());
    event->accept();
    return;
  }
  if (_activeTool && event->button() == Qt::LeftButton) {

    _activeTool->mousePressEvent(event);
    if (event->isAccepted()) {
      return;
    }
  }
  event->ignore();
}

void PathologyViewer::mouseReleaseEvent(QMouseEvent *event)
{
  if (event->button() == Qt::MiddleButton)
  {
    togglePan(false);
    event->accept();
    return;
  }
  if (_activeTool && event->button() == Qt::LeftButton) {
    _activeTool->mouseReleaseEvent(event);
    if (event->isAccepted()) {
      return;
    }
  }
  event->ignore();
}

void PathologyViewer::mouseMoveEvent(QMouseEvent *event)
{
  QPointF imgLoc = this->mapToScene(event->pos()) / this->_sceneScale;
  qobject_cast<QMainWindow*>(this->parentWidget()->parentWidget())->statusBar()->showMessage(QString::fromLocal8Bit("µ±Ç°Í¼ÏñÎ»ÖÃ×ø±êÎª: (") + QString::number(imgLoc.x()) + QString(", ") + QString::number(imgLoc.y()) + QString(")"), 1000);
  if (this->_pan) {
    pan(event->pos());
    event->accept();
    return;
  }
  if (_activeTool) {
    _activeTool->mouseMoveEvent(event);
    if (event->isAccepted()) {
      return;
    }
  }
  event->ignore();
}

void PathologyViewer::mouseDoubleClickEvent(QMouseEvent *event) {
  event->ignore();
  if (_activeTool) {
    _activeTool->mouseDoubleClickEvent(event);
  }
}

void PathologyViewer::keyPressEvent(QKeyEvent *event) {
  event->ignore();
  if (_activeTool) {
    _activeTool->keyPressEvent(event);
  }
}

bool PathologyViewer::isPanning() {
  return _pan;
}

void PathologyViewer::setPanSensitivity(float panSensitivity) {
      if (panSensitivity > 1) {
        _panSensitivity = 1;
      } else if (panSensitivity < 0.01) {
        _panSensitivity = 0.01;
      } else {
        _panSensitivity = panSensitivity;
      }
    };

float PathologyViewer::getPanSensitivity() const {
  return _panSensitivity;
};

void PathologyViewer::setZoomSensitivity(float zoomSensitivity) {
      if (zoomSensitivity > 1) {
        _zoomSensitivity = 1;
      } else if (zoomSensitivity < 0.01) {
        _zoomSensitivity = 0.01;
      } else {
        _zoomSensitivity = zoomSensitivity;
      }
    };

float PathologyViewer::getZoomSensitivity() const {
  return _zoomSensitivity;
};
